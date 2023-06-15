#pragma once

#include "system/angband.h"
#include <memory>
#include <string>
#ifdef WINDOWS
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

namespace curl_util {

class AbstractDataSender;
class AbstractDataReceiver;

/*!
 * @brief libcurl easy session の薄いラッパークラス
 */
class CurlEasySession {
public:
    using CurlHandle = std::unique_ptr<CURL, void (*)(CURL *)>;

    CurlEasySession();
    ~CurlEasySession() = default;
    CurlEasySession(const CurlEasySession &) = delete;
    CurlEasySession &operator=(const CurlEasySession &) = delete;
    CurlEasySession(CurlEasySession &&) = default;
    CurlEasySession &operator=(CurlEasySession &&) = default;

    bool is_valid() const;
    void common_setup(const std::string &url, int timeout_sec = 0);
    void sender_setup(std::shared_ptr<AbstractDataSender> sender);
    void receiver_setup(std::shared_ptr<AbstractDataReceiver> receiver);
    bool perform();

    template <typename T>
    void setopt(CURLoption opt, T param)
    {
        curl_easy_setopt(this->handle_.get(), opt, param);
    }

    template <typename T>
    bool getinfo(CURLINFO info, T arg) const
    {
        return curl_easy_getinfo(this->handle_.get(), info, arg) == CURLE_OK;
    }

private:
    CurlHandle handle_;
    std::shared_ptr<AbstractDataSender> sender_;
    std::shared_ptr<AbstractDataReceiver> receiver_;
};

}
