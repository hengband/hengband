/*!
 * @brief 破邪魔法処理
 * @date 2020/06/05
 * @author Hourier
 */

#include "spell-realm/spells-crusade.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/disturbance-options.h"
#include "player-info/race-info.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-sight.h"
#include "spell/range-calc.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "view/display-messages.h"
#include <cmath>

/*!
 * @brief 一時的聖なるのオーラの継続時間をセットする / Set "tim_sh_holy", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_holy(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_sh_holy && !do_dec) {
            if (player_ptr->tim_sh_holy > v) {
                return false;
            }
        } else if (!player_ptr->tim_sh_holy) {
            msg_print(_("体が聖なるオーラで覆われた。", "You are enveloped by a holy aura!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_sh_holy) {
            msg_print(_("聖なるオーラが消えた。", "The holy aura disappeared."));
            notice = true;
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    player_ptr->tim_sh_holy = v;
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(player_ptr, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 目には目をの残り時間をセットする / Set "tim_eyeeye", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す
 * @details 呪術領域でも使えるが、汎用性と行数の兼ね合いを考えて破邪側に入れた
 */
bool set_tim_eyeeye(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_eyeeye && !do_dec) {
            if (player_ptr->tim_eyeeye > v) {
                return false;
            }
        } else if (!player_ptr->tim_eyeeye) {
            msg_print(_("法の守り手になった気がした！", "You feel like a keeper of commandments!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_eyeeye) {
            msg_print(_("懲罰を執行することができなくなった。", "You lost your aura of retribution."));
            notice = true;
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    player_ptr->tim_eyeeye = v;
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(player_ptr, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

void check_emission(PlayerType *player_ptr)
{
    if (player_ptr->tim_emission > 0) {
        if (player_ptr->lev > 29) {
            map_area(player_ptr, player_ptr->cur_lite);
        }
        if (player_ptr->lev > 24) {
            detect_traps(player_ptr, player_ptr->cur_lite, true);
        }
        if (player_ptr->lev > 19) {
            detect_monsters_evil(player_ptr, player_ptr->cur_lite);
        }
    }
}

void check_demigod(PlayerType *player_ptr)
{
    if (player_ptr->mimic_form == MimicKindType::DEMIGOD) {
        const Dice dice(1, player_ptr->lev * 4);

        dispel_evil(player_ptr, dice.roll());
    }
}

bool has_slay_demon_from_exorcism(const PlayerType *player_ptr)
{
    if (player_ptr->tim_exorcism > 0) {
        if (player_ptr->lev < THRESHOLD_KILL_FROM_EXORCISM) {
            return true;
        }
    }
    return false;
}

bool has_kill_demon_from_exorcism(const PlayerType *player_ptr)
{
    if (player_ptr->tim_exorcism > 0) {
        if (player_ptr->lev >= THRESHOLD_KILL_FROM_EXORCISM) {
            return true;
        }
    }
    return false;
}

bool has_slay_undead_from_exorcism(const PlayerType *player_ptr)
{
    if (player_ptr->tim_exorcism > 0) {
        if (player_ptr->lev < THRESHOLD_KILL_FROM_EXORCISM) {
            return true;
        }
    }
    return false;
}

bool has_kill_undead_from_exorcism(const PlayerType *player_ptr)
{
    if (player_ptr->tim_exorcism > 0) {
        if (player_ptr->lev >= THRESHOLD_KILL_FROM_EXORCISM) {
            return true;
        }
    }
    return false;
}
