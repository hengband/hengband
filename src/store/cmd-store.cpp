#include "store/cmd-store.h"
#include "bot/bot-json-output.h"
#include "cmd-io/macro-util.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
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
#include "store/store-screen.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-list.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "world/world.h"

/*!
 * @brief 入った店舗がある町のIDを決める
 * @param floor 現在のフロア
 * @param store_num 店舗の種類
 * @return 町のID
 * @details 我が家と博物館は全ての町で内容を共有するため、辺境の地の店舗を使う。
 * ダンジョン内の店舗は、ダンジョン用の町 (VALID_TOWNS) の店舗を使う。
 */
static size_t decide_store_town_index(const FloorType &floor, StoreSaleType store_num)
{
    if (floor.is_underground()) {
        return VALID_TOWNS;
    }

    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        return 1;
    }

    return AngbandWorld::get_instance().get_town_index();
}

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
    auto &world = AngbandWorld::get_instance();
    if (world.is_wild_mode()) {
        return;
    }
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, tl::nullopt);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(player_ptr->get_position());
    if (!grid.has(TerrainCharacteristics::STORE)) {
        msg_print(_("ここには店がありません。", "You see no store here."));
        return;
    }

    const auto store_num = grid.get_terrain().store_sale_type;
    const auto town_index = decide_store_town_index(floor, store_num);
    auto &store = TownList::get_instance().get_town(town_index).get_store(store_num);
    if ((store.store_open >= world.game_turn) || ironman_shops) {
        msg_print(_("ドアに鍵がかかっている。", "The doors are locked."));
        return;
    }

    auto maintain_num = (world.game_turn - store.last_visit) / (TURNS_PER_TICK * STORE_TICKS);
    if (maintain_num > 10) {
        maintain_num = 10;
    }

    if (maintain_num > 0) {
        store_maintenance(player_ptr, town_index, store, maintain_num);
        store.last_visit = world.game_turn;
    }

    floor.forget_lite();
    floor.forget_view();
    world.character_icky_depth = 1;
    command_arg = 0;
    command_rep = 0;
    command_new = 0;
    get_com_no_macros = true;
    StoreScreen screen(store, town_index, grid.feat, term_get_size().second);
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);
    display_store(player_ptr, screen);
    auto should_leave = false;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    while (!should_leave) {
        prt("", 1, 0);
        clear_from(screen.get_command_row(0));
        prt(_(" ESC) 建物から出る", " ESC) Exit from Building."), screen.get_command_row(1), 0);
        if (screen.has_multiple_pages()) {
            prt(_(" -)前ページ", " -) Previous page"), screen.get_command_row(2), 0);
            prt(_(" スペース) 次ページ", " SPACE) Next page"), screen.get_command_row(3), 0);
        }

        if (store_num == StoreSaleType::HOME) {
            prt(_("g) アイテムを取る", "g) Get an item."), screen.get_command_row(1), 27);
            prt(_("d) アイテムを置く", "d) Drop an item."), screen.get_command_row(2), 27);
            prt(_("x) 家のアイテムを調べる", "x) eXamine an item in the home."), screen.get_command_row(3), 27);
        } else if (store_num == StoreSaleType::MUSEUM) {
            prt(_("d) アイテムを置く", "d) Drop an item."), screen.get_command_row(1), 27);
            prt(_("r) アイテムの展示をやめる", "r) order to Remove an item."), screen.get_command_row(2), 27);
            prt(_("x) 博物館のアイテムを調べる", "x) eXamine an item in the museum."), screen.get_command_row(3), 27);
        } else {
            prt(_("p) 商品を買う", "p) Purchase an item."), screen.get_command_row(1), 30);
            prt(_("s) アイテムを売る", "s) Sell an item."), screen.get_command_row(2), 30);
            prt(_("x) 商品を調べる", "x) eXamine an item in the shop"), screen.get_command_row(3), 30);
        }

        prt(_("i/e) 持ち物/装備の一覧", "i/e) Inventry/Equipment list"), screen.get_command_row(1), 56);
        if (rogue_like_commands) {
            prt(_("w/T) 装備する/はずす", "w/T) Wear/Take off equipment"), screen.get_command_row(2), 56);
        } else {
            prt(_("w/t) 装備する/はずす", "w/t) Wear/Take off equipment"), screen.get_command_row(2), 56);
        }

        prt(_("コマンド:", "You may: "), screen.get_command_row(0), 0);
        output_bot_json_store_snapshot(player_ptr, screen);
        InputKeyRequestor(player_ptr, true).request_command();
        should_leave = store_process_command(player_ptr, screen);

        const auto should_redraw_store_inventory = rfu.has(StatusRecalculatingFlag::BONUS);
        world.character_icky_depth = 1;
        handle_stuff(player_ptr);
        if (player_ptr->inventory[INVEN_PACK]->bi_id) {
            INVENTORY_IDX i_idx = INVEN_PACK;
            const auto &item_inventory = *player_ptr->inventory[i_idx];
            if (store_num != StoreSaleType::HOME) {
                if (store_num == StoreSaleType::MUSEUM) {
                    msg_print(_("ザックからアイテムがあふれそうなので、あわてて博物館から出た...", "Your pack is so full that you flee the Museum..."));
                } else {
                    msg_print(_("ザックからアイテムがあふれそうなので、あわてて店から出た...", "Your pack is so full that you flee the store..."));
                }

                should_leave = true;
            } else if (!store_check_num(&item_inventory, store)) {
                msg_print(_("ザックからアイテムがあふれそうなので、あわてて家から出た...", "Your pack is so full that you flee your home..."));
                should_leave = true;
            } else {
                msg_print(_("ザックからアイテムがあふれてしまった！", "Your pack overflows!"));
                auto item = item_inventory.clone();
                const auto item_name = describe_flavor(player_ptr, item, 0);
                msg_format(_("%sが落ちた。(%c)", "You drop %s (%c)."), item_name.data(), index_to_label(i_idx));
                vary_item(player_ptr, i_idx, -255);
                handle_stuff(player_ptr);
                const auto item_pos = home_carry(player_ptr, store, &item);
                if (item_pos >= 0) {
                    screen.show_page_containing(item_pos);
                    display_store_inventory(player_ptr, screen);
                }
            }
        }

        if (should_redraw_store_inventory) {
            display_store_inventory(player_ptr, screen);
        }

        if (store.store_open >= world.game_turn) {
            should_leave = true;
        }
    }

    select_floor_music(player_ptr);
    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    world.character_icky_depth = 0;
    command_new = 0;
    command_see = false;
    get_com_no_macros = false;

    msg_erase();
    term_clear();

    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    rfu.set_flags(flags_mwrf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
}
