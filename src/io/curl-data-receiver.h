#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <string>

namespace curl_util {

/*!
 * @brief データ受信抽象クラス
 */
class AbstractDataReceiver {
public:
    virtual ~AbstractDataReceiver() = default;
    virtual void prepare_to_receive() = 0;
    virtual size_t receive(char *buf, size_t n) = 0;
};

/*!
 * @brief 文字列データ受信クラス
 */
class StringReceiver : public AbstractDataReceiver {
public:
    void prepare_to_receive() override;
    size_t receive(char *buf, size_t n) override;
    const std::string &get_received_str() const;
    std::string release_received_str();

private:
    std::string received_str;
};

template <std::derived_from<AbstractDataReceiver> T>
auto create_receiver()
{
    return std::make_shared<T>();
}

}
