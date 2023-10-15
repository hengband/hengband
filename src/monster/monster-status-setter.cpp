#include "monster/monster-status-setter.h"
#include "avatar/avatar.h"
#include "cmd-visual/cmd-draw.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest-completion-checker.h"
#include "floor/cave.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-processor.h"
#include "monster/monster-status.h" //!< @todo 相互依存. 後で何とかする.
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <string>

/*!
 * @brief モンスターをペットにする
 * @param PlayerType プレイヤーへの参照ポインタ
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void set_pet(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    QuestCompletionChecker(player_ptr, m_ptr).complete();
    m_ptr->mflag2.set(MonsterConstantFlagType::PET);
    if (m_ptr->get_monrace().kind_flags.has_none_of(alignment_mask)) {
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
    }
}

/*!
 * @brief モンスターを敵に回す
 * Makes the monster hostile towards the player
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void set_hostile(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    if (player_ptr->phase_out) {
        return;
    }

    m_ptr->mflag2.reset({ MonsterConstantFlagType::PET, MonsterConstantFlagType::FRIENDLY });
}

/*!
 * @brief モンスターを怒らせる
 * Anger the monster
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void anger_monster(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    if (player_ptr->phase_out || !m_ptr->is_friendly()) {
        return;
    }

    const auto m_name = monster_desc(player_ptr, m_ptr, 0);
    msg_format(_("%s^は怒った！", "%s^ gets angry!"), m_name.data());
    set_hostile(player_ptr, m_ptr);
    chg_virtue(player_ptr, Virtue::INDIVIDUALISM, 1);
    chg_virtue(player_ptr, Virtue::HONOUR, -1);
    chg_virtue(player_ptr, Virtue::JUSTICE, -1);
    chg_virtue(player_ptr, Virtue::COMPASSION, -1);
}

/*!
 * @brief モンスターの時限ステータスリストを削除
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return m_idx モンスターの参照ID
 * @return mproc_type 削除したいモンスターの時限ステータスID
 */
static void mproc_remove(FloorType *floor_ptr, MONSTER_IDX m_idx, int mproc_type)
{
    int mproc_idx = get_mproc_idx(floor_ptr, m_idx, mproc_type);
    if (mproc_idx >= 0) {
        floor_ptr->mproc_list[mproc_type][mproc_idx] = floor_ptr->mproc_list[mproc_type][--floor_ptr->mproc_max[mproc_type]];
    }
}

/*!
 * @brief モンスターの睡眠状態値をセットする。0で起きる。 /
 * Set "m_ptr->mtimed[MTIMED_CSLEEP]", notice observable changes
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_csleep(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (v) {
        if (!m_ptr->is_asleep()) {
            mproc_add(floor_ptr, m_idx, MTIMED_CSLEEP);
            notice = true;
        }
    } else {
        if (m_ptr->is_asleep()) {
            mproc_remove(floor_ptr, m_idx, MTIMED_CSLEEP);
            notice = true;
        }
    }

    m_ptr->mtimed[MTIMED_CSLEEP] = (int16_t)v;
    if (!notice) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (m_ptr->ml) {
        if (player_ptr->health_who == m_idx) {
            rfu.set_flag(MainWindowRedrawingFlag::HEALTH);
        }

        if (player_ptr->riding == m_idx) {
            rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    if (m_ptr->get_monrace().brightness_flags.has_any_of(has_ld_mask)) {
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    return true;
}

/*!
 * @brief モンスターの加速状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_FAST]", notice observable changes
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_fast(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!m_ptr->is_accelerated()) {
            mproc_add(floor_ptr, m_idx, MTIMED_FAST);
            notice = true;
        }
    } else {
        if (m_ptr->is_accelerated()) {
            mproc_remove(floor_ptr, m_idx, MTIMED_FAST);
            notice = true;
        }
    }

    m_ptr->mtimed[MTIMED_FAST] = (int16_t)v;
    if (!notice) {
        return false;
    }

    if ((player_ptr->riding == m_idx) && !player_ptr->leaving) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    return true;
}

/*
 * Set "m_ptr->mtimed[MTIMED_SLOW]", notice observable changes
 */
bool set_monster_slow(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!m_ptr->is_decelerated()) {
            mproc_add(floor_ptr, m_idx, MTIMED_SLOW);
            notice = true;
        }
    } else {
        if (m_ptr->is_decelerated()) {
            mproc_remove(floor_ptr, m_idx, MTIMED_SLOW);
            notice = true;
        }
    }

    m_ptr->mtimed[MTIMED_SLOW] = (int16_t)v;
    if (!notice) {
        return false;
    }

    if ((player_ptr->riding == m_idx) && !player_ptr->leaving) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    return true;
}

/*!
 * @brief モンスターの朦朧状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_STUNNED]", notice observable changes
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_stunned(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!m_ptr->is_stunned()) {
            mproc_add(floor_ptr, m_idx, MTIMED_STUNNED);
            notice = true;
        }
    } else {
        if (m_ptr->is_stunned()) {
            mproc_remove(floor_ptr, m_idx, MTIMED_STUNNED);
            notice = true;
        }
    }

    m_ptr->mtimed[MTIMED_STUNNED] = (int16_t)v;
    return notice;
}

/*!
 * @brief モンスターの混乱状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_CONFUSED]", notice observable changes
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_confused(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!m_ptr->is_confused()) {
            mproc_add(floor_ptr, m_idx, MTIMED_CONFUSED);
            notice = true;
        }
    } else {
        if (m_ptr->is_confused()) {
            mproc_remove(floor_ptr, m_idx, MTIMED_CONFUSED);
            notice = true;
        }
    }

    m_ptr->mtimed[MTIMED_CONFUSED] = (int16_t)v;
    return notice;
}

/*!
 * @brief モンスターの恐慌状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_MONFEAR]", notice observable changes
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_monfear(PlayerType *player_ptr, MONSTER_IDX m_idx, int v)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!m_ptr->is_fearful()) {
            mproc_add(floor_ptr, m_idx, MTIMED_MONFEAR);
            notice = true;
        }
    } else {
        if (m_ptr->is_fearful()) {
            mproc_remove(floor_ptr, m_idx, MTIMED_MONFEAR);
            notice = true;
        }
    }

    m_ptr->mtimed[MTIMED_MONFEAR] = (int16_t)v;

    if (!notice) {
        return false;
    }

    if (m_ptr->ml) {
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        if (player_ptr->health_who == m_idx) {
            rfu.set_flag(MainWindowRedrawingFlag::HEALTH);
        }

        if (player_ptr->riding == m_idx) {
            rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    return true;
}

/*!
 * @brief モンスターの無敵状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_INVULNER]", notice observable changes
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @param energy_need TRUEならば無敵解除時に行動ターン消費を行う
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_invulner(PlayerType *player_ptr, MONSTER_IDX m_idx, int v, bool energy_need)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = false;
    v = (v > 200) ? 200 : (v < 0) ? 0
                                  : v;
    if (v) {
        if (!m_ptr->is_invulnerable()) {
            mproc_add(floor_ptr, m_idx, MTIMED_INVULNER);
            notice = true;
        }
    } else {
        if (m_ptr->is_invulnerable()) {
            mproc_remove(floor_ptr, m_idx, MTIMED_INVULNER);
            if (energy_need && !player_ptr->wild_mode) {
                m_ptr->energy_need += ENERGY_NEED();
            }
            notice = true;
        }
    }

    m_ptr->mtimed[MTIMED_INVULNER] = (int16_t)v;
    if (!notice) {
        return false;
    }

    if (m_ptr->ml) {
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        if (player_ptr->health_who == m_idx) {
            rfu.set_flag(MainWindowRedrawingFlag::HEALTH);
        }

        if (player_ptr->riding == m_idx) {
            rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    return true;
}

/*!
 * @brief モンスターの時間停止処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param num 時間停止を行った敵が行動できる回数
 * @param who 時間停止を行う敵の種族番号
 * @param vs_player TRUEならば時間停止開始処理を行う
 * @return 時間停止が行われている状態ならばTRUEを返す
 * @details monster_desc() は視認外のモンスターについて「何か」と返してくるので、この関数ではLOSや透明視等を判定する必要はない
 */
bool set_monster_timewalk(PlayerType *player_ptr, int num, MonsterRaceId who, bool vs_player)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[hack_m_idx];
    if (w_ptr->timewalk_m_idx) {
        return false;
    }

    if (vs_player) {
        const auto m_name = monster_desc(player_ptr, m_ptr, 0);
        std::string mes;
        switch (who) {
        case MonsterRaceId::DIO:
            mes = _("「『ザ・ワールド』！　時は止まった！」", format("%s yells 'The World! Time has stopped!'", m_name.data()));
            break;
        case MonsterRaceId::WONG:
            mes = _("「時よ！」", format("%s yells 'Time!'", m_name.data()));
            break;
        case MonsterRaceId::DIAVOLO:
            mes = _("『キング・クリムゾン』！", format("%s yells 'King Crison!'", m_name.data()));
            break;
        default:
            mes = format(_("%sは時を止めた！", "%s stops the time!"), m_name.data());
            break;
        }

        msg_print(mes);
        msg_print(nullptr);
    }

    w_ptr->timewalk_m_idx = hack_m_idx;
    if (vs_player) {
        do_cmd_redraw(player_ptr);
    }

    while (num--) {
        if (!m_ptr->is_valid()) {
            break;
        }

        process_monster(player_ptr, w_ptr->timewalk_m_idx);
        reset_target(m_ptr);
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
    w_ptr->timewalk_m_idx = 0;
    if (vs_player || (player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx) && projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx))) {
        std::string mes;
        switch (who) {
        case MonsterRaceId::DIAVOLO:
            mes = _("これが我が『キング・クリムゾン』の能力！　『時間を消し去って』飛び越えさせた…！！",
                "This is the ability of my 'King Crimson'! 'Erase the time' and let it jump over... !!");
            break;
        default:
            mes = _("「時は動きだす…」", "You feel time flowing around you once more.");
            break;
        }

        msg_print(mes);
        msg_print(nullptr);
    }

    handle_stuff(player_ptr);
    return true;
}
