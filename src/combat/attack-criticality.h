#pragma once

HIT_POINT critical_norm(player_type *attacker_ptr, WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, combat_options mode);
int calc_monster_critical(DICE_NUMBER dice, DICE_SID sides, HIT_POINT dam);
