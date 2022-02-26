#pragma once

#include "system/angband.h"

#define TOKENIZE_CHECKQUOTE 0x01 /* Special handling of single quotes */

int16_t tokenize(char *buf, int16_t num, char **tokens, BIT_FLAGS mode);
