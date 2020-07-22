#include "core/stuff-handler.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"

/*!
 * @brief 全更新処理をチェックして処理していく
 * Handle "player_ptr->update" and "player_ptr->redraw" and "player_ptr->window"
 * @return なし
 */
void handle_stuff(player_type* player_ptr)
{
    if (player_ptr->update)
        update_creature(player_ptr);
    if (player_ptr->redraw)
        redraw_stuff(player_ptr);
    if (player_ptr->window)
        window_stuff(player_ptr);
}

/*
 * Track the given monster race
 */
void monster_race_track(player_type *player_ptr, MONRACE_IDX r_idx)
{
    player_ptr->monster_race_idx = r_idx;
    player_ptr->window |= (PW_MONSTER);
}

/*
 * Track the given object kind
 */
void object_kind_track(player_type *player_ptr, KIND_OBJECT_IDX k_idx)
{
    player_ptr->object_kind_idx = k_idx;
    player_ptr->window |= (PW_OBJECT);
}

/*
 * Track a new monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx トラッキング対象のモンスターID。0の時キャンセル
 * @param なし
 */
void health_track(player_type *player_ptr, MONSTER_IDX m_idx)
{
    if (m_idx && m_idx == player_ptr->riding)
        return;

    player_ptr->health_who = m_idx;
    player_ptr->redraw |= (PR_HEALTH);
}

bool update_player(player_type *caster_ptr)
{
    caster_ptr->update |= PU_COMBINE | PU_REORDER;
    caster_ptr->window |= PW_INVEN;
    return TRUE;
}

bool redraw_player(player_type *caster_ptr)
{
    if (caster_ptr->csp > caster_ptr->msp) {
        caster_ptr->csp = caster_ptr->msp;
    }

    caster_ptr->redraw |= PR_MANA;
    caster_ptr->update |= PU_COMBINE | PU_REORDER;
    caster_ptr->window |= PW_INVEN;
    return TRUE;
}
