#pragma once

#include "system/angband.h"
#ifdef JP
#include <fmt/base.h>
#include <string_view>
#endif

// 幻覚時のジョークメッセージ最大数 / Hallucination stuff.
constexpr auto MAX_SILLY_ATTACK = _(41, 29);

extern const concptr silly_attacks[MAX_SILLY_ATTACK];
#ifdef JP
extern const fmt::format_string<std::string_view> silly_attacks2[MAX_SILLY_ATTACK];
#endif
