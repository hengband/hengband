#include "market/building-quest.h"
#include "cmd-building/cmd-building.h"
#include "info-reader/fixed-map-parser.h"
#include "dungeon/quest.h"
#include "grid/grid.h"
#include "market/building-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-list.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"

/*!
 * @brief クエスト情報を表示しつつ処理する。/ Display quest information
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param questnum クエストのID
 * @param do_init クエストの開始処理(TRUE)、結果処理か(FALSE)
 * @return なし
 */
static void get_questinfo(player_type *player_ptr, IDX questnum, bool do_init)
{
    for (int i = 0; i < 10; i++) {
        quest_text[i][0] = '\0';
    }

    quest_text_line = 0;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    QUEST_IDX old_quest = floor_ptr->inside_quest;
    floor_ptr->inside_quest = questnum;

    init_flags = INIT_SHOW_TEXT;
    if (do_init)
        init_flags |= INIT_ASSIGN;

    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    floor_ptr->inside_quest = old_quest;

    GAME_TEXT tmp_str[80];
    sprintf(tmp_str, _("クエスト情報 (危険度: %d 階相当)", "Quest Information (Danger level: %d)"), (int)quest[questnum].level);
    prt(tmp_str, 5, 0);
    prt(quest[questnum].name, 7, 0);

    for (int i = 0; i < 10; i++) {
        c_put_str(TERM_YELLOW, quest_text[i], i + 8, 0);
    }
}

/*!
 * @brief クエスト処理のメインルーチン / Request a quest from the Lord.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void castle_quest(player_type *player_ptr)
{
    clear_bldg(4, 18);
    QUEST_IDX q_index = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].special;

    if (!q_index) {
        put_str(_("今のところクエストはありません。", "I don't have a quest for you at the moment."), 8, 0);
        return;
    }

    quest_type *q_ptr;
    q_ptr = &quest[q_index];
    if (q_ptr->status == QUEST_STATUS_COMPLETED) {
        q_ptr->status = QUEST_STATUS_REWARDED;
        get_questinfo(player_ptr, q_index, FALSE);
        reinit_wilderness = TRUE;
        return;
    }

    if (q_ptr->status == QUEST_STATUS_FAILED) {
        get_questinfo(player_ptr, q_index, FALSE);
        q_ptr->status = QUEST_STATUS_FAILED_DONE;
        reinit_wilderness = TRUE;
        return;
    }

    if (q_ptr->status == QUEST_STATUS_TAKEN) {
        put_str(_("あなたは現在のクエストを終了させていません！", "You have not completed your current quest yet!"), 8, 0);
        put_str(_("CTRL-Qを使えばクエストの状態がチェックできます。", "Use CTRL-Q to check the status of your quest."), 9, 0);
        put_str(_("クエストを終わらせたら戻って来て下さい。", "Return when you have completed your quest."), 12, 0);
        return;
    }

    if (q_ptr->status != QUEST_STATUS_UNTAKEN)
        return;

    q_ptr->status = QUEST_STATUS_TAKEN;
    reinit_wilderness = TRUE;
    if (q_ptr->type != QUEST_TYPE_KILL_ANY_LEVEL) {
        get_questinfo(player_ptr, q_index, TRUE);
        return;
    }

    if (q_ptr->r_idx == 0) {
        q_ptr->r_idx = get_mon_num(player_ptr, q_ptr->level + 4 + randint1(6), 0);
    }

    monster_race *r_ptr;
    r_ptr = &r_info[q_ptr->r_idx];
    while ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->rarity != 1)) {
        q_ptr->r_idx = get_mon_num(player_ptr, q_ptr->level + 4 + randint1(6), 0);
        r_ptr = &r_info[q_ptr->r_idx];
    }

    if (q_ptr->max_num == 0) {
        if (randint1(10) > 7)
            q_ptr->max_num = 1;
        else
            q_ptr->max_num = randint1(3) + 1;
    }

    q_ptr->cur_num = 0;
    concptr name = (r_name + r_ptr->name);
#ifdef JP
    msg_format("クエスト: %sを %d体倒す", name, q_ptr->max_num);
#else
    msg_format("Your quest: kill %d %s", q_ptr->max_num, name);
#endif
    get_questinfo(player_ptr, q_index, TRUE);
}
