
static const concptr s_dam = _("損傷:", "dam ");
static const concptr s_random = _("ランダム", "random");
static const concptr s_rng = _("射程", "rng ");
static const concptr s_dur = _("期間:", "dur ");
static const concptr s_range = _("範囲:", "range ");
static const concptr s_heal = _("回復:", "heal ");

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
extern void do_cmd_browse(void);
extern void do_cmd_study(void);
extern void do_cmd_cast(void);
