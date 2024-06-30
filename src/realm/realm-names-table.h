/*!
 * @brief 魔法領域に関する関数マクロ群
 * @date 2021/10/11
 * @author Hourier
 * @todo 後日解体する
 */

#pragma once

#include "locale/localized-string.h"
#include "realm/realm-types.h"
#include "system/angband.h"
#include "util/enum-converter.h"
#include <vector>

constexpr auto VALID_REALM = std::ssize(MAGIC_REALM_RANGE) + std::ssize(TECHNIC_REALM_RANGE);
#define is_magic(A) (((A) > REALM_NONE) && ((A) <= MAX_MAGIC))

enum class ItemKindType : short;
#define tval2realm(A) (i2enum<magic_realm_type>((A)-ItemKindType::LIFE_BOOK + 1))
#define technic2magic(A) (is_magic(A) ? (A) : (A)-MIN_TECHNIC + 1 + MAX_MAGIC)
