#pragma once

#include "system/angband.h"
#include <optional>
#include <string>

#if defined(WORLD_SCORE)

namespace http {

struct Response {
    int status;
    std::string body;
};

class HttpContentBase;
class Client {
public:
    std::optional<Response> get(const std::string &url);
    std::optional<Response> post(const std::string &url, const std::string &post_data, const std::string &media_type);

    std::optional<std::string> user_agent;
};

}

#endif
