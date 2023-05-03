#include "store/cmd-store.h"
#include "cmd-io/macro-util.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-town.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "object/object-info.h"
#include "player-status/player-energy.h"
#include "store/home.h"
#include "store/store-key-processor.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "world/world.h"

#define MIN_STOCK 12

/*!
 * @brief 店舗処理全体のメインルーチン /
 * Enter a store, and interact with it. *
 * @param player_ptr プレイヤーへの参照ポインタ
 * @note
 * <pre>
 * Note that we use the standard "request_command()" function
 * to get a command, allowing us to use "command_arg" and all
 * command macros and other nifty stuff, but we use the special
 * "shopping" argument, to force certain commands to be converted
 * into other commands, normally, we convert "p" (pray) and "m"
 * (cast magic) into "g" (get), and "s" (search) into "d" (drop).
 * </pre>
 */
void do_cmd_store(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode) {
        return;
    }
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, std::nullopt);
    TERM_LEN w, h;
    term_get_size(&w, &h);

    xtra_stock = std::min(14 + 26, ((h > MAIN_TERM_MIN_ROWS) ? (h - MAIN_TERM_MIN_ROWS) : 0));
    store_bottom = MIN_STOCK + xtra_stock;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto *g_ptr = &floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    if (!g_ptr->cave_has_flag(TerrainCharacteristics::STORE)) {
        msg_print(_("ここには店がありません。", "You see no store here."));
        return;
    }

    // TODO:
    //   施設の種類により、一時的に現在地 (player_ptr->town_num) を違う値に偽装して処理している。
    //   我が家および博物館は全ての町で内容を共有するため、現在地を辺境の地 (1) にしている。
    //   ダンジョン内の店の場合、現在地を NO_TOWN にしている。
    //   inner_town_num は、施設内で C コマンドなどを使ったときにそのままでは現在地の偽装がバレる
    //   ため、それを糊塗するためのグローバル変数。
    //   この辺はリファクタしたい。
    StoreSaleType store_num = i2enum<StoreSaleType>(terrains_info[g_ptr->feat].subtype);
    old_town_num = player_ptr->town_num;
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        player_ptr->town_num = 1;
    }

    if (floor_ptr->is_in_dungeon()) {
        player_ptr->town_num = VALID_TOWNS;
    }

    inner_town_num = player_ptr->town_num;
    auto &town = towns_info[player_ptr->town_num];
    auto &store = town.store[enum2i(store_num)];
    if ((store.store_open >= w_ptr->game_turn) || ironman_shops) {
        msg_print(_("ドアに鍵がかかっている。", "The doors are locked."));
        player_ptr->town_num = old_town_num;
        return;
    }

    int maintain_num = (w_ptr->game_turn - store.last_visit) / (TURNS_PER_TICK * STORE_TICKS);
    if (maintain_num > 10) {
        maintain_num = 10;
    }
    if (maintain_num) {
        store_maintenance(player_ptr, player_ptr->town_num, store_num, maintain_num);

        store.last_visit = w_ptr->game_turn;
    }

    forget_lite(floor_ptr);
    forget_view(floor_ptr);
    w_ptr->character_icky_depth = 1;
    command_arg = 0;
    command_rep = 0;
    command_new = 0;
    get_com_no_macros = true;
    cur_store_feat = g_ptr->feat;
    st_ptr = &towns_info[player_ptr->town_num].store[enum2i(store_num)];
    ot_ptr = &owners.at(store_num)[st_ptr->owner];
    store_top = 0;
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);
    display_store(player_ptr, store_num);
    leave_store = false;

    while (!leave_store) {
        prt("", 1, 0);
        clear_from(20 + xtra_stock);
        prt(_(" ESC) 建物から出る", " ESC) Exit from Building."), 21 + xtra_stock, 0);
        if (st_ptr->stock_num > store_bottom) {
            prt(_(" -)前ページ", " -) Previous page"), 22 + xtra_stock, 0);
            prt(_(" スペース) 次ページ", " SPACE) Next page"), 23 + xtra_stock, 0);
        }

        if (store_num == StoreSaleType::HOME) {
            prt(_("g) アイテムを取る", "g) Get an item."), 21 + xtra_stock, 27);
            prt(_("d) アイテムを置く", "d) Drop an item."), 22 + xtra_stock, 27);
            prt(_("x) 家のアイテムを調べる", "x) eXamine an item in the home."), 23 + xtra_stock, 27);
        } else if (store_num == StoreSaleType::MUSEUM) {
            prt(_("d) アイテムを置く", "d) Drop an item."), 21 + xtra_stock, 27);
            prt(_("r) アイテムの展示をやめる", "r) order to Remove an item."), 22 + xtra_stock, 27);
            prt(_("x) 博物館のアイテムを調べる", "x) eXamine an item in the museum."), 23 + xtra_stock, 27);
        } else {
            prt(_("p) 商品を買う", "p) Purchase an item."), 21 + xtra_stock, 30);
            prt(_("s) アイテムを売る", "s) Sell an item."), 22 + xtra_stock, 30);
            prt(_("x) 商品を調べる", "x) eXamine an item in the shop"), 23 + xtra_stock, 30);
        }

        prt(_("i/e) 持ち物/装備の一覧", "i/e) Inventry/Equipment list"), 21 + xtra_stock, 56);
        if (rogue_like_commands) {
            prt(_("w/T) 装備する/はずす", "w/T) Wear/Take off equipment"), 22 + xtra_stock, 56);
        } else {
            prt(_("w/t) 装備する/はずす", "w/t) Wear/Take off equipment"), 22 + xtra_stock, 56);
        }

        prt(_("コマンド:", "You may: "), 20 + xtra_stock, 0);
        InputKeyRequestor(player_ptr, true).request_command();
        store_process_command(player_ptr, store_num);

        bool need_redraw_store_inv = any_bits(player_ptr->update, PU_BONUS);
        w_ptr->character_icky_depth = 1;
        handle_stuff(player_ptr);
        if (player_ptr->inventory_list[INVEN_PACK].bi_id) {
            INVENTORY_IDX item = INVEN_PACK;
            auto *o_ptr = &player_ptr->inventory_list[item];
            if (store_num != StoreSaleType::HOME) {
                if (store_num == StoreSaleType::MUSEUM) {
                    msg_print(_("ザックからアイテムがあふれそうなので、あわてて博物館から出た...", "Your pack is so full that you flee the Museum..."));
                } else {
                    msg_print(_("ザックからアイテムがあふれそうなので、あわてて店から出た...", "Your pack is so full that you flee the store..."));
                }

                leave_store = true;
            } else if (!store_check_num(o_ptr, store_num)) {
                msg_print(_("ザックからアイテムがあふれそうなので、あわてて家から出た...", "Your pack is so full that you flee your home..."));
                leave_store = true;
            } else {
                int item_pos;
                ItemEntity forge;
                ItemEntity *q_ptr;
                msg_print(_("ザックからアイテムがあふれてしまった！", "Your pack overflows!"));
                q_ptr = &forge;
                q_ptr->copy_from(o_ptr);
                const auto item_name = describe_flavor(player_ptr, q_ptr, 0);
                msg_format(_("%sが落ちた。(%c)", "You drop %s (%c)."), item_name.data(), index_to_label(item));
                vary_item(player_ptr, item, -255);
                handle_stuff(player_ptr);

                item_pos = home_carry(player_ptr, q_ptr, store_num);
                if (item_pos >= 0) {
                    store_top = (item_pos / store_bottom) * store_bottom;
                    display_store_inventory(player_ptr, store_num);
                }
            }
        }

        if (need_redraw_store_inv) {
            display_store_inventory(player_ptr, store_num);
        }

        if (st_ptr->store_open >= w_ptr->game_turn) {
            leave_store = true;
        }
    }

    // 現在地の偽装を解除。
    player_ptr->town_num = old_town_num;

    select_floor_music(player_ptr);
    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    w_ptr->character_icky_depth = 0;
    command_new = 0;
    command_see = false;
    get_com_no_macros = false;

    msg_erase();
    term_clear();

    player_ptr->update |= PU_VIEW | PU_LITE | PU_MONSTER_LITE;
    player_ptr->update |= PU_MONSTER_STATUSES;
    player_ptr->redraw |= PR_BASIC | PR_EXTRA | PR_EQUIPPY;
    player_ptr->redraw |= PR_MAP;
    player_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
}
