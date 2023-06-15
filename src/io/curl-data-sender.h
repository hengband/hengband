#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace curl_util {

/*!
 * @brief Tがstd::span<U>に変換可能であることを示すコンセプト
 */
// TODO CIのclang-formatとのバージョンの違いのためか整形が異なるので一時的にOFFにしておく
// clang-format off
template <typename T, typename U>
concept ConvertibleToSpanOf =
    requires(T t) {
        {
            std::span{ t }
        } -> std::convertible_to<std::span<U>>;
    };
// clang-format on

/*!
 * @brief データ送信抽象クラス
 */
class AbstractDataSender {
public:
    virtual ~AbstractDataSender() = default;
    virtual void prepare_to_send() = 0;
    virtual size_t send(char *buf, size_t n) = 0;
    virtual size_t remain_size() = 0;
};

/*!
 * @brief バイト列データ送信クラス
 */
template <ConvertibleToSpanOf<char> T>
class ByteSequenceSender : public AbstractDataSender {
public:
    ByteSequenceSender(const T &send_data)
        : send_data(send_data)
    {
    }
    ByteSequenceSender(T &&send_data)
        : send_data(std::move(send_data))
    {
    }

    void prepare_to_send() override
    {
        this->remain_to_send = this->send_data;
    }

    size_t send(char *buf, size_t n) override
    {
        const auto send_size = std::min<size_t>(n, this->remain_to_send.size());

        std::copy_n(this->remain_to_send.begin(), send_size, buf);
        this->remain_to_send = this->remain_to_send.subspan(send_size);

        return send_size;
    }

    size_t remain_size() override
    {
        return this->remain_to_send.size();
    }

private:
    T send_data;
    std::span<char> remain_to_send;
};

template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, std::vector<char>> || std::same_as<std::remove_cvref_t<T>, std::string>
auto create_sender(T &&send_data)
{
    return std::make_shared<ByteSequenceSender<std::remove_cvref_t<T>>>(std::forward<T>(send_data));
}

}
