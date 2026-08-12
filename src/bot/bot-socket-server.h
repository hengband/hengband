/*!
 * @file bot-socket-server.h
 * @brief 制御サーバのTCPソケットのヘッダ
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <tl/optional.hpp>

/*!
 * @brief 無効なソケットハンドルを表す値
 * @details
 * WindowsのINVALID_SOCKET ((SOCKET)~0) をintptr_tへ変換すると-1になるため、
 * UNIXのファイルディスクリプタと共通の値で無効を表せる。
 */
constexpr intptr_t INVALID_SOCKET_HANDLE = -1;

/*!
 * @brief ループバックアドレスで待ち受け、行単位でJSONをやり取りするサーバ
 * @details
 * 同時に接続できるクライアントは1つだけだが、切断された場合は次の接続を受け付ける。
 * これによりクライアント側は「1コマンド1接続」で操作できる。
 * ソケットハンドルはWindowsのSOCKETとUNIXのファイルディスクリプタの双方を
 * 格納できるようintptr_tで保持する。
 *
 * ゲームの進行を止めないよう、接続と受信の待機には呼び出し側が期限を与える。
 */
class BotSocketServer {
public:
    explicit BotSocketServer(int port);
    ~BotSocketServer();
    BotSocketServer(const BotSocketServer &) = delete;
    BotSocketServer &operator=(const BotSocketServer &) = delete;
    BotSocketServer(BotSocketServer &&) = delete;
    BotSocketServer &operator=(BotSocketServer &&) = delete;

    bool listen_on_loopback();
    bool has_client() const;
    bool accept_client(std::chrono::milliseconds timeout);
    bool wait_readable(std::chrono::milliseconds timeout);
    tl::optional<std::string> receive_line();
    bool send_line(std::string line);

private:
    void close_client();
    void close_listener();

    int port;
    bool is_socket_library_ready = true; //!< ソケットライブラリの初期化に成功したか否か (Windows以外は常にTRUE)
    int socket_library_error = 0; //!< ソケットライブラリの初期化が返したエラー番号 (Windows以外は常に0)
    intptr_t listen_socket = INVALID_SOCKET_HANDLE;
    intptr_t client_socket = INVALID_SOCKET_HANDLE;
    std::string receive_buffer;
};
