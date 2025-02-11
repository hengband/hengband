#include "market/building-quest.h"
#include "core/asking-player.h"
#include "dungeon/quest.h"
#include "info-reader/fixed-map-parser.h"
#include "market/building-util.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"

/*!
 * @brief クエスト情報を処理しつつ取得する。/ Process and get quest information
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param quest_id クエストのID
 * @param do_init クエストの開始処理か(true)、結果処理か(FALSE)
 */
static void get_questinfo(PlayerType *player_ptr, QuestId quest_id, bool do_init)
{
    quest_text_lines.clear();

    auto &floor = *player_ptr->current_floor_ptr;
    const auto old_quest = floor.quest_number;
    floor.quest_number = quest_id;

    init_flags = INIT_SHOW_TEXT;
    if (do_init) {
        init_flags = i2enum<init_flags_type>(init_flags | INIT_ASSIGN);
    }

    parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
    floor.quest_number = old_quest;
}

/*!
 * @brief クエスト情報を処理しつつ表示する。/ Process and display quest information
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param quest_id クエストのID
 * @param do_init クエストの開始処理か(true)、結果処理か(FALSE)
 */
static void print_questinfo(PlayerType *player_ptr, QuestId quest_id, bool do_init)
{
    get_questinfo(player_ptr, quest_id, do_init);

    const auto &quests = QuestList::get_instance();
    const auto &quest = quests.get_quest(quest_id);
    prt(format(_("クエスト情報 (危険度: %d 階相当)", "Quest Information (Danger level: %d)"), quest.level), 5, 0);
    prt(quest.name, 7, 0);

    for (auto i = 0; i < std::ssize(quest_text_lines); i++) {
        c_put_str(TERM_YELLOW, quest_text_lines[i], i + 8, 0);
    }
}

/*!
 * @brief クエスト処理のメインルーチン / Request a quest from the Lord.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void castle_quest(PlayerType *player_ptr)
{
    clear_bldg(4, 18);
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto quest_id = i2enum<QuestId>(floor.get_grid(player_ptr->get_position()).special);
    if (!inside_quest(quest_id)) {
        put_str(_("今のところクエストはありません。", "I don't have a quest for you at the moment."), 8, 0);
        return;
    }

    auto &quests = QuestList::get_instance();
    auto &quest = quests.get_quest(quest_id);
    auto &wilderness = WildernessGrids::get_instance();
    if (quest.status == QuestStatusType::COMPLETED) {
        quest.status = QuestStatusType::REWARDED;
        print_questinfo(player_ptr, quest_id, false);
        wilderness.set_reinitialization(true);
        return;
    }

    if (quest.status == QuestStatusType::TAKEN) {
        put_str(_("あなたは現在のクエストを終了させていません！", "You have not completed your current quest yet!"), 8, 0);
        put_str(_("CTRL-Qを使えばクエストの状態がチェックできます。", "Use CTRL-Q to check the status of your quest."), 9, 0);

        get_questinfo(player_ptr, quest_id, false);
        put_str(format(_("現在のクエスト「%s」", "Current quest is '%s'."), quest.name.data()), 11, 0);

        if (quest.type != QuestKindType::KILL_LEVEL || quest.dungeon == DungeonId::WILDERNESS) {
            put_str(_("クエストを終わらせたら戻って来て下さい。", "Return when you have completed your quest."), 12, 0);
            return;
        }

        put_str(_("このクエストは放棄することができます。", "You can give up this quest."), 12, 0);
        if (!input_check(_("二度と受けられなくなりますが放棄しますか？", "Are you sure to give up this quest? "))) {
            return;
        }

        clear_bldg(4, 18);
        msg_print(_("放棄しました。", "You gave up."));
        msg_print(nullptr);
        record_quest_final_status(&quest, player_ptr->lev, QuestStatusType::FAILED);
    }

    if (quest.status == QuestStatusType::FAILED) {
        print_questinfo(player_ptr, quest_id, false);
        quest.status = QuestStatusType::FAILED_DONE;
        wilderness.set_reinitialization(true);
        return;
    }

    if (quest.status != QuestStatusType::UNTAKEN) {
        return;
    }

    quest.status = QuestStatusType::TAKEN;
    wilderness.set_reinitialization(true);
    print_questinfo(player_ptr, quest_id, true);
}
