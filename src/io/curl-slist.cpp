#include "io/curl-slist.h"

using namespace curl_util;

CurlSList::CurlSList()
    : slist(nullptr, curl_slist_free_all)
{
}

bool CurlSList::is_valid() const
{
    return this->valid;
}

void CurlSList::append(const char *str)
{
    if (!this->valid) {
        return;
    }

    auto result = curl_slist_append(this->slist.get(), str);
    if (result == nullptr) {
        this->valid = false;
        return;
    }

    this->slist.release();
    this->slist.reset(result);
}

void CurlSList::append(const std::string &str)
{
    this->append(str.data());
}

curl_slist *CurlSList::get()
{
    return this->slist.get();
}
