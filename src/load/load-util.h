#pragma once

#include "system/angband.h"

extern FILE *loading_savefile;
extern uint loading_savefile_version;
extern byte load_xor_byte;
extern uint v_check;
extern uint x_check;
extern byte kanji_code;

void load_note(concptr msg);
byte sf_get(void);
void rd_byte(byte *ip);
void rd_u16b(ushort *ip);
void rd_s16b(short *ip);
void rd_u32b(uint *ip);
void rd_s32b(int *ip);
void rd_string(char *str, int max);
void strip_bytes(int n);
bool loading_savefile_version_is_older_than(uint version);
