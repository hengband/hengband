/*!
 * @brief 死亡・引退・切腹時の画面表示
 * @date 2020/02/24
 * @author Hourier
 * @details
 * core、files、view-mainwindowの参照禁止。コールバックで対応すること
 */

#include "player/process-death.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-town.h"
#include "game-option/game-play-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "player-info/class-info.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "world/world.h"

#define GRAVE_LINE_WIDTH 31

/*!
 * @brief 墓石の真ん中に文字列を書き込む /
 * Centers a string within a GRAVE_LINE_WIDTH character string		-JWT-
 * @details
 */
static std::string center_string(const std::string &str)
{
    int j = GRAVE_LINE_WIDTH / 2 - str.length() / 2;
    return std::string(j, ' ').append(str).append(GRAVE_LINE_WIDTH - str.length() - j, ' ');
}

/*!
 * @brief 墓に基本情報を表示
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void show_basic_params(PlayerType *player_ptr)
{
    put_str(center_string(format(_("レベル: %d", "Level: %d"), (int)player_ptr->lev)), 11, 11);

    put_str(center_string(format(_("経験値: %ld", "Exp: %ld"), (long)player_ptr->exp)), 12, 11);

    put_str(center_string(format(_("所持金: %ld", "AU: %ld"), (long)player_ptr->au)), 13, 11);
}

#ifdef JP
/*!
 * @brief プレイヤーを殺したモンスターを表示する (日本語版専用)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return メッセージと余分な行数のペア
 */
static std::pair<std::string, int> show_killing_monster(PlayerType *player_ptr)
{
    char buf[160];
    shape_buffer(player_ptr->died_from.data(), GRAVE_LINE_WIDTH + 1, buf, sizeof(buf));
    char *t = buf + strlen(buf) + 1;
    if (!*t) {
        return std::make_pair(buf, 0);
    }

    std::string killer = t; /* 2nd line */
    if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
    {
        auto i = (killer.length() > 2) ? 0 : killer.length() - 2;
        while (i > 0 && iskanji(killer[i - 1])) {
            --i;
        }
        killer.erase(i);
        killer.append("…");
    } else if (angband_strstr(buf, "『") && suffix(killer.data(), "』")) {
        char *name_head = angband_strstr(buf, "『");
        std::string killer2 = name_head;
        killer2.append(killer);
        if (killer2.length() <= GRAVE_LINE_WIDTH) {
            killer = killer2;
            *name_head = '\0';
        }
    } else if (angband_strstr(buf, "「") && suffix(killer.data(), "」")) {
        char *name_head = angband_strstr(buf, "「");
        std::string killer2 = name_head;
        killer2.append(killer);
        if (killer2.length() <= GRAVE_LINE_WIDTH) {
            killer = killer2;
            *name_head = '\0';
        }
    }

    put_str(center_string(killer), 15, 11);
    return std::make_pair(buf, 1);
}

/*!
 * @brief どこで死んだかを表示する (日本語版専用)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param extra_line 追加の行数
 */
static void show_dead_place(PlayerType *player_ptr, int extra_line)
{
    if (streq(player_ptr->died_from, "ripe") || streq(player_ptr->died_from, "Seppuku")) {
        return;
    }

    std::string place;
    if (player_ptr->current_floor_ptr->dun_level == 0) {
        concptr field_name = player_ptr->town_num ? "街" : "荒野";
        if (streq(player_ptr->died_from, "途中終了")) {
            place = format("%sで死んだ", field_name);
        } else {
            place = format("に%sで殺された", field_name);
        }
    } else if (streq(player_ptr->died_from, "途中終了")) {
        place = format("地下 %d 階で死んだ", (int)player_ptr->current_floor_ptr->dun_level);
    } else {
        place = format("に地下 %d 階で殺された", (int)player_ptr->current_floor_ptr->dun_level);
    }

    put_str(center_string(place), 15 + extra_line, 11);
}

/*!
 * @brief 墓に刻む言葉を細かく表示 (日本語版専用)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void show_tomb_detail(PlayerType *player_ptr)
{
    auto tomb_message = std::make_pair(std::string(), 0);
    if (streq(player_ptr->died_from, "途中終了")) {
        tomb_message.first = "<自殺>";
    } else if (streq(player_ptr->died_from, "ripe")) {
        tomb_message.first = "引退後に天寿を全う";
    } else if (streq(player_ptr->died_from, "Seppuku")) {
        tomb_message.first = "勝利の後、切腹";
    } else {
        tomb_message = show_killing_monster(player_ptr);
    }

    put_str(center_string(tomb_message.first), 14, 11);

    show_dead_place(player_ptr, tomb_message.second);
}
#else

/*!
 * @brief Detailed display of words engraved on the tomb (English version only)
 * @param player_ptr reference pointer to the player
 * @return nothing
 */
static void show_tomb_detail(PlayerType *player_ptr)
{
    put_str(center_string(format("Killed on Level %d", player_ptr->current_floor_ptr->dun_level)), 14, 11);

    char buf[160];
    shape_buffer(format("by %s.", player_ptr->died_from.data()).data(), GRAVE_LINE_WIDTH + 1, buf, sizeof(buf));
    put_str(center_string(buf), 15, 11);
    char *t = buf + strlen(buf) + 1;
    if (!*t) {
        return;
    }

    std::string killer = t; /* 2nd line */
    if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
    {
        if (killer.length() > GRAVE_LINE_WIDTH - 3) {
            killer.erase(GRAVE_LINE_WIDTH - 3);
        }
        killer.append("...");
    }

    put_str(center_string(killer), 16, 11);
}
#endif

/*!
 * @brief 墓石のアスキーアート表示 /
 * Display a "tomb-stone"
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_tomb(PlayerType *player_ptr)
{
    term_clear();
    char buf[1024];
    read_dead_file(buf, sizeof(buf));
    concptr p = w_ptr->total_winner ? _("偉大なる者", "Magnificent") : player_titles[enum2i(player_ptr->pclass)][(player_ptr->lev - 1) / 5].data();

    put_str(center_string(player_ptr->name), 6, 11);

#ifdef JP
#else
    put_str(center_string("the"), 7, 11);
#endif

    put_str(center_string(p), 8, 11);

    put_str(center_string(cp_ptr->title), 10, 11);

    show_basic_params(player_ptr);
    show_tomb_detail(player_ptr);

    time_t ct = time((time_t *)0);
    put_str(center_string(format("%-.24s", ctime(&ct))), 17, 11);
    msg_format(_("さようなら、%s!", "Goodbye, %s!"), player_ptr->name);
}

/*!
 * @brief 死亡/引退/切腹時にインベントリ内のアイテムを*鑑定*する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void inventory_aware(PlayerType *player_ptr)
{
    ItemEntity *o_ptr;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->bi_id) {
            continue;
        }

        object_aware(player_ptr, o_ptr);
        object_known(o_ptr);
    }
}

/*!
 * @brief 死亡/引退/切腹時に我が家のアイテムを*鑑定*する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void home_aware(PlayerType *player_ptr)
{
    ItemEntity *o_ptr;
    store_type *store_ptr;
    for (int i = 1; i < max_towns; i++) {
        store_ptr = &town_info[i].store[enum2i(StoreSaleType::HOME)];
        for (int j = 0; j < store_ptr->stock_num; j++) {
            o_ptr = &store_ptr->stock[j];
            if (!o_ptr->bi_id) {
                continue;
            }

            object_aware(player_ptr, o_ptr);
            object_known(o_ptr);
        }
    }
}

/*!
 * @brief プレイヤーの持ち物を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return Escキーでゲームを終了する時TRUE
 */
static bool show_dead_player_items(PlayerType *player_ptr)
{
    if (player_ptr->equip_cnt) {
        term_clear();
        (void)show_equipment(player_ptr, 0, USE_FULL, AllMatchItemTester());
        prt(_("装備していたアイテム: -続く-", "You are using: -more-"), 0, 0);
        if (inkey() == ESCAPE) {
            return true;
        }
    }

    if (player_ptr->inven_cnt) {
        term_clear();
        (void)show_inventory(player_ptr, 0, USE_FULL, AllMatchItemTester());
        prt(_("持っていたアイテム: -続く-", "You are carrying: -more-"), 0, 0);

        if (inkey() == ESCAPE) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief 我が家にあったアイテムを表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void show_dead_home_items(PlayerType *player_ptr)
{
    for (int l = 1; l < max_towns; l++) {
        store_type *store_ptr;
        store_ptr = &town_info[l].store[enum2i(StoreSaleType::HOME)];
        if (store_ptr->stock_num == 0) {
            continue;
        }

        for (int i = 0, k = 0; i < store_ptr->stock_num; k++) {
            term_clear();
            for (int j = 0; (j < 12) && (i < store_ptr->stock_num); j++, i++) {
                GAME_TEXT o_name[MAX_NLEN];
                ItemEntity *o_ptr;
                o_ptr = &store_ptr->stock[i];
                prt(format("%c) ", I2A(j)), j + 2, 4);
                describe_flavor(player_ptr, o_name, o_ptr, 0);
                c_put_str(tval_to_attr[enum2i(o_ptr->bi_key.tval())], o_name, j + 2, 7);
            }

            prt(format(_("我が家に置いてあったアイテム ( %d ページ): -続く-", "Your home contains (page %d): -more-"), k + 1), 0, 0);
            if (inkey() == ESCAPE) {
                return;
            }
        }
    }
}

/*!
 * @brief キャラクタ情報をファイルに書き出す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param file_character ステータスダンプへのコールバック
 */
static void export_player_info(PlayerType *player_ptr)
{
    prt(_("キャラクターの記録をファイルに書き出すことができます。", "You may now dump a character record to one or more files."), 21, 0);
    prt(_("リターンキーでキャラクターを見ます。ESCで中断します。", "Then, hit RETURN to see the character, or ESC to abort."), 22, 0);
    while (true) {
        char out_val[160] = "";
        put_str(_("ファイルネーム: ", "Filename: "), 23, 0);
        if (!askfor(out_val, 60)) {
            return;
        }
        if (!out_val[0]) {
            break;
        }

        screen_save();
        (void)file_character(player_ptr, out_val);
        screen_load();
    }
}

/*!
 * @brief 自動的にプレイヤーステータスをファイルダンプ出力する
 */
static void file_character_auto(PlayerType *player_ptr)
{
    time_t now_t = time(nullptr);
    struct tm *now_tm = localtime(&now_t);

    char datetime[32];
    char filename[128];

    strftime(datetime, sizeof(datetime), "%Y-%m-%d_%H%M%S", now_tm);
    strnfmt(filename, sizeof(filename), "%s_Autodump_%s.txt", p_ptr->name, datetime);

    screen_save();
    (void)file_character(player_ptr, filename);
    screen_load();
}

/*!
 * @brief 死亡、引退時の簡易ステータス表示
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param display_player ステータス表示へのコールバック
 */
void show_death_info(PlayerType *player_ptr)
{
    inventory_aware(player_ptr);
    home_aware(player_ptr);

    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
    flush();
    msg_erase();

    if (auto_dump) {
        file_character_auto(player_ptr);
    }

    export_player_info(player_ptr);
    (void)display_player(player_ptr, 0);
    prt(_("何かキーを押すとさらに情報が続きます (ESCで中断): ", "Hit any key to see more information (ESC to abort): "), 23, 0);
    if (inkey() == ESCAPE) {
        return;
    }
    if (show_dead_player_items(player_ptr)) {
        return;
    }

    show_dead_home_items(player_ptr);
}
