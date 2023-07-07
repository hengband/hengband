#pragma once

#include "system/angband.h"
#include <functional>
#include <memory>
#include <string>

#if !defined(DISABLE_NET)

#ifdef WINDOWS
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

namespace libcurl {

/*!
 * @brief libcurl easy session の薄いラッパークラス
 */
class EasySession {
public:
    using SendHandler = std::function<size_t(char *, size_t)>;
    using ReceiveHandler = std::function<size_t(char *, size_t)>;
    using ProgressHandler = std::function<bool(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>;

    EasySession();
    ~EasySession();
    EasySession(const EasySession &) = delete;
    EasySession &operator=(const EasySession &) = delete;
    EasySession(EasySession &&);
    EasySession &operator=(EasySession &&);

    bool is_valid() const;
    void common_setup(const std::string &url, int timeout_sec = 0);
    void sender_setup(SendHandler handler, long post_field_size);
    void receiver_setup(ReceiveHandler handler);
    void progress_setup(ProgressHandler handler);
    bool perform();

    template <typename T>
    void setopt(CURLoption opt, T param)
    {
        curl_easy_setopt(this->get_curl_handle(), opt, param);
    }

    template <typename T>
    bool getinfo(CURLINFO info, T arg)
    {
        return curl_easy_getinfo(this->get_curl_handle(), info, arg) == CURLE_OK;
    }

private:
    CURL *get_curl_handle();

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}

#endif
