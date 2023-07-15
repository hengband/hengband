#pragma once

#include "system/angband.h"

// 幻覚時のジョークメッセージ最大数 / Hallucination stuff.
#define MAX_SILLY_ATTACK _(41, 29)

extern const concptr silly_attacks[MAX_SILLY_ATTACK];
#ifdef JP
extern const concptr silly_attacks2[MAX_SILLY_ATTACK];
#endif
