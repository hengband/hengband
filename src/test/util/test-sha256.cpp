/*!
 * @brief sha256ハッシュ値計算クラスのテスト
 *
 * RFC 6234 のテストドライバより抜粋した SHA-256 のテストパターンのハッシュ値を計算して比較する
 */

#include "util/sha256.h"

#include <doctest/doctest.h>

#include <span>
#include <string_view>

namespace {

using namespace std::string_view_literals;

/*
 * RFC 6234 で定義されているテストパターン
 * TEST10_256 のように途中に \x00 を含むものがあるため、
 * 長さがリテラルから決まる sv リテラルで定義する
 */
constexpr auto TEST1 = "abc"sv;
constexpr auto TEST2_1 =
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"sv;
constexpr auto TEST3 = "a"sv; /* times 1000000 */
/* an exact multiple of 512 bits */
constexpr auto TEST4 =
    "01234567012345670123456701234567"
    "01234567012345670123456701234567"sv; /* times 10 */

constexpr auto TEST7_256 =
    "\xbe\x27\x46\xc6\xdb\x52\x76\x5f\xdb\x2f\x88\x70\x0f\x9a\x73"sv;
constexpr auto TEST8_256 =
    "\xe3\xd7\x25\x70\xdc\xdd\x78\x7c\xe3\x88\x7a\xb2\xcd\x68\x46\x52"sv;
constexpr auto TEST9_256 =
    "\x3e\x74\x03\x71\xc8\x10\xc2\xb9\x9f\xc0\x4e\x80\x49\x07\xef\x7c"
    "\xf2\x6b\xe2\x8b\x57\xcb\x58\xa3\xe2\xf3\xc0\x07\x16\x6e\x49\xc1"
    "\x2e\x9b\xa3\x4c\x01\x04\x06\x91\x29\xea\x76\x15\x64\x25\x45\x70"
    "\x3a\x2b\xd9\x01\xe1\x6e\xb0\xe0\x5d\xeb\xa0\x14\xeb\xff\x64\x06"
    "\xa0\x7d\x54\x36\x4e\xff\x74\x2d\xa7\x79\xb0\xb3"sv;
constexpr auto TEST10_256 =
    "\x83\x26\x75\x4e\x22\x77\x37\x2f\x4f\xc1\x2b\x20\x52\x7a\xfe\xf0"
    "\x4d\x8a\x05\x69\x71\xb1\x1a\xd5\x71\x23\xa7\xc1\x37\x76\x00\x00"
    "\xd7\xbe\xf6\xf3\xc1\xf7\xa9\x08\x3a\xa3\x9d\x81\x0d\xb3\x10\x77"
    "\x7d\xab\x8b\x1e\x7f\x02\xb8\x4a\x26\xc7\x73\x32\x5f\x8b\x23\x74"
    "\xde\x7a\x4b\x5a\x58\xcb\x5c\x5c\xf3\x5b\xce\xe6\xfb\x94\x6e\x5b"
    "\xd6\x94\xfa\x59\x3a\x8b\xeb\x3f\x9d\x65\x92\xec\xed\xaa\x66\xca"
    "\x82\xa2\x9d\x0c\x51\xbc\xf9\x33\x62\x30\xe5\xd7\x84\xe4\xc0\xa4"
    "\x3f\x8d\x79\xa3\x0a\x16\x5c\xba\xbe\x45\x2b\x77\x4b\x9c\x71\x09"
    "\xa9\x7d\x13\x8f\x12\x92\x28\x96\x6f\x6c\x0a\xdc\x10\x6a\xad\x5a"
    "\x9f\xdd\x30\x82\x57\x69\xb2\xc6\x71\xaf\x67\x59\xdf\x28\xeb\x39"
    "\x3d\x54\xd6"sv;

struct TestVector {
    std::string_view test_pattern; ///< テストパターン
    long repeat_count; ///< テストパターンの繰り返し回数
    std::byte extra_bits; ///< 追加ビット
    int num_of_extra_bits; ///< 追加ビットのビット数
    std::string_view expected; ///< 期待されるハッシュ値
};

constexpr TestVector TEST_VECTORS[] = {
    { TEST1, 1, std::byte(0), 0, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" },
    { TEST2_1, 1, std::byte(0), 0, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1" },
    { TEST3, 1000000, std::byte(0), 0, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0" },
    { TEST4, 10, std::byte(0), 0, "594847328451bdfa85056225462cc1d867d877fb388df0ce35f25ab5562bfbb5" },
    { ""sv, 0, std::byte(0x68), 5, "d6d3e02a31a84a8caa9718ed6c2057be09db45e7823eb5079ce7a573a3760f95" },
    { "\x19"sv, 1, std::byte(0), 0, "68aa2e2ee5dff96e3355e6c7ee373e3d6a4e17f75f9518d843709c0c9bc3e3d4" },
    { TEST7_256, 1, std::byte(0x60), 3, "77ec1dc89c821ff2a1279089fa091b35b8cd960bcaf7de01c6a7680756beb972" },
    { TEST8_256, 1, std::byte(0), 0, "175ee69b02ba9b58e2b0a5fd13819cea573f3940a94f825128cf4209beabb4e8" },
    { TEST9_256, 1, std::byte(0xA0), 3, "3e9ad6468bbbad2ac3c2cdc292e018ba5fd70b960cf1679777fce708fdb066e9" },
    { TEST10_256, 1, std::byte(0), 0, "97dbca7df46d62c8a422c941dd7e835b8ad3361763f7e9b2d95f4f0da6e1ccbc" },
};

}

TEST_CASE("SHA256 RFC 6234 test vectors")
{
    util::SHA256 hash;
    for (const auto &test : TEST_VECTORS) {
        // 失敗時にどのテストベクタで落ちたのかが分かるようにする
        CAPTURE(test.expected);

        hash.reset();
        const auto test_array = std::as_bytes(std::span(test.test_pattern));
        for (auto i = 0; i < test.repeat_count; ++i) {
            hash.update(test_array.data(), test_array.size());
        }
        hash.final_bits(test.extra_bits, test.num_of_extra_bits);

        CHECK(util::to_string(hash.digest()) == test.expected);
    }
}

TEST_CASE("SHA256 can be reused after reset")
{
    util::SHA256 hash;
    hash.update(std::string_view("abc"));
    const auto first = util::to_string(hash.digest());

    hash.reset();
    hash.update(std::string_view("abc"));

    CHECK(util::to_string(hash.digest()) == first);
}

TEST_CASE("SHA256 accepts a message split into multiple updates")
{
    util::SHA256 whole;
    whole.update(std::string_view("abcdef"));

    util::SHA256 split;
    split.update(std::string_view("abc"));
    split.update(std::string_view("def"));

    CHECK(util::to_string(split.digest()) == util::to_string(whole.digest()));
}
