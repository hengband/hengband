/*!
 * @brief 死亡・引退・切腹時の画面表示
 * @date 2020/02/24
 * @author Hourier
 * @details
 * core、files、view-mainwindowの参照禁止。コールバックで対応すること
 */

#include "player/process-death.h"
#include "core/asking-player.h"
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
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "world/world.h"

constexpr auto GRAVE_LINE_WIDTH = 31;
constexpr auto GRAVE_LINE_START_COL = 11;
constexpr auto GRAVE_PLAYER_NAME_ROW = 6;
constexpr auto GRAVE_PLAYER_TITLE_ROW = 8;
constexpr auto GRAVE_PLAYER_CLASS_ROW = 10;
constexpr auto GRAVE_LEVEL_ROW = 11;
constexpr auto GRAVE_EXP_ROW = 12;
constexpr auto GRAVE_AU_ROW = 13;
constexpr auto GRAVE_KILLER_NAME_ROW = _(14, 15);
constexpr auto GRAVE_DEAD_PLACE_ROW = _(15, 14);
constexpr auto GRAVE_DEAD_DATETIME_ROW = 17;

/*!
 * @brief 墓石に文字列を表示する
 *
 * 墓石のアスキーアート上に与えられた文字列 str を row で指定された行に表示する
 * 表示する位置は GRAVE_LINE_START_COL から GRAVE_LIEN_WIDTH 文字分の幅で、
 * それより str の幅が小さい場合は中央寄せして表示する。
 *
 * @param str 表示する文字列
 * @param row 表示する行
 */
static void show_tomb_line(std::string_view str, int row)
{
    const auto head = GRAVE_LINE_WIDTH / 2 - str.length() / 2;
    const auto tail = GRAVE_LINE_WIDTH - str.length() - head;
    put_str(std::string(head, ' ').append(str).append(tail, ' '), row, GRAVE_LINE_START_COL);
}

/*!
 * @brief 墓に基本情報を表示
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void show_basic_params(PlayerType *player_ptr)
{
    show_tomb_line(format(_("レベル: %d", "Level: %d"), (int)player_ptr->lev), GRAVE_LEVEL_ROW);

    show_tomb_line(format(_("経験値: %ld", "Exp: %ld"), (long)player_ptr->exp), GRAVE_EXP_ROW);

    show_tomb_line(format(_("所持金: %ld", "AU: %ld"), (long)player_ptr->au), GRAVE_AU_ROW);
}

#ifdef JP
/*!
 * @brief プレイヤーを殺したモンスターを墓に表示する (日本語版専用)
 *
 * モンスターの名称を最大で2行で墓石のアスキーアート上に表示する。
 * 名称が1行に収まる場合、そのまま表示する。
 * 名称が3行以上になる場合は、2行で表示できるだけ表示し、2行目の最後を…にして以降を省略する。
 * 2行の場合は基本的に、前詰めで表示するが、モンスターの名称が ○○○『△△△』
 * のようなタイプの場合で『△△△』の途中で改行される場合○○○を1行目に、『△△△』を2行目に
 * 分割して表示することを試みる。但し『△△△』が1行に入り切らない場合はそのまま表示する。
 *
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 続いて死亡した場所を表示するためのオフセット行数
 */
static int show_killing_monster(PlayerType *player_ptr)
{
    const auto lines = shape_buffer(player_ptr->died_from, GRAVE_LINE_WIDTH + 1);
    if (lines.size() == 1) {
        show_tomb_line(lines[0], GRAVE_KILLER_NAME_ROW);
        return 0;
    }

    if (lines.size() >= 3) {
        char buf[GRAVE_LINE_WIDTH + 1];
        angband_strcpy(buf, lines[1], sizeof(buf) - 2);
        angband_strcat(buf, "…", sizeof(buf));
        show_tomb_line(lines[0], GRAVE_KILLER_NAME_ROW);
        show_tomb_line(buf, GRAVE_KILLER_NAME_ROW + 1);
        return 1;
    }

    if (const auto start_pos = lines[0].find("『");
        (start_pos != std::string::npos) && suffix(lines[1], "』")) {
        if (lines[0].length() + lines[1].length() - start_pos <= GRAVE_LINE_WIDTH) {
            const auto &name = lines[0].substr(start_pos).append(lines[1]);
            std::string_view title(lines[0].data(), start_pos);
            show_tomb_line(title, GRAVE_KILLER_NAME_ROW);
            show_tomb_line(name, GRAVE_KILLER_NAME_ROW + 1);
            return 1;
        }
    }

    show_tomb_line(lines[0], GRAVE_KILLER_NAME_ROW);
    show_tomb_line(lines[1], GRAVE_KILLER_NAME_ROW + 1);
    return 1;
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

    show_tomb_line(place, GRAVE_DEAD_PLACE_ROW + extra_line);
}

/*!
 * @brief 墓に刻む言葉を細かく表示 (日本語版専用)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void show_tomb_detail(PlayerType *player_ptr)
{
    auto offset = 0;
    if (streq(player_ptr->died_from, "途中終了")) {
        show_tomb_line("<自殺>", GRAVE_KILLER_NAME_ROW);
    } else if (streq(player_ptr->died_from, "ripe")) {
        show_tomb_line("引退後に天寿を全う", GRAVE_KILLER_NAME_ROW);
    } else if (streq(player_ptr->died_from, "Seppuku")) {
        show_tomb_line("勝利の後、切腹", GRAVE_KILLER_NAME_ROW);
    } else {
        offset = show_killing_monster(player_ptr);
    }

    show_dead_place(player_ptr, offset);
}
#else

/*!
 * @brief Detailed display of words engraved on the tomb (English version only)
 * @param player_ptr reference pointer to the player
 * @return nothing
 */
static void show_tomb_detail(PlayerType *player_ptr)
{
    show_tomb_line(format("Killed on Level %d", player_ptr->current_floor_ptr->dun_level), GRAVE_DEAD_PLACE_ROW);

    auto lines = shape_buffer(format("by %s.", player_ptr->died_from.data()).data(), GRAVE_LINE_WIDTH + 1);
    show_tomb_line(lines[0], GRAVE_KILLER_NAME_ROW);
    if (lines.size() == 1) {
        return;
    }

    if (lines.size() >= 3) {
        if (lines[1].length() > GRAVE_LINE_WIDTH - 3) {
            lines[1].erase(GRAVE_LINE_WIDTH - 3);
        }
        lines[1].append("...");
    }

    show_tomb_line(lines[1], GRAVE_KILLER_NAME_ROW + 1);
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
    read_dead_file();
    concptr p = w_ptr->total_winner ? _("偉大なる者", "Magnificent") : player_titles[enum2i(player_ptr->pclass)][(player_ptr->lev - 1) / 5].data();

    show_tomb_line(player_ptr->name, GRAVE_PLAYER_NAME_ROW);

#ifdef JP
#else
    show_tomb_line("the", GRAVE_PLAYER_TITLE_ROW - 1);
#endif

    show_tomb_line(p, GRAVE_PLAYER_TITLE_ROW);

    show_tomb_line(cp_ptr->title, GRAVE_PLAYER_CLASS_ROW);

    show_basic_params(player_ptr);
    show_tomb_detail(player_ptr);

    time_t ct = time((time_t *)0);
    show_tomb_line(format("%-.24s", ctime(&ct)), GRAVE_DEAD_DATETIME_ROW);
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
        if (!o_ptr->is_valid()) {
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
    for (size_t i = 1; i < towns_info.size(); i++) {
        auto *store_ptr = &towns_info[i].stores[StoreSaleType::HOME];
        for (auto j = 0; j < store_ptr->stock_num; j++) {
            auto *o_ptr = &store_ptr->stock[j];
            if (!o_ptr->is_valid()) {
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
    for (size_t l = 1; l < towns_info.size(); l++) {
        const auto *store_ptr = &towns_info[l].stores[StoreSaleType::HOME];
        if (store_ptr->stock_num == 0) {
            continue;
        }

        for (int i = 0, k = 0; i < store_ptr->stock_num; k++) {
            term_clear();
            for (int j = 0; (j < 12) && (i < store_ptr->stock_num); j++, i++) {
                const auto *o_ptr = &store_ptr->stock[i];
                prt(format("%c) ", I2A(j)), j + 2, 4);
                const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
                c_put_str(tval_to_attr[enum2i(o_ptr->bi_key.tval())], item_name, j + 2, 7);
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

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
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
