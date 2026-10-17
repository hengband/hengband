"""変愚蛮怒の制御サーバ (--control-port) と通信する。

制御サーバは 127.0.0.1 の指定ポートで待ち受け、1 行 1 JSON のリクエストに 1 行 1 JSON の
レスポンスを返す。リクエストはゲームがキー入力を待っている間にしか処理されないため、
keys の次に送ったリクエストには、送ったキーが処理された後の状態が返る。
プロトコルの詳細は tools/bot/README.md を参照。
"""

import itertools
import json
import socket
from typing import Any

DEFAULT_HOST = "127.0.0.1"

#: 1 回のリクエストを待つ秒数
DEFAULT_TIMEOUT = 30.0

#: レスポンスの大きさの上限 (地図付きの state でも収まる大きさ)
MAX_RESPONSE_BYTES = 16 * 1024 * 1024


class ControlError(Exception):
    """制御サーバとのやり取りに失敗した。"""


class ControlRequestError(ControlError):
    """制御サーバがリクエストをエラーとして返した、または応答を解釈できない。"""


class ControlConnectionError(ControlError):
    """制御サーバと通信できない (ゲームが終了した、応答が遅すぎる等)。"""


class ControlTimeoutError(ControlConnectionError):
    """制御サーバの応答が時間内に返らなかった。"""


class ControlClient:
    """制御サーバのクライアント。hbctl.py と同じく 1 リクエストごとに接続する。"""

    def __init__(
        self, port: int, host: str = DEFAULT_HOST, timeout: float = DEFAULT_TIMEOUT
    ) -> None:
        """
        :param port: 制御サーバのポート
        :param host: 制御サーバのホスト
        :param timeout: 1 回のリクエストを待つ秒数の既定値
        """
        self.host = host
        self.port = port
        self.timeout = timeout
        self._ids = itertools.count(1)

    def request(
        self, op: str, timeout: float | None = None, **params: Any
    ) -> dict[str, Any]:
        """リクエストを 1 件送り、成功したレスポンスの result を返す。

        :param op: 操作の名前 (info, screen, keys, state, input, messages, quit など)
        :param timeout: 応答を待つ秒数。None なら既定値
        :param params: リクエストに含める引数
        :return: レスポンスの result
        """
        request_id = next(self._ids)
        payload = {"id": request_id, "op": op, **params}
        limit = self.timeout if timeout is None else timeout
        line = self._exchange(json.dumps(payload) + "\n", limit)
        try:
            response = json.loads(line.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise ControlRequestError(
                f"cannot parse the response to {op}: {error}"
            ) from error

        if not isinstance(response, dict):
            raise ControlRequestError(f"the response to {op} is not a JSON object")

        if not response.get("ok", False):
            raise ControlRequestError(
                f"{op} failed: {response.get('error', 'unknown error')}"
            )

        if response.get("id") != request_id:
            raise ControlRequestError(
                f"the response to {op} has a mismatched id: {response.get('id')!r}"
            )

        result = response.get("result")
        if not isinstance(result, dict):
            raise ControlRequestError(f"the response to {op} has no result")

        return result

    def _exchange(self, text: str, timeout: float) -> bytes:
        """1 行送って 1 行受け取る。

        :param text: 送る行 (改行を含む)
        :param timeout: 応答を待つ秒数
        :return: 受け取った行 (改行を含まない)
        """
        try:
            with socket.create_connection(
                (self.host, self.port), timeout=timeout
            ) as connection:
                connection.sendall(text.encode("utf-8"))
                buffer = bytearray()
                while b"\n" not in buffer:
                    chunk = connection.recv(65536)
                    if not chunk:
                        raise ControlConnectionError(
                            "the game closed the connection without responding"
                        )

                    buffer.extend(chunk)
                    if len(buffer) > MAX_RESPONSE_BYTES:
                        raise ControlRequestError("the response is too large")
        except TimeoutError as error:
            raise ControlTimeoutError(
                f"the game did not respond within {timeout:g} seconds"
            ) from error
        except OSError as error:
            raise ControlConnectionError(
                f"cannot talk to the game at {self.host}:{self.port}: {error}"
            ) from error

        return bytes(buffer).split(b"\n", 1)[0]
