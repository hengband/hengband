#include "player/player-status.h"

enum flag_cause {
    FLAG_CAUSE_INVEN_MAIN_HAND = 0, /*!< アイテムスロット…利手 */
    FLAG_CAUSE_INVEN_SUB_HAND = 1, /*!< アイテムスロット…逆手 */
    FLAG_CAUSE_INVEN_BOW = 2, /*!< アイテムスロット…射撃 */
    FLAG_CAUSE_INVEN_MAIN_RING = 3, /*!< アイテムスロット…利手指 */
    FLAG_CAUSE_INVEN_SUB_RING = 4, /*!< アイテムスロット…逆手指 */
    FLAG_CAUSE_INVEN_NECK = 5, /*!< アイテムスロット…首 */
    FLAG_CAUSE_INVEN_LITE = 6, /*!< アイテムスロット…光源 */
    FLAG_CAUSE_INVEN_BODY = 7, /*!< アイテムスロット…体 */
    FLAG_CAUSE_INVEN_OUTER = 8, /*!< アイテムスロット…体の上 */
    FLAG_CAUSE_INVEN_HEAD = 9, /*!< アイテムスロット…頭部 */
    FLAG_CAUSE_INVEN_ARMS = 10, /*!< アイテムスロット…腕部 */
    FLAG_CAUSE_INVEN_FEET = 11, /*!< アイテムスロット…脚部 */
    FLAG_CAUSE_RACE = 12, /*!< 種族上の体得 */
    FLAG_CAUSE_CLASS = 13, /*!< 職業上の体得 */
    FLAG_CAUSE_PERSONALITY = 14, /*!< 性格上の体得 */
    FLAG_CAUSE_MAGIC_TIME_EFFECT = 15, /*!< 魔法による時限効果 */
    FLAG_CAUSE_MUTATION = 16, /*!< 変異による効果 */
    FLAG_CAUSE_BATTLE_FORM = 17, /*!< 構えによる効果 */
    FLAG_CAUSE_MAX = 18
};

typedef enum melee_type {
    MELEE_TYPE_BAREHAND_TWO = 0,
    MELEE_TYPE_BAREHAND_MAIN = 1,
    MELEE_TYPE_BAREHAND_SUB = 2,
    MELEE_TYPE_WEAPON_MAIN = 3,
    MELEE_TYPE_WEAPON_SUB = 4,
    MELEE_TYPE_WEAPON_TWOHAND = 5,
    MELEE_TYPE_WEAPON_DOUBLE = 6,
    MELEE_TYPE_SHIELD_DOUBLE = 7
} melee_type;

enum aggravate_state {
    AGGRAVATE_NONE = 0x00000000L,
    AGGRAVATE_S_FAIRY = 0x00000001L,
    AGGRAVATE_NORMAL = 0x00000002L,
};

bool has_pass_wall(const player_type *creature_ptr);
bool has_kill_wall(const player_type *creature_ptr);
BIT_FLAGS has_xtra_might(const player_type *creature_ptr);
BIT_FLAGS has_infra_vision(const player_type *creature_ptr);
BIT_FLAGS has_esp_evil(const player_type *creature_ptr);
BIT_FLAGS has_esp_animal(const player_type *creature_ptr);
BIT_FLAGS has_esp_undead(const player_type *creature_ptr);
BIT_FLAGS has_esp_demon(const player_type *creature_ptr);
BIT_FLAGS has_esp_orc(const player_type *creature_ptr);
BIT_FLAGS has_esp_troll(const player_type *creature_ptr);
BIT_FLAGS has_esp_giant(const player_type *creature_ptr);
BIT_FLAGS has_esp_dragon(const player_type *creature_ptr);
BIT_FLAGS has_esp_human(const player_type *creature_ptr);
BIT_FLAGS has_esp_good(const player_type *creature_ptr);
BIT_FLAGS has_esp_nonliving(const player_type *creature_ptr);
BIT_FLAGS has_esp_unique(const player_type *creature_ptr);
BIT_FLAGS has_esp_telepathy(const player_type *creature_ptr);
BIT_FLAGS has_bless_blade(const player_type *creature_ptr);
BIT_FLAGS has_easy2_weapon(const player_type *creature_ptr);
BIT_FLAGS has_down_saving(const player_type *creature_ptr);
BIT_FLAGS has_no_ac(const player_type *creature_ptr);
BIT_FLAGS has_invuln_arrow(const player_type *creature_ptr);
void check_no_flowed(player_type *creature_ptr);
BIT_FLAGS has_mighty_throw(const player_type *creature_ptr);
BIT_FLAGS has_dec_mana(const player_type *creature_ptr);
BIT_FLAGS has_reflect(const player_type *creature_ptr);
BIT_FLAGS has_see_nocto(const player_type *creature_ptr);
BIT_FLAGS has_warning(const player_type *creature_ptr);
BIT_FLAGS has_anti_magic(const player_type *creature_ptr);
BIT_FLAGS has_anti_tele(const player_type *creature_ptr);
BIT_FLAGS has_sh_fire(const player_type *creature_ptr);
BIT_FLAGS has_sh_elec(const player_type *creature_ptr);
BIT_FLAGS has_sh_cold(const player_type *creature_ptr);
BIT_FLAGS has_easy_spell(const player_type *creature_ptr);
BIT_FLAGS has_heavy_spell(const player_type *creature_ptr);
BIT_FLAGS has_hold_exp(const player_type *creature_ptr);
BIT_FLAGS has_see_inv(const player_type *creature_ptr);
BIT_FLAGS has_magic_mastery(const player_type *creature_ptr);
BIT_FLAGS has_free_act(const player_type *creature_ptr);
BIT_FLAGS has_sustain_str(const player_type *creature_ptr);
BIT_FLAGS has_sustain_int(const player_type *creature_ptr);
BIT_FLAGS has_sustain_wis(const player_type *creature_ptr);
BIT_FLAGS has_sustain_dex(const player_type *creature_ptr);
BIT_FLAGS has_sustain_con(const player_type *creature_ptr);
BIT_FLAGS has_sustain_chr(const player_type *creature_ptr);
BIT_FLAGS has_levitation(const player_type *creature_ptr);
bool has_can_swim(const player_type *creature_ptr);
BIT_FLAGS has_slow_digest(const player_type *creature_ptr);
BIT_FLAGS has_regenerate(const player_type *creature_ptr);
void update_curses(player_type *creature_ptr);
BIT_FLAGS has_impact(const player_type *creature_ptr);
void update_extra_blows(player_type *creature_ptr);
BIT_FLAGS has_resist_acid(const player_type *creature_ptr);
BIT_FLAGS has_vuln_acid(const player_type *creature_ptr);
BIT_FLAGS has_resist_elec(const player_type *creature_ptr);
BIT_FLAGS has_vuln_elec(const player_type *creature_ptr);
BIT_FLAGS has_resist_fire(const player_type *creature_ptr);
BIT_FLAGS has_vuln_fire(const player_type *creature_ptr);
BIT_FLAGS has_resist_cold(const player_type *creature_ptr);
BIT_FLAGS has_vuln_cold(const player_type *creature_ptr);
BIT_FLAGS has_resist_pois(const player_type *creature_ptr);
BIT_FLAGS has_resist_conf(const player_type *creature_ptr);
BIT_FLAGS has_resist_sound(const player_type *creature_ptr);
BIT_FLAGS has_resist_lite(const player_type *creature_ptr);
BIT_FLAGS has_vuln_lite(const player_type *creature_ptr);
BIT_FLAGS has_resist_dark(const player_type *creature_ptr);
BIT_FLAGS has_resist_chaos(const player_type *creature_ptr);
BIT_FLAGS has_resist_disen(const player_type *creature_ptr);
BIT_FLAGS has_resist_shard(const player_type *creature_ptr);
BIT_FLAGS has_resist_nexus(const player_type *creature_ptr);
BIT_FLAGS has_resist_blind(const player_type *creature_ptr);
BIT_FLAGS has_resist_neth(const player_type *creature_ptr);
BIT_FLAGS has_resist_time(const player_type *creature_ptr);
BIT_FLAGS has_resist_water(const player_type *creature_ptr);
BIT_FLAGS has_resist_fear(const player_type *creature_ptr);
BIT_FLAGS has_immune_acid(const player_type *creature_ptr);
BIT_FLAGS has_immune_elec(const player_type *creature_ptr);
BIT_FLAGS has_immune_fire(const player_type *creature_ptr);
BIT_FLAGS has_immune_cold(const player_type *creature_ptr);
BIT_FLAGS has_immune_dark(const player_type *creature_ptr);
bool can_attack_with_main_hand(const player_type *creature_ptr);
bool can_attack_with_sub_hand(const player_type *creature_ptr);
bool has_two_handed_weapons(const player_type *creature_ptr);
BIT_FLAGS has_lite(const player_type *creature_ptr);
bool has_disable_two_handed_bonus(const player_type *creature_ptr, int i);
bool has_not_ninja_weapon(const player_type *creature_ptr, int i);
bool has_not_monk_weapon(const player_type *creature_ptr, int i);
bool has_icky_wield_weapon(const player_type *creature_ptr, int i);
bool has_riding_wield_weapon(const player_type *creature_ptr, int i);
bool has_good_luck(const player_type *creature_ptr);
BIT_FLAGS player_aggravate_state(const player_type *creature_ptr);
melee_type player_melee_type(const player_type *creature_ptr);
bool has_aggravate(const player_type *creature_ptr);
