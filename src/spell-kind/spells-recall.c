#include "spell-kind/spells-recall.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの帰還発動及び中止処理 /
 * Recall the player to town or dungeon
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param turns 発動までのターン数
 * @return 常にTRUEを返す
 */
bool recall_player(player_type *creature_ptr, TIME_EFFECT turns)
{
    /*
     * TODO: Recall the player to the last
     * visited town when in the wilderness
     */
    if (creature_ptr->current_floor_ptr->inside_arena || ironman_downward) {
        msg_print(_("何も起こらなかった。", "Nothing happens."));
        return TRUE;
    }

    bool is_special_floor = creature_ptr->current_floor_ptr->dun_level > 0;
    is_special_floor &= max_dlv[creature_ptr->dungeon_idx] > creature_ptr->current_floor_ptr->dun_level;
    is_special_floor &= !creature_ptr->current_floor_ptr->inside_quest;
    is_special_floor &= !creature_ptr->word_recall;
    if (is_special_floor) {
        if (get_check(_("ここは最深到達階より浅い階です。この階に戻って来ますか？ ", "Reset recall depth? "))) {
            max_dlv[creature_ptr->dungeon_idx] = creature_ptr->current_floor_ptr->dun_level;
            if (record_maxdepth)
                exe_write_diary(creature_ptr, DIARY_TRUMP, creature_ptr->dungeon_idx, _("帰還のときに", "when recalled from dungeon"));
        }
    }

    if (creature_ptr->word_recall) {
        creature_ptr->word_recall = 0;
        msg_print(_("張りつめた大気が流れ去った...", "A tension leaves the air around you..."));
        creature_ptr->redraw |= (PR_STATUS);
        return TRUE;
    }

    if (!creature_ptr->current_floor_ptr->dun_level) {
        DUNGEON_IDX select_dungeon;
        select_dungeon = choose_dungeon(_("に帰還", "recall"), 2, 14);
        if (!select_dungeon)
            return FALSE;
        creature_ptr->recall_dungeon = select_dungeon;
    }

    creature_ptr->word_recall = turns;
    msg_print(_("回りの大気が張りつめてきた...", "The air about you becomes charged..."));
    creature_ptr->redraw |= (PR_STATUS);
    return TRUE;
}

bool free_level_recall(player_type *creature_ptr)
{
    DUNGEON_IDX select_dungeon = choose_dungeon(_("にテレポート", "teleport"), 4, 0);
    if (!select_dungeon)
        return FALSE;

    DEPTH max_depth = d_info[select_dungeon].maxdepth;
    if (select_dungeon == DUNGEON_ANGBAND) {
        if (quest[QUEST_OBERON].status != QUEST_STATUS_FINISHED)
            max_depth = 98;
        else if (quest[QUEST_SERPENT].status != QUEST_STATUS_FINISHED)
            max_depth = 99;
    }

    QUANTITY amt = get_quantity(
        format(_("%sの何階にテレポートしますか？", "Teleport to which level of %s? "), d_name + d_info[select_dungeon].name), (QUANTITY)max_depth);
    if (amt <= 0) {
        return FALSE;
    }

    creature_ptr->word_recall = 1;
    creature_ptr->recall_dungeon = select_dungeon;
    max_dlv[creature_ptr->recall_dungeon]
        = ((amt > d_info[select_dungeon].maxdepth) ? d_info[select_dungeon].maxdepth
                                                   : ((amt < d_info[select_dungeon].mindepth) ? d_info[select_dungeon].mindepth : amt));
    if (record_maxdepth)
        exe_write_diary(creature_ptr, DIARY_TRUMP, select_dungeon, _("トランプタワーで", "at Trump Tower"));

    msg_print(_("回りの大気が張りつめてきた...", "The air about you becomes charged..."));

    creature_ptr->redraw |= PR_STATUS;
    return TRUE;
}

/*!
 * @brief フロア・リセット処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return リセット処理が実際に行われたらTRUEを返す
 */
bool reset_recall(player_type *caster_ptr)
{
    int select_dungeon, dummy = 0;
    char ppp[80];
    char tmp_val[160];

    select_dungeon = choose_dungeon(_("をセット", "reset"), 2, 14);
    if (ironman_downward) {
        msg_print(_("何も起こらなかった。", "Nothing happens."));
        return TRUE;
    }

    if (!select_dungeon)
        return FALSE;
    sprintf(ppp, _("何階にセットしますか (%d-%d):", "Reset to which level (%d-%d): "), (int)d_info[select_dungeon].mindepth, (int)max_dlv[select_dungeon]);
    sprintf(tmp_val, "%d", (int)MAX(caster_ptr->current_floor_ptr->dun_level, 1));

    if (!get_string(ppp, tmp_val, 10)) {
        return FALSE;
    }

    dummy = atoi(tmp_val);
    if (dummy < 1)
        dummy = 1;
    if (dummy > max_dlv[select_dungeon])
        dummy = max_dlv[select_dungeon];
    if (dummy < d_info[select_dungeon].mindepth)
        dummy = d_info[select_dungeon].mindepth;

    max_dlv[select_dungeon] = dummy;

    if (record_maxdepth)
        exe_write_diary(caster_ptr, DIARY_TRUMP, select_dungeon, _("フロア・リセットで", "using a scroll of reset recall"));
#ifdef JP
    msg_format("%sの帰還レベルを %d 階にセット。", d_name + d_info[select_dungeon].name, dummy, dummy * 50);
#else
    msg_format("Recall depth set to level %d (%d').", dummy, dummy * 50);
#endif
    return TRUE;
}
