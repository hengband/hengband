#pragma once

#include "system/angband.h"

/*!
 * @brief 銘情報の最大数 / Maximum number of "quarks" (see "io.c")
 * @note
 * Default: assume at most 512 different inscriptions are used<br>
 * Was 512... 256 quarks added for random artifacts<br>
 */
#define QUARK_MAX 768

extern STR_OFFSET quark__num;
extern concptr *quark__str;

concptr quark_str(STR_OFFSET num);
void quark_init(void);
uint16_t quark_add(concptr str);
