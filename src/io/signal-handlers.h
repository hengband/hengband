#pragma once

#include "system/angband.h"
extern int16_t signal_count;

extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);
