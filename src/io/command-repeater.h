#pragma once

#include "system/angband.h"

void repeat_push(COMMAND_CODE what);
bool repeat_pull(COMMAND_CODE *what);
void repeat_check(void);
