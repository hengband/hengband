/*!
 * @brief 騎兵のレイシャルパワー処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "mind/mind-cavalry.h"
#include "cmd-action/cmd-pet.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/smart-learn-types.h"
#include "pet/pet-fall-off.h"
#include "player/player-skill.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-util.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * 荒馬慣らし
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 結果はどうあれ騎乗したらTRUE
 */
bool rodeo(PlayerType *player_ptr)
{
    if (player_ptr->riding) {
        msg_print(_("今は乗馬中だ。", "You ARE riding."));
        return false;
    }

    if (!do_cmd_riding(player_ptr, true)) {
        return true;
    }

    auto &monster = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    const auto &monrace = monster.get_monrace();
    const auto m_name = monster_desc(player_ptr, monster, 0);
    msg_print(_("{}に乗った。", "You ride on {}."), m_name);

    if (monster.is_pet()) {
        return true;
    }

    auto rlev = monrace.level;

    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        rlev = rlev * 3 / 2;
    }
    if (rlev > 60) {
        rlev = 60 + (rlev - 60) / 2;
    }
    if ((randint1(player_ptr->skill_exp[PlayerSkillKindType::RIDING] / 120 + player_ptr->lev * 2 / 3) > rlev) && one_in_(2) &&
        !player_ptr->current_floor_ptr->inside_arena && !AngbandSystem::get_instance().is_phase_out() && monrace.misc_flags.has_not(MonsterMiscType::GUARDIAN) && monrace.misc_flags.has_not(MonsterMiscType::QUESTOR) &&
        (rlev < player_ptr->lev * 3 / 2 + randint0(player_ptr->lev / 5))) {
        msg_print(_("{}を手なずけた。", "You tame {}."), m_name);
        set_pet(player_ptr, monster);
        return true;
    }

    msg_print(_("{}に振り落とされた！", "You have been thrown off by {}."), m_name);
    process_fall_off_horse(player_ptr, 1, true);
    if (!player_ptr->riding || player_ptr->is_dead) { // 落ちる先が無く壁にぶつかったダメージで死んだときもここで抜ける
        return true;
    }

    // 落ちる先が無く振り落とされなかったときも降ろし、重ならないようモンスターを近くの入れる地形へ走り去らせる
    // (フロアに行き先が 1 つも無ければ重なったまま)
    const auto m_idx = player_ptr->riding;
    player_ptr->dismount();
    const auto fled_m_name = monster_desc(player_ptr, monster, 0); // 乗馬中と付かず、見えなくなる前の名前
    if (teleport_away(player_ptr, m_idx, 10, TELEPORT_SPONTANEOUS)) {
        if (monrace.feature_flags.has(MonsterFeatureType::CAN_FLY)) {
            msg_print(_("{}は暴れて飛び去った。", "{} flies away."), str_upcase_first(fled_m_name));
        } else {
            msg_print(_("{}は暴れて走り去った。", "{} bolts away."), str_upcase_first(fled_m_name));
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags);
    rfu.request_riding_status_redrawing();

    return true;
}
