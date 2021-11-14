#pragma once

#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "system/angband.h"

enum flag_cause : uint32_t {
    FLAG_CAUSE_NONE = 0x0U,
    FLAG_CAUSE_INVEN_MAIN_HAND = 0x01U << 0, /*!< アイテムスロット…利手 */
    FLAG_CAUSE_INVEN_SUB_HAND = 0x01U << 1, /*!< アイテムスロット…逆手 */
    FLAG_CAUSE_INVEN_BOW = 0x01U << 2, /*!< アイテムスロット…射撃 */
    FLAG_CAUSE_INVEN_MAIN_RING = 0x01U << 3, /*!< アイテムスロット…利手指 */
    FLAG_CAUSE_INVEN_SUB_RING = 0x01U << 4, /*!< アイテムスロット…逆手指 */
    FLAG_CAUSE_INVEN_NECK = 0x01U << 5, /*!< アイテムスロット…首 */
    FLAG_CAUSE_INVEN_LITE = 0x01U << 6, /*!< アイテムスロット…光源 */
    FLAG_CAUSE_INVEN_BODY = 0x01U << 7, /*!< アイテムスロット…体 */
    FLAG_CAUSE_INVEN_OUTER = 0x01U << 8, /*!< アイテムスロット…体の上 */
    FLAG_CAUSE_INVEN_HEAD = 0x01U << 9, /*!< アイテムスロット…頭部 */
    FLAG_CAUSE_INVEN_ARMS = 0x01U << 10, /*!< アイテムスロット…腕部 */
    FLAG_CAUSE_INVEN_FEET = 0x01U << 11, /*!< アイテムスロット…脚部 */
    FLAG_CAUSE_RACE = 0x01U << 12, /*!< 種族上の体得 */
    FLAG_CAUSE_CLASS = 0x01U << 13, /*!< 職業上の体得 */
    FLAG_CAUSE_PERSONALITY = 0x01U << 14, /*!< 性格上の体得 */
    FLAG_CAUSE_MAGIC_TIME_EFFECT = 0x01U << 15, /*!< 魔法による時限効果 */
    FLAG_CAUSE_MUTATION = 0x01U << 16, /*!< 変異による効果 */
    FLAG_CAUSE_STANCE = 0x01U << 17, /*!< 構えによる効果 */
    FLAG_CAUSE_RIDING = 0x01U << 18, /*!< 乗馬による効果 */
    FLAG_CAUSE_INVEN_PACK = 0x01U << 19, /*!< その他インベントリによる効果 重量超過等 */
    FLAG_CAUSE_ACTION = 0x01U << 20, /*!< ACTIONによる効果 探索モード等 */
    FLAG_CAUSE_MAX = 0x01U << 21
};

enum melee_type {
    MELEE_TYPE_BAREHAND_TWO = 0,
    MELEE_TYPE_BAREHAND_MAIN = 1,
    MELEE_TYPE_BAREHAND_SUB = 2,
    MELEE_TYPE_WEAPON_MAIN = 3,
    MELEE_TYPE_WEAPON_SUB = 4,
    MELEE_TYPE_WEAPON_TWOHAND = 5,
    MELEE_TYPE_WEAPON_DOUBLE = 6,
    MELEE_TYPE_SHIELD_DOUBLE = 7
};

enum aggravate_state {
    AGGRAVATE_NONE = 0x00000000L,
    AGGRAVATE_S_FAIRY = 0x00000001L,
    AGGRAVATE_NORMAL = 0x00000002L,
};

class PlayerType;
BIT_FLAGS convert_inventory_slot_type_to_flag_cause(inventory_slot_type inventory_slot);
BIT_FLAGS check_equipment_flags(PlayerType *player_ptr, tr_type tr_flag);
BIT_FLAGS get_player_flags(PlayerType *player_ptr, tr_type tr_flag);
bool has_pass_wall(PlayerType *player_ptr);
bool has_kill_wall(PlayerType *player_ptr);
BIT_FLAGS has_xtra_might(PlayerType *player_ptr);
BIT_FLAGS has_esp_evil(PlayerType *player_ptr);
BIT_FLAGS has_esp_animal(PlayerType *player_ptr);
BIT_FLAGS has_esp_undead(PlayerType *player_ptr);
BIT_FLAGS has_esp_demon(PlayerType *player_ptr);
BIT_FLAGS has_esp_orc(PlayerType *player_ptr);
BIT_FLAGS has_esp_troll(PlayerType *player_ptr);
BIT_FLAGS has_esp_giant(PlayerType *player_ptr);
BIT_FLAGS has_esp_dragon(PlayerType *player_ptr);
BIT_FLAGS has_esp_human(PlayerType *player_ptr);
BIT_FLAGS has_esp_good(PlayerType *player_ptr);
BIT_FLAGS has_esp_nonliving(PlayerType *player_ptr);
BIT_FLAGS has_esp_unique(PlayerType *player_ptr);
BIT_FLAGS has_esp_telepathy(PlayerType *player_ptr);
BIT_FLAGS has_bless_blade(PlayerType *player_ptr);
BIT_FLAGS has_easy2_weapon(PlayerType *player_ptr);
BIT_FLAGS has_down_saving(PlayerType *player_ptr);
BIT_FLAGS has_no_ac(PlayerType *player_ptr);
BIT_FLAGS has_invuln_arrow(PlayerType *player_ptr);
void check_no_flowed(PlayerType *player_ptr);
BIT_FLAGS has_mighty_throw(PlayerType *player_ptr);
BIT_FLAGS has_dec_mana(PlayerType *player_ptr);
BIT_FLAGS has_reflect(PlayerType *player_ptr);
BIT_FLAGS has_see_nocto(PlayerType *player_ptr);
BIT_FLAGS has_warning(PlayerType *player_ptr);
BIT_FLAGS has_anti_magic(PlayerType *player_ptr);
BIT_FLAGS has_anti_tele(PlayerType *player_ptr);
BIT_FLAGS has_sh_fire(PlayerType *player_ptr);
BIT_FLAGS has_sh_elec(PlayerType *player_ptr);
BIT_FLAGS has_sh_cold(PlayerType *player_ptr);
BIT_FLAGS has_easy_spell(PlayerType *player_ptr);
BIT_FLAGS has_heavy_spell(PlayerType *player_ptr);
BIT_FLAGS has_hold_exp(PlayerType *player_ptr);
BIT_FLAGS has_see_inv(PlayerType *player_ptr);
BIT_FLAGS has_magic_mastery(PlayerType *player_ptr);
BIT_FLAGS has_free_act(PlayerType *player_ptr);
BIT_FLAGS has_sustain_str(PlayerType *player_ptr);
BIT_FLAGS has_sustain_int(PlayerType *player_ptr);
BIT_FLAGS has_sustain_wis(PlayerType *player_ptr);
BIT_FLAGS has_sustain_dex(PlayerType *player_ptr);
BIT_FLAGS has_sustain_con(PlayerType *player_ptr);
BIT_FLAGS has_sustain_chr(PlayerType *player_ptr);
BIT_FLAGS has_levitation(PlayerType *player_ptr);
bool has_can_swim(PlayerType *player_ptr);
BIT_FLAGS has_slow_digest(PlayerType *player_ptr);
BIT_FLAGS has_regenerate(PlayerType *player_ptr);
void update_curses(PlayerType *player_ptr);
BIT_FLAGS has_impact(PlayerType *player_ptr);
BIT_FLAGS has_earthquake(PlayerType *player_ptr);
void update_extra_blows(PlayerType *player_ptr);
BIT_FLAGS has_resist_acid(PlayerType *player_ptr);
BIT_FLAGS has_vuln_acid(PlayerType *player_ptr);
BIT_FLAGS has_resist_elec(PlayerType *player_ptr);
BIT_FLAGS has_vuln_elec(PlayerType *player_ptr);
BIT_FLAGS has_resist_fire(PlayerType *player_ptr);
BIT_FLAGS has_vuln_fire(PlayerType *player_ptr);
BIT_FLAGS has_resist_cold(PlayerType *player_ptr);
BIT_FLAGS has_vuln_cold(PlayerType *player_ptr);
BIT_FLAGS has_resist_pois(PlayerType *player_ptr);
BIT_FLAGS has_resist_conf(PlayerType *player_ptr);
BIT_FLAGS has_resist_sound(PlayerType *player_ptr);
BIT_FLAGS has_resist_lite(PlayerType *player_ptr);
BIT_FLAGS has_vuln_lite(PlayerType *player_ptr);
BIT_FLAGS has_resist_dark(PlayerType *player_ptr);
BIT_FLAGS has_resist_chaos(PlayerType *player_ptr);
BIT_FLAGS has_resist_disen(PlayerType *player_ptr);
BIT_FLAGS has_resist_shard(PlayerType *player_ptr);
BIT_FLAGS has_resist_nexus(PlayerType *player_ptr);
BIT_FLAGS has_resist_blind(PlayerType *player_ptr);
BIT_FLAGS has_resist_neth(PlayerType *player_ptr);
BIT_FLAGS has_resist_time(PlayerType *player_ptr);
BIT_FLAGS has_resist_water(PlayerType *player_ptr);
BIT_FLAGS has_resist_curse(PlayerType *player_ptr);
BIT_FLAGS has_resist_fear(PlayerType *player_ptr);
BIT_FLAGS has_immune_acid(PlayerType *player_ptr);
BIT_FLAGS has_immune_elec(PlayerType *player_ptr);
BIT_FLAGS has_immune_fire(PlayerType *player_ptr);
BIT_FLAGS has_immune_cold(PlayerType *player_ptr);
BIT_FLAGS has_immune_dark(PlayerType *player_ptr);
bool can_attack_with_main_hand(PlayerType *player_ptr);
bool can_attack_with_sub_hand(PlayerType *player_ptr);
bool has_two_handed_weapons(PlayerType *player_ptr);
BIT_FLAGS has_lite(PlayerType *player_ptr);
bool has_disable_two_handed_bonus(PlayerType *player_ptr, int i);
bool has_not_ninja_weapon(PlayerType *player_ptr, int i);
bool has_not_monk_weapon(PlayerType *player_ptr, int i);
bool is_wielding_icky_weapon(PlayerType *player_ptr, int i);
bool is_wielding_icky_riding_weapon(PlayerType *player_ptr, int i);
bool has_good_luck(PlayerType *player_ptr);
BIT_FLAGS player_aggravate_state(PlayerType *player_ptr);
melee_type player_melee_type(PlayerType *player_ptr);
bool has_aggravate(PlayerType *player_ptr);
