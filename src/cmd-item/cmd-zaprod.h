#pragma once

#include "system/angband.h"

class PlayerType;
int rod_effect(PlayerType *player_ptr, OBJECT_SUBTYPE_VALUE sval, DIRECTION dir, bool *use_charge, bool powerful);
void do_cmd_zap_rod(PlayerType *player_ptr);
