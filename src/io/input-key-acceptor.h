#pragma once

#include "system/angband.h"

/*
 * Special key code used for inkey_special()
 */
#define SKEY_MOD_MASK 0x0f00
#define SKEY_MOD_SHIFT 0x0100
#define SKEY_MOD_CONTROL 0x0200

#define SKEY_MASK 0xf000
#define SKEY_DOWN 0xf001
#define SKEY_LEFT 0xf002
#define SKEY_RIGHT 0xf003
#define SKEY_UP 0xf004
#define SKEY_PGUP 0xf005
#define SKEY_PGDOWN 0xf006
#define SKEY_TOP 0xf007
#define SKEY_BOTTOM 0xf008

extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;

extern int num_more;
extern concptr inkey_next;

char inkey(bool do_all_term_refresh = false);
int inkey_special(bool numpad_cursor);
void start_term_fresh(void);
void stop_term_fresh(void);
bool macro_running(void);
