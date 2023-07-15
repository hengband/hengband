#include "net/curl-slist.h"

#if !defined(DISABLE_NET)

namespace libcurl {

class SList::Impl {
public:
    Impl()
        : slist(nullptr, curl_slist_free_all)
    {
    }
    ~Impl() = default;
    Impl(const Impl &) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl(Impl &&) = default;
    Impl &operator=(Impl &&) = default;

    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> slist;
    bool valid = true;
};

SList::SList()
    : pimpl_(std::make_unique<Impl>())
{
}

SList::~SList() = default;
SList::SList(SList &&) = default;
SList &SList::operator=(SList &&) = default;

bool SList::is_valid() const
{
    return pimpl_->valid;
}

void SList::append(const char *str)
{
    if (!pimpl_->valid) {
        return;
    }

    auto result = curl_slist_append(pimpl_->slist.get(), str);
    if (result == nullptr) {
        pimpl_->valid = false;
        return;
    }

    pimpl_->slist.release();
    pimpl_->slist.reset(result);
}

void SList::append(const std::string &str)
{
    this->append(str.data());
}

curl_slist *SList::get()
{
    return pimpl_->slist.get();
}

}

#endif
