#pragma once

typedef struct kamae kamae;

struct kamae
{
	concptr desc;       /* A verbose kamae description */
	PLAYER_LEVEL min_level;  /* Minimum level to use */
	concptr info;
};

/* effects.c */

extern void set_action(ACTION_IDX typ);
extern void reset_tim_flags(void);
extern void dispel_player(void);
extern bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, IDX p, bool do_dec);
extern bool set_blind(player_type *creature_ptr, TIME_EFFECT v);
extern bool set_confused(player_type *creature_ptr, TIME_EFFECT v);
extern bool set_poisoned(player_type *creature_ptr, TIME_EFFECT v);
extern bool set_afraid(player_type *creature_ptr, TIME_EFFECT v);
extern bool set_paralyzed(player_type *creature_ptr, TIME_EFFECT v);
extern bool set_image(TIME_EFFECT v);
extern bool set_fast(TIME_EFFECT v, bool do_dec);
extern bool set_slow(TIME_EFFECT v, bool do_dec);
extern bool set_shield(TIME_EFFECT v, bool do_dec);
extern bool set_tsubureru(TIME_EFFECT v, bool do_dec);
extern bool set_magicdef(TIME_EFFECT v, bool do_dec);
extern bool set_blessed(TIME_EFFECT v, bool do_dec);
extern bool set_hero(TIME_EFFECT v, bool do_dec);
extern bool set_shero(TIME_EFFECT v, bool do_dec);
extern bool set_protevil(TIME_EFFECT v, bool do_dec);
extern bool set_invuln(TIME_EFFECT v, bool do_dec);
extern bool set_tim_invis(TIME_EFFECT v, bool do_dec);
extern bool set_tim_infra(TIME_EFFECT v, bool do_dec);
extern bool set_tim_regen(TIME_EFFECT v, bool do_dec);
extern bool set_tim_stealth(TIME_EFFECT v, bool do_dec);
extern bool set_lightspeed(TIME_EFFECT v, bool do_dec);
extern bool set_tim_levitation(TIME_EFFECT v, bool do_dec);
extern bool set_tim_sh_touki(TIME_EFFECT v, bool do_dec);
extern bool set_tim_sh_fire(TIME_EFFECT v, bool do_dec);
extern bool set_tim_sh_holy(TIME_EFFECT v, bool do_dec);
extern bool set_tim_eyeeye(TIME_EFFECT v, bool do_dec);
extern bool set_resist_magic(TIME_EFFECT v, bool do_dec);
extern bool set_tim_reflect(TIME_EFFECT v, bool do_dec);
extern bool set_multishadow(TIME_EFFECT v, bool do_dec);
extern bool set_dustrobe(TIME_EFFECT v, bool do_dec);
extern bool set_kabenuke(TIME_EFFECT v, bool do_dec);
extern bool set_tsuyoshi(TIME_EFFECT v, bool do_dec);
extern bool set_ele_attack(u32b attack_type, TIME_EFFECT v);
extern bool set_ele_immune(u32b immune_type, TIME_EFFECT v);
extern bool set_oppose_acid(TIME_EFFECT v, bool do_dec);
extern bool set_oppose_elec(TIME_EFFECT v, bool do_dec);
extern bool set_oppose_fire(TIME_EFFECT v, bool do_dec);
extern bool set_oppose_cold(TIME_EFFECT v, bool do_dec);
extern bool set_oppose_pois(TIME_EFFECT v, bool do_dec);
extern bool set_stun(TIME_EFFECT v);
extern bool set_cut(TIME_EFFECT v);
extern bool set_food(TIME_EFFECT v);
extern bool inc_stat(player_type *creature_ptr, int stat);
extern bool dec_stat(player_type *creature_ptr, int stat, int amount, int permanent);
extern bool res_stat(player_type *creature_ptr, int stat);
extern bool hp_player(player_type *creature_ptr, int num);
extern bool do_dec_stat(player_type *creature_ptr, int stat);
extern bool do_res_stat(player_type *creature_ptr, int stat);
extern bool do_inc_stat(int stat);
extern bool restore_level(void);
extern bool lose_all_info(void);
extern void gain_exp_64(s32b amount, u32b amount_frac);
extern void gain_exp(s32b amount);
extern void calc_android_exp(void);
extern void lose_exp(s32b amount);
extern bool drain_exp(s32b drain, s32b slip, int hold_exp_prob);
extern void do_poly_self(void);
extern bool set_ultimate_res(TIME_EFFECT v, bool do_dec);
extern bool set_tim_res_nether(TIME_EFFECT v, bool do_dec);
extern bool set_tim_res_time(TIME_EFFECT v, bool do_dec);
extern bool choose_ele_attack(void);
extern bool choose_ele_immune(TIME_EFFECT turn);
extern bool set_wraith_form(TIME_EFFECT v, bool do_dec);
extern bool set_tim_esp(TIME_EFFECT v, bool do_dec);
extern bool set_superstealth(bool set);
extern void do_poly_wounds(void);
extern void change_race(CHARACTER_IDX new_race, concptr effect_msg);

extern const kamae kamae_shurui[MAX_KAMAE];
extern const kamae kata_shurui[MAX_KATA];
