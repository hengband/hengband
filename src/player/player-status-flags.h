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
    FLAG_CAUSE_BATTLE_FORM = 0x01U << 17, /*!< 構えによる効果 */
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

typedef struct player_type player_type;
BIT_FLAGS convert_inventory_slot_type_to_flag_cause(inventory_slot_type inventory_slot);
BIT_FLAGS check_equipment_flags(player_type *creature_ptr, tr_type tr_flag);
BIT_FLAGS get_player_flags(player_type *creature_ptr, tr_type tr_flag);
bool has_pass_wall(player_type *creature_ptr);
bool has_kill_wall(player_type *creature_ptr);
BIT_FLAGS has_xtra_might(player_type *creature_ptr);
BIT_FLAGS has_esp_evil(player_type *creature_ptr);
BIT_FLAGS has_esp_animal(player_type *creature_ptr);
BIT_FLAGS has_esp_undead(player_type *creature_ptr);
BIT_FLAGS has_esp_demon(player_type *creature_ptr);
BIT_FLAGS has_esp_orc(player_type *creature_ptr);
BIT_FLAGS has_esp_troll(player_type *creature_ptr);
BIT_FLAGS has_esp_giant(player_type *creature_ptr);
BIT_FLAGS has_esp_dragon(player_type *creature_ptr);
BIT_FLAGS has_esp_human(player_type *creature_ptr);
BIT_FLAGS has_esp_good(player_type *creature_ptr);
BIT_FLAGS has_esp_nonliving(player_type *creature_ptr);
BIT_FLAGS has_esp_unique(player_type *creature_ptr);
BIT_FLAGS has_esp_telepathy(player_type *creature_ptr);
BIT_FLAGS has_bless_blade(player_type *creature_ptr);
BIT_FLAGS has_easy2_weapon(player_type *creature_ptr);
BIT_FLAGS has_down_saving(player_type *creature_ptr);
BIT_FLAGS has_no_ac(player_type *creature_ptr);
BIT_FLAGS has_invuln_arrow(player_type *creature_ptr);
void check_no_flowed(player_type *creature_ptr);
BIT_FLAGS has_mighty_throw(player_type *creature_ptr);
BIT_FLAGS has_dec_mana(player_type *creature_ptr);
BIT_FLAGS has_reflect(player_type *creature_ptr);
BIT_FLAGS has_see_nocto(player_type *creature_ptr);
BIT_FLAGS has_warning(player_type *creature_ptr);
BIT_FLAGS has_anti_magic(player_type *creature_ptr);
BIT_FLAGS has_anti_tele(player_type *creature_ptr);
BIT_FLAGS has_sh_fire(player_type *creature_ptr);
BIT_FLAGS has_sh_elec(player_type *creature_ptr);
BIT_FLAGS has_sh_cold(player_type *creature_ptr);
BIT_FLAGS has_easy_spell(player_type *creature_ptr);
BIT_FLAGS has_heavy_spell(player_type *creature_ptr);
BIT_FLAGS has_hold_exp(player_type *creature_ptr);
BIT_FLAGS has_see_inv(player_type *creature_ptr);
BIT_FLAGS has_magic_mastery(player_type *creature_ptr);
BIT_FLAGS has_free_act(player_type *creature_ptr);
BIT_FLAGS has_sustain_str(player_type *creature_ptr);
BIT_FLAGS has_sustain_int(player_type *creature_ptr);
BIT_FLAGS has_sustain_wis(player_type *creature_ptr);
BIT_FLAGS has_sustain_dex(player_type *creature_ptr);
BIT_FLAGS has_sustain_con(player_type *creature_ptr);
BIT_FLAGS has_sustain_chr(player_type *creature_ptr);
BIT_FLAGS has_levitation(player_type *creature_ptr);
bool has_can_swim(player_type *creature_ptr);
BIT_FLAGS has_slow_digest(player_type *creature_ptr);
BIT_FLAGS has_regenerate(player_type *creature_ptr);
void update_curses(player_type *creature_ptr);
BIT_FLAGS has_impact(player_type *creature_ptr);
BIT_FLAGS has_earthquake(player_type *creature_ptr);
void update_extra_blows(player_type *creature_ptr);
BIT_FLAGS has_resist_acid(player_type *creature_ptr);
BIT_FLAGS has_vuln_acid(player_type *creature_ptr);
BIT_FLAGS has_resist_elec(player_type *creature_ptr);
BIT_FLAGS has_vuln_elec(player_type *creature_ptr);
BIT_FLAGS has_resist_fire(player_type *creature_ptr);
BIT_FLAGS has_vuln_fire(player_type *creature_ptr);
BIT_FLAGS has_resist_cold(player_type *creature_ptr);
BIT_FLAGS has_vuln_cold(player_type *creature_ptr);
BIT_FLAGS has_resist_pois(player_type *creature_ptr);
BIT_FLAGS has_resist_conf(player_type *creature_ptr);
BIT_FLAGS has_resist_sound(player_type *creature_ptr);
BIT_FLAGS has_resist_lite(player_type *creature_ptr);
BIT_FLAGS has_vuln_lite(player_type *creature_ptr);
BIT_FLAGS has_resist_dark(player_type *creature_ptr);
BIT_FLAGS has_resist_chaos(player_type *creature_ptr);
BIT_FLAGS has_resist_disen(player_type *creature_ptr);
BIT_FLAGS has_resist_shard(player_type *creature_ptr);
BIT_FLAGS has_resist_nexus(player_type *creature_ptr);
BIT_FLAGS has_resist_blind(player_type *creature_ptr);
BIT_FLAGS has_resist_neth(player_type *creature_ptr);
BIT_FLAGS has_resist_time(player_type *creature_ptr);
BIT_FLAGS has_resist_water(player_type *creature_ptr);
BIT_FLAGS has_resist_curse(player_type *creature_ptr);
BIT_FLAGS has_resist_fear(player_type *creature_ptr);
BIT_FLAGS has_immune_acid(player_type *creature_ptr);
BIT_FLAGS has_immune_elec(player_type *creature_ptr);
BIT_FLAGS has_immune_fire(player_type *creature_ptr);
BIT_FLAGS has_immune_cold(player_type *creature_ptr);
BIT_FLAGS has_immune_dark(player_type *creature_ptr);
bool can_attack_with_main_hand(player_type *creature_ptr);
bool can_attack_with_sub_hand(player_type *creature_ptr);
bool has_two_handed_weapons(player_type *creature_ptr);
BIT_FLAGS has_lite(player_type *creature_ptr);
bool has_disable_two_handed_bonus(player_type *creature_ptr, int i);
bool has_not_ninja_weapon(player_type *creature_ptr, int i);
bool has_not_monk_weapon(player_type *creature_ptr, int i);
bool is_wielding_icky_weapon(player_type *creature_ptr, int i);
bool is_wielding_icky_riding_weapon(player_type *creature_ptr, int i);
bool has_good_luck(player_type *creature_ptr);
BIT_FLAGS player_aggravate_state(player_type *creature_ptr);
melee_type player_melee_type(player_type *creature_ptr);
bool has_aggravate(player_type *creature_ptr);
