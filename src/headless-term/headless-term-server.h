/*!
 * @file headless-term-server.h
 * @brief ヘッドレス端末のTCPサーバのヘッダ
 */

#pragma once

#include <cstdint>
#include <string>
#include <tl/optional.hpp>

/*!
 * @brief ループバックアドレスで待ち受け、行単位でJSONをやり取りするサーバ
 * @details
 * 同時に接続できるクライアントは1つだけだが、切断された場合は次の接続を受け付ける。
 * これによりクライアント側は「1コマンド1接続」で操作できる。
 * ソケットハンドルはWindowsのSOCKETとUNIXのファイルディスクリプタの双方を
 * 格納できるようintptr_tで保持し、-1を無効値として扱う。
 */
class HeadlessTermServer {
public:
    explicit HeadlessTermServer(int port);
    ~HeadlessTermServer();
    HeadlessTermServer(const HeadlessTermServer &) = delete;
    HeadlessTermServer &operator=(const HeadlessTermServer &) = delete;
    HeadlessTermServer(HeadlessTermServer &&) = delete;
    HeadlessTermServer &operator=(HeadlessTermServer &&) = delete;

    bool listen_on_loopback();
    bool ensure_client(int timeout_seconds = -1);
    tl::optional<std::string> receive_line(bool wait = true);
    bool send_line(std::string line);

private:
    void close_client();
    void close_listener();

    int port;
    intptr_t listen_socket = -1;
    intptr_t client_socket = -1;
    std::string receive_buffer;
};
