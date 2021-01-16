#include "player/player-status-flags.h"
#include "artifact/fixed-art-types.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
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
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"

static BIT_FLAGS check_equipment_flags(player_type *creature_ptr, tr_type tr_flag);

/*!
 * @brief 装備による所定の特性フラグを得ているかを一括して取得する関数。
 */
static BIT_FLAGS check_equipment_flags(player_type *creature_ptr, tr_type tr_flag)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    BIT_FLAGS result = 0L;
    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (has_flag(flgs, tr_flag))
            result |= 0x01 << (i - INVEN_RARM);
    }
    return result;
}

/*!
 * @brief クリーチャーが壁破壊進行を持っているかを返す。
 */
bool has_kill_wall(player_type *creature_ptr)
{
    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD || music_singing(creature_ptr, MUSIC_WALL)) {
        return TRUE;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags2 & RF2_KILL_WALL)
            return TRUE;
    }

    return FALSE;
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
    bool pow = FALSE;

    if (creature_ptr->wraith_form || creature_ptr->tim_pass_wall || (!creature_ptr->mimic_form && creature_ptr->prace == RACE_SPECTRE)) {
        pow = TRUE;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (!(riding_r_ptr->flags2 & RF2_PASS_WALL))
            pow = FALSE;
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
 * @brief クリーチャーが赤外線視力修正を持っているかを返す。
 * @param creature_ptr 判定対象のクリーチャー参照ポインタ
 * @return 持っていたら所持前提ビットフラグを返す。
 * @details 種族修正は0より大きければTRUEとする。
 */
BIT_FLAGS has_infra_vision(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    if (tmp_rp_ptr->infra > 0)
        result |= FLAG_CAUSE_RACE;

    if (creature_ptr->muta3 & MUT3_INFRAVIS)
        result |= FLAG_CAUSE_MUTATION;

    if (creature_ptr->tim_infra)
        result |= FLAG_CAUSE_MAGIC_TIME_EFFECT;

    result |= check_equipment_flags(creature_ptr, TR_INFRA);
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
            result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
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
    return check_equipment_flags(creature_ptr, TR_ESP_GOOD);
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

    if (is_time_limit_esp(creature_ptr) || creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->muta3 & MUT3_ESP) {
        result |= 0x01 << FLAG_CAUSE_MUTATION;
    }

    if (is_specific_player_race(creature_ptr, RACE_MIND_FLAYER) && creature_ptr->lev > 29)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (is_specific_player_race(creature_ptr, RACE_SPECTRE) && creature_ptr->lev > 34)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 39)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

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

void has_no_flowed(player_type *creature_ptr)
{
    object_type *o_ptr;
    bool has_sw = FALSE, has_kabe = FALSE;
    OBJECT_IDX this_o_idx, next_o_idx = 0;

    creature_ptr->no_flowed = FALSE;

    if (has_pass_wall(creature_ptr) && !has_kill_wall(creature_ptr))
        creature_ptr->no_flowed = TRUE;

    for (int i = 0; i < INVEN_PACK; i++) {
        if ((creature_ptr->inventory_list[i].tval == TV_NATURE_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            has_sw = TRUE;
        if ((creature_ptr->inventory_list[i].tval == TV_CRAFT_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            has_kabe = TRUE;
    }

    for (this_o_idx = creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        o_ptr = &creature_ptr->current_floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;

        if ((o_ptr->tval == TV_NATURE_BOOK) && (o_ptr->sval == 2))
            has_sw = TRUE;
        if ((o_ptr->tval == TV_CRAFT_BOOK) && (o_ptr->sval == 2))
            has_kabe = TRUE;
    }

    if (has_sw && ((creature_ptr->realm1 == REALM_NATURE) || (creature_ptr->realm2 == REALM_NATURE) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_NATURE - 1][SPELL_SW];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = TRUE;
    }

    if (has_kabe && ((creature_ptr->realm1 == REALM_CRAFT) || (creature_ptr->realm2 == REALM_CRAFT) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_CRAFT - 1][SPELL_WALL];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = TRUE;
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
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->pclass == CLASS_MIRROR_MASTER && creature_ptr->lev > 39)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->special_defense & KAMAE_GENBU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->wraith_form || creature_ptr->magicdef || creature_ptr->tim_reflect) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_REFLECT);
    return result;
}

BIT_FLAGS has_see_nocto(player_type *creature_ptr) { return creature_ptr->pclass == CLASS_NINJA ? FLAG_CAUSE_CLASS : 0L; }

BIT_FLAGS has_warning(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (has_flag(flgs, TR_WARNING)) {
            if (!o_ptr->inscription || !(angband_strchr(quark_str(o_ptr->inscription), '$')))
                result |= 0x01 << (i - INVEN_RARM);
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

    if (creature_ptr->muta3 & MUT3_FIRE_BODY) {
        result |= 0x01 << FLAG_CAUSE_MUTATION;
    }

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (hex_spelling(creature_ptr, HEX_DEMON_AURA) || creature_ptr->ult_res || creature_ptr->tim_sh_fire) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_SH_FIRE);
    return result;
}

BIT_FLAGS has_sh_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta3 & MUT3_ELEC_TOUC)
        result |= 0x01 << FLAG_CAUSE_MUTATION;

    if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK) || creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || (creature_ptr->special_defense & KATA_MUSOU)) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SH_ELEC);
    return result;
}

BIT_FLAGS has_sh_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || hex_spelling(creature_ptr, HEX_ICE_ARMOR)) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
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
        result |= 0x01 << FLAG_CAUSE_PERSONALITY;
    }

    if (creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_DEMON_LORD || creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (is_specific_player_race(creature_ptr, RACE_HOBBIT) || is_specific_player_race(creature_ptr, RACE_SKELETON)
        || is_specific_player_race(creature_ptr, RACE_ZOMBIE) || is_specific_player_race(creature_ptr, RACE_VAMPIRE)
        || is_specific_player_race(creature_ptr, RACE_SPECTRE) || is_specific_player_race(creature_ptr, RACE_BALROG)
        || is_specific_player_race(creature_ptr, RACE_ANDROID)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (is_specific_player_race(creature_ptr, RACE_GOLEM)) {
        if (creature_ptr->lev > 34)
            result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_HOLD_EXP);
    return result;
}

BIT_FLAGS has_see_inv(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 29)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_DEMON_LORD || creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    } else if (is_specific_player_race(creature_ptr, RACE_HIGH_ELF) || is_specific_player_race(creature_ptr, RACE_GOLEM)
        || is_specific_player_race(creature_ptr, RACE_SKELETON) || is_specific_player_race(creature_ptr, RACE_ZOMBIE)
        || is_specific_player_race(creature_ptr, RACE_SPECTRE) || is_specific_player_race(creature_ptr, RACE_ARCHON)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    } else if (is_specific_player_race(creature_ptr, RACE_DARK_ELF) && creature_ptr->lev > 19) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    } else if (is_specific_player_race(creature_ptr, RACE_MIND_FLAYER) && creature_ptr->lev > 14) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    } else if ((is_specific_player_race(creature_ptr, RACE_IMP) || is_specific_player_race(creature_ptr, RACE_BALROG)) && creature_ptr->lev > 9) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->tim_invis) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
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

    if (creature_ptr->muta3 & MUT3_MOTION)
        result |= 0x01 << FLAG_CAUSE_MUTATION;

    if (is_specific_player_race(creature_ptr, RACE_GNOME) || is_specific_player_race(creature_ptr, RACE_GOLEM)
        || is_specific_player_race(creature_ptr, RACE_SPECTRE) || is_specific_player_race(creature_ptr, RACE_ANDROID)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->pclass == CLASS_NINJA && !heavy_armor(creature_ptr) && (!creature_ptr->inventory_list[INVEN_RARM].k_idx || has_right_hand_weapon(creature_ptr))
        && (!creature_ptr->inventory_list[INVEN_LARM].k_idx || has_left_hand_weapon(creature_ptr))) {
        if (creature_ptr->lev > 24)
            result |= 0x01 << FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->pclass == CLASS_MONK || creature_ptr->pclass == CLASS_FORCETRAINER) {
        if (!(heavy_armor(creature_ptr))) {
            if (creature_ptr->lev > 24)
                result |= 0x01 << FLAG_CAUSE_CLASS;
        }
    }

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= 0x01 << FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->ult_res || creature_ptr->magicdef) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_FREE_ACT);
    return result;
}

BIT_FLAGS has_sustain_str(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= 0x01 << FLAG_CAUSE_CLASS;
    }
    if (is_specific_player_race(creature_ptr, RACE_HALF_TROLL) || is_specific_player_race(creature_ptr, RACE_HALF_OGRE)
        || is_specific_player_race(creature_ptr, RACE_HALF_GIANT)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_STR);
    return result;
}

BIT_FLAGS has_sustain_int(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (is_specific_player_race(creature_ptr, RACE_MIND_FLAYER)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_INT);
    return result;
}

BIT_FLAGS has_sustain_wis(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    result = FALSE;
    if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 19)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_MIND_FLAYER)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_WIS);
    return result;
}

BIT_FLAGS has_sustain_dex(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= 0x01 << FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 24)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_DEX);
    return result;
}

BIT_FLAGS has_sustain_con(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= 0x01 << FLAG_CAUSE_CLASS;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_AMBERITE || creature_ptr->prace == RACE_DUNADAN)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_CON);
    return result;
}

BIT_FLAGS has_sustain_chr(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_SUST_CHR);
    return result;
}

BIT_FLAGS has_levitation(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta3 & MUT3_WINGS)
        result = FLAG_CAUSE_MUTATION;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result = FLAG_CAUSE_RACE;
    }

    if (is_specific_player_race(creature_ptr, RACE_DRACONIAN) || is_specific_player_race(creature_ptr, RACE_SPECTRE)
        || is_specific_player_race(creature_ptr, RACE_SPRITE) || is_specific_player_race(creature_ptr, RACE_ARCHON)
        || is_specific_player_race(creature_ptr, RACE_S_FAIRY)) {
        result = FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KAMAE_SUZAKU || (creature_ptr->special_defense & KATA_MUSOU)) {
        result = FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        result = (riding_r_ptr->flags7 & RF7_CAN_FLY) ? result : 0;
    }

    if (creature_ptr->tim_levitation) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_LEVITATION);
    return result;
}

void has_can_swim(player_type *creature_ptr)
{
    creature_ptr->can_swim = FALSE;
    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags7 & (RF7_CAN_SWIM | RF7_AQUATIC))
            creature_ptr->can_swim = TRUE;
    }
}

BIT_FLAGS has_slow_digest(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_NINJA) {
        result = FLAG_CAUSE_CLASS;
    }

    if (creature_ptr->lev > 14 && !creature_ptr->mimic_form && creature_ptr->prace == RACE_HALF_TROLL) {
        if (creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_BERSERKER) {
            result = FLAG_CAUSE_CLASS;
            /* Let's not make Regeneration
             * a disadvantage for the poor warriors who can
             * never learn a spell that satisfies hunger (actually
             * neither can rogues, but half-trolls are not
             * supposed to play rogues) */
        }
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result = FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_GOLEM || creature_ptr->prace == RACE_ZOMBIE || creature_ptr->prace == RACE_SPECTRE
            || creature_ptr->prace == RACE_ANDROID)) {
        result = FLAG_CAUSE_RACE;
    }

    result |= check_equipment_flags(creature_ptr, TR_SLOW_DIGEST);
    return result;
}

BIT_FLAGS has_regenerate(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (is_specific_player_race(creature_ptr, RACE_HALF_TROLL) && creature_ptr->lev > 14) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (is_specific_player_race(creature_ptr, RACE_AMBERITE)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->pclass == CLASS_WARRIOR && creature_ptr->lev > 44) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->muta3 & MUT3_REGEN)
        result |= 0x01 << FLAG_CAUSE_MUTATION;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_MUTATION;
    }

    if (hex_spelling(creature_ptr, HEX_DEMON_AURA) || creature_ptr->ult_res || creature_ptr->tim_regen) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_REGEN);

    if (creature_ptr->muta3 & MUT3_FLESH_ROT)
        result = 0L;

    return result;
}

void has_curses(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->cursed = 0L;

    if (creature_ptr->pseikaku == PERSONALITY_SEXY)
        creature_ptr->cursed |= (TRC_AGGRAVATE);

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_AGGRAVATE))
            creature_ptr->cursed |= TRC_AGGRAVATE;
        if (has_flag(flgs, TR_DRAIN_EXP))
            creature_ptr->cursed |= TRC_DRAIN_EXP;
        if (has_flag(flgs, TR_TY_CURSE))
            creature_ptr->cursed |= TRC_TY_CURSE;
        if (has_flag(flgs, TR_ADD_L_CURSE))
            creature_ptr->cursed |= TRC_ADD_L_CURSE;
        if (has_flag(flgs, TR_ADD_H_CURSE))
            creature_ptr->cursed |= TRC_ADD_H_CURSE;
        if (has_flag(flgs, TR_DRAIN_HP))
            creature_ptr->cursed |= TRC_DRAIN_HP;
        if (has_flag(flgs, TR_DRAIN_MANA))
            creature_ptr->cursed |= TRC_DRAIN_MANA;
        if (has_flag(flgs, TR_CALL_ANIMAL))
            creature_ptr->cursed |= TRC_CALL_ANIMAL;
        if (has_flag(flgs, TR_CALL_DEMON))
            creature_ptr->cursed |= TRC_CALL_DEMON;
        if (has_flag(flgs, TR_CALL_DRAGON))
            creature_ptr->cursed |= TRC_CALL_DRAGON;
        if (has_flag(flgs, TR_CALL_UNDEAD))
            creature_ptr->cursed |= TRC_CALL_UNDEAD;
        if (has_flag(flgs, TR_COWARDICE))
            creature_ptr->cursed |= TRC_COWARDICE;
        if (has_flag(flgs, TR_LOW_MELEE))
            creature_ptr->cursed |= TRC_LOW_MELEE;
        if (has_flag(flgs, TR_LOW_AC))
            creature_ptr->cursed |= TRC_LOW_AC;
        if (has_flag(flgs, TR_LOW_MAGIC))
            creature_ptr->cursed |= TRC_LOW_MAGIC;
        if (has_flag(flgs, TR_FAST_DIGEST))
            creature_ptr->cursed |= TRC_FAST_DIGEST;
        if (has_flag(flgs, TR_SLOW_REGEN))
            creature_ptr->cursed |= TRC_SLOW_REGEN;

        creature_ptr->cursed |= (o_ptr->curse_flags & (0xFFFFFFF0L));
        if (o_ptr->name1 == ART_CHAINSWORD)
            creature_ptr->cursed |= TRC_CHAINSWORD;

        if (has_flag(flgs, TR_TELEPORT)) {
            if (object_is_cursed(o_ptr))
                creature_ptr->cursed |= TRC_TELEPORT;
            else {
                concptr insc = quark_str(o_ptr->inscription);

                /* {.} will stop random teleportation. */
                if (o_ptr->inscription && angband_strchr(insc, '.')) {
                } else {
                    creature_ptr->cursed |= TRC_TELEPORT_SELF;
                }
            }
        }
    }

    if (creature_ptr->cursed & TRC_TELEPORT)
        creature_ptr->cursed &= ~(TRC_TELEPORT_SELF);

    if ((is_specific_player_race(creature_ptr, RACE_S_FAIRY)) && (creature_ptr->pseikaku != PERSONALITY_SEXY) && (creature_ptr->cursed & TRC_AGGRAVATE)) {
        creature_ptr->cursed &= ~(TRC_AGGRAVATE);
    }
}

BIT_FLAGS has_impact(player_type *creature_ptr)
{
    return check_equipment_flags(creature_ptr, TR_IMPACT);
}

void has_extra_blow(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->extra_blows[0] = creature_ptr->extra_blows[1] = 0;

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_BLOWS)) {
            if ((i == INVEN_RARM || i == INVEN_RIGHT) && !has_two_handed_weapons(creature_ptr))
                creature_ptr->extra_blows[0] += o_ptr->pval;
            else if ((i == INVEN_LARM || i == INVEN_LEFT) && !has_two_handed_weapons(creature_ptr))
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

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    } else if (is_specific_player_race(creature_ptr, RACE_YEEK) || is_specific_player_race(creature_ptr, RACE_KLACKON)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    } else if (is_specific_player_race(creature_ptr, RACE_DRACONIAN) && creature_ptr->lev > 14) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= has_immune_acid(creature_ptr);

    result |= check_equipment_flags(creature_ptr, TR_RES_ACID);
    return result;
}

BIT_FLAGS has_vuln_acid(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->muta3 & MUT3_VULN_ELEM) {
        result |= 0x01 << FLAG_CAUSE_MUTATION;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }
    return result;
}

BIT_FLAGS has_resist_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    } else if (is_specific_player_race(creature_ptr, RACE_DRACONIAN) && creature_ptr->lev > 19) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_ELEC);
    result |= has_immune_elec(creature_ptr);
    return result;
}

BIT_FLAGS has_vuln_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->muta3 & MUT3_VULN_ELEM) {
        result |= 0x01 << FLAG_CAUSE_MUTATION;
    }

    if (is_specific_player_race(creature_ptr, RACE_ANDROID)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }
    return result;
}

BIT_FLAGS has_resist_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (is_specific_player_race(creature_ptr, RACE_DRACONIAN) && creature_ptr->lev > 4) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_IMP || creature_ptr->prace == RACE_BALROG)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_FIRE);
    result |= has_immune_fire(creature_ptr);
    return result;
}

BIT_FLAGS has_vuln_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->muta3 & MUT3_VULN_ELEM) {
        result |= 0x01 << FLAG_CAUSE_MUTATION;
    }

    if (is_specific_player_race(creature_ptr, RACE_ENT)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }
    return result;
}

BIT_FLAGS has_resist_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD || creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_ZOMBIE) && creature_ptr->lev > 4) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if ((is_specific_player_race(creature_ptr, RACE_DRACONIAN) || is_specific_player_race(creature_ptr, RACE_SKELETON)) && creature_ptr->lev > 9) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_VAMPIRE || creature_ptr->prace == RACE_SPECTRE)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_COLD);
    result |= has_immune_cold(creature_ptr);
    return result;
}

BIT_FLAGS has_vuln_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->muta3 & MUT3_VULN_ELEM) {
        result |= 0x01 << FLAG_CAUSE_MUTATION;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }
    return result;
}

BIT_FLAGS has_resist_pois(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 19)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->mimic_form == MIMIC_VAMPIRE || creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (is_specific_player_race(creature_ptr, RACE_DRACONIAN) && creature_ptr->lev > 34) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_KOBOLD || creature_ptr->prace == RACE_GOLEM || creature_ptr->prace == RACE_SKELETON
            || creature_ptr->prace == RACE_VAMPIRE || creature_ptr->prace == RACE_SPECTRE || creature_ptr->prace == RACE_ANDROID)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_POIS);
    return result;
}

BIT_FLAGS has_resist_conf(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 29)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN || creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        result |= 0x01 << FLAG_CAUSE_PERSONALITY;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_KLACKON || creature_ptr->prace == RACE_BEASTMAN || creature_ptr->prace == RACE_KUTAR)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->ult_res || creature_ptr->magicdef) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_CONF);
    return result;
}

BIT_FLAGS has_resist_sound(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_BARD) {
        result |= 0x01 << FLAG_CAUSE_CLASS;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_CYCLOPS || creature_ptr->prace == RACE_BEASTMAN)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_SOUND);
    return result;
}

BIT_FLAGS has_resist_lite(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_ELF || creature_ptr->prace == RACE_HIGH_ELF || creature_ptr->prace == RACE_SPRITE)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_LITE);
    return result;
}

BIT_FLAGS has_vuln_lite(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (is_specific_player_race(creature_ptr, RACE_S_FAIRY) || is_specific_player_race(creature_ptr, RACE_VAMPIRE)
        || (creature_ptr->mimic_form == MIMIC_VAMPIRE)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->wraith_form) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

BIT_FLAGS has_resist_dark(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_HALF_ORC || creature_ptr->prace == RACE_HALF_OGRE || creature_ptr->prace == RACE_NIBELUNG
            || creature_ptr->prace == RACE_DARK_ELF || creature_ptr->prace == RACE_VAMPIRE)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_DARK);
    return result;
}

BIT_FLAGS has_resist_chaos(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pclass == CLASS_CHAOS_WARRIOR && creature_ptr->lev > 29)
        result |= 0x01 << FLAG_CAUSE_CLASS;

    if (creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_HALF_TITAN)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_CHAOS);
    return result;
}

BIT_FLAGS has_resist_disen(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_NIBELUNG)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_DISEN);
    return result;
}

BIT_FLAGS has_resist_shard(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_HALF_TITAN || creature_ptr->prace == RACE_SKELETON))
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_SHARDS);
    return result;
}

BIT_FLAGS has_resist_nexus(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_NEXUS);
    return result;
}

BIT_FLAGS has_resist_blind(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        result |= 0x01 << FLAG_CAUSE_PERSONALITY;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_DWARF)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->magicdef) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_BLIND);
    return result;
}

BIT_FLAGS has_resist_neth(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD || creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_ZOMBIE || creature_ptr->prace == RACE_VAMPIRE || creature_ptr->prace == RACE_SPECTRE
            || creature_ptr->prace == RACE_BALROG))
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (creature_ptr->ult_res || creature_ptr->tim_res_nether) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_NETHER);
    return result;
}

BIT_FLAGS has_resist_time(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->tim_res_time) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_TIME);
    return result;
}

BIT_FLAGS has_resist_water(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_MERFOLK)
        result |= 0x01 << FLAG_CAUSE_RACE;

    result |= check_equipment_flags(creature_ptr, TR_RES_WATER);
    return result;
}

BIT_FLAGS has_resist_fear(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->muta3 & MUT3_FEARLESS)
        result |= 0x01 << FLAG_CAUSE_MUTATION;

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
    case CLASS_SAMURAI:
        if (creature_ptr->lev > 29)
            result |= 0x01 << FLAG_CAUSE_CLASS;
        break;
    case CLASS_PALADIN:
    case CLASS_CHAOS_WARRIOR:
        if (creature_ptr->lev > 39)
            result |= 0x01 << FLAG_CAUSE_CLASS;
        break;
    case CLASS_MINDCRAFTER:
        if (creature_ptr->lev > 9)
            result |= 0x01 << FLAG_CAUSE_CLASS;
        break;
    case CLASS_NINJA:
        result |= 0x01 << FLAG_CAUSE_CLASS;
        break;
    }

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_BARBARIAN)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if ((creature_ptr->special_defense & KATA_MUSOU)) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    if (is_hero(creature_ptr) || is_shero(creature_ptr) || creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_RES_FEAR);
    return result;
}

BIT_FLAGS has_immune_acid(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_YEEK && creature_ptr->lev > 19)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_ACID)
            result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_IM_ACID);
    return result;
}

BIT_FLAGS has_immune_elec(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_ELEC)
            result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_IM_ELEC);
    return result;
}

BIT_FLAGS has_immune_fire(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_FIRE)
            result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_IM_FIRE);
    return result;
}

BIT_FLAGS has_immune_cold(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (creature_ptr->ele_immune) {
        if (creature_ptr->special_defense & DEFENSE_COLD)
            result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    result |= check_equipment_flags(creature_ptr, TR_IM_COLD);
    return result;
}

BIT_FLAGS has_immune_dark(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;

    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE)) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (creature_ptr->wraith_form) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    return result;
}

/*
 * @brief 右手(利き手)が武器を持っているかどうかを判定する
 * @detail Includes martial arts and hand combats as weapons.
 */
bool has_right_hand_weapon(player_type *creature_ptr)
{
    if (has_melee_weapon(creature_ptr, INVEN_RARM))
        return TRUE;

    if ((empty_hands(creature_ptr, TRUE) & EMPTY_HAND_RARM) && !has_left_hand_weapon(creature_ptr))
        return TRUE;

    return FALSE;
}

/*
 * @brief 左手(非利き手)が武器を持っているかどうかを判定する
 * @detail Exclude martial arts and hand combats from weapons.
 */
bool has_left_hand_weapon(player_type *creature_ptr) { return has_melee_weapon(creature_ptr, INVEN_LARM); }

bool has_two_handed_weapons(player_type *creature_ptr)
{
    if (can_two_hands_wielding(creature_ptr)) {
        if (has_right_hand_weapon(creature_ptr) && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_LARM)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_RARM])) {
            return TRUE;
        } else if (has_left_hand_weapon(creature_ptr) && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_RARM)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_LARM])) {
            return TRUE;
        }
    }
    return FALSE;
}

BIT_FLAGS has_lite(player_type *creature_ptr)
{
    BIT_FLAGS result = 0L;
    if (creature_ptr->pclass == CLASS_NINJA)
        return 0L;

    if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        result |= 0x01 << FLAG_CAUSE_PERSONALITY;
    }

    if (creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        result |= 0x01 << FLAG_CAUSE_RACE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_VAMPIRE)
        result |= 0x01 << FLAG_CAUSE_RACE;

    if (creature_ptr->ult_res) {
        result |= 0x01 << FLAG_CAUSE_MAGIC_TIME_EFFECT;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        result |= 0x01 << FLAG_CAUSE_BATTLE_FORM;
    }

    result |= has_sh_fire(creature_ptr);

    return result;
}

/*
 * @brief 両手持ちボーナスがもらえないかどうかを判定する。 / Does *not * get two hand wielding bonus.
 * @detail
 *  Only can get hit bonuses when wieids an enough light weapon which is lighter than 5 times of weight limit.
 *  If its weight is 10 times heavier or more than weight limit, gets hit penalty in calc_to_hit().
 */
bool has_disable_two_handed_bonus(player_type *creature_ptr, int i)
{
    if (has_melee_weapon(creature_ptr, INVEN_RARM + i) && has_two_handed_weapons(creature_ptr)) {
        object_type *o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];
        int limit = calc_weapon_weight_limit(creature_ptr) * 2;

        /* Enable when two hand wields an enough light weapon */
        if (limit >= o_ptr->weight / 5)
            return FALSE;
    }

    /* Disable when empty hands, one hand wieldings and heavy weapons */
    return TRUE;
}

bool has_icky_wield_weapon(player_type *creature_ptr, int i)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];
    object_flags(creature_ptr, o_ptr, flgs);

    if ((creature_ptr->pclass == CLASS_PRIEST) && (!(has_flag(flgs, TR_BLESSED))) && ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM))) {
        return TRUE;
    } else if (creature_ptr->pclass == CLASS_SORCERER) {
        if (!((o_ptr->tval == TV_HAFTED) && ((o_ptr->sval == SV_WIZSTAFF) || (o_ptr->sval == SV_NAMAKE_HAMMER)))) {
            return TRUE;
        }
    }
    if (has_not_monk_weapon(creature_ptr, i) || has_not_ninja_weapon(creature_ptr, i)) {
        return TRUE;
    }
    return FALSE;
}

bool has_riding_wield_weapon(player_type *creature_ptr, int i)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];
    object_flags(creature_ptr, o_ptr, flgs);
    if (creature_ptr->riding != 0 && !(o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE))
        && !has_flag(flgs, TR_RIDING)) {
        return TRUE;
    }
    return FALSE;
}

bool has_not_ninja_weapon(player_type *creature_ptr, int i)
{
    tval_type tval = creature_ptr->inventory_list[INVEN_RARM + i].tval - TV_WEAPON_BEGIN;
    OBJECT_SUBTYPE_VALUE sval = creature_ptr->inventory_list[INVEN_RARM + i].sval;
    return creature_ptr->pclass == CLASS_NINJA
        && !((s_info[CLASS_NINJA].w_max[tval][sval] > WEAPON_EXP_BEGINNER) && (creature_ptr->inventory_list[INVEN_LARM - i].tval != TV_SHIELD));
}

bool has_not_monk_weapon(player_type *creature_ptr, int i)
{
    tval_type tval = creature_ptr->inventory_list[INVEN_RARM + i].tval - TV_WEAPON_BEGIN;
    OBJECT_SUBTYPE_VALUE sval = creature_ptr->inventory_list[INVEN_RARM + i].sval;
    return ((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER))
        && !(s_info[creature_ptr->pclass].w_max[tval][sval]);
}

bool has_good_luck(player_type *creature_ptr) { return (creature_ptr->pseikaku == PERSONALITY_LUCKY) || (creature_ptr->muta3 & MUT3_GOOD_LUCK); }
