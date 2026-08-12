#!/usr/bin/env python3
"""変愚蛮怒の制御サーバ (--control-port) を操作するクライアント。

ゲーム側は 127.0.0.1 の指定ポートで待ち受け、1行1JSONのリクエストに
1行1JSONのレスポンスを返す。本ツールは1コマンドにつき1回接続し、
1件のリクエストを送って結果を表示する。

ゲームの画面は普段通り表示されるため、手元での操作と併用できる。

使用例:
    src/hengband -mgcu --control-port=9000 -uBotTest &
    python3 tools/bot/hbctl.py screen
    python3 tools/bot/hbctl.py keys 'jjj'
"""

import argparse
import json
import math
import socket
import sys
from collections.abc import Callable
from typing import Any

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 9000
DEFAULT_TIMEOUT = 30.0
MAX_RESPONSE_BYTES = 16 * 1024 * 1024

#: サブコマンドの処理関数。プロセスの終了コードを返す
CommandFunc = Callable[[argparse.Namespace], int]


class BotControlError(Exception):
    """制御サーバとの通信で発生したエラー。"""


def send_request(host: str, port: int, timeout: float, payload: dict[str, Any]) -> Any:
    """リクエストを1件送信してレスポンスを受け取る。

    :param host: 接続先ホスト
    :param port: 接続先ポート
    :param timeout: 通信のタイムアウト秒数
    :param payload: 送信するリクエストの辞書
    :return: レスポンスとして受け取ったJSONの値
    """
    try:
        with socket.create_connection((host, port), timeout=timeout) as connection:
            connection.settimeout(timeout)
            connection.sendall((json.dumps(payload) + "\n").encode("utf-8"))
            buffer = bytearray()
            while b"\n" not in buffer:
                chunk = connection.recv(65536)
                if not chunk:
                    raise BotControlError("接続が切断されました (レスポンス未受信)")
                if len(buffer) + len(chunk) > MAX_RESPONSE_BYTES:
                    raise BotControlError("レスポンスが大きすぎます")
                buffer.extend(chunk)
    except OSError as error:
        raise BotControlError(
            f"{host}:{port} との通信に失敗しました: {error}"
        ) from error

    line = bytes(buffer).split(b"\n", 1)[0]
    try:
        return json.loads(line.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise BotControlError(f"レスポンスを解釈できません: {error}") from error


def check_response(response: Any) -> dict[str, Any]:
    """レスポンスが成功を示しているか確認する。

    :param response: レスポンスとして受け取ったJSONの値
    :return: 引数のレスポンス
    """
    if not isinstance(response, dict):
        raise BotControlError(
            "サーバのレスポンスはJSONオブジェクトである必要があります"
        )

    if not response.get("ok", False):
        raise BotControlError(
            f"サーバがエラーを返しました: {response.get('error', response)}"
        )

    return response


def render_screen(response: dict[str, Any]) -> str:
    """画面のレスポンスを枠付きのテキストに整形する。

    :param response: screenリクエストのレスポンス
    :return: 表示用の文字列
    """
    width = response["width"]
    lines = response["lines"]
    cursor = response.get("cursor", {})
    border = "+" + "-" * width + "+"
    rendered = [border]
    for y, line in enumerate(lines):
        rendered.append(f"|{line}|{y:3d}")

    rendered.append(border)
    rendered.append(
        "term={term} size={width}x{height} cursor=({x},{y}) visible={visible}".format(
            term=response.get("term", 0),
            width=width,
            height=response["height"],
            x=cursor.get("x", 0),
            y=cursor.get("y", 0),
            visible=cursor.get("visible", False),
        )
    )
    return "\n".join(rendered)


def request(args: argparse.Namespace, payload: dict[str, Any]) -> dict[str, Any]:
    """リクエストを送信し、成功したレスポンスを返す。

    :param args: コマンドライン引数
    :param payload: 送信するリクエストの辞書
    :return: レスポンスの辞書
    """
    return check_response(send_request(args.host, args.port, args.timeout, payload))


def request_screen(
    args: argparse.Namespace, with_attrs: bool = False
) -> dict[str, Any]:
    """画面を取得する。

    :param args: コマンドライン引数
    :param with_attrs: 色属性を含めるか否か
    :return: screenリクエストのレスポンス
    """
    return request(args, {"op": "screen", "term": args.term, "attrs": with_attrs})


def print_json(obj: Any) -> None:
    """JSONを人間が読める形に整形して表示する。

    :param obj: 表示する辞書
    """
    print(json.dumps(obj, ensure_ascii=False, indent=2))


def command_info(args: argparse.Namespace) -> int:
    """接続先の端末情報とビルド情報を表示する。"""
    print_json(request(args, {"op": "info"}))
    return 0


def command_screen(args: argparse.Namespace) -> int:
    """現在の画面を表示する。"""
    response = request_screen(args, with_attrs=args.attrs)
    if args.json:
        print_json(response)
    else:
        print(render_screen(response))

    return 0


def command_keys(args: argparse.Namespace) -> int:
    """キー列を送信し、処理後の画面を表示する。

    キーの注入とその結果の観測は別のリクエストになる。2件目のscreenが
    返る時点でゲームは再びキー入力待ちに戻っているため、
    描画が確定した画面が得られる。
    """
    request(args, {"op": "keys", "keys": args.keys})
    if args.quiet:
        return 0

    print(render_screen(request_screen(args)))
    return 0


def command_state(args: argparse.Namespace) -> int:
    """ゲームの内部状態をJSONで表示する。"""
    print_json(request(args, {"op": "state"}))
    return 0


def command_messages(args: argparse.Namespace) -> int:
    """直近のメッセージ履歴を古い順に表示する。"""
    response = request(args, {"op": "messages", "count": args.count})
    for message in response["messages"]:
        print(message)

    return 0


def command_raw(args: argparse.Namespace) -> int:
    """任意のJSONリクエストを送信して結果をそのまま表示する。"""
    try:
        payload = json.loads(args.payload)
    except json.JSONDecodeError as error:
        raise BotControlError(f"リクエストのJSONを解釈できません: {error}") from error

    print_json(send_request(args.host, args.port, args.timeout, payload))
    return 0


def command_replay(args: argparse.Namespace) -> int:
    """キー列を記述したファイルを順に投入し、最終画面を表示する。

    ファイルの各行が1回分のキー列となる。「#」で始まる行と空行は無視する。
    --fixed-seed と組み合わせると、同じファイルから同じ結果を再現できる。
    """
    try:
        with open(args.keyfile, encoding="utf-8") as keyfile:
            sequences = [line.rstrip("\n") for line in keyfile]
    except UnicodeError as error:
        raise BotControlError(
            f"{args.keyfile} をUTF-8として読めません: {error}"
        ) from error

    for sequence in sequences:
        if not sequence or sequence.startswith("#"):
            continue

        request(args, {"op": "keys", "keys": sequence})

    print(render_screen(request_screen(args)))
    return 0


def command_quit(args: argparse.Namespace) -> int:
    """ゲームを終了させる。"""
    request(args, {"op": "quit"})
    return 0


def port_number(value: str) -> int:
    """--port の値を検証する。

    :param value: コマンドラインで指定された文字列
    :return: 検証したポート番号
    """
    port = int(value)
    if not 1 <= port <= 65535:
        raise argparse.ArgumentTypeError("ポート番号は1〜65535で指定してください")

    return port


def positive_timeout(value: str) -> float:
    """--timeout の値を検証する。

    :param value: コマンドラインで指定された文字列
    :return: 検証したタイムアウト秒数
    """
    timeout = float(value)
    if not math.isfinite(timeout) or timeout <= 0:
        raise argparse.ArgumentTypeError(
            "タイムアウトは0より大きい有限の秒数で指定してください"
        )

    return timeout


def build_parser() -> argparse.ArgumentParser:
    """コマンドライン引数のパーサを構築する。

    :return: 構築したパーサ
    """
    parser = argparse.ArgumentParser(description="変愚蛮怒の制御サーバを操作する")
    parser.add_argument(
        "--host", default=DEFAULT_HOST, help=f"接続先ホスト (既定: {DEFAULT_HOST})"
    )
    parser.add_argument(
        "--port",
        type=port_number,
        default=DEFAULT_PORT,
        help=f"接続先ポート (既定: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--timeout",
        type=positive_timeout,
        default=DEFAULT_TIMEOUT,
        help=f"通信のタイムアウト秒数 (既定: {DEFAULT_TIMEOUT})",
    )
    parser.add_argument(
        "--term", type=int, default=0, help="対象の端末の添字 (既定: 0)"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("info", help="端末情報とビルド情報を表示する").set_defaults(
        func=command_info
    )

    screen_parser = subparsers.add_parser("screen", help="現在の画面を表示する")
    screen_parser.add_argument(
        "--json", action="store_true", help="整形せずJSONのまま表示する"
    )
    screen_parser.add_argument("--attrs", action="store_true", help="色属性を含める")
    screen_parser.set_defaults(func=command_screen)

    keys_parser = subparsers.add_parser("keys", help="キー列を送信する")
    keys_parser.add_argument(
        "keys", help=r"送信するキー列 (「\e」「^X」等のマクロ表記が使える)"
    )
    keys_parser.add_argument(
        "--quiet", action="store_true", help="送信後の画面を表示しない"
    )
    keys_parser.set_defaults(func=command_keys)

    subparsers.add_parser("state", help="ゲームの内部状態を表示する").set_defaults(
        func=command_state
    )

    messages_parser = subparsers.add_parser("messages", help="メッセージ履歴を表示する")
    messages_parser.add_argument(
        "count", type=int, nargs="?", default=20, help="取得する件数 (既定: 20)"
    )
    messages_parser.set_defaults(func=command_messages)

    raw_parser = subparsers.add_parser("raw", help="任意のJSONリクエストを送信する")
    raw_parser.add_argument("payload", help="送信するJSON")
    raw_parser.set_defaults(func=command_raw)

    replay_parser = subparsers.add_parser(
        "replay", help="キー列ファイルを投入して最終画面を表示する"
    )
    replay_parser.add_argument("keyfile", help="1行1キー列のファイル")
    replay_parser.set_defaults(func=command_replay)

    subparsers.add_parser("quit", help="ゲームを終了させる").set_defaults(
        func=command_quit
    )
    return parser


def main() -> int:
    """コマンドラインから起動された時のエントリポイント。

    :return: プロセスの終了コード
    """
    args = build_parser().parse_args()
    func: CommandFunc = args.func
    try:
        return func(args)
    except (BotControlError, OSError) as error:
        print(f"hbctl: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
