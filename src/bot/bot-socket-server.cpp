/*!
 * @file bot-socket-server.cpp
 * @brief 制御サーバのTCPソケットの実装
 */

#include "bot/bot-socket-server.h"
#include "bot/bot-report.h"

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <cerrno>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <chrono>
#include <fmt/format.h>

namespace {

/*!
 * @brief send()に渡すフラグ
 * @details
 * 切断済みのソケットへ送信した際にSIGPIPEを発生させないためのもの。
 * SIGPIPEはsignals_init()により異常終了のハンドラへ割り当てられているため、
 * これを抑止しないと送信エラーとして扱う機会が無いままゲームが落ちる。
 * WindowsのWinsockはシグナルを発生させず、MSG_NOSIGNALも定義していない。
 */
#if defined(WINDOWS) || !defined(MSG_NOSIGNAL)
constexpr int SEND_FLAGS = 0;
#else
constexpr int SEND_FLAGS = MSG_NOSIGNAL;
#endif

/*!
 * @brief 待ち受けソケットに設定するアドレス再利用のオプション
 * @details
 * UNIXのSO_REUSEADDRはTIME_WAIT状態のアドレスへのbind()を許すだけで、
 * 待ち受け中のアドレスへ二重にbind()することはできない。
 * 一方WinsockのSO_REUSEADDRは待ち受け中のアドレスへのbind()まで許してしまい、
 * 同じポートで複数のインスタンスが起動して接続先が不定になる。
 * これを防ぐ、UNIXのSO_REUSEADDRと等価な指定がSO_EXCLUSIVEADDRUSEである。
 */
#ifdef WINDOWS
constexpr int ADDRESS_REUSE_OPTION = SO_EXCLUSIVEADDRUSE;
#else
constexpr int ADDRESS_REUSE_OPTION = SO_REUSEADDR;
#endif

/*!
 * @brief 受信バッファに溜め込むリクエスト1行の上限バイト数
 * @details
 * 改行を送らずにデータを送り続けるクライアントによって、
 * 受信バッファが際限なく伸びてゲームがメモリを食い潰すことを防ぐ。
 */
constexpr size_t MAX_REQUEST_BYTES = 16 * 1024 * 1024;

/*!
 * @brief 接続済みのクライアントとの1回のやり取りを待つ秒数
 * @details
 * 同時に接続できるクライアントは1つで、リクエストの処理中は待ち受けソケットを監視しないため、
 * 接続したまま何も送らないクライアントが居ると待ち受けが塞がったままになる。
 * 期限を設けて切断することで、次のクライアントを受け付けられる状態へ自力で復帰する。
 */
constexpr int CLIENT_IO_TIMEOUT_SECONDS = 30;

/*!
 * @brief ソケットの待機期限
 */
using SocketWaitDeadline = std::chrono::steady_clock::time_point;

/*!
 * @brief ソケットの待機方向
 */
enum class SocketWaitMode {
    READ, //!< 読み込み可能になるまで待つ
    WRITE, //!< 書き込み可能になるまで待つ
};

#ifdef WINDOWS
using socket_length_t = int;
using transfer_size_t = int;
using native_socket_t = SOCKET;
#else
using socket_length_t = socklen_t;
using transfer_size_t = size_t;
using native_socket_t = int;
#endif

/*!
 * @brief ソケットハンドルをプラットフォームのソケットAPIが要求する型へ変換する
 * @param socket 対象のソケットハンドル
 * @return ソケットAPIへ渡せる型に変換したハンドル
 * @details
 * WindowsのSOCKETはUINT_PTRであり、intへ変換するとWin64でハンドル値を切り詰める可能性がある。
 * 保持に使うintptr_tとAPIが要求する型の変換をここに集約する。
 */
native_socket_t native_socket(intptr_t socket)
{
    return static_cast<native_socket_t>(socket);
}

/*!
 * @brief ソケットを閉じる
 * @param socket 対象のソケットハンドル
 */
void close_socket_handle(intptr_t socket)
{
    if (socket == INVALID_SOCKET_HANDLE) {
        return;
    }

#ifdef WINDOWS
    ::closesocket(native_socket(socket));
#else
    ::close(native_socket(socket));
#endif
}

/*!
 * @brief 直前のソケット操作のエラー番号を得る
 * @return エラー番号 (WindowsはWinsockのエラーコード、それ以外はerrno)
 * @details
 * ヘッドレス運用では標準エラー出力だけが失敗原因の手掛かりとなるため、
 * 「ポートが使用中」と「権限が不足」のように区別すべき失敗を診断メッセージで見分けられるようにする。
 * 失敗したAPIの直後、他のソケットAPIを呼ぶ前に取得すること。
 */
int last_socket_error()
{
#ifdef WINDOWS
    return ::WSAGetLastError();
#else
    return errno;
#endif
}

/*!
 * @brief 直前のソケット操作が割り込みによって中断されたか調べる
 * @return 中断された場合TRUE
 * @details WindowsのWinsockはEINTR相当のエラーを返さないため常にFALSEとなる。
 */
bool is_socket_call_interrupted()
{
#ifdef WINDOWS
    return false;
#else
    return errno == EINTR;
#endif
}

/*!
 * @brief 直前のaccept()の失敗が一時的なもので、待ち受けを続けられるか調べる
 * @return 待ち受けを続けられる場合TRUE
 * @details
 * 接続要求が確立前に取り消された場合等、待ち受け自体は壊れていない失敗がある。
 * これらをサーバの異常として扱うと、復帰可能な事象でゲームが終了してしまう。
 */
bool is_accept_failure_retryable()
{
#ifdef WINDOWS
    switch (::WSAGetLastError()) {
    case WSAEINTR:
    case WSAECONNRESET:
    case WSAEWOULDBLOCK:
        return true;
    default:
        return false;
    }
#else
    switch (errno) {
    case EINTR:
    case ECONNABORTED:
    case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
    case EWOULDBLOCK:
#endif
    case EPROTO:
        return true;
    default:
        return false;
    }
#endif
}

/*!
 * @brief 送信時にSIGPIPEが発生しないようソケットを設定する
 * @param socket 対象のソケットハンドル
 * @details
 * MSG_NOSIGNALを持たない代わりにSO_NOSIGPIPEを持つ環境 (macOS等のBSD系) 向けの設定。
 * 双方を持つ環境では重複するが、無害なため条件を分けずに設定する。
 */
void suppress_sigpipe([[maybe_unused]] intptr_t socket)
{
#if !defined(WINDOWS) && defined(SO_NOSIGPIPE)
    int enable = 1;
    (void)::setsockopt(native_socket(socket), SOL_SOCKET, SO_NOSIGPIPE, &enable, sizeof(enable));
#endif
}

/*!
 * @brief タイムアウトの長さから待機の期限を求める
 * @param timeout 待機する長さ
 * @return 待機の期限
 */
SocketWaitDeadline make_deadline(std::chrono::milliseconds timeout)
{
    return std::chrono::steady_clock::now() + timeout;
}

/*!
 * @brief 待機の期限を過ぎているか調べる
 * @param deadline 待機の期限
 * @return 期限を過ぎている場合TRUE
 */
bool is_deadline_expired(const SocketWaitDeadline &deadline)
{
    return deadline <= std::chrono::steady_clock::now();
}

/*!
 * @brief ソケットが読み書き可能になるまで待つ
 * @param socket 対象のソケットハンドル
 * @param deadline 待機の期限
 * @param mode 待機する方向 (読み込みまたは書き込み)
 * @return 可能になった場合TRUE、タイムアウトまたはエラーの場合FALSE
 * @details
 * 割り込みで中断された場合は待ち直すが、その際にタイムアウトが延びないよう、
 * 呼び出し側が一度だけ求めた期限までの残り時間をselect()へ渡す。
 */
bool wait_for_socket(intptr_t socket, const SocketWaitDeadline &deadline, SocketWaitMode mode)
{
#ifndef WINDOWS
    // FD_SET()はFD_SETSIZE以上のディスクリプタに対して未定義動作となる。
    // Windowsのfd_setは値の配列であるためこの制限は無い
    if (native_socket(socket) >= FD_SETSIZE) {
        report_bot_message("the socket descriptor is too large to wait for");
        return false;
    }
#endif

    auto is_first_wait = true;
    while (true) {
        fd_set target;
        FD_ZERO(&target);
        FD_SET(native_socket(socket), &target);
        auto *readable = (mode == SocketWaitMode::READ) ? &target : nullptr;
        auto *writable = (mode == SocketWaitMode::WRITE) ? &target : nullptr;
#ifdef WINDOWS
        const auto nfds = 0;
#else
        const auto nfds = native_socket(socket) + 1;
#endif
        // 期限を過ぎていればtimeoutは0のままとし、即時判定として1度だけselect()を呼ぶ
        timeval timeout{};
        const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) {
            if (!is_first_wait) {
                return false;
            }
        } else {
            timeout.tv_sec = static_cast<decltype(timeout.tv_sec)>(remaining.count() / 1000000);
            timeout.tv_usec = static_cast<decltype(timeout.tv_usec)>(remaining.count() % 1000000);
        }

        is_first_wait = false;
        const auto result = ::select(nfds, readable, writable, nullptr, &timeout);
        if ((result < 0) && is_socket_call_interrupted()) {
            continue;
        }

        return result > 0;
    }
}

/*!
 * @brief クライアントとのやり取りが中断された理由を表す文言を得る
 * @param deadline 待機に用いた期限
 * @return 期限切れの場合はタイムアウトを表す文言、それ以外は待機自体の失敗を表す文言
 * @details wait_for_socket()がFALSEを返した理由を診断メッセージで区別するためのもの。
 */
const char *describe_wait_failure(const SocketWaitDeadline &deadline)
{
    return is_deadline_expired(deadline) ? "timed out" : "failed while";
}

}

/*!
 * @brief 制御サーバのTCPソケットを構築する
 * @param port 待ち受けるポート番号 (1〜65535であること)
 * @details 値域はコマンドライン引数を解釈するparse_runtime_argument()で検証済みである。
 */
BotSocketServer::BotSocketServer(int port)
    : port(port)
{
#ifdef WINDOWS
    WSADATA wsa_data{};
    // WSAStartup()は失敗の理由を戻り値で返し、WSAGetLastError()には設定しない
    this->socket_library_error = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
    this->is_socket_library_ready = this->socket_library_error == 0;
#endif
}

/*!
 * @brief ソケットを全て閉じてサーバを破棄する
 */
BotSocketServer::~BotSocketServer()
{
    this->close_client();
    this->close_listener();
#ifdef WINDOWS
    if (this->is_socket_library_ready) {
        ::WSACleanup();
    }
#endif
}

/*!
 * @brief ループバックアドレスで待ち受けを開始する
 * @return 成功した場合TRUE
 */
bool BotSocketServer::listen_on_loopback()
{
    if (!this->is_socket_library_ready) {
        report_bot_message(fmt::format("failed to initialize the socket library (error {})", this->socket_library_error));
        return false;
    }

    const auto socket = static_cast<intptr_t>(::socket(AF_INET, SOCK_STREAM, 0));
    if (socket == INVALID_SOCKET_HANDLE) {
        report_bot_message(fmt::format("failed to create a listening socket (error {})", last_socket_error()));
        return false;
    }

    int reuse = 1;
    (void)::setsockopt(native_socket(socket), SOL_SOCKET, ADDRESS_REUSE_OPTION, reinterpret_cast<const char *>(&reuse), static_cast<socket_length_t>(sizeof(reuse)));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    // portが1〜65535に収まることはコンストラクタの事前条件
    address.sin_port = ::htons(static_cast<uint16_t>(this->port));
    if (::bind(native_socket(socket), reinterpret_cast<const sockaddr *>(&address), static_cast<socket_length_t>(sizeof(address))) != 0) {
        report_bot_message(fmt::format("failed to bind the listening socket (error {})", last_socket_error()));
        close_socket_handle(socket);
        return false;
    }

    // 同時に相手をするクライアントは1つだが、1コマンドごとに接続と切断を繰り返す使い方のため、
    // 処理中に届いた接続要求が弾かれないよう保留キューは深く取っておく
    if (::listen(native_socket(socket), SOMAXCONN) != 0) {
        report_bot_message(fmt::format("failed to listen on the socket (error {})", last_socket_error()));
        close_socket_handle(socket);
        return false;
    }

    this->listen_socket = socket;
    report_bot_message(fmt::format("listening on 127.0.0.1:{}", this->port));
    return true;
}

/*!
 * @brief クライアントと接続しているか調べる
 * @return 接続している場合TRUE
 */
bool BotSocketServer::has_client() const
{
    return this->client_socket != INVALID_SOCKET_HANDLE;
}

/*!
 * @brief クライアントの接続を期限まで待つ
 * @param timeout 待機する長さ
 * @return クライアントと接続している場合TRUE
 * @details
 * クライアントは1コマンドごとに接続と切断を繰り返すため、
 * 切断されても待ち受けを続けて次の接続を受け付ける。
 * 期限内に接続が無いことは異常ではないため、その旨は通知しない。
 */
bool BotSocketServer::accept_client(std::chrono::milliseconds timeout)
{
    if (this->has_client()) {
        return true;
    }

    if (this->listen_socket == INVALID_SOCKET_HANDLE) {
        return false;
    }

    const auto deadline = make_deadline(timeout);
    while (true) {
        const auto is_readable = wait_for_socket(this->listen_socket, deadline, SocketWaitMode::READ);
        if (is_readable) {
            const auto accepted = static_cast<intptr_t>(::accept(native_socket(this->listen_socket), nullptr, nullptr));
            if (accepted != INVALID_SOCKET_HANDLE) {
                suppress_sigpipe(accepted);
                this->client_socket = accepted;
                return true;
            }

            const auto error_number = last_socket_error();
            if (!is_accept_failure_retryable()) {
                report_bot_message(fmt::format("failed to accept a client connection (error {})", error_number));
                return false;
            }
        }

        // 待機が終了した場合と、一時的な失敗が続いたまま期限を過ぎた場合は接続を諦める
        if (!is_readable || is_deadline_expired(deadline)) {
            return false;
        }

        // 一時的な失敗であれば、期限を延ばさずに次の接続要求を待ち直す
    }
}

/*!
 * @brief 接続済みのクライアントからデータが届くのを期限まで待つ
 * @param timeout 待機する長さ
 * @return 読み込み可能になった場合TRUE
 * @details
 * 期限内に何も届かないことは異常ではないため、その旨は通知しない。
 * リクエストが届いていない時にreceive_line()がゲームの進行を止めるのを防ぐためのもので、
 * 本関数がTRUEを返してからreceive_line()を呼ぶこと。
 */
bool BotSocketServer::wait_readable(std::chrono::milliseconds timeout)
{
    if (!this->has_client()) {
        return false;
    }

    // 前回の受信で1行に満たないデータが残っている場合、届くのを待たずに続きを組み立てられる
    if (this->receive_buffer.find('\n') != std::string::npos) {
        return true;
    }

    return wait_for_socket(this->client_socket, make_deadline(timeout), SocketWaitMode::READ);
}

/*!
 * @brief リクエストを1行受信する
 * @return 受信した行。クライアントが切断された場合はnullopt
 * @details
 * リクエストがTCPのセグメントに分割されて届く場合と、1回の受信に複数行が含まれる場合の
 * どちらにも対応するため、受信した内容は行が揃うまで受信バッファに残す。
 * 改行が現れないまま1行がMAX_REQUEST_BYTESを超えた場合はクライアントを切断する。
 *
 * 期限は行の組み立て全体に掛ける。受信のたびに求め直すと、改行を送らないまま
 * 少しずつデータを送り続けるクライアントが期限を際限なく延ばせてしまう。
 */
tl::optional<std::string> BotSocketServer::receive_line()
{
    const auto deadline = make_deadline(std::chrono::seconds(CLIENT_IO_TIMEOUT_SECONDS));
    while (true) {
        const auto separator = this->receive_buffer.find('\n');
        if (separator != std::string::npos) {
            auto line = this->receive_buffer.substr(0, separator);
            this->receive_buffer.erase(0, separator + 1);
            if (!line.empty() && (line.back() == '\r')) {
                line.pop_back();
            }

            return line;
        }

        if (this->receive_buffer.size() > MAX_REQUEST_BYTES) {
            report_bot_message("the request line is too long");
            this->close_client();
            return tl::nullopt;
        }

        if (this->client_socket == INVALID_SOCKET_HANDLE) {
            return tl::nullopt;
        }

        if (!wait_for_socket(this->client_socket, deadline, SocketWaitMode::READ)) {
            report_bot_message(fmt::format("{} waiting for a request from the client", describe_wait_failure(deadline)));
            this->close_client();
            return tl::nullopt;
        }

        char buffer[4096];
        const auto received = ::recv(native_socket(this->client_socket), buffer, static_cast<transfer_size_t>(sizeof(buffer)), 0);
        if (received < 0) {
            if (is_socket_call_interrupted()) {
                continue;
            }

            this->close_client();
            return tl::nullopt;
        }

        if (received == 0) {
            this->close_client();
            return tl::nullopt;
        }

        this->receive_buffer.append(buffer, static_cast<size_t>(received));
    }
}

/*!
 * @brief レスポンスを1行送信する
 * @param payload 送信する行 (改行は本関数が付加する)
 * @return 送信に成功した場合TRUE
 * @details
 * レスポンスは内部状態のスナップショットで大きくなり得るため、値渡しで受けてコピーを避ける。
 * 受信と同様、レスポンスを読まないクライアントで待ち受けが塞がらないよう、
 * 送信全体に期限を設ける。
 */
bool BotSocketServer::send_line(std::string payload)
{
    if (this->client_socket == INVALID_SOCKET_HANDLE) {
        return false;
    }

    const auto deadline = make_deadline(std::chrono::seconds(CLIENT_IO_TIMEOUT_SECONDS));
    payload.push_back('\n');
    size_t sent_total = 0;
    while (sent_total < payload.size()) {
        if (!wait_for_socket(this->client_socket, deadline, SocketWaitMode::WRITE)) {
            report_bot_message(fmt::format("{} waiting to send a response to the client", describe_wait_failure(deadline)));
            this->close_client();
            return false;
        }

        const auto sent = ::send(native_socket(this->client_socket), payload.data() + sent_total,
            static_cast<transfer_size_t>(payload.size() - sent_total), SEND_FLAGS);
        if (sent <= 0) {
            if ((sent < 0) && is_socket_call_interrupted()) {
                continue;
            }

            this->close_client();
            return false;
        }

        sent_total += static_cast<size_t>(sent);
    }

    return true;
}

/*!
 * @brief クライアントとの接続を閉じて受信バッファを破棄する
 */
void BotSocketServer::close_client()
{
    close_socket_handle(this->client_socket);
    this->client_socket = INVALID_SOCKET_HANDLE;
    this->receive_buffer.clear();
}

/*!
 * @brief 待ち受けソケットを閉じる
 */
void BotSocketServer::close_listener()
{
    close_socket_handle(this->listen_socket);
    this->listen_socket = INVALID_SOCKET_HANDLE;
}
