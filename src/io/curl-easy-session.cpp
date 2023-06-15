#include "io/curl-easy-session.h"
#include "io/curl-data-receiver.h"
#include "io/curl-data-sender.h"

using namespace curl_util;

namespace {

size_t read_callback(char *buf, size_t size, size_t nitems, AbstractDataSender *userdata)
{
    return userdata->send(buf, size * nitems);
}

size_t write_callback(char *buf, size_t size, size_t nitems, AbstractDataReceiver *userdata)
{
    return userdata->receive(buf, size * nitems);
}

}

CurlEasySession::CurlEasySession()
    : handle_(curl_easy_init(), curl_easy_cleanup)
{
}

bool CurlEasySession::is_valid() const
{
    return handle_ != nullptr;
}

void CurlEasySession::common_setup(const std::string &url, int timeout_sec)
{
    this->setopt(CURLOPT_URL, url.data());

    this->setopt(CURLOPT_NOSIGNAL, 1);

    if (timeout_sec > 0) {
        this->setopt(CURLOPT_CONNECTTIMEOUT, timeout_sec);
        this->setopt(CURLOPT_TIMEOUT, timeout_sec);
    }
}

void CurlEasySession::sender_setup(std::shared_ptr<AbstractDataSender> sender)
{
    this->sender_ = std::move(sender);
    if (!this->sender_) {
        return;
    }

    this->sender_->prepare_to_send();
    this->setopt(CURLOPT_READDATA, this->sender_.get());
    this->setopt(CURLOPT_READFUNCTION, read_callback);
    this->setopt(CURLOPT_POSTFIELDSIZE, this->sender_->remain_size());
}

void CurlEasySession::receiver_setup(std::shared_ptr<AbstractDataReceiver> receiver)
{
    this->receiver_ = std::move(receiver);
    if (!this->receiver_) {
        return;
    }

    this->receiver_->prepare_to_receive();
    this->setopt(CURLOPT_WRITEDATA, this->receiver_.get());
    this->setopt(CURLOPT_WRITEFUNCTION, write_callback);
}

bool CurlEasySession::perform()
{
    return curl_easy_perform(handle_.get()) == CURLE_OK;
}
