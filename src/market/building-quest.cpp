#include "market/building-quest.h"
#include "cmd-building/cmd-building.h"
#include "core/asking-player.h"
#include "dungeon/quest.h"
#include "info-reader/fixed-map-parser.h"
#include "market/building-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-list.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"

/*!
 * @brief クエスト情報を処理しつつ取得する。/ Process and get quest information
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param questnum クエストのID
 * @param do_init クエストの開始処理か(true)、結果処理か(FALSE)
 */
static void get_questinfo(PlayerType *player_ptr, QuestId questnum, bool do_init)
{
    for (int i = 0; i < 10; i++) {
        quest_text[i][0] = '\0';
    }

    quest_text_line = 0;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    QuestId old_quest = floor_ptr->quest_number;
    floor_ptr->quest_number = questnum;

    init_flags = INIT_SHOW_TEXT;
    if (do_init)
        init_flags = i2enum<init_flags_type>(init_flags | INIT_ASSIGN);

    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    floor_ptr->quest_number = old_quest;
}

/*!
 * @brief クエスト情報を処理しつつ表示する。/ Process and display quest information
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param questnum クエストのID
 * @param do_init クエストの開始処理か(true)、結果処理か(FALSE)
 */
void print_questinfo(PlayerType *player_ptr, QuestId questnum, bool do_init)
{
    get_questinfo(player_ptr, questnum, do_init);

    GAME_TEXT tmp_str[80];
    sprintf(tmp_str, _("クエスト情報 (危険度: %d 階相当)", "Quest Information (Danger level: %d)"), (int)quest[enum2i(questnum)].level);
    prt(tmp_str, 5, 0);
    prt(quest[enum2i(questnum)].name, 7, 0);

    for (int i = 0; i < 10; i++) {
        c_put_str(TERM_YELLOW, quest_text[i], i + 8, 0);
    }
}

/*!
 * @brief クエスト処理のメインルーチン / Request a quest from the Lord.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void castle_quest(PlayerType *player_ptr)
{
    clear_bldg(4, 18);
    QuestId q_index = i2enum<QuestId>(player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].special);

    if (!inside_quest(q_index)) {
        put_str(_("今のところクエストはありません。", "I don't have a quest for you at the moment."), 8, 0);
        return;
    }

    quest_type *q_ptr;
    q_ptr = &quest[enum2i(q_index)];
    if (q_ptr->status == QuestStatusType::COMPLETED) {
        q_ptr->status = QuestStatusType::REWARDED;
        print_questinfo(player_ptr, q_index, false);
        reinit_wilderness = true;
        return;
    }

    if (q_ptr->status == QuestStatusType::TAKEN) {
        put_str(_("あなたは現在のクエストを終了させていません！", "You have not completed your current quest yet!"), 8, 0);
        put_str(_("CTRL-Qを使えばクエストの状態がチェックできます。", "Use CTRL-Q to check the status of your quest."), 9, 0);

        get_questinfo(player_ptr, q_index, false);
        put_str(format(_("現在のクエスト「%s」", "Current quest is '%s'."), q_ptr->name), 11, 0);

        if (q_ptr->type != QuestKindType::KILL_LEVEL || q_ptr->dungeon == 0) {
            put_str(_("クエストを終わらせたら戻って来て下さい。", "Return when you have completed your quest."), 12, 0);
            return;
        }

        put_str(_("このクエストは放棄することができます。", "You can give up this quest."), 12, 0);

        if (!get_check(_("二度と受けられなくなりますが放棄しますか？", "Are you sure to give up this quest? ")))
            return;

        clear_bldg(4, 18);
        msg_print(_("放棄しました。", "You gave up."));
        msg_print(nullptr);
        record_quest_final_status(q_ptr, player_ptr->lev, QuestStatusType::FAILED);
    }

    if (q_ptr->status == QuestStatusType::FAILED) {
        print_questinfo(player_ptr, q_index, false);
        q_ptr->status = QuestStatusType::FAILED_DONE;
        reinit_wilderness = true;
        return;
    }

    if (q_ptr->status != QuestStatusType::UNTAKEN)
        return;

    q_ptr->status = QuestStatusType::TAKEN;
    reinit_wilderness = true;
    if (q_ptr->type != QuestKindType::KILL_ANY_LEVEL) {
        print_questinfo(player_ptr, q_index, true);
        return;
    }

    if (q_ptr->r_idx == 0) {
        q_ptr->r_idx = get_mon_num(player_ptr, 0, q_ptr->level + 4 + randint1(6), 0);
    }

    monster_race *r_ptr;
    r_ptr = &r_info[q_ptr->r_idx];
    while (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (r_ptr->rarity != 1)) {
        q_ptr->r_idx = get_mon_num(player_ptr, 0, q_ptr->level + 4 + randint1(6), 0);
        r_ptr = &r_info[q_ptr->r_idx];
    }

    if (q_ptr->max_num == 0) {
        if (randint1(10) > 7)
            q_ptr->max_num = 1;
        else
            q_ptr->max_num = randint1(3) + 1;
    }

    q_ptr->cur_num = 0;
    concptr name = r_ptr->name.c_str();
#ifdef JP
    msg_format("クエスト: %sを %d体倒す", name, q_ptr->max_num);
#else
    msg_format("Your quest: kill %d %s", q_ptr->max_num, name);
#endif
    print_questinfo(player_ptr, q_index, true);
}
