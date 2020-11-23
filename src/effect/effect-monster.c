/*!
 * @brief 魔法によるモンスターへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-monster.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-monster-switcher.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "player-info/avatar.h"
#include "spell-kind/blood-curse.h"
#include "spell-kind/spells-polymorph.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "spells-effect-util.h"
#include "sv-definition/sv-other-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ビーム/ボルト/ボール系魔法によるモンスターへの効果があるかないかを判定する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 効果が何もないならFALSE、何かあるならTRUE
 */
static bool is_never_effect(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (!em_ptr->g_ptr->m_idx)
        return FALSE;
    if (em_ptr->who && (em_ptr->g_ptr->m_idx == em_ptr->who))
        return FALSE;
    if ((em_ptr->g_ptr->m_idx == caster_ptr->riding) && !em_ptr->who && !(em_ptr->effect_type == GF_OLD_HEAL) && !(em_ptr->effect_type == GF_OLD_SPEED)
        && !(em_ptr->effect_type == GF_STAR_HEAL))
        return FALSE;
    if (sukekaku && ((em_ptr->m_ptr->r_idx == MON_SUKE) || (em_ptr->m_ptr->r_idx == MON_KAKU)))
        return FALSE;
    if (em_ptr->m_ptr->hp < 0)
        return FALSE;

    return TRUE;
}

/*!
 * @brief 魔法の効果やモンスター種別(MAKE/FEMALE/なし)に応じて表示するメッセージを変更する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void decide_spell_result_description(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    em_ptr->dam = (em_ptr->dam + em_ptr->r) / (em_ptr->r + 1);
    monster_desc(caster_ptr, em_ptr->m_name, em_ptr->m_ptr, 0);
    monster_desc(caster_ptr, em_ptr->m_poss, em_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
    if (caster_ptr->riding && (em_ptr->g_ptr->m_idx == caster_ptr->riding))
        disturb(caster_ptr, TRUE, TRUE);
}

/*!
 * @brief 完全な耐性を持っていたら、モンスターへの効果処理をスキップする
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 完全耐性ならCONTINUE、そうでないなら効果処理の結果
 */
static process_result process_monster_perfect_resistance(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (((em_ptr->r_ptr->flagsr & RFR_RES_ALL) == 0) || em_ptr->effect_type == GF_OLD_CLONE || em_ptr->effect_type == GF_STAR_HEAL
        || em_ptr->effect_type == GF_OLD_HEAL || em_ptr->effect_type == GF_OLD_SPEED || em_ptr->effect_type == GF_CAPTURE || em_ptr->effect_type == GF_PHOTO)
        return switch_effects_monster(caster_ptr, em_ptr);

    em_ptr->note = _("には完全な耐性がある！", " is immune.");
    em_ptr->dam = 0;
    if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_ALL);

    if (em_ptr->effect_type == GF_LITE_WEAK || em_ptr->effect_type == GF_KILL_WALL)
        em_ptr->skipped = TRUE;

    return PROCESS_CONTINUE;
}

/*!
 * @brief ペットの死亡を処理する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_pet_death(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    bool sad = is_pet(em_ptr->m_ptr) && !(em_ptr->m_ptr->ml);
    if (em_ptr->known && em_ptr->note) {
        monster_desc(caster_ptr, em_ptr->m_name, em_ptr->m_ptr, MD_TRUE_NAME);
        if (em_ptr->see_s_msg)
            msg_format("%^s%s", em_ptr->m_name, em_ptr->note);
        else
            caster_ptr->current_floor_ptr->monster_noise = TRUE;
    }

    if (em_ptr->who > 0)
        monster_gain_exp(caster_ptr, em_ptr->who, em_ptr->m_ptr->r_idx);

    monster_death(caster_ptr, em_ptr->g_ptr->m_idx, FALSE);
    delete_monster_idx(caster_ptr, em_ptr->g_ptr->m_idx);
    if (sad)
        msg_print(_("少し悲しい気分がした。", "You feel sad for a moment."));
}

/*!
 * @brief モンスターの睡眠を処理する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_sleep(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->note && em_ptr->seen_msg)
        msg_format("%^s%s", em_ptr->m_name, em_ptr->note);
    else if (em_ptr->see_s_msg)
        message_pain(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam);
    else
        caster_ptr->current_floor_ptr->monster_noise = TRUE;

    if (em_ptr->do_sleep)
        (void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_sleep);
}

/*!
 * @brief モンスターの被ダメージを処理する / If another monster did the damage, hurt the monster by hand
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return モンスターIDがプレーヤー自身だった場合FALSE、モンスターだった場合TRUE
 */
static bool process_monster_damage(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->who <= 0)
        return FALSE;

    if (caster_ptr->health_who == em_ptr->g_ptr->m_idx)
        caster_ptr->redraw |= (PR_HEALTH);
    if (caster_ptr->riding == em_ptr->g_ptr->m_idx)
        caster_ptr->redraw |= (PR_UHEALTH);

    (void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, 0);
    em_ptr->m_ptr->hp -= em_ptr->dam;
    if (em_ptr->m_ptr->hp < 0)
        process_pet_death(caster_ptr, em_ptr);
    else
        process_monster_sleep(caster_ptr, em_ptr);

    return TRUE;
}

/*!
 * @brief 不潔な病人の治療処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 大賞モンスターが不潔な病人だった場合はTRUE、それ以外はFALSE
 */
static bool heal_leaper(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (!em_ptr->heal_leper)
        return FALSE;

    if (em_ptr->seen_msg)
        msg_print(_("不潔な病人は病気が治った！", "The Mangy looking leper is healed!"));

    if (record_named_pet && is_pet(em_ptr->m_ptr) && em_ptr->m_ptr->nickname) {
        char m2_name[MAX_NLEN];
        monster_desc(caster_ptr, m2_name, em_ptr->m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_HEAL_LEPER, m2_name);
    }

    delete_monster_idx(caster_ptr, em_ptr->g_ptr->m_idx);
    return TRUE;
}

/*!
 * @brief モンスターの恐慌処理 / If the player did it, give him experience, check fear
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return モンスターが死んだらTRUE、生きていたらFALSE
 */
static bool process_monster_fear(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    bool fear = FALSE;
    if (mon_take_hit(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, &fear, em_ptr->note_dies))
        return TRUE;

    if (em_ptr->do_sleep)
        anger_monster(caster_ptr, em_ptr->m_ptr);

    if (em_ptr->note && em_ptr->seen_msg)
        msg_format(_("%s%s", "%^s%s"), em_ptr->m_name, em_ptr->note);
    else if (em_ptr->known && (em_ptr->dam || !em_ptr->do_fear))
        message_pain(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam);

    if (((em_ptr->dam > 0) || em_ptr->get_angry) && !em_ptr->do_sleep)
        anger_monster(caster_ptr, em_ptr->m_ptr);

    if ((fear || em_ptr->do_fear) && em_ptr->seen) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), em_ptr->m_name);
    }

    return FALSE;
}

/*!
 * todo 睡眠処理があるので、死に際とは言えない。適切な関数名に要修正
 * @brief モンスターの死に際処理 (魔力吸収を除く)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_last_moment(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->effect_type == GF_DRAIN_MANA)
        return;

    if (process_monster_damage(caster_ptr, em_ptr))
        return;
    if (heal_leaper(caster_ptr, em_ptr))
        return;
    if (process_monster_fear(caster_ptr, em_ptr))
        return;

    if (em_ptr->do_sleep)
        (void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_sleep);
}

/*!
 * @brief 魔法の効果による汎用処理 (変身の有無、現在HPの減算、徳の変化)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_spell_result(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) || (em_ptr->r_ptr->flags1 & RF1_QUESTOR) || (caster_ptr->riding && (em_ptr->g_ptr->m_idx == caster_ptr->riding)))
        em_ptr->do_polymorph = FALSE;

    if (((em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (em_ptr->r_ptr->flags7 & RF7_NAZGUL)) && !caster_ptr->phase_out && (em_ptr->who > 0)
        && (em_ptr->dam > em_ptr->m_ptr->hp))
        em_ptr->dam = em_ptr->m_ptr->hp;

    if ((em_ptr->who > 0) || !em_ptr->slept)
        return;

    if (!(em_ptr->r_ptr->flags3 & RF3_EVIL) || one_in_(5))
        chg_virtue(caster_ptr, V_COMPASSION, -1);
    if (!(em_ptr->r_ptr->flags3 & RF3_EVIL) || one_in_(5))
        chg_virtue(caster_ptr, V_HONOUR, -1);
}

/*!
 * @brief モンスターの朦朧値を蓄積させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param stun_damage 朦朧値
 * @return なし
 */
static void pile_monster_stun(player_type *caster_ptr, effect_monster_type *em_ptr, int *stun_damage)
{
    if ((em_ptr->do_stun == 0) || (em_ptr->r_ptr->flagsr & (RFR_RES_SOUN | RFR_RES_WALL)) || (em_ptr->r_ptr->flags3 & RF3_NO_STUN))
        return;

    if (em_ptr->seen)
        em_ptr->obvious = TRUE;

    if (monster_stunned_remaining(em_ptr->m_ptr)) {
        em_ptr->note = _("はひどくもうろうとした。", " is more dazed.");
        *stun_damage = monster_stunned_remaining(em_ptr->m_ptr) + (em_ptr->do_stun / 2);
    } else {
        em_ptr->note = _("はもうろうとした。", " is dazed.");
        *stun_damage = em_ptr->do_stun;
    }

    (void)set_monster_stunned(caster_ptr, em_ptr->g_ptr->m_idx, *stun_damage);
    em_ptr->get_angry = TRUE;
}

/*!
 * @brief モンスターの混乱値を蓄積させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param stun_damage 混乱値
 * @return なし
 */
static void pile_monster_conf(player_type *caster_ptr, effect_monster_type *em_ptr, int *conf_damage)
{
    if ((em_ptr->do_conf == 0) || (em_ptr->r_ptr->flags3 & RF3_NO_CONF) || (em_ptr->r_ptr->flagsr & RFR_EFF_RES_CHAO_MASK))
        return;

    if (em_ptr->seen)
        em_ptr->obvious = TRUE;

    if (monster_confused_remaining(em_ptr->m_ptr)) {
        em_ptr->note = _("はさらに混乱したようだ。", " looks more confused.");
        *conf_damage = monster_confused_remaining(em_ptr->m_ptr) + (em_ptr->do_conf / 2);
    } else {
        em_ptr->note = _("は混乱したようだ。", " looks confused.");
        *conf_damage = em_ptr->do_conf;
    }

    (void)set_monster_confused(caster_ptr, em_ptr->g_ptr->m_idx, *conf_damage);
    em_ptr->get_angry = TRUE;
}

/*!
 * @brief モンスターを衰弱させる
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_weakening(effect_monster_type *em_ptr)
{
    if (em_ptr->do_time == 0)
        return;

    if (em_ptr->seen)
        em_ptr->obvious = TRUE;

    if (em_ptr->do_time >= em_ptr->m_ptr->maxhp)
        em_ptr->do_time = em_ptr->m_ptr->maxhp - 1;

    if (em_ptr->do_time) {
        em_ptr->note = _("は弱くなったようだ。", " seems weakened.");
        em_ptr->m_ptr->maxhp -= em_ptr->do_time;
        if ((em_ptr->m_ptr->hp - em_ptr->dam) > em_ptr->m_ptr->maxhp)
            em_ptr->dam = em_ptr->m_ptr->hp - em_ptr->m_ptr->maxhp;
    }

    em_ptr->get_angry = TRUE;
}

/*!
 * @brief モンスターを変身させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_polymorph(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (!em_ptr->do_polymorph || (randint1(90) <= em_ptr->r_ptr->level))
        return;

    if (polymorph_monster(caster_ptr, em_ptr->y, em_ptr->x)) {
        if (em_ptr->seen)
            em_ptr->obvious = TRUE;

        em_ptr->note = _("が変身した！", " changes!");
        em_ptr->dam = 0;
    }

    em_ptr->m_ptr = &caster_ptr->current_floor_ptr->m_list[em_ptr->g_ptr->m_idx];
    em_ptr->r_ptr = &r_info[em_ptr->m_ptr->r_idx];
}

/*!
 * @brief モンスターをテレポートさせる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_teleport(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->do_dist == 0)
        return;

    if (em_ptr->seen)
        em_ptr->obvious = TRUE;

    em_ptr->note = _("が消え去った！", " disappears!");

    if (!em_ptr->who)
        chg_virtue(caster_ptr, V_VALOUR, -1);

    teleport_away(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_dist, (!em_ptr->who ? TELEPORT_DEC_VALOUR : 0L) | TELEPORT_PASSIVE);

    em_ptr->y = em_ptr->m_ptr->fy;
    em_ptr->x = em_ptr->m_ptr->fx;
    em_ptr->g_ptr = &caster_ptr->current_floor_ptr->grid_array[em_ptr->y][em_ptr->x];
}

/*!
 * @brief モンスターの異常状態を処理する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @parama tmp_damage 朦朧/混乱値
 * @return なし
 */
static void process_monster_bad_status(player_type *caster_ptr, effect_monster_type *em_ptr, int *tmp_damage)
{
    pile_monster_stun(caster_ptr, em_ptr, tmp_damage);
    pile_monster_conf(caster_ptr, em_ptr, tmp_damage);
    process_monster_weakening(em_ptr);
    process_monster_polymorph(caster_ptr, em_ptr);
    process_monster_teleport(caster_ptr, em_ptr);
    if (em_ptr->do_fear == 0)
        return;

    (void)set_monster_monfear(caster_ptr, em_ptr->g_ptr->m_idx, monster_fear_remaining(em_ptr->m_ptr) + em_ptr->do_fear);
    em_ptr->get_angry = TRUE;
}

/*!
 * @brief モンスターへのダメージに応じたメッセージを表示させ、異常状態を与える
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_bad_stat_damage(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    int tmp_damage = em_ptr->dam;
    em_ptr->dam = mon_damage_mod(caster_ptr, em_ptr->m_ptr, em_ptr->dam, (bool)(em_ptr->effect_type == GF_PSY_SPEAR));
    if ((tmp_damage > 0) && (em_ptr->dam == 0))
        em_ptr->note = _("はダメージを受けていない。", " is unharmed.");

    if (em_ptr->dam > em_ptr->m_ptr->hp)
        em_ptr->note = em_ptr->note_dies;
    else
        process_monster_bad_status(caster_ptr, em_ptr, &tmp_damage);
}

/*!
 * todo 関数名が微妙、もっと適切な関数名が欲しい
 * @brief モンスターへの影響全般を処理する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_spell(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    process_spell_result(caster_ptr, em_ptr);
    process_monster_bad_stat_damage(caster_ptr, em_ptr);
    process_monster_last_moment(caster_ptr, em_ptr);
    if ((em_ptr->effect_type == GF_BLOOD_CURSE) && one_in_(4))
        blood_curse_to_enemy(caster_ptr, em_ptr->who);
}

/*!
 * @brief モンスター闘技場にいる場合の画面更新処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void update_phase_out_stat(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (!caster_ptr->phase_out)
        return;

    caster_ptr->health_who = em_ptr->g_ptr->m_idx;
    caster_ptr->redraw |= (PR_HEALTH);
    handle_stuff(caster_ptr);
}

/*!
 * @brief 魔法効果がペットに及んだ時の処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void postprocess_spell_pet(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if ((em_ptr->dam <= 0) || is_pet(em_ptr->m_ptr) || is_friendly(em_ptr->m_ptr))
        return;

    if (em_ptr->who == 0) {
        if (!(em_ptr->flag & PROJECT_NO_HANGEKI))
            set_target(em_ptr->m_ptr, monster_target_y, monster_target_x);

        return;
    }

    if ((em_ptr->who > 0) && is_pet(em_ptr->m_caster_ptr) && !player_bold(caster_ptr, em_ptr->m_ptr->target_y, em_ptr->m_ptr->target_x))
        set_target(em_ptr->m_ptr, em_ptr->m_caster_ptr->fy, em_ptr->m_caster_ptr->fx);
}

/*!
 * @brief 魔法効果が騎乗モンスターに及んだ時の処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void postprocess_spell_riding(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (!caster_ptr->riding || (caster_ptr->riding != em_ptr->g_ptr->m_idx) || (em_ptr->dam <= 0))
        return;

    if (em_ptr->m_ptr->hp > (em_ptr->m_ptr->maxhp / 3))
        em_ptr->dam = (em_ptr->dam + 1) / 2;

    rakubadam_m = (em_ptr->dam > 200) ? 200 : em_ptr->dam;
}

/*!
 * @brief 写真を撮った時の処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 * @details 写真のフラッシュは弱閃光属性
 */
static void postprocess_spell_photo(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->photo == 0)
        return;

    object_type *q_ptr;
    object_type forge;
    q_ptr = &forge;
    object_prep(caster_ptr, q_ptr, lookup_kind(TV_STATUE, SV_PHOTO));
    q_ptr->pval = em_ptr->photo;
    q_ptr->ident |= (IDENT_FULL_KNOWN);
    (void)drop_near(caster_ptr, q_ptr, -1, caster_ptr->y, caster_ptr->x);
}

/*!
 * @brief モンスター効果の後処理 (ペット関係、記念撮影、グローバル変数更新)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void postprocess_spell(player_type *caster_ptr, effect_monster_type *em_ptr)
{
    postprocess_spell_pet(caster_ptr, em_ptr);
    postprocess_spell_riding(caster_ptr, em_ptr);
    postprocess_spell_photo(caster_ptr, em_ptr);
    project_m_n++;
    project_m_x = em_ptr->x;
    project_m_y = em_ptr->y;
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるモンスターへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param effect_type 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_monster(
    player_type *caster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, bool see_s_msg)
{
    effect_monster_type tmp_effect;
    effect_monster_type *em_ptr = initialize_effect_monster(caster_ptr, &tmp_effect, who, r, y, x, dam, effect_type, flag, see_s_msg);
    if (!is_never_effect(caster_ptr, em_ptr))
        return FALSE;

    decide_spell_result_description(caster_ptr, em_ptr);
    process_result result = process_monster_perfect_resistance(caster_ptr, em_ptr);
    if (result != PROCESS_CONTINUE)
        return (bool)result;

    if (em_ptr->skipped)
        return FALSE;

    process_spell(caster_ptr, em_ptr);
    update_phase_out_stat(caster_ptr, em_ptr);
    if (em_ptr->m_ptr->r_idx)
        update_monster(caster_ptr, em_ptr->g_ptr->m_idx, FALSE);

    lite_spot(caster_ptr, em_ptr->y, em_ptr->x);
    if ((caster_ptr->monster_race_idx == em_ptr->m_ptr->r_idx) && (em_ptr->seen || !em_ptr->m_ptr->r_idx))
        caster_ptr->window |= (PW_MONSTER);

    postprocess_spell(caster_ptr, em_ptr);
    return em_ptr->obvious;
}
