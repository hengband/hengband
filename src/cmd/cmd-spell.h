#pragma once

#define KWD_DAM      _("損傷:", "dam ")
#define KWD_RANGE    _("射程:", "rng ")
#define KWD_DURATION _("期間:", "dur ")
#define KWD_SPHERE   _("範囲:", "range ")
#define KWD_HEAL     _("回復:", "heal ")
#define KWD_RANDOM   _("ランダム", "random")

extern const u32b fake_spell_flags[4];


#define SPELL_NAME   0
#define SPELL_DESC   1
#define SPELL_INFO   2
#define SPELL_CAST   3
#define SPELL_FAIL   4
#define SPELL_STOP   5
#define SPELL_CONT   6
extern concptr exe_spell(player_type *caster_ptr, REALM_IDX realm, SPELL_IDX spell, BIT_FLAGS mode);

extern concptr info_string_dice(concptr str, DICE_NUMBER dice, DICE_SID sides, int base);
extern concptr info_damage(DICE_NUMBER dice, DICE_SID sides, int base);
extern concptr info_duration(int base, DICE_SID sides);
extern concptr info_range(POSITION range);
extern concptr info_heal(DICE_NUMBER dice, DICE_SID sides, int base);
extern concptr info_delay(int base, DICE_SID sides);
extern concptr info_multi_damage(HIT_POINT dam);
extern concptr info_multi_damage_dice(DICE_NUMBER dice, DICE_SID sides);
extern concptr info_power(int power);
extern concptr info_power_dice(DICE_NUMBER dice, DICE_SID sides);
extern concptr info_radius(POSITION rad);
extern concptr info_weight(WEIGHT weight);

/* cmd5.c */
extern void do_cmd_browse(player_type *caster_ptr);
extern void do_cmd_study(player_type *caster_ptr);
extern void do_cmd_cast(player_type *caster_ptr);
