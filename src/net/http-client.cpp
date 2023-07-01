#include "net/http-client.h"
#include "net/curl-easy-session.h"
#include "net/curl-slist.h"
#include <fstream>
#include <string_view>

#if defined(WORLD_SCORE)

namespace http {

namespace {

    constexpr auto HTTP_CONNECTION_TIMEOUT = 10; //!< HTTP接続タイムアウト時間(秒)

    void setup_http_option(libcurl::EasySession &session, const std::optional<std::string> &user_agent)
    {
        if (user_agent.has_value()) {
            session.setopt(CURLOPT_USERAGENT, user_agent->data());
        }

        session.setopt(CURLOPT_FOLLOWLOCATION, 1);
        session.setopt(CURLOPT_MAXREDIRS, 10);
        session.setopt(CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
    }

    std::optional<int> perform_get_request(const std::string &url, const libcurl::EasySession::ReceiveHandler &receiver, const std::optional<std::string> &user_agent,
        const http::Client::GetRequestProgressHandler &progress_handler)
    {
        libcurl::EasySession session;
        if (!session.is_valid()) {
            return std::nullopt;
        }

        session.common_setup(url, HTTP_CONNECTION_TIMEOUT);
        setup_http_option(session, user_agent);

        session.receiver_setup(receiver);
        if (progress_handler) {
            auto handler = [progress_handler](size_t dltotal, size_t dlnow, size_t, size_t) {
                return progress_handler({ dltotal, dlnow });
            };
            session.progress_setup(handler);
        }

        if (!session.perform()) {
            return std::nullopt;
        }

        long status;
        session.getinfo(CURLINFO_RESPONSE_CODE, &status);
        return static_cast<int>(status);
    }
}

/*!
 * @brief HTTP GETリクエストを送信する
 * @param url リクエストの送信先URL
 * @param progress_handler 進捗状況を受け取るコールバック関数
 * @return 送信に成功した場合Responseオブジェクト、失敗した場合std::nullopt
 */
std::optional<Response> Client::get(const std::string &url, GetRequestProgressHandler progress_handler)
{
    Response response{};
    auto receiver = [&response](char *buf, size_t n) {
        response.body.append(buf, n);
        return n;
    };

    const auto status_opt = perform_get_request(url, receiver, this->user_agent, progress_handler);
    if (!status_opt) {
        return std::nullopt;
    }

    response.status = *status_opt;
    return std::make_optional(std::move(response));
}

/*!
 * @brief HTTP GETリクエストを送信し、応答をファイルに保存する
 *
 * ファイルへの保存に成功した場合は、Responseオブジェクトのbodyにファイルパスが格納される。
 *
 * @param url リクエストの送信先URL
 * @param path ファイルの保存先パス
 * @param progress_handler 進捗状況を受け取るコールバック関数
 * @return 送信に成功した場合Responseオブジェクト、失敗した場合std::nullopt
 */
std::optional<Response> Client::get(const std::string &url, const std::filesystem::path &path, GetRequestProgressHandler progress_handler)
{
    std::ofstream ofs(path, std::ios::binary);
    auto receiver = [&ofs](char *buf, size_t n) {
        ofs.write(buf, n);
        return ofs ? n : 0;
    };

    const auto status_opt = perform_get_request(url, receiver, this->user_agent, progress_handler);
    if (!status_opt) {
        return std::nullopt;
    }

    return Response{ *status_opt, path.string() };
}

/*!
 * @brief HTTP POSTリクエストを送信する
 * @param url リクエストの送信先URL
 * @param post_data POSTデータ本体
 * @param media_type POSTデータのメディアタイプ(Content-Typeヘッダの内容)
 * @return 送信に成功した場合Responseオブジェクト、失敗した場合std::nullopt
 */
std::optional<Response> Client::post(const std::string &url, const std::string &post_data, const std::string &media_type)
{
    libcurl::EasySession session;
    libcurl::SList headers;
    headers.append(format("Content-Type: %s", media_type.data()));

    if (!session.is_valid() || !headers.is_valid()) {
        return std::nullopt;
    }

    session.common_setup(url, HTTP_CONNECTION_TIMEOUT);
    setup_http_option(session, this->user_agent);

    session.setopt(CURLOPT_HTTPHEADER, headers.get());
    session.setopt(CURLOPT_POST, 1);

    auto sender = [data = std::string_view(post_data)](char *buf, size_t n) mutable {
        const auto len = data.copy(buf, n);
        data.remove_prefix(len);
        return len;
    };
    session.sender_setup(std::move(sender), post_data.length());

    Response response{};
    auto receiver = [&response](char *buf, size_t n) {
        response.body.append(buf, n);
        return n;
    };
    session.receiver_setup(std::move(receiver));

    if (!session.perform()) {
        return std::nullopt;
    }

    session.getinfo(CURLINFO_RESPONSE_CODE, &response.status);
    return std::make_optional(std::move(response));
}

}

#endif
