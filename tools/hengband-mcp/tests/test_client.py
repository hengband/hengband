import json
import socket
import threading
import time
from collections.abc import Callable, Iterator
from typing import Any

import pytest

from hengband_mcp.client import (
    ControlClient,
    ControlConnectionError,
    ControlRequestError,
    ControlTimeoutError,
)

#: リクエストを受けて、返す内容 (分けて送る場合は複数) を返す。
#: 空なら応答せずに接続を閉じ、None なら応答せずにクライアントが切断するまで待つ
Responder = Callable[[dict[str, Any]], list[bytes] | None]


class FakeServer:
    """制御サーバと同じく、1 行のリクエストを読んで応答する TCP サーバ。

    接続は同時に 1 つで、1 つの接続で続けてリクエストを受け付ける。
    """

    def __init__(self, respond: Responder, close_after_response: bool = False) -> None:
        """
        :param respond: リクエストに応答する関数
        :param close_after_response: 応答するたびに接続を閉じる (制御サーバがクライアントを切断した場合を模す)
        """
        self.respond = respond
        self.close_after_response = close_after_response
        self.requests: list[dict[str, Any]] = []
        self.accepted = 0
        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen()
        self.port = self.listener.getsockname()[1]
        threading.Thread(target=self.serve, daemon=True).start()

    def serve(self) -> None:
        while True:
            try:
                connection, _ = self.listener.accept()
            except OSError:
                return

            self.accepted += 1
            with connection:
                self.serve_connection(connection)

    def serve_connection(self, connection: socket.socket) -> None:
        data = b""
        while True:
            while b"\n" not in data:
                try:
                    chunk = connection.recv(4096)
                except OSError:
                    return

                if not chunk:
                    return

                data += chunk

            line, data = data.split(b"\n", 1)
            request = json.loads(line)
            self.requests.append(request)
            parts = self.respond(request)
            if parts is None:
                # クライアントが諦めて切断するまで待つ
                connection.recv(1)
                return

            if not parts:
                return

            for part in parts:
                connection.sendall(part)
                time.sleep(0.05)

            if self.close_after_response:
                return

    def close(self) -> None:
        self.listener.close()


def reply(request: dict[str, Any], **fields: Any) -> list[bytes]:
    return [json.dumps({"id": request["id"], **fields}).encode() + b"\n"]


@pytest.fixture
def start_server() -> Iterator[Callable[[Responder], FakeServer]]:
    servers: list[FakeServer] = []

    def start(respond: Responder) -> FakeServer:
        server = FakeServer(respond)
        servers.append(server)
        return server

    yield start
    for server in servers:
        server.close()


def test_request_returns_the_result(start_server: Callable[[Responder], FakeServer]) -> None:
    server = start_server(lambda request: reply(request, ok=True, result={"pushed": 1}))
    client = ControlClient(server.port)

    assert client.request("keys", keys="6") == {"pushed": 1}
    assert client.request("input") == {"pushed": 1}
    assert server.requests == [
        {"id": 1, "op": "keys", "keys": "6"},
        {"id": 2, "op": "input"},
    ]
    # 一時ポートを使い果たさないよう、1 つの接続を使い回す
    assert server.accepted == 1


def test_close_releases_the_connection(start_server: Callable[[Responder], FakeServer]) -> None:
    server = start_server(lambda request: reply(request, ok=True, result={}))
    client = ControlClient(server.port)

    client.request("info")
    client.close()
    client.request("info")

    assert server.accepted == 2


def test_request_reconnects_after_being_idle(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    server = start_server(lambda request: reply(request, ok=True, result={}))
    client = ControlClient(server.port, idle_reconnect=0.1)

    client.request("info")
    time.sleep(0.2)
    client.request("info")

    assert server.accepted == 2


def test_request_resends_an_observation_dropped_by_the_game(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    server = start_server(lambda request: reply(request, ok=True, result={"op": request["op"]}))
    server.close_after_response = True
    client = ControlClient(server.port)

    client.request("keys", keys="6")

    # 前の応答の後で切断されていたので、接続し直して送り直す
    assert client.request("input") == {"op": "input"}
    assert server.accepted == 2


def test_request_does_not_resend_keys_dropped_by_the_game(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    server = start_server(lambda request: reply(request, ok=True, result={}))
    server.close_after_response = True
    client = ControlClient(server.port)

    client.request("info")

    # キーは届いたか分からないので送り直さない
    with pytest.raises(ControlConnectionError, match="without responding"):
        client.request("keys", keys="6")

    assert [request["op"] for request in server.requests] == ["info"]


def test_request_reads_a_response_sent_in_pieces(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    def respond(request: dict[str, Any]) -> list[bytes]:
        line = reply(request, ok=True, result={"lines": ["x" * 100000]})[0]
        return [line[:10], line[10:50000], line[50000:]]

    server = start_server(respond)

    assert ControlClient(server.port).request("screen") == {"lines": ["x" * 100000]}


def test_request_raises_the_error_of_the_game(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    server = start_server(lambda request: reply(request, ok=False, error="keys must not be empty"))

    with pytest.raises(ControlRequestError, match="keys failed: keys must not be empty"):
        ControlClient(server.port).request("keys", keys="")


def test_request_rejects_a_mismatched_id(start_server: Callable[[Responder], FakeServer]) -> None:
    server = start_server(lambda request: [b'{"id": 99, "ok": true, "result": {}}\n'])

    with pytest.raises(ControlRequestError, match="mismatched id"):
        ControlClient(server.port).request("info")


def test_request_rejects_a_broken_response(start_server: Callable[[Responder], FakeServer]) -> None:
    server = start_server(lambda request: [b"not json\n"])

    with pytest.raises(ControlRequestError, match="cannot parse"):
        ControlClient(server.port).request("info")


def test_request_reports_a_closed_connection(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    server = start_server(lambda request: [])

    with pytest.raises(ControlConnectionError, match="closed the connection") as error:
        ControlClient(server.port).request("info")

    assert not isinstance(error.value, ControlTimeoutError)


def test_request_times_out_while_the_game_is_busy(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    server = start_server(lambda request: None)

    with pytest.raises(ControlTimeoutError):
        ControlClient(server.port, timeout=0.3).request("input")


def test_request_uses_a_new_connection_after_a_timeout(
    start_server: Callable[[Responder], FakeServer],
) -> None:
    def respond(request: dict[str, Any]) -> list[bytes] | None:
        if request["id"] == 1:
            return None

        return reply(request, ok=True, result={"id": request["id"]})

    server = start_server(respond)
    client = ControlClient(server.port, timeout=0.3)

    with pytest.raises(ControlTimeoutError):
        client.request("input")

    # 遅れて届く応答を取り違えないよう、タイムアウトした接続は使わない
    assert client.request("input", timeout=5.0) == {"id": 2}
    assert server.accepted == 2


def test_request_reports_a_game_that_is_not_listening() -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]

    with pytest.raises(ControlConnectionError):
        ControlClient(port, timeout=5.0).request("info")
