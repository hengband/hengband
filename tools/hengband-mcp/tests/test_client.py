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

#: リクエストを受けて、返す内容 (分けて送る場合は複数) を返す。None なら応答せずに待つ
Responder = Callable[[dict[str, Any]], list[bytes] | None]


class FakeServer:
    """制御サーバと同じく、1 接続ごとに 1 行のリクエストを読んで応答する TCP サーバ。"""

    def __init__(self, respond: Responder) -> None:
        self.respond = respond
        self.requests: list[dict[str, Any]] = []
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

            with connection:
                data = b""
                while not data.endswith(b"\n"):
                    chunk = connection.recv(4096)
                    if not chunk:
                        break

                    data += chunk

                request = json.loads(data)
                self.requests.append(request)
                parts = self.respond(request)
                if parts is None:
                    # クライアントが諦めて切断するまで待つ
                    connection.recv(1)
                    continue

                for part in parts:
                    connection.sendall(part)
                    time.sleep(0.05)

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


def test_request_reports_a_game_that_is_not_listening() -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]

    with pytest.raises(ControlConnectionError):
        ControlClient(port, timeout=5.0).request("info")
