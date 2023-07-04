#pragma once

#include "system/angband.h"
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#if !defined(DISABLE_NET)

namespace http {

struct Response {
    int status;
    std::string body;
};

struct Progress {
    size_t total; ///< 通信の総バイト数。0の場合は不明
    size_t now; ///< 通信済みのバイト数
};

class HttpContentBase;
class Client {
public:
    /*!
     * @brief HTTP GET通信のコールバック関数の型
     * @param progress 進捗状況。libcurlの仕様によりnow/totalが0で呼ばれることがあるのでそれを考慮すること
     * @return 通信を継続する場合はtrue、中断する場合はfalseを返す
     */
    using GetRequestProgressHandler = std::function<bool(Progress)>;

    std::optional<Response> get(const std::string &url, GetRequestProgressHandler progress_handler = {});
    std::optional<Response> get(const std::string &url, const std::filesystem::path &path, GetRequestProgressHandler progress_handler = {});
    std::optional<Response> post(const std::string &url, const std::string &post_data, const std::string &media_type);

    std::optional<std::string> user_agent;
};

}

#endif
