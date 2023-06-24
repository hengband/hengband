#include "net/http-client.h"
#include "net/curl-easy-session.h"
#include "net/curl-slist.h"
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

}

/*!
 * @brief HTTP GETリクエストを送信する
 * @param url リクエストの送信先URL
 * @return 送信に成功した場合Responseオブジェクト、失敗した場合std::nullopt
 */
std::optional<Response> Client::get(const std::string &url)
{
    libcurl::EasySession session;
    if (!session.is_valid()) {
        return std::nullopt;
    }

    session.common_setup(url, HTTP_CONNECTION_TIMEOUT);
    setup_http_option(session, this->user_agent);

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
