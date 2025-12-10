#pragma once

#include "system/angband.h"

class PlayerType;
bool set_tim_sh_holy(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_eyeeye(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
void check_emission(PlayerType *player_ptr);
void check_demigod(PlayerType *player_ptr);
bool has_slay_demon_from_exorcism(const PlayerType *player_ptr);
bool has_kill_demon_from_exorcism(const PlayerType *player_ptr);
bool has_slay_undead_from_exorcism(const PlayerType *player_ptr);
bool has_kill_undead_from_exorcism(const PlayerType *player_ptr);
const auto THRESHOLD_KILL_FROM_EXORCISM = 45;
