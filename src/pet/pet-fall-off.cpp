/*!
 * @brief 落馬処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "pet/pet-fall-off.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-race/monster-race.h"
#include "monster/monster-describer.h"
#include "pet/pet-util.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "view/display-messages.h"

/*!
 * @brief モンスターから直接攻撃を受けた時に落馬するかどうかを判定し、判定アウトならば落馬させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void check_fall_off_horse(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if ((player_ptr->riding == 0) || (monap_ptr->damage == 0))
        return;

    char m_steed_name[MAX_NLEN];
    monster_desc(player_ptr, m_steed_name, &player_ptr->current_floor_ptr->m_list[player_ptr->riding], 0);
    if (process_fall_off_horse(player_ptr, (monap_ptr->damage > 200) ? 200 : monap_ptr->damage, false))
        msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_steed_name);
}

/*!
 * @brief 落馬する可能性を計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 落馬判定を発した際に受けたダメージ量
 * @param force TRUEならば強制的に落馬する
 * @param 乗馬中のモンスターのレベル
 * @return FALSEなら落馬しないことで確定、TRUEなら処理続行
 * @details レベルの低い乗馬からは落馬しにくい
 */
static bool calc_fall_off_possibility(PlayerType *player_ptr, const HIT_POINT dam, const bool force, monster_race *r_ptr)
{
    if (force)
        return true;

    auto cur = player_ptr->skill_exp[PlayerSkillKindType::RIDING];

    int fall_off_level = r_ptr->level;
    if (player_ptr->riding_ryoute)
        fall_off_level += 20;

    PlayerSkill(player_ptr).gain_riding_skill_exp_on_fall_off_check(dam);

    if (randint0(dam / 2 + fall_off_level * 2) >= cur / 30 + 10)
        return true;

    if ((((player_ptr->pclass == PlayerClassType::BEASTMASTER) || (player_ptr->pclass == PlayerClassType::CAVALRY)) && !player_ptr->riding_ryoute)
        || !one_in_(player_ptr->lev * (player_ptr->riding_ryoute ? 2 : 3) + 30)) {
        return false;
    }

    return true;
}

/*!
 * @brief プレイヤーの落馬判定処理
 * @param dam 落馬判定を発した際に受けたダメージ量
 * @param force TRUEならば強制的に落馬する
 * @return 実際に落馬したらTRUEを返す
 */
bool process_fall_off_horse(PlayerType *player_ptr, HIT_POINT dam, bool force)
{
    POSITION sy = 0;
    POSITION sx = 0;
    int sn = 0;
    GAME_TEXT m_name[MAX_NLEN];
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (!player_ptr->riding || player_ptr->wild_mode)
        return false;

    if (dam >= 0 || force) {
        if (!calc_fall_off_possibility(player_ptr, dam, force, r_ptr))
            return false;

        /* Check around the player */
        for (DIRECTION i = 0; i < 8; i++) {
            POSITION y = player_ptr->y + ddy_ddd[i];
            POSITION x = player_ptr->x + ddx_ddd[i];

            grid_type *g_ptr;
            g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

            if (g_ptr->m_idx)
                continue;

            /* Skip non-empty grids */
            if (!g_ptr->cave_has_flag(FloorFeatureType::MOVE) && !g_ptr->cave_has_flag(FloorFeatureType::CAN_FLY)) {
                if (!can_player_ride_pet(player_ptr, g_ptr, false))
                    continue;
            }

            if (g_ptr->cave_has_flag(FloorFeatureType::PATTERN))
                continue;

            /* Count "safe" grids */
            sn++;

            /* Randomize choice */
            if (randint0(sn) > 0)
                continue;

            /* Save the safe location */
            sy = y;
            sx = x;
        }

        if (!sn) {
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("%sから振り落とされそうになって、壁にぶつかった。", "You have nearly fallen from %s but bumped into a wall."), m_name);
            take_hit(player_ptr, DAMAGE_NOESCAPE, r_ptr->level + 3, _("壁への衝突", "bumping into a wall"));
            return false;
        }

        POSITION old_y = player_ptr->y;
        POSITION old_x = player_ptr->x;
        player_ptr->y = sy;
        player_ptr->x = sx;
        lite_spot(player_ptr, old_y, old_x);
        lite_spot(player_ptr, player_ptr->y, player_ptr->x);
        verify_panel(player_ptr);
    }

    player_ptr->riding = 0;
    player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
    player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;

    player_ptr->update |= (PU_BONUS | PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);
    handle_stuff(player_ptr);

    player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
    player_ptr->redraw |= (PR_EXTRA);

    /* Update health track of mount */
    player_ptr->redraw |= (PR_UHEALTH);

    bool fall_dam = false;
    if (player_ptr->levitation && !force) {
        monster_desc(player_ptr, m_name, m_ptr, 0);
        msg_format(_("%sから落ちたが、空中でうまく体勢を立て直して着地した。", "You are thrown from %s but make a good landing."), m_name);
    } else {
        take_hit(player_ptr, DAMAGE_NOESCAPE, r_ptr->level + 3, _("落馬", "Falling from riding"));
        fall_dam = true;
    }

    if (sy && !player_ptr->is_dead)
        (void)move_player_effect(player_ptr, player_ptr->y, player_ptr->x, MPE_DONT_PICKUP | MPE_DONT_SWAP_MON);

    return fall_dam;
}
