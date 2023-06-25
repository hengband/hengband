#pragma once

#include "system/angband.h"
#include <memory>
#include <string>

#if defined(WORLD_SCORE)

#ifdef WINDOWS
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

namespace libcurl {

/*!
 * @brief libcurl string list の薄いラッパークラス
 */
class SList {
public:
    SList();
    ~SList();
    SList(const SList &) = delete;
    SList &operator=(const SList &) = delete;
    SList(SList &&);
    SList &operator=(SList &&);

    bool is_valid() const;
    void append(const char *str);
    void append(const std::string &str);
    curl_slist *get();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}

#endif
