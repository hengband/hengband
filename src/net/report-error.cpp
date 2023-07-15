#include "external-lib/include-json.h"
#include "locale/japanese.h"
#include "net/http-client.h"
#include "system/angband-version.h"
#include <sstream>
#include <string_view>

#if !defined(DISABLE_NET)

namespace {

/*!
 * @brief エラーレポート送信用のWebhook URLを取得する
 * @return 取得に成功した場合はWebhook URL、失敗した場合は空文字列
 */
std::string fetch_webhook_url_for_sending_error_report()
{
    http::Client client;
    constexpr auto url = "https://hengband.github.io/api/report-error";
    const auto response = client.get(url);
    if (!response || (response->status != 200)) {
        return "";
    }

    const auto json = nlohmann::json::parse(response->body);
    return json["webhook_url"];
}

/*!
 * @brief 送信するエラーレポートのJSONオブジェクトを作成する
 *
 * @param message エラーメッセージ
 * @return 作成したJSONオブジェクト
 */
nlohmann::json create_report_json(std::string_view message)
{
    nlohmann::json webhook;
    webhook["username"] = "Hengband Error Report";
    constexpr auto conv_error_msg = "Cannot convert to UTF-8";
    nlohmann::json embed;
    embed["title"] = sys_to_utf8(get_version()).value_or(conv_error_msg);
    embed["description"] = sys_to_utf8(message).value_or(conv_error_msg);
    embed["color"] = 0xff0000; // red
    webhook["embeds"].push_back(std::move(embed));

    return webhook;
}

}

/*!
 * @brief エラーレポートを送信する
 *
 * @param description 送信するエラーの説明
 * @return 送信に成功した場合はtrue、失敗した場合はfalse
 */
bool report_error(std::string_view description)
{
    const auto webhook_url = fetch_webhook_url_for_sending_error_report();
    if (webhook_url.empty()) {
        return false;
    }

    const auto report_json = create_report_json(description);

    http::Client client;
    const auto response = client.post(webhook_url, report_json.dump(), "application/json");

    return (response && (response->status == 200));
}

#endif // !defined(DISABLE_NET)
