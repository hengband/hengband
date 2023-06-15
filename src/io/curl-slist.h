#pragma once

#include "system/angband.h"
#include <memory>
#include <string>
#ifdef WINDOWS
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

namespace curl_util {

/*!
 * @brief libcurl string list の薄いラッパークラス
 */
class CurlSList {
public:
    using SList = std::unique_ptr<curl_slist, void (*)(curl_slist *)>;

    CurlSList();
    ~CurlSList() = default;
    CurlSList(const CurlSList &) = delete;
    CurlSList &operator=(const CurlSList &) = delete;
    CurlSList(CurlSList &&) = default;
    CurlSList &operator=(CurlSList &&) = default;

    bool is_valid() const;
    void append(const char *str);
    void append(const std::string &str);
    curl_slist *get();

private:
    SList slist;
    bool valid = true;
};

}
