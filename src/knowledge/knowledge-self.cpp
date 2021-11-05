﻿/*!
 * @brief 自己に関する情報を表示する
 * @date 2020/04/24
 * @author Hourier
 */

#include "knowledge-self.h"
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
#include "system/object-type-definition.h"
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
    if (!open_temporary_file(&fff, file_name))
        return;

    std::string alg = PlayerAlignment(player_ptr).get_alignment_description();
    fprintf(fff, _("現在の属性 : %s\n\n", "Your alignment : %s\n\n"), alg.c_str());
    dump_virtues(player_ptr, fff);
    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("八つの徳", "Virtues"), 0, 0);
    fd_kill(file_name);
}

/*!
 * @brief 自分に関する情報を画面に表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_yourself(PlayerType *player_ptr, FILE *fff)
{
    if (!fff)
        return;

    char temp[80 * 10];
    shape_buffer(race_explanations[enum2i(player_ptr->prace)].data(), 78, temp, sizeof(temp));
    fprintf(fff, "\n\n");
    fprintf(fff, _("種族: %s\n", "Race: %s\n"), race_info[enum2i(player_ptr->prace)].title);
    concptr t = temp;

    for (int i = 0; i < 10; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }

    auto short_pclass = enum2i(player_ptr->pclass);
    shape_buffer(class_explanations[short_pclass].data(), 78, temp, sizeof(temp));
    fprintf(fff, "\n");
    fprintf(fff, _("職業: %s\n", "Class: %s\n"), class_info[short_pclass].title);

    t = temp;
    for (int i = 0; i < 10; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }

    shape_buffer(personality_explanations[player_ptr->ppersonality].data(), 78, temp, sizeof(temp));
    fprintf(fff, "\n");
    fprintf(fff, _("性格: %s\n", "Pesonality: %s\n"), personality_info[player_ptr->ppersonality].title);

    t = temp;
    for (int i = 0; i < A_MAX; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }

    fprintf(fff, "\n");
    if (player_ptr->realm1) {
        shape_buffer(realm_explanations[technic2magic(player_ptr->realm1) - 1].data(), 78, temp, sizeof(temp));
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[player_ptr->realm1]);

        t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;

            fprintf(fff, "%s\n", t);
            t += strlen(t) + 1;
        }
    }

    fprintf(fff, "\n");
    if (player_ptr->realm2) {
        shape_buffer(realm_explanations[technic2magic(player_ptr->realm2) - 1].data(), 78, temp, sizeof(temp));
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[player_ptr->realm2]);

        t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;

            fprintf(fff, "%s\n", t);
            t += strlen(t) + 1;
        }
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
    if (n == 0)
        return;

    size_t max_len = 75;
    std::string s = "";
    std::string l = "";
    for (int c = 0; c < PLAYER_CLASS_TYPE_MAX; c++) {
        if (w_ptr->sf_winner.has_not(i2enum<PlayerClassType>(c)))
            continue;

        auto &cl = class_info[c];
        auto t = std::string(cl.title);

        if (w_ptr->sf_retired.has_not(i2enum<PlayerClassType>(c)))
            t = "(" + t + ")";

        if (l.size() + t.size() + 2 > max_len) {
            fprintf(fff, " %s\n", str_rtrim(l).c_str());
            l = "";
        }
        if (l.size() > 0)
            l += ", ";
        l += t;
    }

    if (l.size() > 0)
        fprintf(fff, " %s\n", str_rtrim(l).c_str());
}

/*
 * List virtues & status
 *
 */
void do_cmd_knowledge_stat(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    update_playtime();
    uint32_t play_time = w_ptr->play_time;
    uint32_t all_time = w_ptr->sf_play_time + play_time;
    fprintf(fff, _("現在のプレイ時間 : %d:%02d:%02d\n", "Current Play Time is %d:%02d:%02d\n"), play_time / (60 * 60), (play_time / 60) % 60, play_time % 60);
    fprintf(fff, _("合計のプレイ時間 : %d:%02d:%02d\n", "  Total play Time is %d:%02d:%02d\n"), all_time / (60 * 60), (all_time / 60) % 60, all_time % 60);
    fputs("\n", fff);

    int percent
        = (int)(((long)player_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) / (2 * player_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (player_ptr->hitdie + 1))));

    if (player_ptr->knowledge & KNOW_HPRATE)
        fprintf(fff, _("現在の体力ランク : %d/100\n\n", "Your current Life Rating is %d/100.\n\n"), percent);
    else
        fprintf(fff, _("現在の体力ランク : ???\n\n", "Your current Life Rating is ???.\n\n"));

    fprintf(fff, _("能力の最大値\n\n", "Limits of maximum stats\n\n"));
    for (int v_nr = 0; v_nr < A_MAX; v_nr++) {
        if ((player_ptr->knowledge & KNOW_STAT) || player_ptr->stat_max[v_nr] == player_ptr->stat_max_max[v_nr])
            fprintf(fff, "%s 18/%d\n", stat_names[v_nr], player_ptr->stat_max_max[v_nr] - 18);
        else
            fprintf(fff, "%s ???\n", stat_names[v_nr]);
    }

    dump_yourself(player_ptr, fff);
    dump_winner_classes(fff);
    angband_fclose(fff);

    (void)show_file(player_ptr, true, file_name, _("自分に関する情報", "HP-rate & Max stat"), 0, 0);
    fd_kill(file_name);
}

/*
 * List my home
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_knowledge_home(PlayerType *player_ptr)
{
    parse_fixed_map(player_ptr, "w_info.txt", 0, 0, w_ptr->max_wild_y, w_ptr->max_wild_x);

    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    store_type *store_ptr;
    store_ptr = &town_info[1].store[enum2i(StoreSaleType::HOME)];

    if (store_ptr->stock_num) {
#ifdef JP
        TERM_LEN x = 1;
#endif
        fprintf(fff, _("  [ 我が家のアイテム ]\n", "  [Home Inventory]\n"));
        concptr paren = ")";
        GAME_TEXT o_name[MAX_NLEN];
        for (int i = 0; i < store_ptr->stock_num; i++) {
#ifdef JP
            if ((i % 12) == 0)
                fprintf(fff, "\n ( %d ページ )\n", x++);
            describe_flavor(player_ptr, o_name, &store_ptr->stock[i], 0);
            if (strlen(o_name) <= 80 - 3) {
                fprintf(fff, "%c%s %s\n", I2A(i % 12), paren, o_name);
            } else {
                int n;
                char *t;
                for (n = 0, t = o_name; n < 80 - 3; n++, t++)
                    if (iskanji(*t)) {
                        t++;
                        n++;
                    }
                if (n == 81 - 3)
                    n = 79 - 3; /* 最後が漢字半分 */

                fprintf(fff, "%c%s %.*s\n", I2A(i % 12), paren, n, o_name);
                fprintf(fff, "   %.77s\n", o_name + n);
            }
#else
            describe_flavor(player_ptr, o_name, &store_ptr->stock[i], 0);
            fprintf(fff, "%c%s %s\n", I2A(i % 12), paren, o_name);
#endif
        }

        fprintf(fff, "\n\n");
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("我が家のアイテム", "Home Inventory"), 0, 0);
    fd_kill(file_name);
}
