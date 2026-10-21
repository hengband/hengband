#pragma once

#include "system/angband.h"
#include <string>

/*
 * Special key code used for inkey_special()
 */
constexpr auto SKEY_MOD_MASK = 0x0f00;
constexpr auto SKEY_MOD_SHIFT = 0x0100;
constexpr auto SKEY_MOD_CONTROL = 0x0200;

constexpr auto SKEY_MASK = 0xf000;
constexpr auto SKEY_DOWN = 0xf001;
constexpr auto SKEY_LEFT = 0xf002;
constexpr auto SKEY_RIGHT = 0xf003;
constexpr auto SKEY_UP = 0xf004;
constexpr auto SKEY_PGUP = 0xf005;
constexpr auto SKEY_PGDOWN = 0xf006;
constexpr auto SKEY_HOME = 0xf007;
constexpr auto SKEY_END = 0xf008;

//! マクロのトリガーとして扱うキー列の最大の長さ
constexpr size_t MAX_MACRO_TRIGGER_LENGTH = 1023;

extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;

extern int num_more;
extern concptr inkey_next;

char inkey(bool do_all_term_refresh = false);
int inkey_special(bool numpad_cursor);
std::string inkey_macro_trigger();
void fresh_all_terms();
void start_term_fresh();
void stop_term_fresh();
bool macro_running();
