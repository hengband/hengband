/*!
 * @brief SHA-256ハッシュ値計算クラスの定義
 *
 * RFC 6234のリファレンス実装を参考にC++20で実装
 */

/***************** See RFC 6234 for details. *******************/
/* Copyright (c) 2011 IETF Trust and the persons identified as */
/* authors of the code.  All rights reserved.                  */
/* See sha256.h for terms of use and redistribution.           */

#include "util/sha256.h"
#include "system/angband-exceptions.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <span>
#include <sstream>

namespace util {

namespace {

    constexpr auto shift_right(uint32_t word, uint32_t shift)
    {
        return word >> shift;
    }

    constexpr auto rotate_right(uint32_t word, uint32_t shift)
    {
        return (word >> shift) | (word << (32 - shift));
    }

    constexpr auto big_sigma0(uint32_t word)
    {
        return rotate_right(word, 2) ^ rotate_right(word, 13) ^ rotate_right(word, 22);
    }

    constexpr auto big_sigma1(uint32_t word)
    {
        return rotate_right(word, 6) ^ rotate_right(word, 11) ^ rotate_right(word, 25);
    }

    constexpr auto small_sigma0(uint32_t word)
    {
        return rotate_right(word, 7) ^ rotate_right(word, 18) ^ shift_right(word, 3);
    }

    constexpr auto small_sigma1(uint32_t word)
    {
        return rotate_right(word, 17) ^ rotate_right(word, 19) ^ shift_right(word, 10);
    }

    constexpr auto sha_ch(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & (y ^ z)) ^ z;
    }

    constexpr auto sha_maj(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & (y | z)) | (y & z);
    }

    /// @brief SHA-256の初期ハッシュ値
    constexpr std::array<uint32_t, SHA256::DIGEST_SIZE / 4> INITIAL_HASH{ {
        // clang-format off
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
        0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
        // clang-format on
    } };
}

struct SHA256::Impl {
    void add_length(size_t length);
    void process_message_block();
    void finalize(std::byte pad_byte);
    void pad_message(std::byte pad_byte);

    std::array<uint32_t, SHA256::DIGEST_SIZE / 4> hash = INITIAL_HASH;
    uint64_t length = 0;
    int message_block_index = 0;
    std::array<std::byte, SHA256::BLOCK_SIZE> message_block{};
    bool computed = false;
};

SHA256::SHA256()
    : pimpl(std::make_unique<Impl>())
{
}

SHA256::~SHA256() = default;

/*!
 * @brief ハッシュ値計算をリセットする
 */
void SHA256::reset()
{
    this->pimpl->hash = INITIAL_HASH;
    this->pimpl->length = 0;
    this->pimpl->message_block_index = 0;
    this->pimpl->message_block.fill(std::byte(0));
    this->pimpl->computed = false;
}

/*!
 * @brief メッセージ(バイト列)をハッシュ値に追加する
 *
 * @param message_array 追加するメッセージ
 * @param length メッセージのバイト数
 */
void SHA256::update(const std::byte *message_array, size_t length)
{
    if (length == 0) {
        return;
    }

    if (this->pimpl->computed) {
        THROW_EXCEPTION(std::logic_error, "SHA256::update() called after compute.");
    }

    this->pimpl->add_length(8 * length);

    std::span remain(message_array, length);
    while (!remain.empty()) {
        const auto copy_size = std::min<int>(remain.size(), BLOCK_SIZE - this->pimpl->message_block_index);
        std::copy_n(remain.begin(), copy_size, this->pimpl->message_block.begin() + this->pimpl->message_block_index);
        this->pimpl->message_block_index += copy_size;
        remain = remain.subspan(copy_size);

        if (this->pimpl->message_block_index == BLOCK_SIZE) {
            this->pimpl->process_message_block();
        }
    }
}

/*!
 * @brief メッセージ(文字列)をハッシュ値に追加する
 *
 * @param message 追加するメッセージ
 */
void SHA256::update(std::string_view message)
{
    const auto message_as_byte = std::as_bytes(std::span(message.begin(), message.end()));
    this->update(message_as_byte.data(), message_as_byte.size_bytes());
}

/*!
 * @brief 最終ブロックのビット列をハッシュ値に追加する
 *
 * 最終ブロックのデータが1バイトに満たない場合に使用される。
 * 一般的にはバイト単位での追加を行うため、通常このメソッドは呼び出されない。
 *
 * @param message_bits 追加するビット列
 * @param length 追加するビット列の長さ(0～7)
 */
void SHA256::final_bits(std::byte message_bits, size_t length)
{
    if (this->pimpl->computed) {
        THROW_EXCEPTION(std::logic_error, "SHA256::final_bits() called after compute.");
    }

    if (length >= 8) {
        THROW_EXCEPTION(std::invalid_argument, "SHA256::final_bits() called with length >= 8.");
    }

    // clang-format off
    static constexpr std::array<std::byte, 8> masks{ {
        std::byte(0x00), std::byte(0x80), std::byte(0xC0), std::byte(0xE0),
        std::byte(0xF0), std::byte(0xF8), std::byte(0xFC), std::byte(0xFE) } };
    static constexpr std::array<std::byte, 8> markbits{ {
        std::byte(0x80), std::byte(0x40), std::byte(0x20), std::byte(0x10),
        std::byte(0x08), std::byte(0x04), std::byte(0x02), std::byte(0x01) } };
    // clang-format on

    this->pimpl->add_length(length);
    this->pimpl->finalize((message_bits & masks[length]) | markbits[length]);
}

/*!
 * @brief ハッシュ値を取得する
 *
 * @return ハッシュ値
 */
SHA256::Digest SHA256::digest()
{
    if (!this->pimpl->computed) {
        this->pimpl->finalize(std::byte(0x80));
    }

    Digest result{};
    for (auto i = 0; i < SHA256::DIGEST_SIZE; ++i) {
        result[i] = std::byte(this->pimpl->hash[i / 4] >> 8 * (3 - (i % 4)));
    }

    return result;
}

void SHA256::Impl::add_length(size_t len)
{
    if (std::numeric_limits<decltype(this->length)>::max() - len < this->length) {
        THROW_EXCEPTION(std::overflow_error, "SHA256::add_length() overflow.");
    }

    this->length += len;
}

void SHA256::Impl::process_message_block()
{
    static constexpr std::array<uint32_t, BLOCK_SIZE> k{ {
        // clang-format off
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
        0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
        0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
        0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
        0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
        0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
        0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
        0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        // clang-format on
    } };
    std::array<uint32_t, BLOCK_SIZE> w{};

    for (auto i = 0, i4 = 0; i4 < BLOCK_SIZE; ++i, i4 += 4) {
        w[i] = (static_cast<uint32_t>(this->message_block[i4]) << 24) |
               (static_cast<uint32_t>(this->message_block[i4 + 1]) << 16) |
               (static_cast<uint32_t>(this->message_block[i4 + 2]) << 8) |
               (static_cast<uint32_t>(this->message_block[i4 + 3]));
    }

    for (auto i = 16; i < BLOCK_SIZE; ++i) {
        w[i] = small_sigma1(w[i - 2]) + w[i - 7] + small_sigma0(w[i - 15]) + w[i - 16];
    }

    auto h = this->hash;

    for (auto i = 0; i < BLOCK_SIZE; ++i) {
        const auto tmp1 = h[7] + big_sigma1(h[4]) + sha_ch(h[4], h[5], h[6]) + k[i] + w[i];
        const auto tmp2 = big_sigma0(h[0]) + sha_maj(h[0], h[1], h[2]);
        std::copy_backward(h.begin(), h.end() - 1, h.end());
        h[4] += tmp1;
        h[0] = tmp1 + tmp2;
    }

    std::transform(this->hash.begin(), this->hash.end(), h.begin(),
        this->hash.begin(), std::plus<uint32_t>{});

    this->message_block_index = 0;
}

void SHA256::Impl::finalize(std::byte pad_byte)
{
    this->pad_message(pad_byte);
    this->message_block.fill(std::byte(0));
    this->length = 0;
    this->computed = true;
}

void SHA256::Impl::pad_message(std::byte pad_byte)
{
    this->message_block[this->message_block_index++] = pad_byte;

    const std::span whole(this->message_block);

    if (const auto remain = whole.subspan(this->message_block_index); remain.size() < 8) {
        std::fill(remain.begin(), remain.end(), std::byte(0));
        this->process_message_block();
    }

    const auto zerofill_span = whole.first(BLOCK_SIZE - 8).subspan(this->message_block_index);
    std::fill(zerofill_span.begin(), zerofill_span.end(), std::byte(0));

    const auto length_span = whole.last(8);
    for (auto i = 0; i < 8; ++i) {
        length_span[i] = std::byte((this->length >> (BLOCK_SIZE - 8 - i * 8)) & 0xff);
    }

    this->process_message_block();
}

/*!
 * @brief ファイルのSHA-256ハッシュ値を計算する
 *
 * @param path ファイルパス
 * @return ハッシュ値を返す。ファイルの読み込みに失敗した場合はstd::nulloptを返す。
 */
std::optional<SHA256::Digest> SHA256::compute_filehash(const std::filesystem::path &path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return std::nullopt;
    }

    SHA256 hash;
    std::array<char, 1024> buf{};
    const auto buf_as_bytes = std::as_bytes(std::span(buf));
    while (ifs) {
        ifs.read(buf.data(), buf.size());
        hash.update(buf_as_bytes.data(), ifs.gcount());
    }

    if (ifs.bad()) {
        return std::nullopt;
    }

    return hash.digest();
}

/*!
 * @brief ハッシュ値を文字列に変換する
 *
 * @param digest ハッシュ値
 * @return ハッシュ値を16進数文字列表記に変換したもの
 */
std::string to_string(const SHA256::Digest &digest)
{
    std::stringstream ss;
    for (const auto byte : digest) {
        ss << std::hex << std::setfill('0') << std::setw(2) << std::to_integer<int>(byte);
    }

    return ss.str();
}

}
