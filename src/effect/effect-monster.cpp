/*!
 * @brief 魔法によるモンスターへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-monster.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-monster-switcher.h"
#include "effect/effect-monster-util.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-list.h"
#include "floor/floor-object.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "object-enchant/special-object-flags.h"
#include "spell-kind/blood-curse.h"
#include "spell-kind/spells-polymorph.h"
#include "spell-kind/spells-teleport.h"
#include "sv-definition/sv-other-types.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/health-bar-tracker.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>

/*!
 * @brief ビーム/ボルト/ボール系魔法によるモンスターへの効果があるかないかを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 効果が何もないならFALSE、何かあるならTRUE
 */
static ProcessResult is_affective(EffectMonster *em_ptr)
{
    if (!em_ptr->g_ptr->has_monster()) {
        return ProcessResult::PROCESS_FALSE;
    }
    if (is_monster(em_ptr->src_idx) && (em_ptr->g_ptr->m_idx == em_ptr->src_idx)) {
        return ProcessResult::PROCESS_FALSE;
    }
    if (sukekaku && ((em_ptr->m_ptr->r_idx == MonsterRaceId::SUKE) || (em_ptr->m_ptr->r_idx == MonsterRaceId::KAKU))) {
        return ProcessResult::PROCESS_FALSE;
    }
    if (em_ptr->m_ptr->hp < 0) {
        return ProcessResult::PROCESS_FALSE;
    }
    if (is_monster(em_ptr->src_idx) || !em_ptr->m_ptr->is_riding()) {
        return ProcessResult::PROCESS_TRUE;
    }

    switch (em_ptr->attribute) {
    case AttributeType::OLD_HEAL:
    case AttributeType::OLD_SPEED:
    case AttributeType::STAR_HEAL:
        return ProcessResult::PROCESS_TRUE;
    case AttributeType::OLD_SLOW:
    case AttributeType::OLD_SLEEP:
    case AttributeType::OLD_CLONE:
    case AttributeType::OLD_CONF:
    case AttributeType::OLD_POLY:
    case AttributeType::GENOCIDE:
    case AttributeType::E_GENOCIDE:
        return ProcessResult::PROCESS_CONTINUE;
    default:
        break;
    }

    return ProcessResult::PROCESS_FALSE;
}

/*!
 * @brief 魔法の効果やモンスター種別(MAKE/FEMALE/なし)に応じて表示するメッセージを変更する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void make_description_of_affecred_monster(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    em_ptr->dam = (em_ptr->dam + em_ptr->r) / (em_ptr->r + 1);
    angband_strcpy(em_ptr->m_name, monster_desc(player_ptr, em_ptr->m_ptr, 0), sizeof(em_ptr->m_name));
    angband_strcpy(em_ptr->m_poss, monster_desc(player_ptr, em_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE), sizeof(em_ptr->m_poss));
}

/*!
 * @brief モンスターへの効果属性による耐性及び効果を処理する( / Proccess affecting to monster by effect.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 完全な耐性が発動したらCONTINUE、そうでないなら効果処理の結果
 * @details
 * 完全な耐性を持っていたら、一部属性を除いて影響は及ぼさない
 * デバッグ属性、モンスター打撃、モンスター射撃であれば貫通する
 */
static ProcessResult exe_affect_monster_by_effect(PlayerType *player_ptr, EffectMonster *em_ptr, std::optional<CapturedMonsterType *> cap_mon_ptr)
{
    const std::vector<AttributeType> effect_arrtibute = {
        AttributeType::OLD_CLONE,
        AttributeType::STAR_HEAL,
        AttributeType::OLD_HEAL,
        AttributeType::OLD_SPEED,
        AttributeType::CAPTURE,
        AttributeType::PHOTO,
    };
    const auto check = [em_ptr](const AttributeType attribute) {
        return em_ptr->attribute == attribute;
    };

    ProcessResult result = is_affective(em_ptr);
    if (result != ProcessResult::PROCESS_TRUE) {
        if (result == ProcessResult::PROCESS_CONTINUE) {
            em_ptr->note = _("には効果がなかった。", " is unaffected.");
            em_ptr->dam = 0;
        }
        return result;
    }

    bool do_effect = em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_ALL);
    do_effect |= std::any_of(effect_arrtibute.cbegin(), effect_arrtibute.cend(), check);

    if (do_effect) {
        return switch_effects_monster(player_ptr, em_ptr, cap_mon_ptr);
    }

    bool ignore_res_all = (em_ptr->attribute == AttributeType::DEBUG);
    ignore_res_all |= (em_ptr->attribute == AttributeType::MONSTER_MELEE);
    ignore_res_all |= (em_ptr->attribute == AttributeType::MONSTER_SHOOT);

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL) && ignore_res_all) {
        return switch_effects_monster(player_ptr, em_ptr);
    }

    em_ptr->note = _("には完全な耐性がある！", " is immune.");
    em_ptr->dam = 0;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_ALL);
    }

    if (em_ptr->attribute == AttributeType::LITE_WEAK || em_ptr->attribute == AttributeType::KILL_WALL) {
        em_ptr->skipped = true;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief ペットの死亡を処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void effect_damage_killed_pet(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    auto &floor = FloorList::get_instance().get_floor(0);
    bool sad = em_ptr->m_ptr->is_pet() && !(em_ptr->m_ptr->ml);
    if (em_ptr->known && !em_ptr->note.empty()) {
        angband_strcpy(em_ptr->m_name, monster_desc(player_ptr, em_ptr->m_ptr, MD_TRUE_NAME), sizeof(em_ptr->m_name));
        if (em_ptr->see_s_msg) {
            msg_format("%s^%s", em_ptr->m_name, em_ptr->note.data());
        } else {
            floor.monster_noise = true;
        }
    }

    if (is_monster(em_ptr->src_idx)) {
        monster_gain_exp(player_ptr, em_ptr->src_idx, em_ptr->m_ptr->r_idx);
    }

    monster_death(player_ptr, em_ptr->g_ptr->m_idx, false, em_ptr->attribute);
    delete_monster_idx(player_ptr, em_ptr->g_ptr->m_idx);
    if (sad) {
        msg_print(_("少し悲しい気分がした。", "You feel sad for a moment."));
    }
}

/*!
 * @brief モンスターの睡眠を処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void effect_damage_makes_sleep(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    auto &floor = FloorList::get_instance().get_floor(0);
    if (!em_ptr->note.empty() && em_ptr->seen_msg) {
        msg_format("%s^%s", em_ptr->m_name, em_ptr->note.data());
    } else if (em_ptr->see_s_msg) {
        const auto m_name = monster_desc(player_ptr, em_ptr->m_ptr, 0);
        const auto pain_message = em_ptr->m_ptr->get_pain_message(m_name, em_ptr->dam);
        if (pain_message) {
            msg_print(*pain_message);
        }
    } else {
        floor.monster_noise = true;
    }

    if (em_ptr->do_sleep) {
        (void)set_monster_csleep(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_sleep);
    }
}

/*!
 * @brief モンスターからモンスターへのダメージを処理する / Hurt the monster by damages another monster did.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return ダメージを処理しなかった(モンスターIDがプレイヤー自身)場合はFALSE、処理した(モンスターだった)場合TRUE
 * @details
 * モンスターIDがプレイヤー(0)の場合は処理しない。
 */
static bool deal_effect_damage_from_monster(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (!is_monster(em_ptr->src_idx)) {
        return false;
    }

    HealthBarTracker::get_instance().set_flag_if_tracking(em_ptr->g_ptr->m_idx);
    if (em_ptr->m_ptr->is_riding()) {
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
    }

    (void)set_monster_csleep(player_ptr, em_ptr->g_ptr->m_idx, 0);
    em_ptr->m_ptr->hp -= em_ptr->dam;
    if (em_ptr->m_ptr->hp < 0) {
        effect_damage_killed_pet(player_ptr, em_ptr);
    } else {
        effect_damage_makes_sleep(player_ptr, em_ptr);
    }

    return true;
}

/*!
 * @brief 不潔な病人の治療処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 大賞モンスターが不潔な病人だった場合はTRUE、それ以外はFALSE
 */
static bool heal_leaper(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (!em_ptr->heal_leper) {
        return false;
    }

    if (em_ptr->seen_msg) {
        msg_print(_("不潔な病人は病気が治った！", "The Mangy looking leper is healed!"));
    }

    if (record_named_pet && em_ptr->m_ptr->is_named_pet()) {
        const auto m2_name = monster_desc(player_ptr, em_ptr->m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(*player_ptr->current_floor_ptr, DiaryKind::NAMED_PET, RECORD_NAMED_PET_HEAL_LEPER, m2_name);
    }

    delete_monster_idx(player_ptr, em_ptr->g_ptr->m_idx);
    return true;
}

/*!
 * @brief プレイヤー起因の効果によるダメージを処理 / Deal damages from player and fear by them.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return モンスターが死んだらTRUE、生きていたらFALSE
 * @details
 * em_ptr->do_fearによる恐怖メッセージもここで表示。
 */
static bool deal_effect_damage_from_player(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    bool fear = false;
    MonsterDamageProcessor mdp(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, &fear, em_ptr->attribute);
    if (mdp.mon_take_hit(em_ptr->note_dies)) {
        return true;
    }

    if (em_ptr->do_sleep) {
        anger_monster(player_ptr, em_ptr->m_ptr);
    }

    if (!em_ptr->note.empty() && em_ptr->seen) {
        msg_format(_("%s%s", "%s^%s"), em_ptr->m_name, em_ptr->note.data());
    } else if (em_ptr->known && (em_ptr->dam || !em_ptr->do_fear)) {
        const auto m_name = monster_desc(player_ptr, em_ptr->m_ptr, 0);
        const auto pain_message = em_ptr->m_ptr->get_pain_message(m_name, em_ptr->dam);
        if (pain_message) {
            msg_print(*pain_message);
        }
    }

    if (((em_ptr->dam > 0) || em_ptr->get_angry) && !em_ptr->do_sleep) {
        anger_monster(player_ptr, em_ptr->m_ptr);
    }

    if ((fear || em_ptr->do_fear) && em_ptr->seen) {
        sound(SOUND_FLEE);
        msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), em_ptr->m_name);
    }

    return false;
}

/*!
 * @brief モンスターに効果のダメージを与える / Deal effect damage to monster.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @details
 * 以下のいずれかの処理を行う。
 * 1.モンスターによる効果ダメージの処理
 * 2.不潔な病人を癒す処理
 * 3.プレイヤーによる効果ダメージの処理
 * 4.睡眠する処理
 */
static void deal_effect_damage_to_monster(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->attribute == AttributeType::DRAIN_MANA) {
        return;
    }

    // モンスターによる効果
    if (deal_effect_damage_from_monster(player_ptr, em_ptr)) {
        return;
    }

    // プレイヤーによる効果
    if (heal_leaper(player_ptr, em_ptr)) {
        return;
    }
    if (deal_effect_damage_from_player(player_ptr, em_ptr)) {
        return;
    }

    if (em_ptr->do_sleep) {
        (void)set_monster_csleep(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_sleep);
    }
}

/*!
 * @brief プレイヤーが眠っている敵に効果を及ぼした場合の徳の変化
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void effect_makes_change_virtues(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (is_monster(em_ptr->src_idx) || !em_ptr->slept) {
        return;
    }

    if (em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::EVIL) || one_in_(5)) {
        chg_virtue(player_ptr, Virtue::COMPASSION, -1);
    }
    if (em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::EVIL) || one_in_(5)) {
        chg_virtue(player_ptr, Virtue::HONOUR, -1);
    }
}

/*!
 * @brief 魔法効果に対する強制処理(変身の強制解除、死なない処理)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void affected_monster_prevents_bad_status(EffectMonster *em_ptr)
{
    const auto *r_ptr = em_ptr->r_ptr;
    auto can_avoid_polymorph = r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    can_avoid_polymorph |= r_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    can_avoid_polymorph |= em_ptr->m_ptr->is_riding();
    if (can_avoid_polymorph) {
        em_ptr->do_polymorph = false;
    }

    auto should_alive = r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    should_alive |= r_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    should_alive |= r_ptr->population_flags.has(MonsterPopulationType::NAZGUL);
    if (should_alive && !AngbandSystem::get_instance().is_phase_out() && is_monster(em_ptr->src_idx) && (em_ptr->dam > em_ptr->m_ptr->hp)) {
        em_ptr->dam = em_ptr->m_ptr->hp;
    }
}

/*!
 * @brief モンスターの朦朧値を蓄積させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param stun_damage 朦朧値
 */
static void effect_damage_piles_stun(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    const auto *r_ptr = em_ptr->r_ptr;
    auto can_avoid_stun = em_ptr->do_stun == 0;
    can_avoid_stun |= r_ptr->resistance_flags.has_any_of({ MonsterResistanceType::RESIST_SOUND, MonsterResistanceType::RESIST_FORCE });
    can_avoid_stun |= r_ptr->resistance_flags.has(MonsterResistanceType::NO_STUN);
    if (can_avoid_stun) {
        return;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    int turns = 0;
    if (em_ptr->m_ptr->get_remaining_stun()) {
        em_ptr->note = _("はひどくもうろうとした。", " is more dazed.");
        turns = em_ptr->m_ptr->get_remaining_stun() + (em_ptr->do_stun / 2);
    } else {
        em_ptr->note = _("はもうろうとした。", " is dazed.");
        turns = em_ptr->do_stun;
    }

    (void)set_monster_stunned(player_ptr, em_ptr->g_ptr->m_idx, turns);
    em_ptr->get_angry = true;
}

/*!
 * @brief モンスターの混乱値を蓄積させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param stun_damage 混乱値
 */
static void effect_damage_piles_confusion(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if ((em_ptr->do_conf == 0) || (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::NO_CONF)) || em_ptr->r_ptr->resistance_flags.has_any_of(RFR_EFF_RESIST_CHAOS_MASK)) {
        return;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    int turns = 0;
    if (em_ptr->m_ptr->is_confused()) {
        em_ptr->note = _("はさらに混乱したようだ。", " looks more confused.");
        turns = em_ptr->m_ptr->get_remaining_confusion() + (em_ptr->do_conf / 2);
    } else {
        em_ptr->note = _("は混乱したようだ。", " looks confused.");
        turns = em_ptr->do_conf;
    }

    (void)set_monster_confused(player_ptr, em_ptr->g_ptr->m_idx, turns);
    em_ptr->get_angry = true;
}

/*!
 * @brief モンスターの恐怖値を蓄積させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param stun_damage 恐怖値
 * @details
 * 打撃ダメージによる恐怖もあるため、メッセージは後で表示。
 */
static void effect_damage_piles_fear(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->do_fear == 0 || em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::NO_FEAR)) {
        return;
    }

    (void)set_monster_monfear(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_fear() + em_ptr->do_fear);
    em_ptr->get_angry = true;
}

/*!
 * @brief モンスターを衰弱させる
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void effect_damage_makes_weak(EffectMonster *em_ptr)
{
    if (em_ptr->do_time == 0) {
        return;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->do_time >= em_ptr->m_ptr->maxhp) {
        em_ptr->do_time = em_ptr->m_ptr->maxhp - 1;
    }

    if (em_ptr->do_time) {
        em_ptr->note = _("は弱くなったようだ。", " seems weakened.");
        em_ptr->m_ptr->maxhp -= em_ptr->do_time;
        if ((em_ptr->m_ptr->hp - em_ptr->dam) > em_ptr->m_ptr->maxhp) {
            em_ptr->dam = em_ptr->m_ptr->hp - em_ptr->m_ptr->maxhp;
        }
    }

    em_ptr->get_angry = true;
}

/*!
 * @brief モンスターを変身させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void effect_damage_makes_polymorph(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (!em_ptr->do_polymorph || (randint1(90) <= em_ptr->r_ptr->level)) {
        return;
    }

    if (polymorph_monster(player_ptr, em_ptr->y, em_ptr->x)) {
        if (em_ptr->seen) {
            em_ptr->obvious = true;
        }

        em_ptr->note = _("が変身した！", " changes!");
        em_ptr->dam = 0;
    }

    em_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[em_ptr->g_ptr->m_idx];
    em_ptr->r_ptr = &em_ptr->m_ptr->get_monrace();
}

/*!
 * @brief モンスターをテレポートさせる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void effect_damage_makes_teleport(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->do_dist == 0) {
        return;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    em_ptr->note = _("が消え去った！", " disappears!");

    if (is_monster(em_ptr->src_idx)) {
        chg_virtue(player_ptr, Virtue::VALOUR, -1);
    }

    teleport_flags tflag = i2enum<teleport_flags>((is_monster(em_ptr->src_idx) ? TELEPORT_DEC_VALOUR : TELEPORT_SPONTANEOUS) | TELEPORT_PASSIVE);
    teleport_away(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_dist, tflag);

    em_ptr->y = em_ptr->m_ptr->fy;
    em_ptr->x = em_ptr->m_ptr->fx;
    em_ptr->g_ptr = &player_ptr->current_floor_ptr->grid_array[em_ptr->y][em_ptr->x];
}

/*!
 * @brief モンスターへのダメージに応じたメッセージを表示させ、異常状態を与える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @details
 * 以下の判定と処理を行う。
 * 1.全耐性または無敵でダメージが通らなかった場合
 * 2.ダメージ量が現HPを上回る場合
 * 3.通常時(デバフをかける)
 */
static void effect_damage_gives_bad_status(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    int tmp_damage = em_ptr->dam;
    em_ptr->dam = mon_damage_mod(player_ptr, em_ptr->m_ptr, em_ptr->dam, (bool)(em_ptr->attribute == AttributeType::PSY_SPEAR));
    if ((tmp_damage > 0) && (em_ptr->dam == 0) && em_ptr->seen) {
        em_ptr->note = _("はダメージを受けていない。", " is unharmed.");
    }

    if (em_ptr->dam > em_ptr->m_ptr->hp) {
        em_ptr->note = em_ptr->note_dies;
    } else {
        effect_damage_piles_stun(player_ptr, em_ptr);
        effect_damage_piles_confusion(player_ptr, em_ptr);
        effect_damage_piles_fear(player_ptr, em_ptr);
        effect_damage_makes_weak(em_ptr);
        effect_damage_makes_polymorph(player_ptr, em_ptr);
        effect_damage_makes_teleport(player_ptr, em_ptr);
    }
}

/*!
 * @brief 効果によるモンスターへのダメージと付随効果を処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @details
 * 以下の処理を行う。
 * 1.奇襲による徳の変化
 * 2.完全な耐性及び無敵によるダメージのカット
 * 3.ダメージによる付随効果の処理(混乱/朦朧/恐怖/衰弱/変身/テレポート)
 * 4.ダメージ処理及び恐怖メッセージ
 * 5.悪魔領域血の呪いによる事後処理
 */
static void exe_affect_monster_by_damage(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    effect_makes_change_virtues(player_ptr, em_ptr);
    affected_monster_prevents_bad_status(em_ptr);
    effect_damage_gives_bad_status(player_ptr, em_ptr);
    deal_effect_damage_to_monster(player_ptr, em_ptr);
    if ((em_ptr->attribute == AttributeType::BLOOD_CURSE) && one_in_(4)) {
        blood_curse_to_enemy(player_ptr, em_ptr->g_ptr->m_idx);
    }
}

/*!
 * @brief モンスター闘技場にいる場合の画面更新処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void update_phase_out_stat(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (!AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    HealthBarTracker::get_instance().set_trackee(em_ptr->g_ptr->m_idx);
    handle_stuff(player_ptr);
}

/*!
 * @brief 魔法効果がペットに及んだ時の処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void postprocess_by_effected_pet(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    auto *m_ptr = em_ptr->m_ptr;
    if ((em_ptr->dam <= 0) || m_ptr->is_pet() || m_ptr->is_friendly()) {
        return;
    }

    if (is_player(em_ptr->src_idx)) {
        if (!(em_ptr->flag & PROJECT_NO_HANGEKI)) {
            m_ptr->set_target(monster_target_y, monster_target_x);
        }

        return;
    }

    const auto &m_caster_ref = *em_ptr->m_caster_ptr;
    if (is_monster(em_ptr->src_idx) && m_caster_ref.is_pet() && !player_ptr->is_located_at({ m_ptr->target_y, m_ptr->target_x })) {
        m_ptr->set_target(m_caster_ref.fy, m_caster_ref.fx);
    }
}

/*!
 * @brief 魔法効果が騎乗モンスターに及んだ時の処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void postprocess_by_riding_pet_effected(EffectMonster *em_ptr, FallOffHorseEffect *fall_off_horse_effect)
{
    if (!em_ptr->m_ptr->is_riding() || (em_ptr->dam <= 0)) {
        return;
    }

    if (em_ptr->m_ptr->hp > (em_ptr->m_ptr->maxhp / 3)) {
        em_ptr->dam = (em_ptr->dam + 1) / 2;
    }

    if (fall_off_horse_effect) {
        fall_off_horse_effect->set_shake_off(em_ptr->dam);
    }
}

/*!
 * @brief 写真を撮った時の処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @details 写真のフラッシュは弱閃光属性
 */
static void postprocess_by_taking_photo(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->photo == 0) {
        return;
    }

    ItemEntity item({ ItemKindType::STATUE, SV_PHOTO });
    item.pval = em_ptr->photo;
    item.ident |= (IDENT_FULL_KNOWN);
    (void)drop_near(player_ptr, &item, -1, player_ptr->y, player_ptr->x);
}

/*!
 * @brief モンスター効果の後処理 (ペット関係、記念撮影、グローバル変数更新)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 */
static void exe_affect_monster_postprocess(PlayerType *player_ptr, EffectMonster *em_ptr, FallOffHorseEffect *fall_off_horse_effect)
{
    postprocess_by_effected_pet(player_ptr, em_ptr);
    postprocess_by_riding_pet_effected(em_ptr, fall_off_horse_effect);
    postprocess_by_taking_photo(player_ptr, em_ptr);
    project_m_n++;
    project_m_x = em_ptr->x;
    project_m_y = em_ptr->y;
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるモンスターへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param src_idx 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param attribute 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * 以下の処理を行う。
 * 1.魔法効果による効果に対する耐性による軽減計算及び効果の発動
 * 2.魔法効果によるダメージの処理とダメージによる効果の発動
 * 3.ペット及び撮影による事後効果
 */
bool affect_monster(
    PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType attribute, BIT_FLAGS flag, bool see_s_msg,
    std::optional<CapturedMonsterType *> cap_mon_ptr, FallOffHorseEffect *fall_off_horse_effect)
{
    EffectMonster tmp_effect(player_ptr, src_idx, r, y, x, dam, attribute, flag, see_s_msg);
    auto *em_ptr = &tmp_effect;
    auto target_m_idx = em_ptr->g_ptr->m_idx;

    make_description_of_affecred_monster(player_ptr, em_ptr);

    if (is_monster(target_m_idx) && em_ptr->m_ptr->is_riding()) {
        disturb(player_ptr, true, true);
    }

    ProcessResult result = exe_affect_monster_by_effect(player_ptr, em_ptr, cap_mon_ptr);
    if (result != ProcessResult::PROCESS_CONTINUE) {
        return result == ProcessResult::PROCESS_TRUE;
    }

    if (em_ptr->skipped) {
        return false;
    }

    exe_affect_monster_by_damage(player_ptr, em_ptr);

    update_phase_out_stat(player_ptr, em_ptr);
    const auto monster_is_valid = em_ptr->m_ptr->is_valid();
    if (monster_is_valid) {
        update_monster(player_ptr, target_m_idx, false);
    }

    lite_spot(player_ptr, em_ptr->y, em_ptr->x);
    if (LoreTracker::get_instance().is_tracking(em_ptr->m_ptr->r_idx) && (em_ptr->seen || !monster_is_valid)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }

    exe_affect_monster_postprocess(player_ptr, em_ptr, fall_off_horse_effect);
    return em_ptr->obvious;
}
