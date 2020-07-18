#include "object-hook/hook-quest.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "system/artifact-type-definition.h"
#include "object-enchant/trg-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "world/world.h"

/*!
 * @brief オブジェクトが賞金首の報酬対象になるかを返す
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが報酬対象になるならTRUEを返す
 */
bool object_is_bounty(player_type *player_ptr, object_type *o_ptr)
{
    int i;

    if (o_ptr->tval != TV_CORPSE)
        return FALSE;

    if (vanilla_town)
        return FALSE;

    if (player_ptr->today_mon > 0 && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name)))
        return TRUE;

    if (o_ptr->pval == MON_TSUCHINOKO)
        return TRUE;

    for (i = 0; i < MAX_BOUNTY; i++)
        if (o_ptr->pval == current_world_ptr->bounty_r_idx[i])
            break;
    if (i < MAX_BOUNTY)
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトがクエストの達成目的か否かを返す。
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す。
 */
bool object_is_quest_target(player_type *player_ptr, object_type *o_ptr)
{
    if (player_ptr->current_floor_ptr->inside_quest == 0)
        return FALSE;

    ARTIFACT_IDX a_idx = quest[player_ptr->current_floor_ptr->inside_quest].k_idx;
    if (a_idx == 0)
        return FALSE;

    artifact_type *a_ptr = &a_info[a_idx];
    if ((a_ptr->gen_flags & TRG_INSTA_ART) != 0)
        return FALSE;

    return (o_ptr->tval == a_ptr->tval) && (o_ptr->sval == a_ptr->sval);
}
