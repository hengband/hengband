#pragma once

#include "system/angband.h"

FILE *saving_savefile;
byte save_xor_byte;
u32b v_stamp;
u32b x_stamp;

void wr_byte(byte v);
void wr_u16b(u16b v);
void wr_s16b(s16b v);
void wr_u32b(u32b v);
void wr_s32b(s32b v);
void wr_string(concptr str);
