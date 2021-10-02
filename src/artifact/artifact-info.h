/*!
 * @file artifact-info.h
 * @brief アーティファクトの発動効果取得関数ヘッダ
 */

#pragma once

#include <optional>

struct activation_type;
struct object_type;;
int activation_index(const object_type *o_ptr);
std::optional<const activation_type *> find_activation_info(const object_type *o_ptr);
