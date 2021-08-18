#pragma once

#include "system/angband.h"
extern short signal_count;

extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);
