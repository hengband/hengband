#include "monster/monster-status-setter.h"
#include "avatar/avatar.h"
#include "cmd-visual/cmd-draw.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest-completion-checker.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-processor.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "tracking/health-bar-tracker.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <string>

/*!
 * @brief モンスターをペットにする
 * @param PlayerType プレイヤーへの参照ポインタ
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void set_pet(PlayerType *player_ptr, MonsterEntity &monster)
{
    QuestCompletionChecker(player_ptr, monster).complete();
    monster.mflag2.set(MonsterConstantFlagType::PET);
    if (monster.get_monrace().kind_flags.has_none_of(alignment_mask)) {
        monster.sub_align = SUB_ALIGN_NEUTRAL;
    }
}

/*!
 * @brief モンスターを怒らせる
 * Anger the monster
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void anger_monster(PlayerType *player_ptr, MonsterEntity &monster)
{
    if (AngbandSystem::get_instance().is_phase_out() || !monster.is_friendly()) {
        return;
    }

    const auto m_name = monster_desc(player_ptr, monster, 0);
    msg_format(_("%s^は怒った！", "%s^ gets angry!"), m_name.data());
    monster.set_hostile();
    chg_virtue(player_ptr, Virtue::INDIVIDUALISM, 1);
    chg_virtue(player_ptr, Virtue::HONOUR, -1);
    chg_virtue(player_ptr, Virtue::JUSTICE, -1);
    chg_virtue(player_ptr, Virtue::COMPASSION, -1);
}

/*!
 * @brief モンスターの睡眠状態値をセットする。0で起きる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_csleep(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (v) {
        if (!monster.is_asleep()) {
            floor.add_mproc(m_idx, MonsterTimedEffect::SLEEP);
            notice = true;
        }
    } else {
        if (monster.is_asleep()) {
            floor.remove_mproc(m_idx, MonsterTimedEffect::SLEEP);
            notice = true;
        }
    }

    monster.mtimed[MonsterTimedEffect::SLEEP] = (int16_t)v;
    if (!notice) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (monster.ml) {
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
        if (monster.is_riding()) {
            rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    if (monster.get_monrace().brightness_flags.has_any_of(has_ld_mask)) {
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    return true;
}

/*!
 * @brief モンスターの加速状態値をセット
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_fast(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!monster.is_accelerated()) {
            floor.add_mproc(m_idx, MonsterTimedEffect::FAST);
            notice = true;
        }
    } else {
        if (monster.is_accelerated()) {
            floor.remove_mproc(m_idx, MonsterTimedEffect::FAST);
            notice = true;
        }
    }

    monster.mtimed[MonsterTimedEffect::FAST] = (int16_t)v;
    if (!notice) {
        return false;
    }

    if (monster.is_riding() && !player_ptr->leaving) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    return true;
}

/*
 * @brief モンスターの原則状態値をセット
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_slow(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!monster.is_decelerated()) {
            floor.add_mproc(m_idx, MonsterTimedEffect::SLOW);
            notice = true;
        }
    } else {
        if (monster.is_decelerated()) {
            floor.remove_mproc(m_idx, MonsterTimedEffect::SLOW);
            notice = true;
        }
    }

    monster.mtimed[MonsterTimedEffect::SLOW] = (int16_t)v;
    if (!notice) {
        return false;
    }

    if (monster.is_riding() && !player_ptr->leaving) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    return true;
}

/*!
 * @brief モンスターの朦朧状態値をセット
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_stunned(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!monster.is_stunned()) {
            floor.add_mproc(m_idx, MonsterTimedEffect::STUN);
            notice = true;
        }
    } else {
        if (monster.is_stunned()) {
            floor.remove_mproc(m_idx, MonsterTimedEffect::STUN);
            notice = true;
        }
    }

    monster.mtimed[MonsterTimedEffect::STUN] = (int16_t)v;
    return notice;
}

/*!
 * @brief モンスターの混乱状態値をセット
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_confused(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!monster.is_confused()) {
            floor.add_mproc(m_idx, MonsterTimedEffect::CONFUSION);
            notice = true;
        }
    } else {
        if (monster.is_confused()) {
            floor.remove_mproc(m_idx, MonsterTimedEffect::CONFUSION);
            notice = true;
        }
    }

    monster.mtimed[MonsterTimedEffect::CONFUSION] = (int16_t)v;
    return notice;
}

/*!
 * @brief モンスターの恐慌状態値をセット
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_monfear(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!monster.is_fearful()) {
            floor.add_mproc(m_idx, MonsterTimedEffect::FEAR);
            notice = true;
        }
    } else {
        if (monster.is_fearful()) {
            floor.remove_mproc(m_idx, MonsterTimedEffect::FEAR);
            notice = true;
        }
    }

    monster.mtimed[MonsterTimedEffect::FEAR] = (int16_t)v;

    if (!notice) {
        return false;
    }

    if (monster.ml) {
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
        if (monster.is_riding()) {
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    return true;
}

/*!
 * @brief モンスターの無敵状態値をセット
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @param energy_need TRUEならば無敵解除時に行動ターン消費を行う
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_invulner(PlayerType *player_ptr, MONSTER_IDX m_idx, int v, bool energy_need)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!monster.is_invulnerable()) {
            floor.add_mproc(m_idx, MonsterTimedEffect::INVULNERABILITY);
            notice = true;
        }
    } else {
        if (monster.is_invulnerable()) {
            floor.remove_mproc(m_idx, MonsterTimedEffect::INVULNERABILITY);
            if (energy_need && !AngbandWorld::get_instance().is_wild_mode()) {
                monster.energy_need += ENERGY_NEED();
            }
            notice = true;
        }
    }

    monster.mtimed[MonsterTimedEffect::INVULNERABILITY] = (int16_t)v;
    if (!notice) {
        return false;
    }

    if (monster.ml) {
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
        if (monster.is_riding()) {
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    return true;
}

/*!
 * @brief モンスターの時間停止処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 時間停止を行う敵のモンスターID
 * @param num 時間停止を行った敵が行動できる回数
 * @param vs_player TRUEならば時間停止開始処理を行う
 * @return 時間停止が行われている状態ならばTRUEを返す
 * @details monster_desc() は視認外のモンスターについて「何か」と返してくるので、この関数ではLOSや透明視等を判定する必要はない
 */
bool set_monster_timewalk(PlayerType *player_ptr, MONSTER_IDX m_idx, int num, bool vs_player)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    auto &world = AngbandWorld::get_instance();
    const auto &monrace = monster.get_real_monrace();
    if (world.timewalk_m_idx) {
        return false;
    }

    if (vs_player) {
        const auto m_name = monster_desc(player_ptr, monster, 0);
        const auto time_message = monrace.get_message(m_name, MonsterMessageType::MESSAGE_TIMESTOP);
        if (time_message) {
            msg_print(*time_message);
        }
        msg_erase();
    }

    world.timewalk_m_idx = m_idx;
    if (vs_player) {
        do_cmd_redraw(player_ptr);
    }

    while (num--) {
        if (!monster.is_valid()) {
            break;
        }

        process_monster(player_ptr, world.timewalk_m_idx);
        monster.reset_target();
        handle_stuff(player_ptr);
        if (vs_player) {
            term_xtra(TERM_XTRA_DELAY, 500);
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags);
    world.timewalk_m_idx = 0;
    const auto p_pos = player_ptr->get_position();
    const auto m_pos = monster.get_position();
    auto should_output_message = floor.has_los_at(m_pos);
    should_output_message &= projectable(floor, p_pos, m_pos);
    if (vs_player || should_output_message) {
        const auto m_name = monster_desc(player_ptr, monster, 0);
        const auto time_message = monrace.get_message(m_name, MonsterMessageType::MESSAGE_TIMESTART);
        if (time_message) {
            msg_print(*time_message);
        }
        msg_erase();
    }

    handle_stuff(player_ptr);
    return true;
}
