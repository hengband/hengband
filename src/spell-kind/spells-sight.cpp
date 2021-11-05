﻿#include "spell-kind/spells-sight.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "locale/english.h"
#include "lore/lore-store.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 視界内モンスターに魔法効果を与える / Apply a "project()" directly to all viewable monsters
 * @param typ 属性効果
 * @param dam 効果量
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * Note that affected monsters are NOT auto-tracked by this usage.
 *
 * To avoid misbehavior when monster deaths have side-effects,
 * this is done in two passes. -- JDL
 * </pre>
 */
bool project_all_los(PlayerType *player_ptr, EFFECT_ID typ, HIT_POINT dam)
{
    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;
        if (!player_has_los_bold(player_ptr, y, x) || !projectable(player_ptr, player_ptr->y, player_ptr->x, y, x))
            continue;

        m_ptr->mflag.set(MFLAG::LOS);
    }

    BIT_FLAGS flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
    bool obvious = false;
    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (m_ptr->mflag.has_not(MFLAG::LOS))
            continue;

        m_ptr->mflag.reset(MFLAG::LOS);
        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (project(player_ptr, 0, 0, y, x, dam, typ, flg).notice)
            obvious = true;
    }

    return obvious;
}

/*!
 * @brief 視界内モンスターを加速する処理 / Speed monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool speed_monsters(PlayerType *player_ptr)
{
    return (project_all_los(player_ptr, GF_OLD_SPEED, player_ptr->lev));
}

/*!
 * @brief 視界内モンスターを加速する処理 / Slow monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool slow_monsters(PlayerType *player_ptr, int power)
{
    return (project_all_los(player_ptr, GF_OLD_SLOW, power));
}

/*!
 * @brief 視界内モンスターを眠らせる処理 / Sleep monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool sleep_monsters(PlayerType *player_ptr, int power)
{
    return (project_all_los(player_ptr, GF_OLD_SLEEP, power));
}

/*!
 * @brief 視界内の邪悪なモンスターをテレポート・アウェイさせる処理 / Banish evil monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool banish_evil(PlayerType *player_ptr, int dist)
{
    return (project_all_los(player_ptr, GF_AWAY_EVIL, dist));
}

/*!
 * @brief 視界内のアンデッド・モンスターを恐怖させる処理 / Turn undead
 * @return 効力があった場合TRUEを返す
 */
bool turn_undead(PlayerType *player_ptr)
{
    bool tester = (project_all_los(player_ptr, GF_TURN_UNDEAD, player_ptr->lev));
    if (tester)
        chg_virtue(player_ptr, V_UNLIFE, -1);
    return tester;
}

/*!
 * @brief 視界内のアンデッド・モンスターにダメージを与える処理 / Dispel undead monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_undead(PlayerType *player_ptr, HIT_POINT dam)
{
    bool tester = (project_all_los(player_ptr, GF_DISP_UNDEAD, dam));
    if (tester)
        chg_virtue(player_ptr, V_UNLIFE, -2);
    return tester;
}

/*!
 * @brief 視界内の邪悪なモンスターにダメージを与える処理 / Dispel evil monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_evil(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_DISP_EVIL, dam));
}

/*!
 * @brief 視界内の善良なモンスターにダメージを与える処理 / Dispel good monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_good(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_DISP_GOOD, dam));
}

/*!
 * @brief 視界内のあらゆるモンスターにダメージを与える処理 / Dispel all monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_monsters(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_DISP_ALL, dam));
}

/*!
 * @brief 視界内の生命のあるモンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_living(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_DISP_LIVING, dam));
}

/*!
 * @brief 視界内の悪魔系モンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_demons(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_DISP_DEMON, dam));
}

/*!
 * @brief 視界内のモンスターに「聖戦」効果を与える処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool crusade(PlayerType *player_ptr)
{
    return (project_all_los(player_ptr, GF_CRUSADE, player_ptr->lev * 4));
}

/*!
 * @brief 視界内モンスターを怒らせる処理 / Wake up all monsters, and speed up "los" monsters.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param who 怒らせる原因を起こしたモンスター(0ならばプレイヤー)
 */
void aggravate_monsters(PlayerType *player_ptr, MONSTER_IDX who)
{
    bool sleep = false;
    bool speed = false;
    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;
        if (i == who)
            continue;

        if (m_ptr->cdis < MAX_SIGHT * 2) {
            if (monster_csleep_remaining(m_ptr)) {
                (void)set_monster_csleep(player_ptr, i, 0);
                sleep = true;
            }

            if (!is_pet(m_ptr))
                m_ptr->mflag2.set(MFLAG2::NOPET);
        }

        if (player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx)) {
            if (!is_pet(m_ptr)) {
                (void)set_monster_fast(player_ptr, i, monster_fast_remaining(m_ptr) + 100);
                speed = true;
            }
        }
    }

    if (speed)
        msg_print(_("付近で何かが突如興奮したような感じを受けた！", "You feel a sudden stirring nearby!"));
    else if (sleep)
        msg_print(_("何かが突如興奮したような騒々しい音が遠くに聞こえた！", "You hear a sudden stirring in the distance!"));
    if (player_ptr->riding)
        player_ptr->update |= PU_BONUS;
}

/*!
 * @brief パニック・モンスター効果(プレイヤー視界範囲内) / Confuse monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monsters(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_OLD_CONF, dam));
}

/*!
 * @brief チャーム・モンスター効果(プレイヤー視界範囲内) / Charm monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monsters(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_CHARM, dam));
}

/*!
 * @brief 動物魅了効果(プレイヤー視界範囲内) / Charm Animals
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animals(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_CONTROL_ANIMAL, dam));
}

/*!
 * @brief モンスター朦朧効果(プレイヤー視界範囲内) / Stun monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monsters(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_STUN, dam));
}

/*!
 * @brief モンスター停止効果(プレイヤー視界範囲内) / Stasis monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stasis_monsters(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_STASIS, dam));
}

/*!
 * @brief モンスター精神攻撃効果(プレイヤー視界範囲内) / Mindblast monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool mindblast_monsters(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_PSI, dam));
}

/*!
 * @brief モンスター追放効果(プレイヤー視界範囲内) / Banish all monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dist 効力（距離）
 * @return 作用が実際にあった場合TRUEを返す
 */
bool banish_monsters(PlayerType *player_ptr, int dist)
{
    return (project_all_los(player_ptr, GF_AWAY_ALL, dist));
}

/*!
 * @brief 邪悪退散効果(プレイヤー視界範囲内) / Turn evil
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_evil(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_TURN_EVIL, dam));
}

/*!
 * @brief 全モンスター退散効果(プレイヤー視界範囲内) / Turn everyone
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_monsters(PlayerType *player_ptr, HIT_POINT dam)
{
    return (project_all_los(player_ptr, GF_TURN_ALL, dam));
}

/*!
 * @brief 死の光線(プレイヤー視界範囲内) / Death-ray all monsters (note: OBSCENELY powerful)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool deathray_monsters(PlayerType *player_ptr)
{
    return (project_all_los(player_ptr, GF_DEATH_RAY, player_ptr->lev * 200));
}

/*!
 * @brief 調査したモンスターの情報を表示する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param m_ptr モンスター情報への参照ポインタ
 * @param r_ptr モンスター種族への参照ポインタ
 */
void probed_monster_info(char *buf, PlayerType *player_ptr, monster_type *m_ptr, monster_race *r_ptr)
{
    if (!is_original_ap(m_ptr)) {
        if (m_ptr->mflag2.has(MFLAG2::KAGE))
            m_ptr->mflag2.reset(MFLAG2::KAGE);

        m_ptr->ap_r_idx = m_ptr->r_idx;
        lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);
    }

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, m_ptr, MD_IGNORE_HALLU | MD_INDEF_HIDDEN);

    SPEED speed = m_ptr->mspeed - 110;
    if (monster_fast_remaining(m_ptr))
        speed += 10;
    if (monster_slow_remaining(m_ptr))
        speed -= 10;
    if (ironman_nightmare)
        speed += 5;

    concptr align;
    if ((r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)) == (RF3_EVIL | RF3_GOOD))
        align = _("善悪", "good&evil");
    else if (r_ptr->flags3 & RF3_EVIL)
        align = _("邪悪", "evil");
    else if (r_ptr->flags3 & RF3_GOOD)
        align = _("善良", "good");
    else if ((m_ptr->sub_align & (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) == (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD))
        align = _("中立(善悪)", "neutral(good&evil)");
    else if (m_ptr->sub_align & SUB_ALIGN_EVIL)
        align = _("中立(邪悪)", "neutral(evil)");
    else if (m_ptr->sub_align & SUB_ALIGN_GOOD)
        align = _("中立(善良)", "neutral(good)");
    else
        align = _("中立", "neutral");

    sprintf(buf, _("%s ... 属性:%s HP:%d/%d AC:%d 速度:%s%d 経験:", "%s ... align:%s HP:%d/%d AC:%d speed:%s%d exp:"), m_name, align, (int)m_ptr->hp,
        (int)m_ptr->maxhp, r_ptr->ac, (speed > 0) ? "+" : "", speed);

    if (r_ptr->next_r_idx) {
        strcat(buf, format("%d/%d ", m_ptr->exp, r_ptr->next_exp));
    } else {
        strcat(buf, "xxx ");
    }

    if (monster_csleep_remaining(m_ptr))
        strcat(buf, _("睡眠 ", "sleeping "));
    if (monster_stunned_remaining(m_ptr))
        strcat(buf, _("朦朧 ", "stunned "));
    if (monster_fear_remaining(m_ptr))
        strcat(buf, _("恐怖 ", "scared "));
    if (monster_confused_remaining(m_ptr))
        strcat(buf, _("混乱 ", "confused "));
    if (monster_invulner_remaining(m_ptr))
        strcat(buf, _("無敵 ", "invulnerable "));
    buf[strlen(buf) - 1] = '\0';
}

/*!
 * @brief 周辺モンスターを調査する / Probe nearby monsters
 * @return 効力があった場合TRUEを返す
 */
bool probing(PlayerType *player_ptr)
{
    bool cu = Term->scr->cu;
    bool cv = Term->scr->cv;
    Term->scr->cu = 0;
    Term->scr->cv = 1;

    bool probe = false;
    char buf[256];
    for (int i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        if (!player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx))
            continue;
        if (!m_ptr->ml)
            continue;

        if (!probe)
            msg_print(_("調査中...", "Probing..."));
        msg_print(nullptr);

        probed_monster_info(buf, player_ptr, m_ptr, r_ptr);
        prt(buf, 0, 0);

        message_add(buf);
        player_ptr->window_flags |= (PW_MESSAGE);
        handle_stuff(player_ptr);
        move_cursor_relative(m_ptr->fy, m_ptr->fx);
        inkey();
        term_erase(0, 0, 255);
        if (lore_do_probe(player_ptr, m_ptr->r_idx)) {
            strcpy(buf, (r_ptr->name.c_str()));
#ifdef JP
            msg_format("%sについてさらに詳しくなった気がする。", buf);
#else
            plural_aux(buf);
            msg_format("You now know more about %s.", buf);
#endif
            msg_print(nullptr);
        }

        probe = true;
    }

    Term->scr->cu = cu;
    Term->scr->cv = cv;
    term_fresh();

    if (probe) {
        chg_virtue(player_ptr, V_KNOWLEDGE, 1);
        msg_print(_("これで全部です。", "That's all."));
    }

    return (probe);
}
