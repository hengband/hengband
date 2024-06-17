#pragma once

#include "system/h-basic.h"

#ifdef JP
#define _(JAPANESE, ENGLISH) (JAPANESE)
#else
#define _(JAPANESE, ENGLISH) (ENGLISH)
#endif

//!< 普通のマクロ関数と違って展開後のカッコがないので注意.
#ifdef JP
#define N(JAPANESE, ENGLISH) JAPANESE, ENGLISH
#else
#define N(JAPANESE, ENGLISH) ENGLISH
#endif
