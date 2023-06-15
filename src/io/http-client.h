#pragma once

#include <memory>
#include <optional>
#include <string>

namespace curl_util {
class AbstractDataReceiver;
class AbstractDataSender;
}

class HttpContentBase;
class HttpClient {
public:
    std::optional<int> perform_get_request(const std::string &url, std::shared_ptr<curl_util::AbstractDataReceiver> receiver = nullptr);
    std::optional<int> perform_post_request(const std::string &url, std::shared_ptr<curl_util::AbstractDataSender> sender, const std::string &media_type, std::shared_ptr<curl_util::AbstractDataReceiver> receiver = nullptr);

    std::optional<std::string> user_agent;
};
