#pragma once

#include "system/angband.h"

#define KWD_DAM _("損傷:", "dam ")
#define KWD_RANGE    _("射程:", "rng ")
#define KWD_DURATION _("期間:", "dur ")
#define KWD_SPHERE   _("範囲:", "range ")
#define KWD_HEAL     _("回復:", "heal ")
#define KWD_RANDOM   _("ランダム", "random")

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
