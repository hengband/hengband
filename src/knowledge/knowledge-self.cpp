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
#include "floor/floor-town.h"
#include "info-reader/fixed-map-parser.h"
#include "io-dump/dump-util.h"
#include "player-info/alignment.h"
#include "player-info/class-info.h"
#include "player/player-personality.h"
#include "player/player-status-table.h"
#include "player/race-info-table.h"
#include "realm/realm-names-table.h"
#include "store/store-util.h"
#include "system/item-entity.h"
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
    fprintf(fff, _("現在の属性 : %s\n\n", "Your alignment : %s\n\n"), alg.data());
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

    fprintf(fff, "\n\n");
    fprintf(fff, _("種族: %s\n", "Race: %s\n"), race_info[enum2i(player_ptr->prace)].title);
    dump_explanation(race_explanations[enum2i(player_ptr->prace)], fff);

    auto short_pclass = enum2i(player_ptr->pclass);
    fprintf(fff, "\n");
    fprintf(fff, _("職業: %s\n", "Class: %s\n"), class_info.at(player_ptr->pclass).title.data());
    dump_explanation(class_explanations[short_pclass].data(), fff);

    fprintf(fff, "\n");
    fprintf(fff, _("性格: %s\n", "Pesonality: %s\n"), personality_info[player_ptr->ppersonality].title);
    dump_explanation(personality_explanations[player_ptr->ppersonality], fff);

    fprintf(fff, "\n");
    if (player_ptr->realm1) {
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[player_ptr->realm1]);
        dump_explanation(realm_explanations[technic2magic(player_ptr->realm1) - 1], fff);
    }

    fprintf(fff, "\n");
    if (player_ptr->realm2) {
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[player_ptr->realm2]);
        dump_explanation(realm_explanations[technic2magic(player_ptr->realm2) - 1], fff);
    }
}

/*!
 * @brief 勝利済みの職業をダンプする
 * @param fff ファイルストリームのポインタ
 */
static void dump_winner_classes(FILE *fff)
{
    int n = w_ptr->sf_winner.count();
    concptr ss = n > 1 ? _("", "s") : "";
    fprintf(fff, _("*勝利*済みの職業%s : %d\n", "Class of *Winner%s* : %d\n"), ss, n);
    if (n == 0) {
        return;
    }

    size_t max_len = 75;
    std::string s = "";
    std::string l = "";
    for (int c = 0; c < PLAYER_CLASS_TYPE_MAX; c++) {
        if (w_ptr->sf_winner.has_not(i2enum<PlayerClassType>(c))) {
            continue;
        }

        auto &cl = class_info.at(i2enum<PlayerClassType>(c));
        std::string t = cl.title;

        if (w_ptr->sf_retired.has_not(i2enum<PlayerClassType>(c))) {
            t = "(" + t + ")";
        }

        if (l.size() + t.size() + 2 > max_len) {
            fprintf(fff, " %s\n", str_rtrim(l).data());
            l = "";
        }
        if (l.size() > 0) {
            l += ", ";
        }
        l += t;
    }

    if (l.size() > 0) {
        fprintf(fff, " %s\n", str_rtrim(l).data());
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

    w_ptr->update_playtime();
    uint32_t play_time = w_ptr->play_time;
    uint32_t all_time = w_ptr->sf_play_time + play_time;
    fprintf(fff, _("現在のプレイ時間 : %d:%02d:%02d\n", "Current Play Time is %d:%02d:%02d\n"), play_time / (60 * 60), (play_time / 60) % 60, play_time % 60);
    fprintf(fff, _("合計のプレイ時間 : %d:%02d:%02d\n", "  Total play Time is %d:%02d:%02d\n"), all_time / (60 * 60), (all_time / 60) % 60, all_time % 60);
    fputs("\n", fff);

    int percent = (int)(((long)player_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) / (2 * player_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (player_ptr->hitdie + 1))));

    if (player_ptr->knowledge & KNOW_HPRATE) {
        fprintf(fff, _("現在の体力ランク : %d/100\n\n", "Your current Life Rating is %d/100.\n\n"), percent);
    } else {
        fprintf(fff, _("現在の体力ランク : ???\n\n", "Your current Life Rating is ???.\n\n"));
    }

    fprintf(fff, _("能力の最大値\n\n", "Limits of maximum stats\n\n"));
    for (int v_nr = 0; v_nr < A_MAX; v_nr++) {
        if ((player_ptr->knowledge & KNOW_STAT) || player_ptr->stat_max[v_nr] == player_ptr->stat_max_max[v_nr]) {
            fprintf(fff, "%s 18/%d\n", stat_names[v_nr], player_ptr->stat_max_max[v_nr] - 18);
        } else {
            fprintf(fff, "%s ???\n", stat_names[v_nr]);
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
    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, w_ptr->max_wild_y, w_ptr->max_wild_x);

    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    constexpr auto home_inventory = _("我が家のアイテム", "Home Inventory");
    const auto &store = towns_info[1].stores[StoreSaleType::HOME];
    if (store.stock_num == 0) {
        angband_fclose(fff);
        FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, home_inventory);
        fd_kill(file_name);
        return;
    }

#ifdef JP
    TERM_LEN x = 1;
#endif
    fprintf(fff, _("  [ 我が家のアイテム ]\n", "  [Home Inventory]\n"));
    constexpr auto close_bracket = ")";
    for (auto i = 0; i < store.stock_num; i++) {
#ifdef JP
        if ((i % 12) == 0) {
            fprintf(fff, "\n ( %d ページ )\n", x++);
        }

        const auto item_name = describe_flavor(player_ptr, &store.stock[i], 0);
        const int item_length = item_name.length();
        if (item_length <= 80 - 3) {
            fprintf(fff, "%c%s %s\n", I2A(i % 12), close_bracket, item_name.data());
            continue;
        }

        /* 最後が漢字半分 */
        constexpr auto max_length = 81 - 3;
        const auto n = item_length >= max_length ? 79 - 3 : item_length;
        fprintf(fff, "%c%s %.*s\n", I2A(i % 12), close_bracket, n, item_name.substr(0, n).data());
        fprintf(fff, "   %.77s\n", item_name.substr(n).data());
#else
        const auto item_name = describe_flavor(player_ptr, &store.stock[i], 0);
        fprintf(fff, "%c%s %s\n", I2A(i % 12), close_bracket, item_name.data());
#endif
    }

    fprintf(fff, "\n\n");
    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, home_inventory);
    fd_kill(file_name);
}
