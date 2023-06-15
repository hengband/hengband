#include "io/http-client.h"
#include "io/curl-data-receiver.h"
#include "io/curl-data-sender.h"
#include "io/curl-easy-session.h"
#include "io/curl-slist.h"
#include "system/angband.h"
#include <algorithm>
#include <span>

namespace {

constexpr auto HTTP_TIMEOUT = 30; //!< HTTP接続タイムアウト時間(秒)

void setup_http_option(curl_util::CurlEasySession &session, const std::optional<std::string> &user_agent)
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
 * @param receiver 応答データ受信クラスのオブジェクト
 * @return 送信に成功した場合HTTPステータスコード、失敗した場合std::nullopt
 */
std::optional<int> HttpClient::perform_get_request(const std::string &url, std::shared_ptr<curl_util::AbstractDataReceiver> receiver)
{
    curl_util::CurlEasySession session;
    if (!session.is_valid()) {
        return std::nullopt;
    }

    session.common_setup(url, HTTP_TIMEOUT);
    setup_http_option(session, url);

    session.receiver_setup(std::move(receiver));

    if (!session.perform()) {
        return std::nullopt;
    }

    long response_code;
    session.getinfo(CURLINFO_RESPONSE_CODE, &response_code);
    return response_code;
}

/*!
 * @brief HTTP POSTリクエストを送信する
 * @param url リクエストの送信先URL
 * @param sender POSTデータ送信クラスのオブジェクト
 * @param media_type POSTデータのメディアタイプ(Content-Typeヘッダの内容)
 * @param receiver 応答データ受信クラスのオブジェクト
 * @return 送信に成功した場合HTTPステータスコード、失敗した場合std::nullopt
 */
std::optional<int> HttpClient::perform_post_request(const std::string &url, std::shared_ptr<curl_util::AbstractDataSender> sender, const std::string &media_type, std::shared_ptr<curl_util::AbstractDataReceiver> receiver)
{
    curl_util::CurlEasySession session;
    curl_util::CurlSList headers;
    headers.append(format("Content-Type: %s", media_type.data()));

    if (!session.is_valid() || !headers.is_valid()) {
        return std::nullopt;
    }

    session.common_setup(url, HTTP_TIMEOUT);
    setup_http_option(session, url);

    session.setopt(CURLOPT_HTTPHEADER, headers.get());
    session.setopt(CURLOPT_POST, 1);

    session.sender_setup(sender);
    session.receiver_setup(receiver);

    if (!session.perform()) {
        return std::nullopt;
    }

    long response_code;
    session.getinfo(CURLINFO_RESPONSE_CODE, &response_code);
    return response_code;
}
