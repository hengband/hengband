/*!
 * @file artifact-info.h
 * @brief アーティファクトの発動効果取得関数ヘッダ
 */

#pragma once

#include <optional>

enum class RandomArtActType : short;
struct activation_type;
struct object_type;
RandomArtActType activation_index(const object_type *o_ptr);
std::optional<const activation_type *> find_activation_info(const object_type *o_ptr);
