#include "core/stuff-handler.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "player/player-status.h"
#include "system/player-type-definition.h"

/*!
 * @brief 全更新処理をチェックして処理していく
 * Handle "player_ptr->update" and "player_ptr->redraw" and "player_ptr->window"
 */
void handle_stuff(PlayerType *player_ptr)
{
    if (player_ptr->update) {
        update_creature(player_ptr);
    }
    if (player_ptr->redraw) {
        redraw_stuff(player_ptr);
    }
    if (player_ptr->window_flags) {
        window_stuff(player_ptr);
    }
}

/*
 * Track the given monster race
 */
void monster_race_track(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    player_ptr->monster_race_idx = r_idx;
    player_ptr->window_flags |= (PW_MONSTER);
}

/*
 * Track the given object kind
 */
void object_kind_track(PlayerType *player_ptr, KIND_OBJECT_IDX k_idx)
{
    player_ptr->baseitem_info_idx = k_idx;
    player_ptr->window_flags |= (PW_OBJECT);
}

/*
 * Track a new monster
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx トラッキング対象のモンスターID。0の時キャンセル
 * @param なし
 */
void health_track(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    if (m_idx && m_idx == player_ptr->riding) {
        return;
    }

    player_ptr->health_who = m_idx;
    player_ptr->redraw |= (PR_HEALTH);
}

bool update_player(PlayerType *player_ptr)
{
    player_ptr->update |= PU_COMBINE | PU_REORDER;
    player_ptr->window_flags |= PW_INVEN;
    return true;
}

bool redraw_player(PlayerType *player_ptr)
{
    if (player_ptr->csp > player_ptr->msp) {
        player_ptr->csp = player_ptr->msp;
    }

    player_ptr->redraw |= PR_MANA;
    player_ptr->update |= PU_COMBINE | PU_REORDER;
    player_ptr->window_flags |= PW_INVEN;
    return true;
}
