#pragma once

class PlayerType;
int staff_effect(PlayerType *player_ptr, int sval, bool *use_charge, bool powerful, bool magic, bool known);
void do_cmd_use_staff(PlayerType *player_ptr);
