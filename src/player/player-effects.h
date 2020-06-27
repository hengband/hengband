#pragma once

#include "system/angband.h"

typedef struct kamae {
	concptr desc;       /* A verbose kamae description */
	PLAYER_LEVEL min_level;  /* Minimum level to use */
	concptr info;
} kamae;

extern const kamae kamae_shurui[MAX_KAMAE];
extern const kamae kata_shurui[MAX_KATA];

void set_action(player_type *creature_ptr, ACTION_IDX typ);
void reset_tim_flags(player_type *creature_ptr);
void dispel_player(player_type *creature_ptr);
bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, MIMIC_RACE_IDX p, bool do_dec);
bool set_blind(player_type *creature_ptr, TIME_EFFECT v);
bool set_confused(player_type *creature_ptr, TIME_EFFECT v);
bool set_poisoned(player_type *creature_ptr, TIME_EFFECT v);
bool set_afraid(player_type *creature_ptr, TIME_EFFECT v);
bool set_paralyzed(player_type *creature_ptr, TIME_EFFECT v);
bool set_image(player_type *creature_ptr, TIME_EFFECT v);
bool set_fast(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_slow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_shield(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tsubureru(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_magicdef(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_blessed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_hero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_shero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_protevil(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_invuln(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_invis(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_infra(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_regen(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_stealth(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_lightspeed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_levitation(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_sh_touki(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_sh_fire(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_sh_holy(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_eyeeye(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_resist_magic(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_reflect(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_multishadow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_dustrobe(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_kabenuke(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tsuyoshi(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_ele_attack(player_type *creature_ptr, u32b attack_type, TIME_EFFECT v);
bool set_ele_immune(player_type *creature_ptr, u32b immune_type, TIME_EFFECT v);
bool set_oppose_acid(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_elec(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_fire(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_cold(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_pois(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_stun(player_type *creature_ptr, TIME_EFFECT v);
bool set_cut(player_type *creature_ptr, TIME_EFFECT v);
bool set_food(player_type *creature_ptr, TIME_EFFECT v);
bool inc_stat(player_type *creature_ptr, int stat);
bool dec_stat(player_type *creature_ptr, int stat, int amount, int permanent);
bool res_stat(player_type *creature_ptr, int stat);
bool hp_player(player_type *creature_ptr, int num);
bool do_dec_stat(player_type *creature_ptr, int stat);
bool do_res_stat(player_type *creature_ptr, int stat);
bool do_inc_stat(player_type *creature_ptr, int stat);
bool restore_level(player_type *creature_ptr);
bool lose_all_info(player_type *creature_ptr);
void gain_exp_64(player_type *creature_ptr, s32b amount, u32b amount_frac);
void gain_exp(player_type *creature_ptr, s32b amount);
void calc_android_exp(player_type *creature_ptr);
void lose_exp(player_type *creature_ptr, s32b amount);
bool drain_exp(player_type *creature_ptr, s32b drain, s32b slip, int hold_exp_prob);
void do_poly_self(player_type *creature_ptr);
bool set_ultimate_res(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_nether(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_time(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool choose_ele_attack(player_type *creature_ptr);
bool choose_ele_immune(player_type *creature_ptr, TIME_EFFECT turn);
bool set_wraith_form(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_esp(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_superstealth(player_type *creature_ptr, bool set);
void do_poly_wounds(player_type *creature_ptr);
void change_race(player_type *creature_ptr, player_race_type new_race, concptr effect_msg);
bool drop_weapons(player_type *creature_ptr);

void calc_timelimit_status(player_type *creature_ptr);
