#pragma once

#include "angband.h"

#define TOKENIZE_CHECKQUOTE 0x01  /* Special handling of single quotes */

s16b tokenize(char *buf, s16b num, char **tokens, BIT_FLAGS mode);
