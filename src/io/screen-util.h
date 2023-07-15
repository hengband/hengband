#pragma once

#include "system/angband.h"

void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p);
void resize_map(void);
bool panel_contains(POSITION y, POSITION x);
