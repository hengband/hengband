#include "store/store-key-processor.h"
#include "autopick/autopick-pref-processor.h"
#include "cmd-action/cmd-mind.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-diary.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-knowledge.h"
#include "cmd-io/cmd-lore.h"
#include "cmd-io/cmd-macro.h"
#include "cmd-io/cmd-process-screen.h"
#include "cmd-item/cmd-destroy.h"
#include "cmd-item/cmd-equipment.h"
#include "cmd-item/cmd-item.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-visual/cmd-draw.h"
#include "cmd-visual/cmd-visuals.h"
#include "game-option/input-options.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "player-base/player-class.h"
#include "store/home.h"
#include "store/museum.h"
#include "store/purchase-order.h"
#include "store/sell-order.h"
#include "store/store-screen.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/player-type-definition.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "window/display-sub-windows.h"
#include "world/world.h"

/*!
 * @brief 店舗処理コマンド選択のメインルーチン /
 * Process a command in a store
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param screen コマンドの対象となる店舗の画面
 * @return 店から出るならtrue
 * @note
 * <pre>
 * Note that we must allow the use of a few "special" commands
 * in the stores which are not allowed in the dungeon, and we
 * must disable some commands which are allowed in the dungeon
 * but not in the stores, to prevent chaos.
 * </pre>
 */
bool store_process_command(PlayerType *player_ptr, StoreScreen &screen)
{
    const auto store_num = screen.get_store().get_sale_type();
    repeat_check();
    if (rogue_like_commands && (command_cmd == 'l')) {
        command_cmd = 'x';
    }

    auto &world = AngbandWorld::get_instance();
    switch (command_cmd) {
    case ESCAPE: {
        return true;
    }
    case '-': {
        /* 日本語版追加 */
        /* 1 ページ戻るコマンド: 我が家のページ数が多いので重宝するはず By BUG */
        if (!screen.has_multiple_pages()) {
            msg_print(_("これで全部です。", "Entire inventory is shown."));
        } else {
            screen.turn_page_backward();
            display_store_inventory(player_ptr, screen);
        }

        return false;
    }
    case ' ': {
        if (!screen.has_multiple_pages()) {
            msg_print(_("これで全部です。", "Entire inventory is shown."));
        } else {
            screen.turn_page_forward();
            display_store_inventory(player_ptr, screen);
        }

        return false;
    }
    case KTRL('R'): {
        do_cmd_redraw(player_ptr);
        display_store(player_ptr, screen);
        return false;
    }
    case 'g': {
        store_purchase(player_ptr, screen);
        return false;
    }
    case 'd': {
        store_sell(player_ptr, screen);
        return false;
    }
    case 'x': {
        store_examine(player_ptr, screen);
        return false;
    }
    case '\r': {
        return false;
    }
    case 'w': {
        do_cmd_wield(player_ptr);
        return false;
    }
    case 't': {
        do_cmd_takeoff(player_ptr);
        return false;
    }
    case 'k': {
        do_cmd_destroy(player_ptr);
        return false;
    }
    case 'e': {
        do_cmd_equip(player_ptr);
        return false;
    }
    case 'i': {
        do_cmd_inven(player_ptr);
        return false;
    }
    case 'I': {
        do_cmd_observe(player_ptr);
        return false;
    }
    case KTRL('I'): {
        toggle_inventory_equipment();
        return false;
    }
    case 'b': {
        PlayerClass pc(player_ptr);
        if (pc.can_browse()) {
            do_cmd_mind_browse(player_ptr);
        } else if (pc.equals(PlayerClassType::ELEMENTALIST)) {
            do_cmd_element_browse(player_ptr);
        } else if (pc.equals(PlayerClassType::SMITH)) {
            do_cmd_kaji(player_ptr, true);
        } else if (pc.equals(PlayerClassType::MAGIC_EATER)) {
            do_cmd_magic_eater(player_ptr, true, false);
        } else if (pc.equals(PlayerClassType::SNIPER)) {
            do_cmd_snipe_browse(player_ptr);
        } else {
            do_cmd_browse(player_ptr);
        }

        return false;
    }
    case '{': {
        do_cmd_inscribe(player_ptr);
        return false;
    }
    case '}': {
        do_cmd_uninscribe(player_ptr);
        return false;
    }
    case '?': {
        do_cmd_help(player_ptr);
        return false;
    }
    case '/': {
        do_cmd_query_symbol(player_ptr);
        return false;
    }
    case 'C': {
        world.set_town_index(old_town_num);
        do_cmd_player_status(player_ptr);
        world.set_town_index(inner_town_num);
        display_store(player_ptr, screen);
        return false;
    }
    case '!':
        term_user();
        return false;
    case '"': {
        world.set_town_index(old_town_num);
        do_cmd_pref(player_ptr);
        world.set_town_index(inner_town_num);
        return false;
    }
    case '@': {
        world.set_town_index(old_town_num);
        do_cmd_macros(player_ptr);
        world.set_town_index(inner_town_num);
        return false;
    }
    case '%': {
        world.set_town_index(old_town_num);
        do_cmd_visuals(player_ptr);
        world.set_town_index(inner_town_num);
        return false;
    }
    case '&': {
        world.set_town_index(old_town_num);
        do_cmd_colors(player_ptr);
        world.set_town_index(inner_town_num);
        return false;
    }
    case '=': {
        do_cmd_options(player_ptr);
        (void)combine_and_reorder_home(player_ptr, StoreSaleType::HOME);
        do_cmd_redraw(player_ptr);
        display_store(player_ptr, screen);
        return false;
    }
    case ':': {
        do_cmd_note();
        return false;
    }
    case 'V': {
        do_cmd_version();
        return false;
    }
    case KTRL('F'): {
        do_cmd_feeling(player_ptr);
        return false;
    }
    case KTRL('O'): {
        do_cmd_message_one();
        return false;
    }
    case KTRL('P'): {
        do_cmd_messages(0);
        return false;
    }
    case '|': {
        do_cmd_diary(player_ptr);
        return false;
    }
    case '~': {
        do_cmd_knowledge(player_ptr);
        return false;
    }
    case '(': {
        do_cmd_load_screen();
        return false;
    }
    case ')': {
        do_cmd_save_screen(player_ptr);
        return false;
    }
    default: {
        if ((store_num == StoreSaleType::MUSEUM) && (command_cmd == 'r')) {
            museum_remove_object(player_ptr, screen);
        } else {
            msg_print(_("そのコマンドは店の中では使えません。", "That command does not work in stores."));
        }

        return false;
    }
    }
}
