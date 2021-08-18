#pragma once

#include "system/angband.h"

#define TOKENIZE_CHECKQUOTE 0x01  /* Special handling of single quotes */

short tokenize(char *buf, short num, char **tokens, BIT_FLAGS mode);
