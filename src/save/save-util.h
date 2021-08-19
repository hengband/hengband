﻿#pragma once

#include "system/angband.h"

extern FILE *saving_savefile;
extern byte save_xor_byte;
extern u32b v_stamp;
extern u32b x_stamp;

void wr_byte(byte v);
void wr_u16b(ushort v);
void wr_s16b(short v);
void wr_u32b(u32b v);
void wr_s32b(int v);
void wr_string(concptr str);
