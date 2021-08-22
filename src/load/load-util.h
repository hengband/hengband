#pragma once

#include "system/angband.h"

extern FILE *loading_savefile;
extern uint32_t loading_savefile_version;
extern byte load_xor_byte;
extern uint32_t v_check;
extern uint32_t x_check;
extern byte kanji_code;

void load_note(concptr msg);
byte sf_get(void);
void rd_byte(byte *ip);
void rd_u16b(uint16_t *ip);
void rd_s16b(int16_t *ip);
void rd_u32b(uint32_t *ip);
void rd_s32b(int32_t *ip);
void rd_string(char *str, int max);
void strip_bytes(int n);
bool loading_savefile_version_is_older_than(uint32_t version);
