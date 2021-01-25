#pragma once

#include "system/angband.h"

extern concptr KWD_DAM;
extern concptr KWD_RANGE;
extern concptr KWD_DURATION;
extern concptr KWD_SPHERE;
extern concptr KWD_HEAL;
extern concptr KWD_RANDOM;

extern const u32b fake_spell_flags[4];

concptr info_string_dice(concptr str, DICE_NUMBER dice, DICE_SID sides, int base);
concptr info_damage(DICE_NUMBER dice, DICE_SID sides, int base);
concptr info_duration(int base, DICE_SID sides);
concptr info_range(POSITION range);
concptr info_heal(DICE_NUMBER dice, DICE_SID sides, int base);
concptr info_delay(int base, DICE_SID sides);
concptr info_multi_damage(HIT_POINT dam);
concptr info_multi_damage_dice(DICE_NUMBER dice, DICE_SID sides);
concptr info_power(int power);
concptr info_power_dice(DICE_NUMBER dice, DICE_SID sides);
concptr info_radius(POSITION rad);
concptr info_weight(WEIGHT weight);

void do_cmd_browse(player_type *caster_ptr);
void do_cmd_study(player_type *caster_ptr);
void do_cmd_cast(player_type *caster_ptr);
