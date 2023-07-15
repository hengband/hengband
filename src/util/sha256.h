/*!
 * @brief SHA-256ハッシュ値計算クラスの宣言
 */

/***************** See RFC 6234 for details. *******************/
/*
   Copyright (c) 2011 IETF Trust and the persons identified as
   authors of the code.  All rights reserved.

   Redistribution and use in source and binary forms, with or
   without modification, are permitted provided that the following
   conditions are met:

   - Redistributions of source code must retain the above
     copyright notice, this list of conditions and
     the following disclaimer.

   - Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.

   - Neither the name of Internet Society, IETF or IETF Trust, nor
     the names of specific contributors, may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace util {

/*!
 * @brief SHA-256ハッシュ値計算クラス
 */
class SHA256 {
public:
    static constexpr auto DIGEST_SIZE = 32; ///< ハッシュ値のバイト数
    static constexpr auto BLOCK_SIZE = 64; ///< ブロックサイズ
    using Digest = std::array<std::byte, DIGEST_SIZE>; ///< ハッシュ値の型

    SHA256();
    ~SHA256();
    SHA256(const SHA256 &) = delete;
    SHA256 &operator=(const SHA256 &) = delete;
    SHA256(SHA256 &&) = delete;
    SHA256 &operator=(SHA256 &&) = delete;

    void reset();
    void update(const std::byte *message_array, size_t length);
    void update(std::string_view message);
    void final_bits(std::byte message_bits, size_t length);
    Digest digest();

    static std::optional<Digest> compute_filehash(const std::filesystem::path &path);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

std::string to_string(const SHA256::Digest &hash);
}
