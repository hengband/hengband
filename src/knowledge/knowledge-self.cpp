/*!
 * @brief 自己に関する情報を表示する
 * @date 2020/04/24
 * @author Hourier
 */

#include "knowledge/knowledge-self.h"
#include "avatar/avatar.h"
#include "birth/birth-explanations-table.h"
#include "core/show-file.h"
#include "flavor/flavor-describer.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/wilderness-reader.h"
#include "io-dump/dump-util.h"
#include "player-info/alignment.h"
#include "player-info/class-info.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-status-table.h"
#include "player/race-info-table.h"
#include "store/store-util.h"
#include "system/floor/town-list.h"
#include "system/inner-game-data.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/buffer-shaper.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <string>

/*
 * List virtues & status
 */
void do_cmd_knowledge_virtues(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    std::string alg = PlayerAlignment(player_ptr).get_alignment_description();
    fmt::println(fff, _("現在の属性 : {}\n", "Your alignment : {}\n"), alg);
    dump_virtues(player_ptr, fff);
    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("八つの徳", "Virtues"));
    fd_kill(file_name);
}

static void dump_explanation(std::string_view explanation, FILE *fff)
{
    for (const auto &line : shape_buffer(explanation, 78)) {
        fputs(line.data(), fff);
        fputc('\n', fff);
    }
}

/*!
 * @brief 自分に関する情報を画面に表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_yourself(PlayerType *player_ptr, FILE *fff)
{
    if (!fff) {
        return;
    }

    fmt::println(fff, "\n");
    fmt::println(fff, _("種族: {}", "Race: {}"), race_info[enum2i(player_ptr->prace)].title);
    dump_explanation(race_explanations[enum2i(player_ptr->prace)], fff);

    fmt::print(fff, "\n");
    fmt::println(fff, _("職業: {}", "Class: {}"), class_info.at(player_ptr->pclass).title);
    auto short_pclass = enum2i(player_ptr->pclass);
    dump_explanation(class_explanations[short_pclass], fff);

    fmt::print(fff, "\n");
    fmt::println(fff, _("性格: {}", "Personality: {}"), personality_info[player_ptr->ppersonality].title);
    dump_explanation(personality_explanations[player_ptr->ppersonality], fff);

    fmt::print(fff, "\n");
    PlayerRealm pr(player_ptr);
    if (pr.realm1().is_available()) {
        fmt::println(fff, _("魔法: {}", "Realm: {}"), pr.realm1().get_name());
        dump_explanation(pr.realm1().get_explanation(), fff);
    }

    fmt::print(fff, "\n");
    if (pr.realm2().is_available()) {
        fmt::println(fff, _("魔法: {}", "Realm: {}"), pr.realm2().get_name());
        dump_explanation(pr.realm2().get_explanation(), fff);
    }
}

/*!
 * @brief 勝利済みの職業をダンプする
 * @param fff ファイルストリームのポインタ
 */
static void dump_winner_classes(FILE *fff)
{
    const auto &igd = InnerGameData::get_instance();
    const int n = igd.get_won_classes_count();
#ifdef JP
    fmt::println(fff, "*勝利*済みの職業 : {}", n);
#else
    fmt::println(fff, "Class{} of *Winner* : {}", (n == 1) ? "" : "es", n);
#endif
    if (n == 0) {
        return;
    }

    size_t max_len = 75;
    std::string s = "";
    std::string l = "";
    for (int c = 0; c < PLAYER_CLASS_TYPE_MAX; c++) {
        const auto pclass_enum = i2enum<PlayerClassType>(c);
        if (igd.has_not_won_class(pclass_enum)) {
            continue;
        }

        auto &player_class = class_info.at(i2enum<PlayerClassType>(c));
        std::string t = player_class.title.string();
        if (igd.has_not_retired_class(pclass_enum)) {
            t = "(" + t + ")";
        }

        if (l.size() + t.size() + 2 > max_len) {
            fmt::println(fff, " {}", str_rtrim(l));
            l = "";
        }
        if (l.size() > 0) {
            l += ", ";
        }
        l += t;
    }

    if (l.size() > 0) {
        fmt::println(fff, " {}", str_rtrim(l));
    }
}

/*
 * List virtues & status
 *
 */
void do_cmd_knowledge_stat(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    auto &world = AngbandWorld::get_instance();
    world.play_time.update();
    const auto play_time = world.play_time.elapsed_sec();
    const auto all_time = InnerGameData::get_instance().get_total_play_time() + play_time;
    fmt::println(fff, _("現在のプレイ時間 : {}:{:02}:{:02}", "Current Play Time is {}:{:02}:{:02}"), play_time / (60 * 60), (play_time / 60) % 60, play_time % 60);
    fmt::println(fff, _("合計のプレイ時間 : {}:{:02}:{:02}", "  Total play Time is {}:{:02}:{:02}"), all_time / (60 * 60), (all_time / 60) % 60, all_time % 60);
    fputs("\n", fff);

    if (player_ptr->knowledge & KNOW_HPRATE) {
        fmt::println(fff, _("現在の体力ランク : {}/100\n", "Your current Life Rating is {}/100.\n"), player_ptr->calc_life_rating());
    } else {
        fmt::println(fff, _("現在の体力ランク : ???\n", "Your current Life Rating is ???.\n"));
    }

    fmt::println(fff, _("能力の最大値\n", "Limits of maximum stats\n"));
    for (int v_nr = 0; v_nr < A_MAX; v_nr++) {
        if ((player_ptr->knowledge & KNOW_STAT) || player_ptr->stat_max[v_nr] == player_ptr->stat_max_max[v_nr]) {
            fmt::println(fff, "{} 18/{}", stat_names[v_nr], player_ptr->stat_max_max[v_nr] - 18);
        } else {
            fmt::println(fff, "{} ???", stat_names[v_nr]);
        }
    }

    dump_yourself(player_ptr, fff);
    dump_winner_classes(fff);
    angband_fclose(fff);

    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("自分に関する情報", "HP-rate & Max stat"));
    fd_kill(file_name);
}

/*
 * List my home
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_knowledge_home(PlayerType *player_ptr)
{
    (void)apply_wilderness_definition();

    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    constexpr auto home_inventory = _("我が家のアイテム", "Home Inventory");
    const auto &store = TownList::get_instance().get_home();
    if (store.stock_num == 0) {
        angband_fclose(fff);
        FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, home_inventory);
        fd_kill(file_name);
        return;
    }

#ifdef JP
    TERM_LEN x = 1;
#endif
    fmt::println(fff, _("  [ 我が家のアイテム ]", "  [Home Inventory]"));
    constexpr auto close_bracket = ")";
    for (auto i = 0; i < store.stock_num; i++) {
        const auto item_name = describe_flavor(player_ptr, *store.stock[i], 0);
#ifdef JP
        if ((i % 12) == 0) {
            fmt::println(fff, "\n ( {} ページ )", x++);
        }

        // 1行に収まらない名前は、全角文字の途中で切らないように複数行に分ける。
        // 1行目の先頭には "a) " が付くので、2行目以降も同じ幅だけ字下げする
        constexpr auto max_line_length = 80 - 3;
        const auto lines = str_separate(item_name, max_line_length);
        fmt::println(fff, "{:c}{} {}", I2A(i % 12), close_bracket, lines.front());
        for (auto j = 1U; j < lines.size(); ++j) {
            fmt::println(fff, "   {}", lines[j]);
        }
#else
        fmt::println(fff, "{:c}{} {}", I2A(i % 12), close_bracket, item_name);
#endif
    }

    fmt::println(fff, "\n");
    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, home_inventory);
    fd_kill(file_name);
}
