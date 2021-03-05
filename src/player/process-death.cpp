﻿/*!
 * @brief 死亡・引退・切腹時の画面表示
 * @date 2020/02/24
 * @author Hourier
 * @details
 * core、files、view-mainwindowの参照禁止。コールバックで対応すること
 */

#include "process-death.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-town.h"
#include "game-option/game-play-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "player/player-class.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "world/world.h"

#define GRAVE_LINE_WIDTH 31

/*!
 * @brief 墓石の真ん中に文字列を書き込む /
 * Centers a string within a GRAVE_LINE_WIDTH character string		-JWT-
 * @return なし
 * @details
 */
static void center_string(char *buf, concptr str)
{
    int i = strlen(str);
    int j = GRAVE_LINE_WIDTH / 2 - i / 2;
    (void)sprintf(buf, "%*s%s%*s", j, "", str, GRAVE_LINE_WIDTH - i - j, "");
}

/*!
 * @brief 墓に基本情報を表示
 * @param dead_ptr プレーヤーへの参照ポインタ
 * @param buf 墓テンプレ
 * @return なし
 */
static void show_basic_params(player_type *dead_ptr, char *buf)
{
    char tomb_message[160];
    (void)sprintf(tomb_message, _("レベル: %d", "Level: %d"), (int)dead_ptr->lev);
    center_string(buf, tomb_message);
    put_str(buf, 11, 11);

    (void)sprintf(tomb_message, _("経験値: %ld", "Exp: %ld"), (long)dead_ptr->exp);
    center_string(buf, tomb_message);
    put_str(buf, 12, 11);

    (void)sprintf(tomb_message, _("所持金: %ld", "AU: %ld"), (long)dead_ptr->au);
    center_string(buf, tomb_message);
    put_str(buf, 13, 11);
}

#ifdef JP
/*!
 * @brief プレーヤーを殺したモンスターを表示する (日本語版専用)
 * @param dead_ptr プレーヤーへの参照ポインタ
 * @param buf 墓テンプレ
 * @param tomb_message 墓碑に刻む言葉
 * @return 追加の行数
 */
static int show_killing_monster(player_type *dead_ptr, char *buf, char *tomb_message, size_t tomb_message_size)
{
    shape_buffer(dead_ptr->died_from, GRAVE_LINE_WIDTH + 1, tomb_message, tomb_message_size);
    char *t;
    t = tomb_message + strlen(tomb_message) + 1;
    if (!*t)
        return 0;

    char killer[MAX_MONSTER_NAME];
    strcpy(killer, t); /* 2nd line */
    if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
    {
        for (t = killer + strlen(killer) - 2; iskanji(*(t - 1)); t--) /* Loop */
            ;
        strcpy(t, "…");
    } else if (angband_strstr(tomb_message, "『") && suffix(killer, "』")) {
        char killer2[MAX_MONSTER_NAME];
        char *name_head = angband_strstr(tomb_message, "『");
        sprintf(killer2, "%s%s", name_head, killer);
        if (strlen(killer2) <= GRAVE_LINE_WIDTH) {
            strcpy(killer, killer2);
            *name_head = '\0';
        }
    } else if (angband_strstr(tomb_message, "「") && suffix(killer, "」")) {
        char killer2[MAX_MONSTER_NAME];
        char *name_head = angband_strstr(tomb_message, "「");
        sprintf(killer2, "%s%s", name_head, killer);
        if (strlen(killer2) <= GRAVE_LINE_WIDTH) {
            strcpy(killer, killer2);
            *name_head = '\0';
        }
    }

    center_string(buf, killer);
    put_str(buf, 15, 11);
    return 1;
}

/*!
 * @brief どこで死んだかを表示する (日本語版専用)
 * @param dead_ptr プレーヤーへの参照ポインタ
 * @param buf 墓テンプレ
 * @param tomb_message 表示する文字列
 * @param extra_line 追加の行数
 * @return なし
 */
static void show_dead_place(player_type *dead_ptr, char *buf, char *tomb_message, int extra_line)
{
    if (streq(dead_ptr->died_from, "ripe") || streq(dead_ptr->died_from, "Seppuku"))
        return;

    if (dead_ptr->current_floor_ptr->dun_level == 0) {
        concptr field_name = dead_ptr->town_num ? "街" : "荒野";
        if (streq(dead_ptr->died_from, "途中終了")) {
            sprintf(tomb_message, "%sで死んだ", field_name);
        } else {
            sprintf(tomb_message, "に%sで殺された", field_name);
        }
    } else {
        if (streq(dead_ptr->died_from, "途中終了")) {
            sprintf(tomb_message, "地下 %d 階で死んだ", (int)dead_ptr->current_floor_ptr->dun_level);
        } else {
            sprintf(tomb_message, "に地下 %d 階で殺された", (int)dead_ptr->current_floor_ptr->dun_level);
        }
    }

    center_string(buf, tomb_message);
    put_str(buf, 15 + extra_line, 11);
}

/*!
 * @brief 墓に刻む言葉を細かく表示 (日本語版専用)
 * @param dead_ptr プレーヤーへの参照ポインタ
 * @param buf 墓テンプレ
 * @return なし
 */
static void show_tomb_detail(player_type *dead_ptr, char *buf)
{
    char tomb_message[160];
    int extra_line = 0;
    if (streq(dead_ptr->died_from, "途中終了")) {
        strcpy(tomb_message, "<自殺>");
    } else if (streq(dead_ptr->died_from, "ripe")) {
        strcpy(tomb_message, "引退後に天寿を全う");
    } else if (streq(dead_ptr->died_from, "Seppuku")) {
        strcpy(tomb_message, "勝利の後、切腹");
    } else {
        extra_line = show_killing_monster(dead_ptr, buf, tomb_message, sizeof(tomb_message));
    }

    center_string(buf, tomb_message);
    put_str(buf, 14, 11);

    show_dead_place(dead_ptr, buf, tomb_message, extra_line);
}
#else

/*!
 * @brief Detailed display of words engraved on the tomb (English version only)
 * @param dead_ptr reference pointer to the player
 * @param buf template of the tomb
 * @return nothing
 */
static void show_tomb_detail(player_type *dead_ptr, char *buf)
{
    char tomb_message[160];
    (void)sprintf(tomb_message, "Killed on Level %d", dead_ptr->current_floor_ptr->dun_level);
    center_string(buf, tomb_message);
    put_str(buf, 14, 11);

    shape_buffer(format("by %s.", dead_ptr->died_from), GRAVE_LINE_WIDTH + 1, tomb_message, sizeof(tomb_message));
    center_string(buf, tomb_message);
    char *t;
    put_str(buf, 15, 11);
    t = tomb_message + strlen(tomb_message) + 1;
    if (!*t)
        return;

    char killer[MAX_MONSTER_NAME];
    strcpy(killer, t); /* 2nd line */
    if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
    {
        int dummy_len = strlen(killer);
        strcpy(killer + MIN(dummy_len, GRAVE_LINE_WIDTH - 3), "...");
    }

    center_string(buf, killer);
    put_str(buf, 16, 11);
}
#endif

/*!
 * @brief 墓石のアスキーアート表示 /
 * Display a "tomb-stone"
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void print_tomb(player_type *dead_ptr)
{
    term_clear();
    char buf[1024];
    read_dead_file(buf, sizeof(buf));
    concptr p = (current_world_ptr->total_winner || (dead_ptr->lev > PY_MAX_LEVEL)) ? _("偉大なる者", "Magnificent")
                                                                                    : player_title[dead_ptr->pclass][(dead_ptr->lev - 1) / 5];

    center_string(buf, dead_ptr->name);
    put_str(buf, 6, 11);

#ifdef JP
#else
    center_string(buf, "the");
    put_str(buf, 7, 11);
#endif

    center_string(buf, p);
    put_str(buf, 8, 11);

    center_string(buf, cp_ptr->title);
    put_str(buf, 10, 11);

    show_basic_params(dead_ptr, buf);
    show_tomb_detail(dead_ptr, buf);

    char current_time[80];
    time_t ct = time((time_t *)0);
    (void)sprintf(current_time, "%-.24s", ctime(&ct));
    center_string(buf, current_time);
    put_str(buf, 17, 11);
    msg_format(_("さようなら、%s!", "Goodbye, %s!"), dead_ptr->name);
}

/*!
 * @brief 死亡/引退/切腹時にインベントリ内のアイテムを*鑑定*する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void inventory_aware(player_type *creature_ptr)
{
    object_type *o_ptr;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_aware(creature_ptr, o_ptr);
        object_known(o_ptr);
    }
}

/*!
 * @brief 死亡/引退/切腹時に我が家のアイテムを*鑑定*する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void home_aware(player_type *creature_ptr)
{
    object_type *o_ptr;
    store_type *store_ptr;
    for (int i = 1; i < max_towns; i++) {
        store_ptr = &town_info[i].store[STORE_HOME];
        for (int j = 0; j < store_ptr->stock_num; j++) {
            o_ptr = &store_ptr->stock[j];
            if (!o_ptr->k_idx)
                continue;

            object_aware(creature_ptr, o_ptr);
            object_known(o_ptr);
        }
    }
}

/*!
 * @brief プレーヤーの持ち物を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return Escキーでゲームを終了する時TRUE
 */
static bool show_dead_player_items(player_type *creature_ptr)
{
    if (creature_ptr->equip_cnt) {
        term_clear();
        (void)show_equipment(creature_ptr, 0, USE_FULL, TV_NONE);
        prt(_("装備していたアイテム: -続く-", "You are using: -more-"), 0, 0);
        if (inkey() == ESCAPE)
            return TRUE;
    }

    if (creature_ptr->inven_cnt) {
        term_clear();
        (void)show_inventory(creature_ptr, 0, USE_FULL, TV_NONE);
        prt(_("持っていたアイテム: -続く-", "You are carrying: -more-"), 0, 0);

        if (inkey() == ESCAPE)
            return TRUE;
    }

    return FALSE;
}

/*!
 * @brief 我が家にあったアイテムを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void show_dead_home_items(player_type *creature_ptr)
{
    for (int l = 1; l < max_towns; l++) {
        store_type *store_ptr;
        store_ptr = &town_info[l].store[STORE_HOME];
        if (store_ptr->stock_num == 0)
            continue;

        for (int i = 0, k = 0; i < store_ptr->stock_num; k++) {
            term_clear();
            for (int j = 0; (j < 12) && (i < store_ptr->stock_num); j++, i++) {
                GAME_TEXT o_name[MAX_NLEN];
                char tmp_val[80];
                object_type *o_ptr;
                o_ptr = &store_ptr->stock[i];
                sprintf(tmp_val, "%c) ", I2A(j));
                prt(tmp_val, j + 2, 4);
                describe_flavor(creature_ptr, o_name, o_ptr, 0);
                c_put_str(tval_to_attr[o_ptr->tval], o_name, j + 2, 7);
            }

            prt(format(_("我が家に置いてあったアイテム ( %d ページ): -続く-", "Your home contains (page %d): -more-"), k + 1), 0, 0);
            if (inkey() == ESCAPE)
                return;
        }
    }
}

/*!
 * @brief キャラクタ情報をファイルに書き出す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param file_character ステータスダンプへのコールバック
 * @return なし
 */
static void export_player_info(player_type *creature_ptr, update_playtime_pf update_playtime, display_player_pf display_player)
{
    prt(_("キャラクターの記録をファイルに書き出すことができます。", "You may now dump a character record to one or more files."), 21, 0);
    prt(_("リターンキーでキャラクターを見ます。ESCで中断します。", "Then, hit RETURN to see the character, or ESC to abort."), 22, 0);
    while (TRUE) {
        char out_val[160];
        put_str(_("ファイルネーム: ", "Filename: "), 23, 0);
        strcpy(out_val, "");
        if (!askfor(out_val, 60))
            return;
        if (!out_val[0])
            break;

        screen_save();
        (void)file_character(creature_ptr, out_val, update_playtime, display_player);
        screen_load();
    }
}

/*!
 * @brief 自動的にプレイヤーステータスをファイルダンプ出力する
 * @return なし
 */
static void file_character_auto(player_type *creature_ptr, update_playtime_pf update_playtime, display_player_pf display_player)
{
    time_t now_t = time(NULL);
    struct tm *now_tm = localtime(&now_t);

    char datetime[32];
    char filename[128];

    strftime(datetime, sizeof(datetime), "%Y-%m-%d_%H%M%S", now_tm);
    strnfmt(filename, sizeof(filename), "%s_Autodump_%s.txt", p_ptr->name, datetime);

    screen_save();
    (void)file_character(creature_ptr, filename, update_playtime, display_player);
    screen_load();
}

/*!
 * @brief 死亡、引退時の簡易ステータス表示
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param handle_stuff 更新処理チェックへのコールバック
 * @param file_character ステータスダンプへのコールバック
 * @param update_playtime プレイ時間更新処理へのコールバック
 * @param display_player ステータス表示へのコールバック
 * @return なし
 */
void show_death_info(player_type *creature_ptr, update_playtime_pf update_playtime, display_player_pf display_player)
{
    inventory_aware(creature_ptr);
    home_aware(creature_ptr);

    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    flush();
    msg_erase();

    if (auto_dump)
        file_character_auto(creature_ptr, update_playtime, display_player);

    export_player_info(creature_ptr, update_playtime, display_player);
    (*update_playtime)();
    (*display_player)(creature_ptr, 0);
    prt(_("何かキーを押すとさらに情報が続きます (ESCで中断): ", "Hit any key to see more information (ESC to abort): "), 23, 0);
    if (inkey() == ESCAPE)
        return;
    if (show_dead_player_items(creature_ptr))
        return;

    show_dead_home_items(creature_ptr);
}
