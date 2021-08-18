#pragma once

#include "system/angband.h"

extern FILE *saving_savefile;
extern byte save_xor_byte;
extern uint v_stamp;
extern uint x_stamp;

void wr_byte(byte v);
void wr_u16b(ushort v);
void wr_s16b(short v);
void wr_u32b(uint v);
void wr_s32b(int v);
void wr_string(concptr str);
