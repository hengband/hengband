#pragma once

struct ae_type;
class CapturedMonsterType;
class PlayerType;
bool exe_monster_capture(PlayerType *player_ptr, ae_type *ae_ptr, CapturedMonsterType *cap_mon_ptr);
