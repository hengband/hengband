#include "spell-kind/spells-sight.h"
#include "avatar/avatar.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/birth-options.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "locale/english.h"
#include "monster-race/monster-kind-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "term/screen-processor.h"
#include "tracking/lore-tracker.h"
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
bool project_all_los(PlayerType *player_ptr, AttributeType typ, int dam)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        if (!monster.is_valid()) {
            continue;
        }

        const auto m_pos = monster.get_position();
        if (!floor.has_los_at(m_pos) || !projectable(floor, p_pos, m_pos)) {
            continue;
        }

        monster.mflag.set(MonsterTemporaryFlagType::LOS);
    }

    BIT_FLAGS flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
    auto obvious = false;
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        if (monster.mflag.has_not(MonsterTemporaryFlagType::LOS)) {
            continue;
        }

        monster.mflag.reset(MonsterTemporaryFlagType::LOS);
        const auto m_pos = monster.get_position();
        if (project(player_ptr, 0, 0, m_pos.y, m_pos.x, dam, typ, flg).notice) {
            obvious = true;
        }
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
    return project_all_los(player_ptr, AttributeType::OLD_SPEED, player_ptr->lev);
}

/*!
 * @brief 視界内モンスターを加速する処理 / Slow monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool slow_monsters(PlayerType *player_ptr, int power)
{
    return project_all_los(player_ptr, AttributeType::OLD_SLOW, power);
}

/*!
 * @brief 視界内モンスターを眠らせる処理 / Sleep monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool sleep_monsters(PlayerType *player_ptr, int power)
{
    return project_all_los(player_ptr, AttributeType::OLD_SLEEP, power);
}

/*!
 * @brief 視界内の邪悪なモンスターをテレポート・アウェイさせる処理 / Banish evil monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool banish_evil(PlayerType *player_ptr, int dist)
{
    return project_all_los(player_ptr, AttributeType::AWAY_EVIL, dist);
}

/*!
 * @brief 視界内のアンデッド・モンスターを恐怖させる処理 / Turn undead
 * @return 効力があった場合TRUEを返す
 */
bool turn_undead(PlayerType *player_ptr)
{
    bool tester = (project_all_los(player_ptr, AttributeType::TURN_UNDEAD, player_ptr->lev));
    if (tester) {
        chg_virtue(player_ptr, Virtue::UNLIFE, -1);
    }
    return tester;
}

/*!
 * @brief 視界内のアンデッド・モンスターにダメージを与える処理 / Dispel undead monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_undead(PlayerType *player_ptr, int dam)
{
    bool tester = (project_all_los(player_ptr, AttributeType::DISP_UNDEAD, dam));
    if (tester) {
        chg_virtue(player_ptr, Virtue::UNLIFE, -2);
    }
    return tester;
}

/*!
 * @brief 視界内の邪悪なモンスターにダメージを与える処理 / Dispel evil monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_evil(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::DISP_EVIL, dam);
}

/*!
 * @brief 視界内の善良なモンスターにダメージを与える処理 / Dispel good monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_good(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::DISP_GOOD, dam);
}

/*!
 * @brief 視界内のあらゆるモンスターにダメージを与える処理 / Dispel all monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_monsters(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::DISP_ALL, dam);
}

/*!
 * @brief 視界内の生命のあるモンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_living(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::DISP_LIVING, dam);
}

/*!
 * @brief 視界内の悪魔系モンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_demons(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::DISP_DEMON, dam);
}

/*!
 * @brief 視界内のモンスターに「聖戦」効果を与える処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool crusade(PlayerType *player_ptr)
{
    return project_all_los(player_ptr, AttributeType::CRUSADE, player_ptr->lev * 4);
}

/*!
 * @brief 視界内モンスターを怒らせる処理 / Wake up all monsters, and speed up "los" monsters.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param src_idx 怒らせる原因を起こしたモンスター(0ならばプレイヤー)
 */
void aggravate_monsters(PlayerType *player_ptr, MONSTER_IDX src_idx)
{
    auto sleep = false;
    auto speed = false;
    auto &floor = *player_ptr->current_floor_ptr;
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        if (!monster.is_valid()) {
            continue;
        }
        if (i == src_idx) {
            continue;
        }

        if (monster.cdis < MAX_PLAYER_SIGHT * 2) {
            if (monster.is_asleep()) {
                (void)set_monster_csleep(player_ptr, i, 0);
                sleep = true;
            }

            if (!monster.is_pet()) {
                monster.mflag2.set(MonsterConstantFlagType::NOPET);
            }
        }

        if (floor.has_los_at({ monster.fy, monster.fx }) && !monster.is_pet()) {
            (void)set_monster_fast(player_ptr, i, monster.get_remaining_acceleration() + 100);
            speed = true;
        }
    }

    if (speed) {
        msg_print(_("付近で何かが突如興奮したような感じを受けた！", "You feel a sudden stirring nearby!"));
    } else if (sleep) {
        msg_print(_("何かが突如興奮したような騒々しい音が遠くに聞こえた！", "You hear a sudden stirring in the distance!"));
    }

    if (player_ptr->riding) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }
}

/*!
 * @brief パニック・モンスター効果(プレイヤー視界範囲内) / Confuse monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monsters(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::OLD_CONF, dam);
}

/*!
 * @brief チャーム・モンスター効果(プレイヤー視界範囲内) / Charm monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monsters(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::CHARM, dam);
}

/*!
 * @brief 動物魅了効果(プレイヤー視界範囲内) / Charm Animals
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animals(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::CONTROL_ANIMAL, dam);
}

/*!
 * @brief モンスター朦朧効果(プレイヤー視界範囲内) / Stun monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monsters(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::STUN, dam);
}

/*!
 * @brief モンスター停止効果(プレイヤー視界範囲内) / Stasis monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stasis_monsters(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::STASIS, dam);
}

/*!
 * @brief モンスター精神攻撃効果(プレイヤー視界範囲内) / Mindblast monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool mindblast_monsters(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::PSI, dam);
}

/*!
 * @brief モンスター追放効果(プレイヤー視界範囲内) / Banish all monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dist 効力（距離）
 * @return 作用が実際にあった場合TRUEを返す
 */
bool banish_monsters(PlayerType *player_ptr, int dist)
{
    return project_all_los(player_ptr, AttributeType::AWAY_ALL, dist);
}

/*!
 * @brief 邪悪退散効果(プレイヤー視界範囲内) / Turn evil
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_evil(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::TURN_EVIL, dam);
}

/*!
 * @brief 全モンスター退散効果(プレイヤー視界範囲内) / Turn everyone
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_monsters(PlayerType *player_ptr, int dam)
{
    return project_all_los(player_ptr, AttributeType::TURN_ALL, dam);
}

/*!
 * @brief 死の光線(プレイヤー視界範囲内) / Death-ray all monsters (note: OBSCENELY powerful)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool deathray_monsters(PlayerType *player_ptr)
{
    return project_all_los(player_ptr, AttributeType::DEATH_RAY, player_ptr->lev * 200);
}

/*!
 * @brief 調査したモンスターの情報を表示する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param m_ptr モンスター情報への参照ポインタ
 * @param r_ptr モンスター種族への参照ポインタ
 * @return 調査結果 善悪アライメント、最大HP、残りHP、AC、速度、ステータス
 */
std::string probed_monster_info(PlayerType *player_ptr, MonsterEntity &monster, const MonraceDefinition &monrace)
{
    if (!monster.is_original_ap()) {
        if (monster.mflag2.has(MonsterConstantFlagType::KAGE)) {
            monster.mflag2.reset(MonsterConstantFlagType::KAGE);
        }

        monster.ap_r_idx = monster.r_idx;
        lite_spot(player_ptr, monster.get_position());
    }

    const auto m_name = monster_desc(player_ptr, monster, MD_IGNORE_HALLU | MD_INDEF_HIDDEN);

    concptr align;
    if (monrace.kind_flags.has_all_of(alignment_mask)) {
        align = _("善悪", "good&evil");
    } else if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        align = _("邪悪", "evil");
    } else if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        align = _("善良", "good");
    } else if ((monster.sub_align & (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) == (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) {
        align = _("中立(善悪)", "neutral(good&evil)");
    } else if (monster.sub_align & SUB_ALIGN_EVIL) {
        align = _("中立(邪悪)", "neutral(evil)");
    } else if (monster.sub_align & SUB_ALIGN_GOOD) {
        align = _("中立(善良)", "neutral(good)");
    } else {
        align = _("中立", "neutral");
    }

    const auto speed = monster.get_temporary_speed() - STANDARD_SPEED;
    constexpr auto mes = _("%s ... 属性:%s HP:%d/%d AC:%d 速度:%s%d 経験:", "%s ... align:%s HP:%d/%d AC:%d speed:%s%d exp:");
    auto result = format(mes, m_name.data(), align, (int)monster.hp, (int)monster.maxhp, monrace.ac, (speed > 0) ? "+" : "", speed);

    if (monrace.get_next().is_valid()) {
        result.append(format("%d/%d ", monster.exp, monrace.next_exp));
    } else {
        result.append("xxx ");
    }

    if (monster.is_asleep()) {
        result.append(_("睡眠 ", "sleeping "));
    }
    if (monster.is_stunned()) {
        result.append(_("朦朧 ", "stunned "));
    }
    if (monster.is_fearful()) {
        result.append(_("恐怖 ", "scared "));
    }
    if (monster.is_confused()) {
        result.append(_("混乱 ", "confused "));
    }
    if (monster.is_invulnerable()) {
        result.append(_("無敵 ", "invulnerable "));
    }
    return result;
}

/*!
 * @brief 周辺モンスターを調査する / Probe nearby monsters
 * @return 効力があった場合TRUEを返す
 */
bool probing(PlayerType *player_ptr)
{
    bool cu = game_term->scr->cu;
    bool cv = game_term->scr->cv;
    game_term->scr->cu = 0;
    game_term->scr->cv = 1;

    auto &floor = *player_ptr->current_floor_ptr;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto probe = false;
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        auto &monrace = monster.get_monrace();
        if (!monster.is_valid()) {
            continue;
        }
        if (!floor.has_los_at({ monster.fy, monster.fx })) {
            continue;
        }
        if (!monster.ml) {
            continue;
        }

        if (!probe) {
            msg_print(_("調査中...", "Probing..."));
        }
        msg_erase();

        const auto probe_result = probed_monster_info(player_ptr, monster, monrace);
        prt(probe_result, 0, 0);

        message_add(probe_result);
        rfu.set_flag(SubWindowRedrawingFlag::MESSAGE);
        handle_stuff(player_ptr);
        move_cursor_relative(monster.fy, monster.fx);
        inkey();
        term_erase(0, 0);
        const auto mes = monrace.probe_lore();
        if (mes) {
            msg_print(*mes);
            msg_erase();
            if (LoreTracker::get_instance().is_tracking(monster.r_idx)) {
                RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
            }
        }

        probe = true;
    }

    game_term->scr->cu = cu;
    game_term->scr->cv = cv;
    term_fresh();

    if (probe) {
        chg_virtue(player_ptr, Virtue::KNOWLEDGE, 1);
        msg_print(_("これで全部です。", "That's all."));
    }

    return probe;
}
