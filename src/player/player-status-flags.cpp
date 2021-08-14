#include "player/player-status-flags.h"
#include "artifact/fixed-art-types.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "player-info/equipment-info.h"
#include "player-status/player-basic-statistics.h"
#include "player-status/player-hand-types.h"
#include "player-status/player-infravision.h"
#include "player-status/player-speed.h"
#include "player-status/player-stealth.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"

#define SPELL_SW 22
#define SPELL_WALL 20

BIT_FLAGS convert_inventory_slot_type_to_flag_cause(inventory_slot_type inventory_slot)
{
    switch (inventory_slot) {
    case INVEN_MAIN_HAND:
        return FLAG_CAUSE_INVEN_MAIN_HAND;
    case INVEN_SUB_HAND:
        return FLAG_CAUSE_INVEN_SUB_HAND;
    case INVEN_BOW:
        return FLAG_CAUSE_INVEN_BOW;
    case INVEN_MAIN_RING:
        return FLAG_CAUSE_INVEN_MAIN_RING;
    case INVEN_SUB_RING:
        return FLAG_CAUSE_INVEN_SUB_RING;
    case INVEN_NECK:
        return FLAG_CAUSE_INVEN_NECK;
    case INVEN_LITE:
        return FLAG_CAUSE_INVEN_LITE;
    case INVEN_BODY:
        return FLAG_CAUSE_INVEN_BODY;
    case INVEN_OUTER:
        return FLAG_CAUSE_INVEN_OUTER;
    case INVEN_HEAD:
        return FLAG_CAUSE_INVEN_HEAD;
    case INVEN_ARMS:
        return FLAG_CAUSE_INVEN_ARMS;
    case INVEN_FEET:
        return FLAG_CAUSE_INVEN_FEET;

    default:
        return 0;
    }
}

/*!
 * @brief 装備による所定の特性フラグを得ているかを一括して取得する関数。
 */
BIT_FLAGS check_equipment_flags(player_type *creature_ptr, tr_type tr_flag)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    BIT_FLAGS result = 0L;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (has_flag(flgs, tr_flag))
            set_bits(result, convert_inventory_slot_type_to_flag_cause(static_cast<inventory_slot_type>(i)));
    }
    return result;
}

BIT_FLAGS player_flags_brand_pois(player_type *creature_ptr)
{
    BIT_FLAGS result = check_equipment_flags(creature_ptr, TR_BRAND_POIS);

    if (creature_ptr->special_attack & ATTACK_POIS)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_acid(player_type *creature_ptr)
{
    BIT_FLAGS result = check_equipment_flags(creature_ptr, TR_BRAND_ACID);

    if (creature_ptr->special_attack & ATTACK_ACID)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = check_equipment_flags(creature_ptr, TR_BRAND_ELEC);

    if (creature_ptr->special_attack & ATTACK_ELEC)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = check_equipment_flags(creature_ptr, TR_BRAND_FIRE);

    if (creature_ptr->special_attack & ATTACK_FIRE)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = check_equipment_flags(creature_ptr, TR_BRAND_COLD);

    if (creature_ptr->special_attack & ATTACK_COLD)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

/*!
 * @brief プレイヤーの所持するフラグのうち、tr_flagに対応するものを返す
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @param tr_flag 要求する装備フラグ
 */
BIT_FLAGS get_player_flags(player_type *creature_ptr, tr_type tr_flag)
{
    switch (tr_flag) {
    case TR_STR:
        return PlayerStrength(creature_ptr).get_all_flags();
    case TR_INT:
        return PlayerIntelligence(creature_ptr).get_all_flags();
    case TR_WIS:
        return PlayerWisdom(creature_ptr).get_all_flags();
    case TR_DEX:
        return PlayerDexterity(creature_ptr).get_all_flags();
    case TR_CON:
        return PlayerConstitution(creature_ptr).get_all_flags();
    case TR_CHR:
        return PlayerCharisma(creature_ptr).get_all_flags();
    case TR_MAGIC_MASTERY:
        return has_magic_mastery(creature_ptr);
    case TR_FORCE_WEAPON:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_STEALTH:
        return PlayerStealth(creature_ptr).get_all_flags();
    case TR_SEARCH:
        return 0;
    case TR_INFRA:
        return PlayerInfravision(creature_ptr).get_all_flags();
    case TR_TUNNEL:
        return 0;
    case TR_SPEED:
        return PlayerSpeed(creature_ptr).get_all_flags();
    case TR_BLOWS:
        return 0;
    case TR_CHAOTIC:
    case TR_VAMPIRIC:
    case TR_SLAY_ANIMAL:
    case TR_SLAY_EVIL:
    case TR_SLAY_UNDEAD:
    case TR_SLAY_DEMON:
    case TR_SLAY_ORC:
    case TR_SLAY_TROLL:
    case TR_SLAY_GIANT:
    case TR_SLAY_DRAGON:
    case TR_KILL_DRAGON:
    case TR_VORPAL:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_EARTHQUAKE:
        return has_earthquake(creature_ptr);
    case TR_BRAND_POIS:
        return player_flags_brand_pois(creature_ptr);
    case TR_BRAND_ACID:
        return player_flags_brand_acid(creature_ptr);
    case TR_BRAND_ELEC:
        return player_flags_brand_elec(creature_ptr);
    case TR_BRAND_FIRE:
        return player_flags_brand_fire(creature_ptr);
    case TR_BRAND_COLD:
        return player_flags_brand_cold(creature_ptr);

    case TR_SUST_STR:
        return has_sustain_str(creature_ptr);
    case TR_SUST_INT:
        return has_sustain_int(creature_ptr);
    case TR_SUST_WIS:
        return has_sustain_wis(creature_ptr);
    case TR_SUST_DEX:
        return has_sustain_dex(creature_ptr);
    case TR_SUST_CON:
        return has_sustain_con(creature_ptr);
    case TR_SUST_CHR:
        return has_sustain_chr(creature_ptr);
    case TR_RIDING:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_EASY_SPELL:
        return has_easy_spell(creature_ptr);
    case TR_IM_ACID:
        return has_immune_acid(creature_ptr);
    case TR_IM_ELEC:
        return has_immune_elec(creature_ptr);
    case TR_IM_FIRE:
        return has_immune_fire(creature_ptr);
    case TR_IM_COLD:
        return has_immune_cold(creature_ptr);
    case TR_THROW:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_REFLECT:
        return has_reflect(creature_ptr);
    case TR_FREE_ACT:
        return has_free_act(creature_ptr);
    case TR_HOLD_EXP:
        return has_hold_exp(creature_ptr);
    case TR_RES_ACID:
        return has_resist_acid(creature_ptr);
    case TR_RES_ELEC:
        return has_resist_elec(creature_ptr);
    case TR_RES_FIRE:
        return has_resist_fire(creature_ptr);
    case TR_RES_COLD:
        return has_resist_cold(creature_ptr);
    case TR_RES_POIS:
        return has_resist_pois(creature_ptr);
    case TR_RES_FEAR:
        return has_resist_fear(creature_ptr);
    case TR_RES_LITE:
        return has_resist_lite(creature_ptr);
    case TR_RES_DARK:
        return has_resist_dark(creature_ptr);
    case TR_RES_BLIND:
        return has_resist_blind(creature_ptr);
    case TR_RES_CONF:
        return has_resist_conf(creature_ptr);
    case TR_RES_SOUND:
        return has_resist_sound(creature_ptr);
    case TR_RES_SHARDS:
        return has_resist_shard(creature_ptr);
    case TR_RES_NETHER:
        return has_resist_neth(creature_ptr);
    case TR_RES_NEXUS:
        return has_resist_nexus(creature_ptr);
    case TR_RES_CHAOS:
        return has_resist_chaos(creature_ptr);
    case TR_RES_DISEN:
        return has_resist_disen(creature_ptr);
    case TR_RES_TIME:
        return has_resist_time(creature_ptr);
    case TR_RES_WATER:
        return has_resist_water(creature_ptr);
    case TR_RES_CURSE:
        return has_resist_curse(creature_ptr);

    case TR_SH_FIRE:
        return has_sh_fire(creature_ptr);
    case TR_SH_ELEC:
        return has_sh_elec(creature_ptr);
    case TR_SLAY_HUMAN:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_SH_COLD:
        return has_sh_cold(creature_ptr);
    case TR_NO_TELE:
        return has_anti_tele(creature_ptr);
    case TR_NO_MAGIC:
        return has_anti_magic(creature_ptr);
    case TR_DEC_MANA:
        return has_dec_mana(creature_ptr);
    case TR_TY_CURSE:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_WARNING:
        return has_warning(creature_ptr);
    case TR_HIDE_TYPE:
    case TR_SHOW_MODS:
    case TR_SLAY_GOOD:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_LEVITATION:
        return has_levitation(creature_ptr);
    case TR_LITE_1:
        return has_lite(creature_ptr);
    case TR_SEE_INVIS:
        return has_see_inv(creature_ptr);
    case TR_TELEPATHY:
        return has_esp_telepathy(creature_ptr);
    case TR_SLOW_DIGEST:
        return has_slow_digest(creature_ptr);
    case TR_REGEN:
        return has_regenerate(creature_ptr);
    case TR_XTRA_MIGHT:
        return has_xtra_might(creature_ptr);
    case TR_XTRA_SHOTS:
    case TR_IGNORE_ACID:
    case TR_IGNORE_ELEC:
    case TR_IGNORE_FIRE:
    case TR_IGNORE_COLD:
    case TR_ACTIVATE:
    case TR_DRAIN_EXP:
    case TR_TELEPORT:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_AGGRAVATE:
        return 0;
    case TR_BLESSED:
        return has_bless_blade(creature_ptr);
    case TR_ES_ATTACK:
    case TR_ES_AC:
    case TR_KILL_GOOD:
    case TR_KILL_ANIMAL:
    case TR_KILL_EVIL:
    case TR_KILL_UNDEAD:
    case TR_KILL_DEMON:
    case TR_KILL_ORC:
    case TR_KILL_TROLL:
    case TR_KILL_GIANT:
    case TR_KILL_HUMAN:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_ESP_ANIMAL:
        return has_esp_animal(creature_ptr);
    case TR_ESP_UNDEAD:
        return has_esp_undead(creature_ptr);
    case TR_ESP_DEMON:
        return has_esp_demon(creature_ptr);
    case TR_ESP_ORC:
        return has_esp_orc(creature_ptr);
    case TR_ESP_TROLL:
        return has_esp_troll(creature_ptr);
    case TR_ESP_GIANT:
        return has_esp_giant(creature_ptr);
    case TR_ESP_DRAGON:
        return has_esp_dragon(creature_ptr);
    case TR_ESP_HUMAN:
        return has_esp_human(creature_ptr);
    case TR_ESP_EVIL:
        return has_esp_evil(creature_ptr);
    case TR_ESP_GOOD:
        return has_esp_good(creature_ptr);
    case TR_ESP_NONLIVING:
        return has_esp_nonliving(creature_ptr);
    case TR_ESP_UNIQUE:
        return has_esp_unique(creature_ptr);
    case TR_FULL_NAME:
    case TR_FIXED_FLAVOR:
    case TR_ADD_L_CURSE:
    case TR_ADD_H_CURSE:
    case TR_DRAIN_HP:
    case TR_DRAIN_MANA:
    case TR_LITE_2:
    case TR_LITE_3:
    case TR_LITE_M1:
    case TR_LITE_M2:
    case TR_LITE_M3:
    case TR_LITE_FUEL:
    case TR_CALL_ANIMAL:
    case TR_CALL_DEMON:
    case TR_CALL_DRAGON:
    case TR_CALL_UNDEAD:
    case TR_COWARDICE:
    case TR_LOW_MELEE:
    case TR_LOW_AC:
    case TR_HARD_SPELL:
    case TR_FAST_DIGEST:
    case TR_SLOW_REGEN:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_MIGHTY_THROW:
        return has_mighty_throw(creature_ptr);
    case TR_EASY2_WEAPON:
        return has_easy2_weapon(creature_ptr);
    case TR_DOWN_SAVING:
        return has_down_saving(creature_ptr);
    case TR_NO_AC:
        return has_no_ac(creature_ptr);
    case TR_HEAVY_SPELL:
        return has_heavy_spell(creature_ptr);
    case TR_INVULN_ARROW:
        return has_invuln_arrow(creature_ptr);
    case TR_DARK_SOURCE:
    case TR_SUPPORTIVE:
    case TR_BERS_RAGE:
    case TR_BRAND_MAGIC:
        return check_equipment_flags(creature_ptr, tr_flag);
    case TR_IMPACT:
        return has_impact(creature_ptr);
    case TR_VUL_ACID:
        return has_vuln_acid(creature_ptr);
    case TR_VUL_COLD:
        return has_vuln_cold(creature_ptr);
    case TR_VUL_ELEC:
        return has_vuln_elec(creature_ptr);
    case TR_VUL_FIRE:
        return has_vuln_fire(creature_ptr);
    case TR_VUL_LITE:
        return has_vuln_lite(creature_ptr);
    case TR_IM_DARK:
        return has_immune_dark(creature_ptr);

    case TR_FLAG_MAX:
        break;
    }
    return 0;
}

/*!
 * @brief クリーチャーが壁破壊進行を持っているかを返す。
 */
bool has_kill_wall(player_type *creature_ptr)
{
    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD || music_singing(creature_ptr, MUSIC_WALL)) {
        return true;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags2 & RF2_KILL_WALL)
            return true;
    }

    return false;
}

/*!
 * @brief クリーチャーが壁通過を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたらTRUE
 * @details
 * * 時限で幽体化、壁抜けをもつか種族幽霊ならばひとまずTRUE。
 * * 但し騎乗中は乗騎が壁抜けを持っていなければ不能になる。
 */
bool has_pass_wall(player_type *creature_ptr)
{
    bool pow = false;

    if (creature_ptr->wraith_form || creature_ptr->tim_pass_wall || (!creature_ptr->mimic_form && creature_ptr->prace == player_race_type::SPECTRE)) {
        pow = true;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (!(riding_r_ptr->flags2 & RF2_PASS_WALL))
            pow = false;
    }

    return pow;
}

/*!
 * @brief クリーチャーが強力射を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_xtra_might(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    result |= check_equipment_flags(creature_ptr, TR_XTRA_MIGHT);
    return result;
}

/*!
 * @brief クリーチャーが邪悪感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_evil(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_DETECT_EVIL))
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }
    result |= check_equipment_flags(creature_ptr, TR_ESP_EVIL);
    return result;
}

/*!
 * @brief クリーチャーが自然界の動物感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_animal(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_ANIMAL);
}

/*!
 * @brief クリーチャーがアンデッド感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_undead(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_UNDEAD);
}

/*!
 * @brief クリーチャーが悪魔感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_demon(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_DEMON);
}

/*!
 * @brief クリーチャーがオーク感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_orc(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_ORC);
}

/*!
 * @brief クリーチャーがトロル感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_troll(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_TROLL);
}

/*!
 * @brief クリーチャーが巨人感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_giant(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_GIANT);
}

/*!
 * @brief クリーチャーがドラゴン感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_dragon(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_DRAGON);
}

/*!
 * @brief クリーチャーが人間感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_human(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_HUMAN);
}

/*!
 * @brief クリーチャーが善良感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_good(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_GOOD);
}

/*!
 * @brief クリーチャーが無生物感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_nonliving(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_NONLIVING);
}

/*!
 * @brief クリーチャーがユニーク感知を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_unique(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_ESP_UNIQUE);
}

/*!
 * @brief クリーチャーがテレパシーを持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_telepathy(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (is_time_limit_esp(creature_ptr) || creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }
    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->muta.has(MUTA::ESP)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_race_has_flag(creature_ptr, TR_TELEPATHY))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 39)
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_TELEPATHY);
    return result;
}

BIT_FLAGS has_bless_blade(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_BLESSED);
}

BIT_FLAGS has_easy2_weapon(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_EASY2_WEAPON);
}

BIT_FLAGS has_down_saving(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_DOWN_SAVING);
}

BIT_FLAGS has_no_ac(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_NO_AC);
}

BIT_FLAGS has_invuln_arrow(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->blind)
        return result;
    result |= check_equipment_flags(creature_ptr, TR_INVULN_ARROW);
    return result;
}

void check_no_flowed(player_type *creature_ptr)
{
    object_type *o_ptr;
    bool has_sw = false, has_kabe = false;

    creature_ptr->no_flowed = false;

    if (has_pass_wall(creature_ptr) && !has_kill_wall(creature_ptr)) {
        creature_ptr->no_flowed = true;
        return;
    }

    if (!creature_ptr->realm1) {
        creature_ptr->no_flowed = false;
        return;
    }

    for (int i = 0; i < INVEN_PACK; i++) {
        if ((creature_ptr->inventory_list[i].tval == TV_NATURE_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            has_sw = true;
        if ((creature_ptr->inventory_list[i].tval == TV_CRAFT_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            has_kabe = true;
    }

    for (const auto this_o_idx : creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].o_idx_list) {
        o_ptr = &creature_ptr->current_floor_ptr->o_list[this_o_idx];

        if ((o_ptr->tval == TV_NATURE_BOOK) && (o_ptr->sval == 2))
            has_sw = true;
        if ((o_ptr->tval == TV_CRAFT_BOOK) && (o_ptr->sval == 2))
            has_kabe = true;
    }

    if (has_sw && ((creature_ptr->realm1 == REALM_NATURE) || (creature_ptr->realm2 == REALM_NATURE) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_NATURE - 1][SPELL_SW];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = true;
    }

    if (has_kabe && ((creature_ptr->realm1 == REALM_CRAFT) || (creature_ptr->realm2 == REALM_CRAFT) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_CRAFT - 1][SPELL_WALL];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = true;
    }
}

BIT_FLAGS has_mighty_throw(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_MIGHTY_THROW);
}

BIT_FLAGS has_dec_mana(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_DEC_MANA);
}

BIT_FLAGS has_reflect(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_BERSERKER && creature_ptr->lev > 39)
        result |= FLAG_CAUSE_CLASS;

    if (creature_ptr->pclass == CLASS_MIRROR_MASTER && creature_ptr->lev > 39)
        result |= FLAG_CAUSE_CLASS;

    if (creature_ptr->special_defense & KAMAE_GENBU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->wraith_form || creature_ptr->magicdef || creature_ptr->tim_reflect) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::EARTH, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_REFLECT);
    return result;
}

BIT_FLAGS has_see_nocto(player_type *creature_ptr)
{
    return (creature_ptr->pclass == CLASS_NINJA) ? FLAG_CAUSE_CLASS : FLAG_CAUSE_NONE;
}

BIT_FLAGS has_warning(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (has_flag(flgs, TR_WARNING)) {
            if (!o_ptr->inscription || !(angband_strchr(quark_str(o_ptr->inscription), '$')))
                set_bits(result, convert_inventory_slot_type_to_flag_cause(static_cast<inventory_slot_type>(i)));
        }
    }
    return result;
}

BIT_FLAGS has_anti_magic(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_NO_MAGIC);
}

BIT_FLAGS has_anti_tele(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_NO_TELE);
}

BIT_FLAGS has_sh_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta.has(MUTA::FIRE_BODY)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_race_has_flag(creature_ptr, TR_SH_FIRE))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (hex_spelling(creature_ptr, HEX_DEMON_AURA) || creature_ptr->ult_res || creature_ptr->tim_sh_fire) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_SH_FIRE);
    return result;
}

BIT_FLAGS has_sh_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_SH_ELEC))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->muta.has(MUTA::ELEC_TOUC))
        result |= FLAG_CAUSE_MUTATION;

    if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK) || creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || (creature_ptr->special_defense & KATA_MUSOU)) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SH_ELEC);
    return result;
}

BIT_FLAGS has_sh_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_SH_COLD))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || hex_spelling(creature_ptr, HEX_ICE_ARMOR)) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_SH_COLD);
    return result;
}

BIT_FLAGS has_easy_spell(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_EASY_SPELL);
}

BIT_FLAGS has_heavy_spell(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_HEAVY_SPELL);
}

BIT_FLAGS has_hold_exp(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (player_race_has_flag(creature_ptr, TR_HOLD_EXP))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_HOLD_EXP);
    return result;
}

BIT_FLAGS has_see_inv(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 29)
        result |= FLAG_CAUSE_CLASS;

    if (player_race_has_flag(creature_ptr, TR_SEE_INVIS))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->tim_invis) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_SEE_INVIS);
    return result;
}

BIT_FLAGS has_magic_mastery(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_MAGIC_MASTERY);
}

BIT_FLAGS has_free_act(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta.has(MUTA::MOTION))
        result |= FLAG_CAUSE_MUTATION;

    if (player_race_has_flag(creature_ptr, TR_FREE_ACT))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->pclass == CLASS_NINJA && !heavy_armor(creature_ptr)
        && (!creature_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(creature_ptr))
        && (!creature_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(creature_ptr))) {
        if (creature_ptr->lev > 24)
            result |= FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->pclass == CLASS_MONK || creature_ptr->pclass == CLASS_FORCETRAINER) {
        if (!(heavy_armor(creature_ptr))) {
            if (creature_ptr->lev > 24)
                result |= FLAG_CAUSE_CLASS;
        }
    }

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->ult_res || creature_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_FREE_ACT);
    return result;
}

BIT_FLAGS has_sustain_str(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= FLAG_CAUSE_CLASS;
    }

    if (player_race_has_flag(creature_ptr, TR_SUST_STR))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_STR);
    return result;
}

BIT_FLAGS has_sustain_int(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_SUST_INT))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_INT);
    return result;
}

BIT_FLAGS has_sustain_wis(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 19)
        result |= FLAG_CAUSE_CLASS;

    if (player_race_has_flag(creature_ptr, TR_SUST_WIS))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_WIS);
    return result;
}

BIT_FLAGS has_sustain_dex(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 24)
        result |= FLAG_CAUSE_CLASS;

    if (player_race_has_flag(creature_ptr, TR_SUST_DEX))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_DEX);
    return result;
}

BIT_FLAGS has_sustain_con(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= FLAG_CAUSE_CLASS;
    }

    if (player_race_has_flag(creature_ptr, TR_SUST_CON))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_CON);
    return result;
}

BIT_FLAGS has_sustain_chr(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_SUST_CHR))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_CHR);
    return result;
}

BIT_FLAGS has_levitation(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta.has(MUTA::WINGS)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_race_has_flag(creature_ptr, TR_LEVITATION))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KAMAE_SUZAKU || (creature_ptr->special_defense & KATA_MUSOU)) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->tim_levitation) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_LEVITATION);

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        result = (riding_r_ptr->flags7 & RF7_CAN_FLY) ? FLAG_CAUSE_RIDING : FLAG_CAUSE_NONE;
    }
    return result;
}

bool has_can_swim(player_type *creature_ptr)
{
    bool can_swim = false;
    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags7 & (RF7_CAN_SWIM | RF7_AQUATIC))
            can_swim = true;
    }

    return can_swim;
}

BIT_FLAGS has_slow_digest(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_NINJA) {
        result |= FLAG_CAUSE_CLASS;
    }

    if (player_race_has_flag(creature_ptr, TR_SLOW_DIGEST))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_SLOW_DIGEST);
    return result;
}

BIT_FLAGS has_regenerate(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_WARRIOR && creature_ptr->lev > 44) {
        result |= FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->muta.has(MUTA::REGEN))
        result |= FLAG_CAUSE_MUTATION;

    if (player_race_has_flag(creature_ptr, TR_REGEN))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (hex_spelling(creature_ptr, HEX_DEMON_AURA) || creature_ptr->ult_res || creature_ptr->tim_regen) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_REGEN);

    if (creature_ptr->muta.has(MUTA::FLESH_ROT))
        result = 0L;

    return result;
}

void update_curses(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->cursed.clear();
    creature_ptr->cursed_special.clear();

    if (creature_ptr->pseikaku == PERSONALITY_SEXY)
        creature_ptr->cursed.set(TRC::AGGRAVATE);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_AGGRAVATE))
            creature_ptr->cursed.set(TRC::AGGRAVATE);
        if (has_flag(flgs, TR_DRAIN_EXP))
            creature_ptr->cursed.set(TRC::DRAIN_EXP);
        if (has_flag(flgs, TR_TY_CURSE))
            creature_ptr->cursed.set(TRC::TY_CURSE);
        if (has_flag(flgs, TR_ADD_L_CURSE))
            creature_ptr->cursed.set(TRC::ADD_L_CURSE);
        if (has_flag(flgs, TR_ADD_H_CURSE))
            creature_ptr->cursed.set(TRC::ADD_H_CURSE);
        if (has_flag(flgs, TR_DRAIN_HP))
            creature_ptr->cursed.set(TRC::DRAIN_HP);
        if (has_flag(flgs, TR_DRAIN_MANA))
            creature_ptr->cursed.set(TRC::DRAIN_MANA);
        if (has_flag(flgs, TR_CALL_ANIMAL))
            creature_ptr->cursed.set(TRC::CALL_ANIMAL);
        if (has_flag(flgs, TR_CALL_DEMON))
            creature_ptr->cursed.set(TRC::CALL_DEMON);
        if (has_flag(flgs, TR_CALL_DRAGON))
            creature_ptr->cursed.set(TRC::CALL_DRAGON);
        if (has_flag(flgs, TR_CALL_UNDEAD))
            creature_ptr->cursed.set(TRC::CALL_UNDEAD);
        if (has_flag(flgs, TR_COWARDICE))
            creature_ptr->cursed.set(TRC::COWARDICE);
        if (has_flag(flgs, TR_LOW_MELEE))
            creature_ptr->cursed.set(TRC::LOW_MELEE);
        if (has_flag(flgs, TR_LOW_AC))
            creature_ptr->cursed.set(TRC::LOW_AC);
        if (has_flag(flgs, TR_HARD_SPELL))
            creature_ptr->cursed.set(TRC::HARD_SPELL);
        if (has_flag(flgs, TR_FAST_DIGEST))
            creature_ptr->cursed.set(TRC::FAST_DIGEST);
        if (has_flag(flgs, TR_SLOW_REGEN))
            creature_ptr->cursed.set(TRC::SLOW_REGEN);
        if (has_flag(flgs, TR_BERS_RAGE))
            creature_ptr->cursed.set(TRC::BERS_RAGE);

        auto obj_curse_flags = o_ptr->curse_flags;
        obj_curse_flags.reset({ TRC::CURSED, TRC::HEAVY_CURSE, TRC::PERMA_CURSE });
        creature_ptr->cursed.set(obj_curse_flags);
        if (o_ptr->name1 == ART_CHAINSWORD)
            creature_ptr->cursed_special.set(TRCS::CHAINSWORD);

        if (has_flag(flgs, TR_TELEPORT)) {
            if (object_is_cursed(o_ptr))
                creature_ptr->cursed.set(TRC::TELEPORT);
            else {
                concptr insc = quark_str(o_ptr->inscription);

                /* {.} will stop random teleportation. */
                if (o_ptr->inscription && angband_strchr(insc, '.')) {
                } else {
                    creature_ptr->cursed_special.set(TRCS::TELEPORT_SELF);
                }
            }
        }
    }

    if (creature_ptr->cursed.has(TRC::TELEPORT))
        creature_ptr->cursed_special.reset(TRCS::TELEPORT_SELF);
}

BIT_FLAGS has_impact(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_IMPACT);
}

BIT_FLAGS has_earthquake(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_EARTHQUAKE);
}

void update_extra_blows(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->extra_blows[0] = creature_ptr->extra_blows[1] = 0;

    const melee_type melee_type = player_melee_type(creature_ptr);
    const bool two_handed = (melee_type == MELEE_TYPE_WEAPON_TWOHAND || melee_type == MELEE_TYPE_BAREHAND_TWO);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_BLOWS)) {
            if ((i == INVEN_MAIN_HAND || i == INVEN_MAIN_RING) && !two_handed)
                creature_ptr->extra_blows[0] += o_ptr->pval;
            else if ((i == INVEN_SUB_HAND || i == INVEN_SUB_RING) && !two_handed)
                creature_ptr->extra_blows[1] += o_ptr->pval;
            else {
                creature_ptr->extra_blows[0] += o_ptr->pval;
                creature_ptr->extra_blows[1] += o_ptr->pval;
            }
        }
    }
}

BIT_FLAGS has_resist_acid(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_ACID))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::SEA, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= has_immune_acid(creature_ptr);

    result |= check_equipment_flags(creature_ptr, TR_RES_ACID);
    return result;
}

BIT_FLAGS has_vuln_acid(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_race_has_flag(creature_ptr, TR_VUL_ACID))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_VUL_ACID);
    return result;
}

BIT_FLAGS has_resist_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_ELEC))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::SKY, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_ELEC);
    result |= has_immune_elec(creature_ptr);
    return result;
}

BIT_FLAGS has_vuln_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_race_has_flag(creature_ptr, TR_VUL_ELEC))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_VUL_ELEC);
    return result;
}

BIT_FLAGS has_resist_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_FIRE))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::FIRE, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_FIRE);
    result |= has_immune_fire(creature_ptr);
    return result;
}

BIT_FLAGS has_vuln_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_race_has_flag(creature_ptr, TR_VUL_FIRE))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_VUL_FIRE);
    return result;
}

BIT_FLAGS has_resist_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_COLD))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::ICE, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_COLD);
    result |= has_immune_cold(creature_ptr);
    return result;
}

BIT_FLAGS has_vuln_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_race_has_flag(creature_ptr, TR_VUL_COLD))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_VUL_COLD);
    return result;
}

BIT_FLAGS has_resist_pois(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 19)
        result |= FLAG_CAUSE_CLASS;

    if (player_race_has_flag(creature_ptr, TR_RES_POIS))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::DEATH, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_POIS);
    return result;
}

BIT_FLAGS has_resist_conf(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 29)
        result |= FLAG_CAUSE_CLASS;

    if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN || creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (player_race_has_flag(creature_ptr, TR_RES_CONF))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res || creature_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (has_element_resist(creature_ptr, ElementRealm::CHAOS, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_CONF);
    return result;
}

BIT_FLAGS has_resist_sound(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_BARD) {
        result |= FLAG_CAUSE_CLASS;
    }
    if (player_race_has_flag(creature_ptr, TR_RES_SOUND))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_SOUND);
    return result;
}

BIT_FLAGS has_resist_lite(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_LITE))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_LITE);
    return result;
}

BIT_FLAGS has_vuln_lite(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_VUL_LITE))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->wraith_form) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_VUL_LITE);
    return result;
}

BIT_FLAGS has_resist_dark(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_DARK))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::DARKNESS, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_DARK);
    return result;
}

BIT_FLAGS has_resist_chaos(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_CHAOS_WARRIOR && creature_ptr->lev > 29)
        result |= FLAG_CAUSE_CLASS;

    if (player_race_has_flag(creature_ptr, TR_RES_CHAOS))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::CHAOS, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_CHAOS);
    return result;
}

BIT_FLAGS has_resist_disen(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_DISEN))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::DEATH, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_DISEN);
    return result;
}

BIT_FLAGS has_resist_shard(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_SHARDS))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::EARTH, 1))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_SHARDS);
    return result;
}

BIT_FLAGS has_resist_nexus(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_NEXUS))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_NEXUS);
    return result;
}

BIT_FLAGS has_resist_blind(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (player_race_has_flag(creature_ptr, TR_RES_BLIND))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_BLIND);
    return result;
}

BIT_FLAGS has_resist_neth(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_NETHER))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->tim_res_nether) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::DARKNESS, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_RES_NETHER);
    return result;
}

BIT_FLAGS has_resist_time(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_TIME))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->tim_res_time) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_TIME);
    return result;
}

BIT_FLAGS has_resist_water(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_WATER))
        result |= FLAG_CAUSE_RACE;

    result |= check_equipment_flags(creature_ptr, TR_RES_WATER);
    return result;
}

/*!
 * @brief 呪力耐性を所持しているかどうか
 * @param プレイヤー情報への参照ポインタ
 * @return 呪力耐性を所持していればTRUE、なければFALSE
 */
BIT_FLAGS has_resist_curse(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_RES_CURSE))
        result |= FLAG_CAUSE_RACE;

    result |= check_equipment_flags(creature_ptr, TR_RES_CURSE);
    return result;
}

BIT_FLAGS has_resist_fear(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta.has(MUTA::FEARLESS))
        result |= FLAG_CAUSE_MUTATION;

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
    case CLASS_SAMURAI:
        if (creature_ptr->lev > 29)
            result |= FLAG_CAUSE_CLASS;
        break;
    case CLASS_PALADIN:
    case CLASS_CHAOS_WARRIOR:
        if (creature_ptr->lev > 39)
            result |= FLAG_CAUSE_CLASS;
        break;
    case CLASS_MINDCRAFTER:
        if (creature_ptr->lev > 9)
            result |= FLAG_CAUSE_CLASS;
        break;
    case CLASS_NINJA:
        result |= FLAG_CAUSE_CLASS;
        break;

    default:
        break;
    }

    if (player_race_has_flag(creature_ptr, TR_RES_FEAR))
        result |= FLAG_CAUSE_RACE;

    if ((creature_ptr->special_defense & KATA_MUSOU)) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    if (is_hero(creature_ptr) || is_shero(creature_ptr) || creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_FEAR);
    return result;
}

BIT_FLAGS has_immune_acid(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_IM_ACID))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_ACID)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::SEA, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_IM_ACID);
    return result;
}

BIT_FLAGS has_immune_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_IM_ELEC))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_ELEC)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::SKY, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_IM_ELEC);
    return result;
}

BIT_FLAGS has_immune_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_IM_FIRE))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_FIRE)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::FIRE, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_IM_FIRE);
    return result;
}

BIT_FLAGS has_immune_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_IM_COLD))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_COLD)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (has_element_resist(creature_ptr, ElementRealm::ICE, 30))
        result |= FLAG_CAUSE_CLASS;

    result |= check_equipment_flags(creature_ptr, TR_IM_COLD);
    return result;
}

BIT_FLAGS has_immune_dark(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (player_race_has_flag(creature_ptr, TR_IM_DARK))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->wraith_form) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_IM_DARK);
    return result;
}

melee_type player_melee_type(player_type *creature_ptr)
{
    if (has_two_handed_weapons(creature_ptr))
        return MELEE_TYPE_WEAPON_TWOHAND;

    if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND)) {
        if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND)) {
            return MELEE_TYPE_WEAPON_DOUBLE;
        }
        return MELEE_TYPE_WEAPON_MAIN;
    }

    if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND))
        return MELEE_TYPE_WEAPON_SUB;

    if (empty_hands(creature_ptr, false) == (EMPTY_HAND_MAIN | EMPTY_HAND_SUB))
        return MELEE_TYPE_BAREHAND_TWO;

    if (empty_hands(creature_ptr, false) == EMPTY_HAND_MAIN)
        return MELEE_TYPE_BAREHAND_MAIN;

    if (empty_hands(creature_ptr, false) == EMPTY_HAND_SUB)
        return MELEE_TYPE_BAREHAND_SUB;

    return MELEE_TYPE_SHIELD_DOUBLE;
}

/*
 * @brief 利き手で攻撃可能かどうかを判定する
 *        利き手で攻撃可能とは、利き手に武器を持っているか、
 *        利き手が素手かつ左手も素手もしくは盾を装備している事を意味する。
 * @details Includes martial arts and hand combats as weapons.
 */
bool can_attack_with_main_hand(player_type *creature_ptr)
{
    if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND))
        return true;

    if ((empty_hands(creature_ptr, true) & EMPTY_HAND_MAIN) && !can_attack_with_sub_hand(creature_ptr))
        return true;

    return false;
}

/*
 * @brief 非利き手で攻撃可能かどうかを判定する
 *        非利き手で攻撃可能とは、非利き手に武器を持っている事に等しい
 * @details Exclude martial arts and hand combats from weapons.
 */
bool can_attack_with_sub_hand(player_type *creature_ptr)
{
    return has_melee_weapon(creature_ptr, INVEN_SUB_HAND);
}

/*
 * @brief 両手持ち状態かどうかを判定する
 */
bool has_two_handed_weapons(player_type *creature_ptr)
{
    if (can_two_hands_wielding(creature_ptr)) {
        if (can_attack_with_main_hand(creature_ptr) && (empty_hands(creature_ptr, false) == EMPTY_HAND_SUB)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_MAIN_HAND])) {
            return true;
        } else if (can_attack_with_sub_hand(creature_ptr) && (empty_hands(creature_ptr, false) == EMPTY_HAND_MAIN)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_SUB_HAND])) {
            return true;
        }
    }
    return false;
}

BIT_FLAGS has_lite(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->pclass == CLASS_NINJA)
        return 0L;

    if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (player_race_has_flag(creature_ptr, TR_LITE_1))
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= FLAG_CAUSE_BATTLE_FORM;
    }

    result |= has_sh_fire(creature_ptr);

    return result;
}

/*
 * @brief 両手持ちボーナスがもらえないかどうかを判定する。 / Does *not * get two hand wielding bonus.
 * @details
 *  Only can get hit bonuses when wieids an enough light weapon which is lighter than 5 times of weight limit.
 *  If its weight is 10 times heavier or more than weight limit, gets hit penalty in calc_to_hit().
 */
bool has_disable_two_handed_bonus(player_type *creature_ptr, int i)
{
    if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND + i) && has_two_handed_weapons(creature_ptr)) {
        object_type *o_ptr = &creature_ptr->inventory_list[INVEN_MAIN_HAND + i];
        int limit = calc_weapon_weight_limit(creature_ptr);

        /* Enable when two hand wields an enough light weapon */
        if (limit >= o_ptr->weight / 5)
            return false;
    }

    /* Disable when empty hands, one hand wieldings and heavy weapons */
    return true;
}

/*!
 * @brief ふさわしくない武器を持っているかどうかを返す。
 * @todo 相応しい時にFALSEで相応しくない時にTRUEという負論理は良くない、後で修正する
 */
bool has_icky_wield_weapon(player_type *creature_ptr, int i)
{
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_type *o_ptr = &creature_ptr->inventory_list[INVEN_MAIN_HAND + i];
    object_flags(creature_ptr, o_ptr, flgs);

    bool has_no_weapon = (o_ptr->tval == TV_NONE) || (o_ptr->tval == TV_SHIELD);
    if (creature_ptr->pclass == CLASS_PRIEST) {
        bool is_suitable_weapon = has_flag(flgs, TR_BLESSED);
        is_suitable_weapon |= (o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM);
        return !has_no_weapon && !is_suitable_weapon;
    }

    if (creature_ptr->pclass == CLASS_SORCERER) {
        bool is_suitable_weapon = o_ptr->tval == TV_HAFTED;
        is_suitable_weapon &= (o_ptr->sval == SV_WIZSTAFF) || (o_ptr->sval == SV_NAMAKE_HAMMER);
        return !has_no_weapon && !is_suitable_weapon;
    }

    if (has_not_monk_weapon(creature_ptr, i) || has_not_ninja_weapon(creature_ptr, i))
        return true;

    return false;
}

bool has_riding_wield_weapon(player_type *creature_ptr, int i)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    o_ptr = &creature_ptr->inventory_list[INVEN_MAIN_HAND + i];
    object_flags(creature_ptr, o_ptr, flgs);
    if (creature_ptr->riding != 0 && !(o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE))
        && !has_flag(flgs, TR_RIDING)) {
        return true;
    }
    return false;
}

bool has_not_ninja_weapon(player_type *creature_ptr, int i)
{
    if (!has_melee_weapon(creature_ptr, INVEN_MAIN_HAND + i)) {
        return false;
    }
    int tval = creature_ptr->inventory_list[INVEN_MAIN_HAND + i].tval - TV_WEAPON_BEGIN;
    OBJECT_SUBTYPE_VALUE sval = creature_ptr->inventory_list[INVEN_MAIN_HAND + i].sval;
    return creature_ptr->pclass == CLASS_NINJA
        && !((s_info[CLASS_NINJA].w_max[tval][sval] > WEAPON_EXP_BEGINNER) && (creature_ptr->inventory_list[INVEN_SUB_HAND - i].tval != TV_SHIELD));
}

bool has_not_monk_weapon(player_type *creature_ptr, int i)
{
    if (!has_melee_weapon(creature_ptr, INVEN_MAIN_HAND + i)) {
        return false;
    }
    int tval = creature_ptr->inventory_list[INVEN_MAIN_HAND + i].tval - TV_WEAPON_BEGIN;
    OBJECT_SUBTYPE_VALUE sval = creature_ptr->inventory_list[INVEN_MAIN_HAND + i].sval;
    return ((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER)) && !(s_info[creature_ptr->pclass].w_max[tval][sval]);
}

bool has_good_luck(player_type *creature_ptr)
{
    return (creature_ptr->pseikaku == PERSONALITY_LUCKY) || (creature_ptr->muta.has(MUTA::GOOD_LUCK));
}

BIT_FLAGS player_aggravate_state(player_type *creature_ptr)
{
    if (creature_ptr->cursed.has(TRC::AGGRAVATE)) {
        if ((is_specific_player_race(creature_ptr, player_race_type::S_FAIRY)) && (creature_ptr->pseikaku != PERSONALITY_SEXY)) {
            return AGGRAVATE_S_FAIRY;
        }
        return AGGRAVATE_NORMAL;
    }

    return AGGRAVATE_NONE;
}

bool has_aggravate(player_type *creature_ptr)
{
    return player_aggravate_state(creature_ptr) == AGGRAVATE_NORMAL;
}
