#pragma once

#include "system/angband.h"

extern FILE *saving_savefile;
extern byte save_xor_byte;
extern uint32_t v_stamp;
extern uint32_t x_stamp;

void wr_byte(byte v);
void wr_u16b(uint16_t v);
void wr_s16b(int16_t v);
void wr_u32b(uint32_t v);
void wr_s32b(int32_t v);
void wr_string(concptr str);
