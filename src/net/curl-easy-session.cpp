#include "net/curl-easy-session.h"

#if !defined(DISABLE_NET)

namespace libcurl {

namespace {

    size_t write_callback(char *buf, size_t size, size_t nitems, EasySession::SendHandler *userdata)
    {
        return (*userdata)(buf, size * nitems);
    }

    size_t read_callback(char *buf, size_t size, size_t nitems, EasySession::ReceiveHandler *userdata)
    {
        return (*userdata)(buf, size * nitems);
    }

    int progress_callback(EasySession::ProgressHandler *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
    {
        return (*userdata)(dltotal, dlnow, ultotal, ulnow) ? 0 : 1;
    }
}

class EasySession::Impl {
public:
    Impl()
        : handle_(curl_easy_init(), curl_easy_cleanup)
    {
    }
    ~Impl() = default;
    Impl(const Impl &) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl(Impl &&) = default;
    Impl &operator=(Impl &&) = default;

    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle_;
    SendHandler send_handler_;
    ReceiveHandler receive_handler_;
    ProgressHandler progress_handler_;
};

EasySession::EasySession()
    : pimpl_(std::make_unique<Impl>())
{
}

EasySession::~EasySession() = default;
EasySession::EasySession(EasySession &&) = default;
EasySession &EasySession::operator=(EasySession &&) = default;

bool EasySession::is_valid() const
{
    return pimpl_->handle_ != nullptr;
}

void EasySession::common_setup(const std::string &url, int timeout_sec)
{
    this->setopt(CURLOPT_URL, url.data());

    this->setopt(CURLOPT_NOSIGNAL, 1);

    if (timeout_sec > 0) {
        this->setopt(CURLOPT_CONNECTTIMEOUT, timeout_sec);
    }
}

void EasySession::sender_setup(SendHandler handler, long post_field_size)
{
    pimpl_->send_handler_ = std::move(handler);
    if (!pimpl_->send_handler_) {
        return;
    }

    this->setopt(CURLOPT_READDATA, &pimpl_->send_handler_);
    this->setopt(CURLOPT_READFUNCTION, read_callback);
    this->setopt(CURLOPT_POSTFIELDSIZE, post_field_size);
}

void EasySession::receiver_setup(ReceiveHandler handler)
{
    pimpl_->receive_handler_ = std::move(handler);
    if (!pimpl_->receive_handler_) {
        return;
    }

    this->setopt(CURLOPT_WRITEDATA, &pimpl_->receive_handler_);
    this->setopt(CURLOPT_WRITEFUNCTION, write_callback);
}

void EasySession::progress_setup(ProgressHandler handler)
{
    pimpl_->progress_handler_ = std::move(handler);
    if (!pimpl_->progress_handler_) {
        this->setopt(CURLOPT_NOPROGRESS, 1);
        return;
    }

    this->setopt(CURLOPT_NOPROGRESS, 0);
    this->setopt(CURLOPT_XFERINFODATA, &pimpl_->progress_handler_);
    this->setopt(CURLOPT_XFERINFOFUNCTION, progress_callback);
}

bool EasySession::perform()
{
    return curl_easy_perform(pimpl_->handle_.get()) == CURLE_OK;
}

CURL *EasySession::get_curl_handle()
{
    return pimpl_->handle_.get();
}

}

#endif
