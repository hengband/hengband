#pragma once

#include "system/angband.h"
extern int16_t signal_count;

extern void signals_ignore_tstp();
extern void signals_handle_tstp();
extern void signals_init();
