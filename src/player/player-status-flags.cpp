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
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/mimic-info-table.h"
#include "player-status/player-basic-statistics.h"
#include "player-status/player-hand-types.h"
#include "player-status/player-infravision.h"
#include "player-status/player-speed.h"
#include "player-status/player-stealth.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
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

namespace {

/*!
 * @brief 指定した特性フラグが得られている要因となる flag_cause 型のうち以下の基本的な物のフラグ集合を取得する
 * 装備品のアイテムスロット / 種族上の体得 / 職業上の体得
 * @param tr_flag 特性フラグ
 * @return tr_flag が得られる要因となるフラグの集合
 */
BIT_FLAGS common_cause_flags(player_type *player_ptr, tr_type tr_flag)
{
    BIT_FLAGS result = check_equipment_flags(player_ptr, tr_flag);

    if (PlayerRace(player_ptr).tr_flags().has(tr_flag)) {
        set_bits(result, FLAG_CAUSE_RACE);
    }

    if (PlayerClass(player_ptr).tr_flags().has(tr_flag)) {
        set_bits(result, FLAG_CAUSE_CLASS);
    }

    if (PlayerClass(player_ptr).form_tr_flags().has(tr_flag)) {
        set_bits(result, FLAG_CAUSE_BATTLE_FORM);
    }

    return result;
}

}

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
BIT_FLAGS check_equipment_flags(player_type *player_ptr, tr_type tr_flag)
{
    object_type *o_ptr;
    BIT_FLAGS result = 0L;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        auto flgs = object_flags(o_ptr);

        if (flgs.has(tr_flag))
            set_bits(result, convert_inventory_slot_type_to_flag_cause(i2enum<inventory_slot_type>(i)));
    }
    return result;
}

BIT_FLAGS player_flags_brand_pois(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_BRAND_POIS);

    if (player_ptr->special_attack & ATTACK_POIS)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_acid(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_BRAND_ACID);

    if (player_ptr->special_attack & ATTACK_ACID)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_elec(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_BRAND_ELEC);

    if (player_ptr->special_attack & ATTACK_ELEC)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_fire(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_BRAND_FIRE);

    if (player_ptr->special_attack & ATTACK_FIRE)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

BIT_FLAGS player_flags_brand_cold(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_BRAND_COLD);

    if (player_ptr->special_attack & ATTACK_COLD)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    return result;
}

/*!
 * @brief プレイヤーの所持するフラグのうち、tr_flagに対応するものを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tr_flag 要求する装備フラグ
 */
BIT_FLAGS get_player_flags(player_type *player_ptr, tr_type tr_flag)
{
    switch (tr_flag) {
    case TR_STR:
        return PlayerStrength(player_ptr).get_all_flags();
    case TR_INT:
        return PlayerIntelligence(player_ptr).get_all_flags();
    case TR_WIS:
        return PlayerWisdom(player_ptr).get_all_flags();
    case TR_DEX:
        return PlayerDexterity(player_ptr).get_all_flags();
    case TR_CON:
        return PlayerConstitution(player_ptr).get_all_flags();
    case TR_CHR:
        return PlayerCharisma(player_ptr).get_all_flags();
    case TR_MAGIC_MASTERY:
        return has_magic_mastery(player_ptr);
    case TR_FORCE_WEAPON:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_STEALTH:
        return PlayerStealth(player_ptr).get_all_flags();
    case TR_SEARCH:
        return 0;
    case TR_INFRA:
        return PlayerInfravision(player_ptr).get_all_flags();
    case TR_TUNNEL:
        return 0;
    case TR_SPEED:
        return PlayerSpeed(player_ptr).get_all_flags();
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
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_EARTHQUAKE:
        return has_earthquake(player_ptr);
    case TR_BRAND_POIS:
        return player_flags_brand_pois(player_ptr);
    case TR_BRAND_ACID:
        return player_flags_brand_acid(player_ptr);
    case TR_BRAND_ELEC:
        return player_flags_brand_elec(player_ptr);
    case TR_BRAND_FIRE:
        return player_flags_brand_fire(player_ptr);
    case TR_BRAND_COLD:
        return player_flags_brand_cold(player_ptr);

    case TR_SUST_STR:
        return has_sustain_str(player_ptr);
    case TR_SUST_INT:
        return has_sustain_int(player_ptr);
    case TR_SUST_WIS:
        return has_sustain_wis(player_ptr);
    case TR_SUST_DEX:
        return has_sustain_dex(player_ptr);
    case TR_SUST_CON:
        return has_sustain_con(player_ptr);
    case TR_SUST_CHR:
        return has_sustain_chr(player_ptr);
    case TR_RIDING:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_EASY_SPELL:
        return has_easy_spell(player_ptr);
    case TR_IM_ACID:
        return has_immune_acid(player_ptr);
    case TR_IM_ELEC:
        return has_immune_elec(player_ptr);
    case TR_IM_FIRE:
        return has_immune_fire(player_ptr);
    case TR_IM_COLD:
        return has_immune_cold(player_ptr);
    case TR_THROW:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_REFLECT:
        return has_reflect(player_ptr);
    case TR_FREE_ACT:
        return has_free_act(player_ptr);
    case TR_HOLD_EXP:
        return has_hold_exp(player_ptr);
    case TR_RES_ACID:
        return has_resist_acid(player_ptr);
    case TR_RES_ELEC:
        return has_resist_elec(player_ptr);
    case TR_RES_FIRE:
        return has_resist_fire(player_ptr);
    case TR_RES_COLD:
        return has_resist_cold(player_ptr);
    case TR_RES_POIS:
        return has_resist_pois(player_ptr);
    case TR_RES_FEAR:
        return has_resist_fear(player_ptr);
    case TR_RES_LITE:
        return has_resist_lite(player_ptr);
    case TR_RES_DARK:
        return has_resist_dark(player_ptr);
    case TR_RES_BLIND:
        return has_resist_blind(player_ptr);
    case TR_RES_CONF:
        return has_resist_conf(player_ptr);
    case TR_RES_SOUND:
        return has_resist_sound(player_ptr);
    case TR_RES_SHARDS:
        return has_resist_shard(player_ptr);
    case TR_RES_NETHER:
        return has_resist_neth(player_ptr);
    case TR_RES_NEXUS:
        return has_resist_nexus(player_ptr);
    case TR_RES_CHAOS:
        return has_resist_chaos(player_ptr);
    case TR_RES_DISEN:
        return has_resist_disen(player_ptr);
    case TR_RES_TIME:
        return has_resist_time(player_ptr);
    case TR_RES_WATER:
        return has_resist_water(player_ptr);
    case TR_RES_CURSE:
        return has_resist_curse(player_ptr);

    case TR_SH_FIRE:
        return has_sh_fire(player_ptr);
    case TR_SH_ELEC:
        return has_sh_elec(player_ptr);
    case TR_SLAY_HUMAN:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_SH_COLD:
        return has_sh_cold(player_ptr);
    case TR_NO_TELE:
        return has_anti_tele(player_ptr);
    case TR_NO_MAGIC:
        return has_anti_magic(player_ptr);
    case TR_DEC_MANA:
        return has_dec_mana(player_ptr);
    case TR_TY_CURSE:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_WARNING:
        return has_warning(player_ptr);
    case TR_HIDE_TYPE:
    case TR_SHOW_MODS:
    case TR_SLAY_GOOD:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_LEVITATION:
        return has_levitation(player_ptr);
    case TR_LITE_1:
        return has_lite(player_ptr);
    case TR_SEE_INVIS:
        return has_see_inv(player_ptr);
    case TR_TELEPATHY:
        return has_esp_telepathy(player_ptr);
    case TR_SLOW_DIGEST:
        return has_slow_digest(player_ptr);
    case TR_REGEN:
        return has_regenerate(player_ptr);
    case TR_XTRA_MIGHT:
        return has_xtra_might(player_ptr);
    case TR_XTRA_SHOTS:
    case TR_IGNORE_ACID:
    case TR_IGNORE_ELEC:
    case TR_IGNORE_FIRE:
    case TR_IGNORE_COLD:
    case TR_ACTIVATE:
    case TR_DRAIN_EXP:
    case TR_TELEPORT:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_AGGRAVATE:
        return 0;
    case TR_BLESSED:
        return has_bless_blade(player_ptr);
    case TR_XXX_93:
    case TR_XXX_94:
    case TR_KILL_GOOD:
    case TR_KILL_ANIMAL:
    case TR_KILL_EVIL:
    case TR_KILL_UNDEAD:
    case TR_KILL_DEMON:
    case TR_KILL_ORC:
    case TR_KILL_TROLL:
    case TR_KILL_GIANT:
    case TR_KILL_HUMAN:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_ESP_ANIMAL:
        return has_esp_animal(player_ptr);
    case TR_ESP_UNDEAD:
        return has_esp_undead(player_ptr);
    case TR_ESP_DEMON:
        return has_esp_demon(player_ptr);
    case TR_ESP_ORC:
        return has_esp_orc(player_ptr);
    case TR_ESP_TROLL:
        return has_esp_troll(player_ptr);
    case TR_ESP_GIANT:
        return has_esp_giant(player_ptr);
    case TR_ESP_DRAGON:
        return has_esp_dragon(player_ptr);
    case TR_ESP_HUMAN:
        return has_esp_human(player_ptr);
    case TR_ESP_EVIL:
        return has_esp_evil(player_ptr);
    case TR_ESP_GOOD:
        return has_esp_good(player_ptr);
    case TR_ESP_NONLIVING:
        return has_esp_nonliving(player_ptr);
    case TR_ESP_UNIQUE:
        return has_esp_unique(player_ptr);
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
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_MIGHTY_THROW:
        return has_mighty_throw(player_ptr);
    case TR_EASY2_WEAPON:
        return has_easy2_weapon(player_ptr);
    case TR_DOWN_SAVING:
        return has_down_saving(player_ptr);
    case TR_NO_AC:
        return has_no_ac(player_ptr);
    case TR_HEAVY_SPELL:
        return has_heavy_spell(player_ptr);
    case TR_INVULN_ARROW:
        return has_invuln_arrow(player_ptr);
    case TR_DARK_SOURCE:
    case TR_SUPPORTIVE:
    case TR_BERS_RAGE:
    case TR_BRAND_MAGIC:
        return check_equipment_flags(player_ptr, tr_flag);
    case TR_IMPACT:
        return has_impact(player_ptr);
    case TR_VUL_ACID:
        return has_vuln_acid(player_ptr);
    case TR_VUL_COLD:
        return has_vuln_cold(player_ptr);
    case TR_VUL_ELEC:
        return has_vuln_elec(player_ptr);
    case TR_VUL_FIRE:
        return has_vuln_fire(player_ptr);
    case TR_VUL_LITE:
        return has_vuln_lite(player_ptr);
    case TR_IM_DARK:
        return has_immune_dark(player_ptr);

    case TR_FLAG_MAX:
        break;
    }
    return 0;
}

/*!
 * @brief プレイヤーが壁破壊進行を持っているかを返す。
 */
bool has_kill_wall(player_type *player_ptr)
{
    if (player_ptr->mimic_form == MIMIC_DEMON_LORD || music_singing(player_ptr, MUSIC_WALL)) {
        return true;
    }

    if (player_ptr->riding) {
        monster_type *riding_m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags2 & RF2_KILL_WALL)
            return true;
    }

    return false;
}

/*!
 * @brief プレイヤーが壁通過を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたらTRUE
 * @details
 * * 時限で幽体化、壁抜けをもつか種族幽霊ならばひとまずTRUE。
 * * 但し騎乗中は乗騎が壁抜けを持っていなければ不能になる。
 */
bool has_pass_wall(player_type *player_ptr)
{
    bool pow = false;

    if (player_ptr->wraith_form || player_ptr->tim_pass_wall || (!player_ptr->mimic_form && player_ptr->prace == PlayerRaceType::SPECTRE)) {
        pow = true;
    }

    if (player_ptr->riding) {
        monster_type *riding_m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (!(riding_r_ptr->flags2 & RF2_PASS_WALL))
            pow = false;
    }

    return pow;
}

/*!
 * @brief プレイヤーが強力射を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_xtra_might(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_XTRA_MIGHT);
}

/*!
 * @brief プレイヤーが邪悪感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_evil(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_ESP_EVIL);
    if (player_ptr->realm1 == REALM_HEX) {
        if (SpellHex(player_ptr).is_spelling_specific(HEX_DETECT_EVIL))
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }
    return result;
}

/*!
 * @brief プレイヤーが自然界の動物感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_animal(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_ANIMAL);
}

/*!
 * @brief プレイヤーがアンデッド感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_undead(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_UNDEAD);
}

/*!
 * @brief プレイヤーが悪魔感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_demon(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_DEMON);
}

/*!
 * @brief プレイヤーがオーク感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_orc(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_ORC);
}

/*!
 * @brief プレイヤーがトロル感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_troll(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_TROLL);
}

/*!
 * @brief プレイヤーが巨人感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_giant(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_GIANT);
}

/*!
 * @brief プレイヤーがドラゴン感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_dragon(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_DRAGON);
}

/*!
 * @brief プレイヤーが人間感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_human(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_HUMAN);
}

/*!
 * @brief プレイヤーが善良感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_good(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_GOOD);
}

/*!
 * @brief プレイヤーが無生物感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_nonliving(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_NONLIVING);
}

/*!
 * @brief プレイヤーがユニーク感知を持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_unique(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_ESP_UNIQUE);
}

/*!
 * @brief プレイヤーがテレパシーを持っているかを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 */
BIT_FLAGS has_esp_telepathy(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_TELEPATHY);

    if (is_time_limit_esp(player_ptr) || player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (player_ptr->muta.has(MUTA::ESP)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    return result;
}

BIT_FLAGS has_bless_blade(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_BLESSED);
}

BIT_FLAGS has_easy2_weapon(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_EASY2_WEAPON);
}

BIT_FLAGS has_down_saving(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_DOWN_SAVING);
}

BIT_FLAGS has_no_ac(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_NO_AC);
}

BIT_FLAGS has_invuln_arrow(player_type *player_ptr)
{
    if (player_ptr->blind)
        return 0;

    return common_cause_flags(player_ptr, TR_INVULN_ARROW);
}

void check_no_flowed(player_type *player_ptr)
{
    object_type *o_ptr;
    bool has_sw = false, has_kabe = false;

    player_ptr->no_flowed = false;

    if (has_pass_wall(player_ptr) && !has_kill_wall(player_ptr)) {
        player_ptr->no_flowed = true;
        return;
    }

    if (!player_ptr->realm1) {
        player_ptr->no_flowed = false;
        return;
    }

    for (int i = 0; i < INVEN_PACK; i++) {
        if ((player_ptr->inventory_list[i].tval == TV_NATURE_BOOK) && (player_ptr->inventory_list[i].sval == 2))
            has_sw = true;
        if ((player_ptr->inventory_list[i].tval == TV_CRAFT_BOOK) && (player_ptr->inventory_list[i].sval == 2))
            has_kabe = true;
    }

    for (const auto this_o_idx : player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list) {
        o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];

        if ((o_ptr->tval == TV_NATURE_BOOK) && (o_ptr->sval == 2))
            has_sw = true;
        if ((o_ptr->tval == TV_CRAFT_BOOK) && (o_ptr->sval == 2))
            has_kabe = true;
    }

    if (has_sw && ((player_ptr->realm1 == REALM_NATURE) || (player_ptr->realm2 == REALM_NATURE) || (player_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_NATURE - 1][SPELL_SW];
        if (player_ptr->lev >= s_ptr->slevel)
            player_ptr->no_flowed = true;
    }

    if (has_kabe && ((player_ptr->realm1 == REALM_CRAFT) || (player_ptr->realm2 == REALM_CRAFT) || (player_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_CRAFT - 1][SPELL_WALL];
        if (player_ptr->lev >= s_ptr->slevel)
            player_ptr->no_flowed = true;
    }
}

BIT_FLAGS has_mighty_throw(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_MIGHTY_THROW);
}

BIT_FLAGS has_dec_mana(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_DEC_MANA);
}

BIT_FLAGS has_reflect(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_REFLECT);

    if (player_ptr->ult_res || player_ptr->wraith_form || player_ptr->magicdef || player_ptr->tim_reflect) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_see_nocto(player_type *player_ptr)
{
    return (player_ptr->pclass == CLASS_NINJA) ? FLAG_CAUSE_CLASS : FLAG_CAUSE_NONE;
}

BIT_FLAGS has_warning(player_type *player_ptr)
{
    BIT_FLAGS result = 0L;
    object_type *o_ptr;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        auto flgs = object_flags(o_ptr);

        if (flgs.has(TR_WARNING)) {
            if (!o_ptr->inscription || !(angband_strchr(quark_str(o_ptr->inscription), '$')))
                set_bits(result, convert_inventory_slot_type_to_flag_cause(i2enum<inventory_slot_type>(i)));
        }
    }
    return result;
}

BIT_FLAGS has_anti_magic(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_NO_MAGIC);
}

BIT_FLAGS has_anti_tele(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_NO_TELE);
}

BIT_FLAGS has_sh_fire(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SH_FIRE);

    if (player_ptr->muta.has(MUTA::FIRE_BODY)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (SpellHex(player_ptr).is_spelling_specific(HEX_DEMON_AURA) || player_ptr->ult_res || player_ptr->tim_sh_fire) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sh_elec(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SH_ELEC);

    if (player_ptr->muta.has(MUTA::ELEC_TOUC))
        result |= FLAG_CAUSE_MUTATION;

    if (SpellHex(player_ptr).is_spelling_specific(HEX_SHOCK_CLOAK) || player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sh_cold(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SH_COLD);

    if (player_ptr->ult_res || SpellHex(player_ptr).is_spelling_specific(HEX_ICE_ARMOR)) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_easy_spell(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_EASY_SPELL);
}

BIT_FLAGS has_heavy_spell(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_HEAVY_SPELL);
}

BIT_FLAGS has_hold_exp(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_HOLD_EXP);

    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_see_inv(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SEE_INVIS);

    if (player_ptr->ult_res || player_ptr->tim_invis) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_magic_mastery(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_MAGIC_MASTERY);
}

BIT_FLAGS has_free_act(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_FREE_ACT);

    if (player_ptr->muta.has(MUTA::MOTION))
        result |= FLAG_CAUSE_MUTATION;

    if (player_ptr->ult_res || player_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sustain_str(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SUST_STR);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sustain_int(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SUST_INT);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sustain_wis(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SUST_WIS);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sustain_dex(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SUST_DEX);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sustain_con(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SUST_CON);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_sustain_chr(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SUST_CHR);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_levitation(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_LEVITATION);

    if (player_ptr->muta.has(MUTA::WINGS)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    if (player_ptr->ult_res || player_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (player_ptr->tim_levitation) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    // 乗馬中は実際に浮遊するかどうかは乗馬中のモンスターに依存する
    if (player_ptr->riding) {
        monster_type *riding_m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        result = (riding_r_ptr->flags7 & RF7_CAN_FLY) ? FLAG_CAUSE_RIDING : FLAG_CAUSE_NONE;
    }

    return result;
}

bool has_can_swim(player_type *player_ptr)
{
    bool can_swim = false;
    if (player_ptr->riding) {
        monster_type *riding_m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags7 & (RF7_CAN_SWIM | RF7_AQUATIC))
            can_swim = true;
    }

    return can_swim;
}

BIT_FLAGS has_slow_digest(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_SLOW_DIGEST);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_regenerate(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_REGEN);

    if (player_ptr->muta.has(MUTA::REGEN))
        result |= FLAG_CAUSE_MUTATION;

    if (SpellHex(player_ptr).is_spelling_specific(HEX_DEMON_AURA) || player_ptr->ult_res || player_ptr->tim_regen) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (player_ptr->muta.has(MUTA::FLESH_ROT))
        result = 0L;

    return result;
}

void update_curses(player_type *player_ptr)
{
    object_type *o_ptr;
    player_ptr->cursed.clear();
    player_ptr->cursed_special.clear();

    if (player_ptr->ppersonality == PERSONALITY_SEXY)
        player_ptr->cursed.set(TRC::AGGRAVATE);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        auto flgs = object_flags(o_ptr);
        if (flgs.has(TR_AGGRAVATE))
            player_ptr->cursed.set(TRC::AGGRAVATE);
        if (flgs.has(TR_DRAIN_EXP))
            player_ptr->cursed.set(TRC::DRAIN_EXP);
        if (flgs.has(TR_TY_CURSE))
            player_ptr->cursed.set(TRC::TY_CURSE);
        if (flgs.has(TR_ADD_L_CURSE))
            player_ptr->cursed.set(TRC::ADD_L_CURSE);
        if (flgs.has(TR_ADD_H_CURSE))
            player_ptr->cursed.set(TRC::ADD_H_CURSE);
        if (flgs.has(TR_DRAIN_HP))
            player_ptr->cursed.set(TRC::DRAIN_HP);
        if (flgs.has(TR_DRAIN_MANA))
            player_ptr->cursed.set(TRC::DRAIN_MANA);
        if (flgs.has(TR_CALL_ANIMAL))
            player_ptr->cursed.set(TRC::CALL_ANIMAL);
        if (flgs.has(TR_CALL_DEMON))
            player_ptr->cursed.set(TRC::CALL_DEMON);
        if (flgs.has(TR_CALL_DRAGON))
            player_ptr->cursed.set(TRC::CALL_DRAGON);
        if (flgs.has(TR_CALL_UNDEAD))
            player_ptr->cursed.set(TRC::CALL_UNDEAD);
        if (flgs.has(TR_COWARDICE))
            player_ptr->cursed.set(TRC::COWARDICE);
        if (flgs.has(TR_LOW_MELEE))
            player_ptr->cursed.set(TRC::LOW_MELEE);
        if (flgs.has(TR_LOW_AC))
            player_ptr->cursed.set(TRC::LOW_AC);
        if (flgs.has(TR_HARD_SPELL))
            player_ptr->cursed.set(TRC::HARD_SPELL);
        if (flgs.has(TR_FAST_DIGEST))
            player_ptr->cursed.set(TRC::FAST_DIGEST);
        if (flgs.has(TR_SLOW_REGEN))
            player_ptr->cursed.set(TRC::SLOW_REGEN);
        if (flgs.has(TR_BERS_RAGE))
            player_ptr->cursed.set(TRC::BERS_RAGE);

        auto obj_curse_flags = o_ptr->curse_flags;
        obj_curse_flags.reset({ TRC::CURSED, TRC::HEAVY_CURSE, TRC::PERMA_CURSE });
        player_ptr->cursed.set(obj_curse_flags);
        if (o_ptr->name1 == ART_CHAINSWORD)
            player_ptr->cursed_special.set(TRCS::CHAINSWORD);

        if (flgs.has(TR_TELEPORT)) {
            if (o_ptr->is_cursed())
                player_ptr->cursed.set(TRC::TELEPORT);
            else {
                concptr insc = quark_str(o_ptr->inscription);

                /* {.} will stop random teleportation. */
                if (o_ptr->inscription && angband_strchr(insc, '.')) {
                } else {
                    player_ptr->cursed_special.set(TRCS::TELEPORT_SELF);
                }
            }
        }
    }

    if (player_ptr->cursed.has(TRC::TELEPORT))
        player_ptr->cursed_special.reset(TRCS::TELEPORT_SELF);
}

BIT_FLAGS has_impact(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_IMPACT);
}

BIT_FLAGS has_earthquake(player_type *player_ptr)
{
    return common_cause_flags(player_ptr, TR_EARTHQUAKE);
}

void update_extra_blows(player_type *player_ptr)
{
    object_type *o_ptr;
    player_ptr->extra_blows[0] = player_ptr->extra_blows[1] = 0;

    const melee_type melee_type = player_melee_type(player_ptr);
    const bool two_handed = (melee_type == MELEE_TYPE_WEAPON_TWOHAND || melee_type == MELEE_TYPE_BAREHAND_TWO);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        auto flgs = object_flags(o_ptr);
        if (flgs.has(TR_BLOWS)) {
            if ((i == INVEN_MAIN_HAND || i == INVEN_MAIN_RING) && !two_handed)
                player_ptr->extra_blows[0] += o_ptr->pval;
            else if ((i == INVEN_SUB_HAND || i == INVEN_SUB_RING) && !two_handed)
                player_ptr->extra_blows[1] += o_ptr->pval;
            else {
                player_ptr->extra_blows[0] += o_ptr->pval;
                player_ptr->extra_blows[1] += o_ptr->pval;
            }
        }
    }
}

BIT_FLAGS has_resist_acid(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_ACID);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= has_immune_acid(player_ptr);

    return result;
}

BIT_FLAGS has_vuln_acid(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_VUL_ACID);

    if (player_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    return result;
}

BIT_FLAGS has_resist_elec(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_ELEC);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= has_immune_elec(player_ptr);

    return result;
}

BIT_FLAGS has_vuln_elec(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_VUL_ELEC);

    if (player_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    return result;
}

BIT_FLAGS has_resist_fire(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_FIRE);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= has_immune_fire(player_ptr);

    return result;
}

BIT_FLAGS has_vuln_fire(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_VUL_FIRE);

    if (player_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    return result;
}

BIT_FLAGS has_resist_cold(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_COLD);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= has_immune_cold(player_ptr);

    return result;
}

BIT_FLAGS has_vuln_cold(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_VUL_COLD);

    if (player_ptr->muta.has(MUTA::VULN_ELEM)) {
        result |= FLAG_CAUSE_MUTATION;
    }

    return result;
}

BIT_FLAGS has_resist_pois(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_POIS);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_conf(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_CONF);

    if (player_ptr->ppersonality == PERSONALITY_CHARGEMAN || player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (player_ptr->ult_res || player_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_sound(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_SOUND);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_lite(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_LITE);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_vuln_lite(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_VUL_LITE);

    if (player_ptr->wraith_form) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_dark(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_DARK);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_chaos(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_CHAOS);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_disen(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_DISEN);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_shard(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_SHARDS);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_nexus(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_NEXUS);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_blind(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_BLIND);

    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (player_ptr->ult_res || player_ptr->magicdef) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_neth(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_NETHER);

    if (player_ptr->ult_res || player_ptr->tim_res_nether) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_time(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_TIME);

    if (player_ptr->ult_res || player_ptr->tim_res_time) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_water(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_WATER);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }


    return result;
}

/*!
 * @brief 呪力耐性を所持しているかどうか
 * @param プレイヤー情報への参照ポインタ
 * @return 呪力耐性を所持していればTRUE、なければFALSE
 */
BIT_FLAGS has_resist_curse(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_CURSE);

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_fear(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_RES_FEAR);

    if (player_ptr->muta.has(MUTA::FEARLESS))
        result |= FLAG_CAUSE_MUTATION;

    if (is_hero(player_ptr) || is_shero(player_ptr) || player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_immune_acid(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_IM_ACID);

    if (player_ptr->ele_immune) {
        if (player_ptr->special_defense & DEFENSE_ACID)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_immune_elec(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_IM_ELEC);

    if (player_ptr->ele_immune) {
        if (player_ptr->special_defense & DEFENSE_ELEC)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_immune_fire(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_IM_FIRE);

    if (player_ptr->ele_immune) {
        if (player_ptr->special_defense & DEFENSE_FIRE)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_immune_cold(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_IM_COLD);

    if (player_ptr->ele_immune) {
        if (player_ptr->special_defense & DEFENSE_COLD)
            result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_immune_dark(player_type *player_ptr)
{
    BIT_FLAGS result = common_cause_flags(player_ptr, TR_IM_DARK);

    if (player_ptr->wraith_form) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

melee_type player_melee_type(player_type *player_ptr)
{
    if (has_two_handed_weapons(player_ptr))
        return MELEE_TYPE_WEAPON_TWOHAND;

    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
        if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            return MELEE_TYPE_WEAPON_DOUBLE;
        }
        return MELEE_TYPE_WEAPON_MAIN;
    }

    if (has_melee_weapon(player_ptr, INVEN_SUB_HAND))
        return MELEE_TYPE_WEAPON_SUB;

    if (empty_hands(player_ptr, false) == (EMPTY_HAND_MAIN | EMPTY_HAND_SUB))
        return MELEE_TYPE_BAREHAND_TWO;

    if (empty_hands(player_ptr, false) == EMPTY_HAND_MAIN)
        return MELEE_TYPE_BAREHAND_MAIN;

    if (empty_hands(player_ptr, false) == EMPTY_HAND_SUB)
        return MELEE_TYPE_BAREHAND_SUB;

    return MELEE_TYPE_SHIELD_DOUBLE;
}

/*
 * @brief 利き手で攻撃可能かどうかを判定する
 *        利き手で攻撃可能とは、利き手に武器を持っているか、
 *        利き手が素手かつ左手も素手もしくは盾を装備している事を意味する。
 * @details Includes martial arts and hand combats as weapons.
 */
bool can_attack_with_main_hand(player_type *player_ptr)
{
    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND))
        return true;

    if ((empty_hands(player_ptr, true) & EMPTY_HAND_MAIN) && !can_attack_with_sub_hand(player_ptr))
        return true;

    return false;
}

/*
 * @brief 非利き手で攻撃可能かどうかを判定する
 *        非利き手で攻撃可能とは、非利き手に武器を持っている事に等しい
 * @details Exclude martial arts and hand combats from weapons.
 */
bool can_attack_with_sub_hand(player_type *player_ptr)
{
    return has_melee_weapon(player_ptr, INVEN_SUB_HAND);
}

/*
 * @brief 両手持ち状態かどうかを判定する
 */
bool has_two_handed_weapons(player_type *player_ptr)
{
    if (can_two_hands_wielding(player_ptr)) {
        if (can_attack_with_main_hand(player_ptr) && (empty_hands(player_ptr, false) == EMPTY_HAND_SUB)
            && player_ptr->inventory_list[INVEN_MAIN_HAND].allow_two_hands_wielding()) {
            return true;
        } else if (can_attack_with_sub_hand(player_ptr) && (empty_hands(player_ptr, false) == EMPTY_HAND_MAIN)
            && player_ptr->inventory_list[INVEN_SUB_HAND].allow_two_hands_wielding()) {
            return true;
        }
    }
    return false;
}

BIT_FLAGS has_lite(player_type *player_ptr)
{
    BIT_FLAGS result = 0L;
    if (player_ptr->pclass == CLASS_NINJA)
        return 0L;

    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        result |= FLAG_CAUSE_PERSONALITY;
    }

    if (PlayerRace(player_ptr).tr_flags().has(TR_LITE_1))
        result |= FLAG_CAUSE_RACE;

    if (player_ptr->ult_res) {
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= has_sh_fire(player_ptr);

    return result;
}

/*
 * @brief 両手持ちボーナスがもらえないかどうかを判定する。 / Does *not * get two hand wielding bonus.
 * @details
 *  Only can get hit bonuses when wieids an enough light weapon which is lighter than 5 times of weight limit.
 *  If its weight is 10 times heavier or more than weight limit, gets hit penalty in calc_to_hit().
 */
bool has_disable_two_handed_bonus(player_type *player_ptr, int i)
{
    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i) && has_two_handed_weapons(player_ptr)) {
        object_type *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
        int limit = calc_weapon_weight_limit(player_ptr);

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
bool is_wielding_icky_weapon(player_type *player_ptr, int i)
{
    auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
    auto flgs = object_flags(o_ptr);

    auto has_no_weapon = (o_ptr->tval == TV_NONE) || (o_ptr->tval == TV_SHIELD);
    if (player_ptr->pclass == CLASS_PRIEST) {
        auto is_suitable_weapon = flgs.has(TR_BLESSED);
        is_suitable_weapon |= (o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM);
        return !has_no_weapon && !is_suitable_weapon;
    }

    if (player_ptr->pclass == CLASS_SORCERER) {
        auto is_suitable_weapon = o_ptr->tval == TV_HAFTED;
        is_suitable_weapon &= (o_ptr->sval == SV_WIZSTAFF) || (o_ptr->sval == SV_NAMAKE_HAMMER);
        return !has_no_weapon && !is_suitable_weapon;
    }

    return has_not_monk_weapon(player_ptr, i) || has_not_ninja_weapon(player_ptr, i);
}

/*!
 * @brief 乗馬にふさわしくない武器を持って乗馬しているかどうかを返す.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i 武器を持っている手。0ならば利き手、1ならば反対の手
 */
bool is_wielding_icky_riding_weapon(player_type *player_ptr, int i)
{
    auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
    auto flgs = object_flags(o_ptr);
    auto has_no_weapon = (o_ptr->tval == TV_NONE) || (o_ptr->tval == TV_SHIELD);
    auto is_suitable = o_ptr->is_lance() || flgs.has(TR_RIDING);
    return (player_ptr->riding > 0) && !has_no_weapon && !is_suitable;
}

bool has_not_ninja_weapon(player_type *player_ptr, int i)
{
    if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
        return false;
    }
    int tval = player_ptr->inventory_list[INVEN_MAIN_HAND + i].tval - TV_WEAPON_BEGIN;
    OBJECT_SUBTYPE_VALUE sval = player_ptr->inventory_list[INVEN_MAIN_HAND + i].sval;
    return player_ptr->pclass == CLASS_NINJA
        && !((s_info[CLASS_NINJA].w_max[tval][sval] > WEAPON_EXP_BEGINNER) && (player_ptr->inventory_list[INVEN_SUB_HAND - i].tval != TV_SHIELD));
}

bool has_not_monk_weapon(player_type *player_ptr, int i)
{
    if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
        return false;
    }
    int tval = player_ptr->inventory_list[INVEN_MAIN_HAND + i].tval - TV_WEAPON_BEGIN;
    OBJECT_SUBTYPE_VALUE sval = player_ptr->inventory_list[INVEN_MAIN_HAND + i].sval;
    return ((player_ptr->pclass == CLASS_MONK) || (player_ptr->pclass == CLASS_FORCETRAINER)) && !(s_info[player_ptr->pclass].w_max[tval][sval]);
}

bool has_good_luck(player_type *player_ptr)
{
    return (player_ptr->ppersonality == PERSONALITY_LUCKY) || (player_ptr->muta.has(MUTA::GOOD_LUCK));
}

BIT_FLAGS player_aggravate_state(player_type *player_ptr)
{
    if (player_ptr->cursed.has(TRC::AGGRAVATE)) {
        if ((PlayerRace(player_ptr).equals(PlayerRaceType::S_FAIRY)) && (player_ptr->ppersonality != PERSONALITY_SEXY)) {
            return AGGRAVATE_S_FAIRY;
        }
        return AGGRAVATE_NORMAL;
    }

    return AGGRAVATE_NONE;
}

bool has_aggravate(player_type *player_ptr)
{
    return player_aggravate_state(player_ptr) == AGGRAVATE_NORMAL;
}
