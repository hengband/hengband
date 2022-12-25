/*!
 * @brief 騎兵のレイシャルパワー処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "mind/mind-cavalry.h"
#include "cmd-action/cmd-pet.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/smart-learn-types.h"
#include "pet/pet-fall-off.h"
#include "player/player-skill.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
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

    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    const auto m_name = monster_desc(player_ptr, m_ptr, 0);
    msg_format(_("%sに乗った。", "You ride on %s."), m_name.data());

    if (m_ptr->is_pet()) {
        return true;
    }

    auto rlev = r_ptr->level;

    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        rlev = rlev * 3 / 2;
    }
    if (rlev > 60) {
        rlev = 60 + (rlev - 60) / 2;
    }
    if ((randint1(player_ptr->skill_exp[PlayerSkillKindType::RIDING] / 120 + player_ptr->lev * 2 / 3) > rlev) && one_in_(2) &&
        !player_ptr->current_floor_ptr->inside_arena && !player_ptr->phase_out && !(r_ptr->flags7 & (RF7_GUARDIAN)) && !(r_ptr->flags1 & (RF1_QUESTOR)) &&
        (rlev < player_ptr->lev * 3 / 2 + randint0(player_ptr->lev / 5))) {
        msg_format(_("%sを手なずけた。", "You tame %s."), m_name.data());
        set_pet(player_ptr, m_ptr);
    } else {
        msg_format(_("%sに振り落とされた！", "You have been thrown off by %s."), m_name.data());
        process_fall_off_horse(player_ptr, 1, true);

        /* 落馬処理に失敗してもとにかく乗馬解除 */
        player_ptr->riding = 0;
    }

    return true;
}
