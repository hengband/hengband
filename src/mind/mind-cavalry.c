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
#include "view/display-messages.h"

/*!
 * 荒馬慣らし
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 結果はどうあれ騎乗したらTRUE
 */
bool rodeo(player_type *creature_ptr)
{
    GAME_TEXT m_name[MAX_NLEN];
    monster_type *m_ptr;
    monster_race *r_ptr;
    int rlev;

    if (creature_ptr->riding) {
        msg_print(_("今は乗馬中だ。", "You ARE riding."));
        return FALSE;
    }

    if (!do_cmd_riding(creature_ptr, TRUE))
        return TRUE;

    m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
    r_ptr = &r_info[m_ptr->r_idx];
    monster_desc(creature_ptr, m_name, m_ptr, 0);
    msg_format(_("%sに乗った。", "You ride on %s."), m_name);

    if (is_pet(m_ptr))
        return TRUE;

    rlev = r_ptr->level;

    if (r_ptr->flags1 & RF1_UNIQUE)
        rlev = rlev * 3 / 2;
    if (rlev > 60)
        rlev = 60 + (rlev - 60) / 2;
    if ((randint1(creature_ptr->skill_exp[GINOU_RIDING] / 120 + creature_ptr->lev * 2 / 3) > rlev) && one_in_(2)
        && !creature_ptr->current_floor_ptr->inside_arena && !creature_ptr->phase_out && !(r_ptr->flags7 & (RF7_GUARDIAN)) && !(r_ptr->flags1 & (RF1_QUESTOR))
        && (rlev < creature_ptr->lev * 3 / 2 + randint0(creature_ptr->lev / 5))) {
        msg_format(_("%sを手なずけた。", "You tame %s."), m_name);
        set_pet(creature_ptr, m_ptr);
    } else {
        msg_format(_("%sに振り落とされた！", "You have been thrown off by %s."), m_name);
        process_fall_off_horse(creature_ptr, 1, TRUE);

        /* 落馬処理に失敗してもとにかく乗馬解除 */
        creature_ptr->riding = 0;
    }

    return TRUE;
}
