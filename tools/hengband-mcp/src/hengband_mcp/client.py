"""変愚蛮怒の制御サーバ (--control-port) と通信する。

制御サーバは 127.0.0.1 の指定ポートで待ち受け、1 行 1 JSON のリクエストに 1 行 1 JSON の
レスポンスを返す。リクエストはゲームがキー入力を待っている間にしか処理されないため、
keys の次に送ったリクエストには、送ったキーが処理された後の状態が返る。
プロトコルの詳細は tools/bot/README.md を参照。
"""

import itertools
import json
import socket
import time
from typing import Any

DEFAULT_HOST = "127.0.0.1"

#: 1 回のリクエストを待つ秒数
DEFAULT_TIMEOUT = 30.0

#: レスポンスの大きさの上限 (地図付きの state でも収まる大きさ)
MAX_RESPONSE_BYTES = 16 * 1024 * 1024

#: 前回のやり取りからこの秒数が過ぎた接続は使わずに接続し直す。
#: 制御サーバは 30 秒間やり取りの無いクライアントを切断するため、それより短くする
IDLE_RECONNECT_SECONDS = 20.0

#: 接続が切れていた場合に、接続し直して送り直してよい op (ゲームの状態を変えないもの)
RETRYABLE_OPS = frozenset({"info", "screen", "state", "input", "messages"})


class ControlError(Exception):
    """制御サーバとのやり取りに失敗した。"""


class ControlRequestError(ControlError):
    """制御サーバがリクエストをエラーとして返した、または応答を解釈できない。"""


class ControlConnectionError(ControlError):
    """制御サーバと通信できない (ゲームが終了した、応答が遅すぎる等)。"""


class ControlTimeoutError(ControlConnectionError):
    """制御サーバの応答が時間内に返らなかった。"""


class ControlDroppedError(ControlConnectionError):
    """制御サーバが、応答を 1 バイトも返さずに接続を閉じた。"""


class ControlClient:
    """制御サーバのクライアント。

    1 つの接続でリクエストを続けて送る。リクエストごとに接続すると、キーを 1 つ送るたびに
    keys と input で 2 回接続することになり、閉じた接続 (TIME_WAIT) で一時ポートを使い果たす
    (Windows では WinError 10048)。制御サーバは同時に 1 つのクライアントしか受け付けないため、
    ほかのクライアント (hbctl.py など) に譲るときは close() で切断する。
    """

    def __init__(
        self,
        port: int,
        host: str = DEFAULT_HOST,
        timeout: float = DEFAULT_TIMEOUT,
        idle_reconnect: float = IDLE_RECONNECT_SECONDS,
    ) -> None:
        """
        :param port: 制御サーバのポート
        :param host: 制御サーバのホスト
        :param timeout: 1 回のリクエストを待つ秒数の既定値
        :param idle_reconnect: 前回のやり取りからこの秒数が過ぎた接続は使わずに接続し直す
        """
        self.host = host
        self.port = port
        self.timeout = timeout
        self.idle_reconnect = idle_reconnect
        self._ids = itertools.count(1)
        self._connection: socket.socket | None = None
        self._buffer = bytearray()
        self._last_used = 0.0

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
        line = self._exchange(json.dumps(payload) + "\n", limit, op in RETRYABLE_OPS)
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

    def close(self) -> None:
        """接続を閉じる。次のリクエストで接続し直す。"""
        if self._connection is not None:
            self._connection.close()
            self._connection = None

        self._buffer.clear()

    def _exchange(self, text: str, timeout: float, retryable: bool) -> bytes:
        """1 行送って 1 行受け取る。

        制御サーバは、やり取りの無いまま 30 秒が過ぎたクライアントを、届いているリクエストを
        読まずに切断する。ゲームが長い処理 (休憩など) を続けた後の入力待ちでも起こる。
        そのため、使い回した接続が応答の無いまま閉じられた場合は、リクエストは処理されていない
        とみなし、ゲームの状態を変えないリクエストに限って新しい接続で送り直す。

        :param text: 送る行 (改行を含む)
        :param timeout: 応答を待つ秒数
        :param retryable: 接続が切れていた場合に送り直してよいか
        :return: 受け取った行 (改行を含まない)
        """
        if (
            self._connection is not None
            and time.monotonic() - self._last_used > self.idle_reconnect
        ):
            # 制御サーバに切断されているかもしれないので使わない
            self.close()

        reused = self._connection is not None
        try:
            return self._send_and_receive(text, timeout)
        except ControlDroppedError:
            if not (reused and retryable):
                raise

        return self._send_and_receive(text, timeout)

    def _send_and_receive(self, text: str, timeout: float) -> bytes:
        """接続していなければ接続し、1 行送って 1 行受け取る。失敗したら接続を閉じる。

        :param text: 送る行 (改行を含む)
        :param timeout: 応答を待つ秒数
        :return: 受け取った行 (改行を含まない)
        """
        try:
            if self._connection is None:
                self._connection = socket.create_connection(
                    (self.host, self.port), timeout=timeout
                )

            connection = self._connection
            connection.settimeout(timeout)
            connection.sendall(text.encode("utf-8"))
            while (end := self._buffer.find(b"\n")) < 0:
                chunk = connection.recv(65536)
                if not chunk and not self._buffer:
                    raise ControlDroppedError(
                        "the game closed the connection without responding"
                    )

                if not chunk:
                    raise ControlConnectionError(
                        "the game closed the connection in the middle of a response"
                    )

                self._buffer.extend(chunk)
                if len(self._buffer) > MAX_RESPONSE_BYTES:
                    raise ControlRequestError("the response is too large")
        except TimeoutError as error:
            # 遅れて届く応答を次のリクエストの応答と取り違えないよう、接続ごと捨てる
            self.close()
            raise ControlTimeoutError(
                f"the game did not respond within {timeout:g} seconds"
            ) from error
        except (ConnectionResetError, ConnectionAbortedError, BrokenPipeError) as error:
            dropped = not self._buffer
            self.close()
            if dropped:
                raise ControlDroppedError(
                    f"the game closed the connection without responding: {error}"
                ) from error

            raise ControlConnectionError(
                f"cannot talk to the game at {self.host}:{self.port}: {error}"
            ) from error
        except OSError as error:
            self.close()
            raise ControlConnectionError(
                f"cannot talk to the game at {self.host}:{self.port}: {error}"
            ) from error
        except ControlError:
            self.close()
            raise

        line = bytes(self._buffer[:end])
        del self._buffer[: end + 1]
        self._last_used = time.monotonic()
        return line
