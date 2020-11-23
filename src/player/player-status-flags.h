#include "player/player-status.h"

enum flag_cause {
    FLAG_CAUSE_INVEN_RARM = 0, /*!< アイテムスロット…右手 */
    FLAG_CAUSE_INVEN_LARM = 1, /*!< アイテムスロット…左手 */
    FLAG_CAUSE_INVEN_BOW = 2, /*!< アイテムスロット…射撃 */
    FLAG_CAUSE_INVEN_RIGHT = 3, /*!< アイテムスロット…右手指 */
    FLAG_CAUSE_INVEN_LEFT = 4, /*!< アイテムスロット…左手指 */
    FLAG_CAUSE_INVEN_NECK = 5, /*!< アイテムスロット…首 */
    FLAG_CAUSE_INVEN_LITE = 6, /*!< アイテムスロット…光源 */
    FLAG_CAUSE_INVEN_BODY = 7, /*!< アイテムスロット…体 */
    FLAG_CAUSE_INVEN_OUTER = 8, /*!< アイテムスロット…体の上 */
    FLAG_CAUSE_INVEN_HEAD = 9, /*!< アイテムスロット…頭部 */
    FLAG_CAUSE_INVEN_HANDS = 10, /*!< アイテムスロット…腕部 */
    FLAG_CAUSE_INVEN_FEET = 11, /*!< アイテムスロット…脚部 */
    FLAG_CAUSE_RACE = 12, /*!< 種族上の体得 */
    FLAG_CAUSE_CLASS = 13, /*!< 職業上の体得 */
    FLAG_CAUSE_PERSONALITY = 14, /*!< 性格上の体得 */
    FLAG_CAUSE_MAGIC_TIME_EFFECT = 15 /*!< 魔法による時限効果 */
};

bool have_pass_wall(player_type *creature_ptr);
bool have_kill_wall(player_type *creature_ptr);
BIT_FLAGS have_xtra_might(player_type *creature_ptr);
BIT_FLAGS have_esp_evil(player_type *creature_ptr);
BIT_FLAGS have_esp_animal(player_type *creature_ptr);
BIT_FLAGS have_esp_undead(player_type *creature_ptr);
BIT_FLAGS have_esp_demon(player_type *creature_ptr);
BIT_FLAGS have_esp_orc(player_type *creature_ptr);
BIT_FLAGS have_esp_troll(player_type *creature_ptr);
BIT_FLAGS have_esp_giant(player_type *creature_ptr);
BIT_FLAGS have_esp_dragon(player_type *creature_ptr);
void have_esp_human(player_type *creature_ptr);
void have_esp_good(player_type *creature_ptr);
void have_esp_nonliving(player_type *creature_ptr);
void have_esp_unique(player_type *creature_ptr);
void have_esp_telepathy(player_type *creature_ptr);
void have_bless_blade(player_type *creature_ptr);
void have_easy2_weapon(player_type *creature_ptr);
void have_down_saving(player_type *creature_ptr);
void have_no_ac(player_type *creature_ptr);
void have_no_flowed(player_type *creature_ptr);
void have_mighty_throw(player_type *creature_ptr);
void have_dec_mana(player_type *creature_ptr);
void have_reflect(player_type *creature_ptr);
void have_see_nocto(player_type *creature_ptr);
void have_warning(player_type *creature_ptr);
void have_anti_magic(player_type *creature_ptr);
void have_anti_tele(player_type *creature_ptr);
void have_sh_fire(player_type *creature_ptr);
void have_sh_elec(player_type *creature_ptr);
void have_sh_cold(player_type *creature_ptr);
void have_easy_spell(player_type *creature_ptr);
void have_heavy_spell(player_type *creature_ptr);
void have_hold_exp(player_type *creature_ptr);
void have_see_inv(player_type *creature_ptr);
void have_free_act(player_type *creature_ptr);
void have_sustain_str(player_type *creature_ptr);
void have_sustain_int(player_type *creature_ptr);
void have_sustain_wis(player_type *creature_ptr);
void have_sustain_dex(player_type *creature_ptr);
void have_sustain_con(player_type *creature_ptr);
void have_sustain_chr(player_type *creature_ptr);
void have_levitation(player_type *creature_ptr);
void have_can_swim(player_type *creature_ptr);
void have_slow_digest(player_type *creature_ptr);
void have_regenerate(player_type *creature_ptr);
void have_curses(player_type *creature_ptr);
void have_impact(player_type *creature_ptr);
void have_extra_blow(player_type *creature_ptr);
void have_resist_acid(player_type *creature_ptr);
void have_resist_elec(player_type *creature_ptr);
void have_resist_fire(player_type *creature_ptr);
void have_resist_cold(player_type *creature_ptr);
void have_resist_pois(player_type *creature_ptr);
void have_resist_conf(player_type *creature_ptr);
void have_resist_sound(player_type *creature_ptr);
void have_resist_lite(player_type *creature_ptr);
void have_resist_dark(player_type *creature_ptr);
void have_resist_chaos(player_type *creature_ptr);
void have_resist_disen(player_type *creature_ptr);
void have_resist_shard(player_type *creature_ptr);
void have_resist_nexus(player_type *creature_ptr);
void have_resist_blind(player_type *creature_ptr);
void have_resist_neth(player_type *creature_ptr);
void have_resist_time(player_type *creature_ptr);
void have_resist_water(player_type *creature_ptr);
void have_resist_fear(player_type *creature_ptr);
void have_immune_acid(player_type *creature_ptr);
void have_immune_elec(player_type *creature_ptr);
void have_immune_fire(player_type *creature_ptr);
void have_immune_cold(player_type *creature_ptr);
bool have_right_hand_weapon(player_type *creature_ptr);
bool have_left_hand_weapon(player_type *creature_ptr);
bool have_two_handed_weapons(player_type *creature_ptr);
void have_lite(player_type *creature_ptr);
bool is_disable_two_handed_bonus(player_type *creature_ptr, int i);
bool is_not_ninja_weapon(player_type *creature_ptr, int i);
bool is_not_monk_weapon(player_type *creature_ptr, int i);
bool is_icky_wield_weapon(player_type *creature_ptr, int i);
bool is_riding_wield_weapon(player_type *creature_ptr, int i);
bool have_good_luck(player_type *creature_ptr);
