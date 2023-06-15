#include "io/curl-data-receiver.h"

using namespace curl_util;

void StringReceiver::prepare_to_receive()
{
    this->received_str.clear();
}

size_t StringReceiver::receive(char *buf, size_t n)
{
    this->received_str.append(buf, n);
    return n;
}

/*!
 * @brief 受信した文字列への参照を取得する
 *
 * @return 受信した文字列への参照
 */
const std::string &StringReceiver::get_received_str() const
{
    return this->received_str;
}

/*!
 * @brief 受信した文字列をムーブセマンティクスにより取得する
 *
 * @return 受信した文字列
 */
std::string StringReceiver::release_received_str()
{
    return std::move(this->received_str);
}
