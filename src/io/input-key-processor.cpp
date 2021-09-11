/*!
 * @brief キー入力に応じてゲーム内コマンドを実行する
 * @date 2020/05/10
 * @author Hourier
 * @todo Ctrl+C がShift+Q に認識されている。仕様の可能性も高いが要確認
 */

#include "io/input-key-processor.h"
#include "autopick/autopick-pref-processor.h"
#include "cmd-action/cmd-hissatsu.h"
#include "cmd-action/cmd-mane.h"
#include "cmd-action/cmd-mind.h"
#include "cmd-action/cmd-move.h"
#include "cmd-action/cmd-open-close.h"
#include "cmd-action/cmd-others.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-action/cmd-racial.h"
#include "cmd-action/cmd-shoot.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-action/cmd-travel.h"
#include "cmd-action/cmd-tunnel.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-autopick.h"
#include "cmd-io/cmd-diary.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-floor.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-knowledge.h"
#include "cmd-io/cmd-lore.h"
#include "cmd-io/cmd-macro.h"
#include "cmd-io/cmd-process-screen.h"
#include "cmd-io/cmd-save.h"
#include "cmd-item/cmd-destroy.h"
#include "cmd-item/cmd-eat.h"
#include "cmd-item/cmd-equipment.h"
#include "cmd-item/cmd-item.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-item/cmd-quaff.h"
#include "cmd-item/cmd-read.h"
#include "cmd-item/cmd-refill.h"
#include "cmd-item/cmd-throw.h"
#include "cmd-item/cmd-usestaff.h"
#include "cmd-item/cmd-zaprod.h"
#include "cmd-item/cmd-zapwand.h"
#include "cmd-visual/cmd-draw.h"
#include "cmd-visual/cmd-map.h"
#include "cmd-visual/cmd-visuals.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/special-internal-keys.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h" //!< @do_cmd_quest() がある。後で移設する.
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/runtime-arguments.h"
#include "io-dump/random-art-info-dumper.h"
#include "io/command-repeater.h"
#include "io/files-util.h"
#include "io/input-key-requester.h" //!< @todo 相互依存している、後で何とかする.
#include "io/record-play-movie.h"
#include "io/write-diary.h"
#include "knowledge/knowledge-autopick.h"
#include "knowledge/knowledge-quests.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-blue-mage.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-magic-eater.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "mind/snipe-types.h"
#include "player-info/class-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "store/cmd-store.h"
#include "store/home.h"
#include "store/store-util.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "wizard/cmd-wizard.h"
#include "world/world.h"

/*!
 * @brief ウィザードモードへの導入処理
 * / Verify use of "wizard" mode
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 実際にウィザードモードへ移行したらTRUEを返す。
 */
bool enter_wizard_mode(player_type *player_ptr)
{
    if (!current_world_ptr->noscore) {
        if (!allow_debug_opts || arg_wizard) {
            msg_print(_("ウィザードモードは許可されていません。 ", "Wizard mode is not permitted."));
            return false;
        }

        msg_print(_("ウィザードモードはデバッグと実験のためのモードです。 ", "Wizard mode is for debugging and experimenting."));
        msg_print(_("一度ウィザードモードに入るとスコアは記録されません。", "The game will not be scored if you enter wizard mode."));
        msg_print(nullptr);
        if (!get_check(_("本当にウィザードモードに入りたいのですか? ", "Are you sure you want to enter wizard mode? "))) {
            return false;
        }

        exe_write_diary(
            player_ptr, DIARY_DESCRIPTION, 0, _("ウィザードモードに突入してスコアを残せなくなった。", "gave up recording score to enter wizard mode."));
        current_world_ptr->noscore |= 0x0002;
    }

    return true;
}

/*!
 * @brief デバッグコマンドへの導入処理
 * / Verify use of "debug" commands
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 実際にデバッグコマンドへ移行したらTRUEを返す。
 */
static bool enter_debug_mode(player_type *player_ptr)
{
    if (!current_world_ptr->noscore) {
        if (!allow_debug_opts) {
            msg_print(_("デバッグコマンドは許可されていません。 ", "Use of debug command is not permitted."));
            return false;
        }

        msg_print(_("デバッグ・コマンドはデバッグと実験のためのコマンドです。 ", "The debug commands are for debugging and experimenting."));
        msg_print(_("デバッグ・コマンドを使うとスコアは記録されません。", "The game will not be scored if you use debug commands."));
        msg_print(nullptr);
        if (!get_check(_("本当にデバッグ・コマンドを使いますか? ", "Are you sure you want to use debug commands? "))) {
            return false;
        }

        exe_write_diary(
            player_ptr, DIARY_DESCRIPTION, 0, _("デバッグモードに突入してスコアを残せなくなった。", "gave up sending score to use debug commands."));
        current_world_ptr->noscore |= 0x0008;
    }

    return true;
}

/*!
 * @brief プレイヤーから受けた入力コマンドの分岐処理。
 * / Parse and execute the current command Give "Warning" on illegal commands.
 * @todo Make some "blocks"
 */
void process_command(player_type *player_ptr)
{
    COMMAND_CODE old_now_message = now_message;
    repeat_check();
    now_message = 0;
    if ((player_ptr->pclass == CLASS_SNIPER) && (player_ptr->concent))
        player_ptr->reset_concent = true;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    switch (command_cmd) {
    case ESCAPE:
    case ' ':
    case '\r':
    case '\n': {
        /* Ignore */
        break;
    }
    case KTRL('W'): {
        if (current_world_ptr->wizard) {
            current_world_ptr->wizard = false;
            msg_print(_("ウィザードモード解除。", "Wizard mode off."));
        } else if (enter_wizard_mode(player_ptr)) {
            current_world_ptr->wizard = true;
            msg_print(_("ウィザードモード突入。", "Wizard mode on."));
        }

        player_ptr->update |= (PU_MONSTERS);
        player_ptr->redraw |= (PR_TITLE);
        break;
    }
    case KTRL('A'): {
        if (enter_debug_mode(player_ptr)) {
            do_cmd_debug(player_ptr);
        }

        break;
    }
    case 'w': {
        if (!player_ptr->wild_mode)
            do_cmd_wield(player_ptr);

        break;
    }
    case 't': {
        if (!player_ptr->wild_mode)
            do_cmd_takeoff(player_ptr);

        break;
    }
    case 'd': {
        if (!player_ptr->wild_mode)
            do_cmd_drop(player_ptr);

        break;
    }
    case 'k': {
        do_cmd_destroy(player_ptr);
        break;
    }
    case 'e': {
        do_cmd_equip(player_ptr);
        break;
    }
    case 'i': {
        do_cmd_inven(player_ptr);
        break;
    }
    case 'I': {
        do_cmd_observe(player_ptr);
        break;
    }

    case KTRL('I'): {
        toggle_inventory_equipment(player_ptr);
        break;
    }
    case '+': {
        if (!player_ptr->wild_mode)
            do_cmd_alter(player_ptr);

        break;
    }
    case 'T': {
        if (!player_ptr->wild_mode)
            do_cmd_tunnel(player_ptr);

        break;
    }
    case ';': {
        do_cmd_walk(player_ptr, false);
        break;
    }
    case '-': {
        do_cmd_walk(player_ptr, true);
        break;
    }
    case '.': {
        if (!player_ptr->wild_mode)
            do_cmd_run(player_ptr);

        break;
    }
    case ',': {
        do_cmd_stay(player_ptr, always_pickup);
        break;
    }
    case 'g': {
        do_cmd_stay(player_ptr, !always_pickup);
        break;
    }
    case 'R': {
        do_cmd_rest(player_ptr);
        break;
    }
    case 's': {
        do_cmd_search(player_ptr);
        break;
    }
    case 'S': {
        if (player_ptr->action == ACTION_SEARCH)
            set_action(player_ptr, ACTION_NONE);
        else
            set_action(player_ptr, ACTION_SEARCH);

        break;
    }
    case SPECIAL_KEY_STORE: {
        do_cmd_store(player_ptr);
        break;
    }
    case SPECIAL_KEY_BUILDING: {
        do_cmd_building(player_ptr);
        break;
    }
    case SPECIAL_KEY_QUEST: {
        do_cmd_quest(player_ptr);
        break;
    }
    case '<': {
        if (!player_ptr->wild_mode && !floor_ptr->dun_level && !floor_ptr->inside_arena && !floor_ptr->inside_quest) {
            if (vanilla_town)
                break;

            if (player_ptr->ambush_flag) {
                msg_print(_("襲撃から逃げるにはマップの端まで移動しなければならない。", "To flee the ambush you have to reach the edge of the map."));
                break;
            }

            if (player_ptr->food < PY_FOOD_WEAK) {
                msg_print(_("その前に食事をとらないと。", "You must eat something here."));
                break;
            }

            change_wild_mode(player_ptr, false);
        } else
            do_cmd_go_up(player_ptr);

        break;
    }
    case '>': {
        if (player_ptr->wild_mode)
            change_wild_mode(player_ptr, false);
        else
            do_cmd_go_down(player_ptr);

        break;
    }
    case 'o': {
        do_cmd_open(player_ptr);
        break;
    }
    case 'c': {
        do_cmd_close(player_ptr);
        break;
    }
    case 'j': {
        do_cmd_spike(player_ptr);
        break;
    }
    case 'B': {
        do_cmd_bash(player_ptr);
        break;
    }
    case 'D': {
        do_cmd_disarm(player_ptr);
        break;
    }
    case 'G': {
        if (player_ptr->pclass == CLASS_SORCERER || player_ptr->pclass == CLASS_RED_MAGE || player_ptr->pclass == CLASS_ELEMENTALIST)
            msg_print(_("呪文を学習する必要はない！", "You don't have to learn spells!"));
        else if (player_ptr->pclass == CLASS_SAMURAI)
            do_cmd_gain_hissatsu(player_ptr);
        else if (player_ptr->pclass == CLASS_MAGIC_EATER)
            import_magic_device(player_ptr);
        else
            do_cmd_study(player_ptr);

        break;
    }
    case 'b': {
        if ((player_ptr->pclass == CLASS_MINDCRAFTER) || (player_ptr->pclass == CLASS_BERSERKER) || (player_ptr->pclass == CLASS_NINJA)
            || (player_ptr->pclass == CLASS_MIRROR_MASTER))
            do_cmd_mind_browse(player_ptr);
        else if (player_ptr->pclass == CLASS_ELEMENTALIST)
            do_cmd_element_browse(player_ptr);
        else if (player_ptr->pclass == CLASS_SMITH)
            do_cmd_kaji(player_ptr, true);
        else if (player_ptr->pclass == CLASS_MAGIC_EATER)
            do_cmd_magic_eater(player_ptr, true, false);
        else if (player_ptr->pclass == CLASS_SNIPER)
            do_cmd_snipe_browse(player_ptr);
        else
            do_cmd_browse(player_ptr);

        break;
    }
    case 'm': {
        if (player_ptr->wild_mode) {
            break;
        }

        if ((player_ptr->pclass == CLASS_WARRIOR) || (player_ptr->pclass == CLASS_ARCHER) || (player_ptr->pclass == CLASS_CAVALRY)) {
            msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
            break;
        }

        if (floor_ptr->dun_level && d_info[player_ptr->dungeon_idx].flags.has(DF::NO_MAGIC) && (player_ptr->pclass != CLASS_BERSERKER)
            && (player_ptr->pclass != CLASS_SMITH)) {
            msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
            msg_print(nullptr);
            break;
        }

        if (player_ptr->anti_magic && (player_ptr->pclass != CLASS_BERSERKER) && (player_ptr->pclass != CLASS_SMITH)) {
            concptr which_power = _("魔法", "magic");
            switch (player_ptr->pclass) {
            case CLASS_MINDCRAFTER:
                which_power = _("超能力", "psionic powers");
                break;
            case CLASS_IMITATOR:
                which_power = _("ものまね", "imitation");
                break;
            case CLASS_SAMURAI:
                which_power = _("必殺剣", "hissatsu");
                break;
            case CLASS_MIRROR_MASTER:
                which_power = _("鏡魔法", "mirror magic");
                break;
            case CLASS_NINJA:
                which_power = _("忍術", "ninjutsu");
                break;
            case CLASS_ELEMENTALIST:
                which_power = _("元素魔法", "magic");
                break;
            default:
                if (mp_ptr->spell_book == TV_LIFE_BOOK)
                    which_power = _("祈り", "prayer");
                break;
            }

            msg_format(_("反魔法バリアが%sを邪魔した！", "An anti-magic shell disrupts your %s!"), which_power);
            PlayerEnergy(player_ptr).reset_player_turn();
            break;
        }

        if (is_shero(player_ptr) && (player_ptr->pclass != CLASS_BERSERKER)) {
            msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
            PlayerEnergy(player_ptr).reset_player_turn();
            break;
        }

        if ((player_ptr->pclass == CLASS_MINDCRAFTER) || (player_ptr->pclass == CLASS_BERSERKER) || (player_ptr->pclass == CLASS_NINJA)
            || (player_ptr->pclass == CLASS_MIRROR_MASTER))
            do_cmd_mind(player_ptr);
        else if (player_ptr->pclass == CLASS_ELEMENTALIST)
            do_cmd_element(player_ptr);
        else if (player_ptr->pclass == CLASS_IMITATOR)
            do_cmd_mane(player_ptr, false);
        else if (player_ptr->pclass == CLASS_MAGIC_EATER)
            do_cmd_magic_eater(player_ptr, false, false);
        else if (player_ptr->pclass == CLASS_SAMURAI)
            do_cmd_hissatsu(player_ptr);
        else if (player_ptr->pclass == CLASS_BLUE_MAGE)
            do_cmd_cast_learned(player_ptr);
        else if (player_ptr->pclass == CLASS_SMITH)
            do_cmd_kaji(player_ptr, false);
        else if (player_ptr->pclass == CLASS_SNIPER)
            do_cmd_snipe(player_ptr);
        else
            (void)do_cmd_cast(player_ptr);

        break;
    }
    case 'p': {
        do_cmd_pet(player_ptr);
        break;
    }
    case '{': {
        do_cmd_inscribe(player_ptr);
        break;
    }
    case '}': {
        do_cmd_uninscribe(player_ptr);
        break;
    }
    case 'A': {
        do_cmd_activate(player_ptr);
        break;
    }
    case 'E': {
        do_cmd_eat_food(player_ptr);
        break;
    }
    case 'F': {
        do_cmd_refill(player_ptr);
        break;
    }
    case 'f': {
        do_cmd_fire(player_ptr, SP_NONE);
        break;
    }
    case 'v': {
        (void)ThrowCommand(player_ptr).do_cmd_throw(1, false, -1);
        break;
    }
    case 'a': {
        do_cmd_aim_wand(player_ptr);
        break;
    }
    case 'z': {
        if (use_command && rogue_like_commands) {
            do_cmd_use(player_ptr);
        } else {
            do_cmd_zap_rod(player_ptr);
        }

        break;
    }
    case 'q': {
        do_cmd_quaff_potion(player_ptr);
        break;
    }
    case 'r': {
        do_cmd_read_scroll(player_ptr);
        break;
    }
    case 'u': {
        if (use_command && !rogue_like_commands)
            do_cmd_use(player_ptr);
        else
            do_cmd_use_staff(player_ptr);

        break;
    }
    case 'U': {
        do_cmd_racial_power(player_ptr);
        break;
    }
    case 'M': {
        do_cmd_view_map(player_ptr);
        break;
    }
    case 'L': {
        do_cmd_locate(player_ptr);
        break;
    }
    case 'l': {
        do_cmd_look(player_ptr);
        break;
    }
    case '*': {
        do_cmd_target(player_ptr);
        break;
    }
    case '?': {
        do_cmd_help(player_ptr);
        break;
    }
    case '/': {
        do_cmd_query_symbol(player_ptr);
        break;
    }
    case 'C': {
        do_cmd_player_status(player_ptr);
        break;
    }
    case '!': {
        (void)term_user(0);
        break;
    }
    case '"': {
        do_cmd_pref(player_ptr);
        break;
    }
    case '$': {
        do_cmd_reload_autopick(player_ptr);
        break;
    }
    case '_': {
        do_cmd_edit_autopick(player_ptr);
        break;
    }
    case '@': {
        do_cmd_macros(player_ptr);
        break;
    }
    case '%': {
        do_cmd_visuals(player_ptr);
        do_cmd_redraw(player_ptr);
        break;
    }
    case '&': {
        do_cmd_colors(player_ptr);
        do_cmd_redraw(player_ptr);
        break;
    }
    case '=': {
        do_cmd_options(player_ptr);
        (void)combine_and_reorder_home(player_ptr, STORE_HOME);
        do_cmd_redraw(player_ptr);
        break;
    }
    case ':': {
        do_cmd_note();
        break;
    }
    case 'V': {
        do_cmd_version();
        break;
    }
    case KTRL('F'): {
        do_cmd_feeling(player_ptr);
        break;
    }
    case KTRL('O'): {
        do_cmd_message_one();
        break;
    }
    case KTRL('P'): {
        do_cmd_messages(old_now_message);
        break;
    }
    case KTRL('Q'): {
        do_cmd_checkquest(player_ptr);
        break;
    }
    case KTRL('R'): {
        now_message = old_now_message;
        do_cmd_redraw(player_ptr);
        break;
    }
    case KTRL('S'): {
        do_cmd_save_game(player_ptr, false);
        break;
    }
    case KTRL('T'): {
        do_cmd_time(player_ptr);
        break;
    }
    case KTRL('X'):
    case SPECIAL_KEY_QUIT: {
        do_cmd_save_and_exit(player_ptr);
        break;
    }
    case 'Q': {
        do_cmd_suicide(player_ptr);
        break;
    }
    case '|': {
        do_cmd_diary(player_ptr);
        break;
    }
    case '~': {
        do_cmd_knowledge(player_ptr);
        break;
    }
    case '(': {
        do_cmd_load_screen();
        break;
    }
    case ')': {
        do_cmd_save_screen(player_ptr);
        break;
    }
    case ']': {
        prepare_movie_hooks(player_ptr);
        break;
    }
    case KTRL('V'): {
        spoil_random_artifact(player_ptr, "randifact.txt");
        break;
    }
    case '`': {
        if (!player_ptr->wild_mode)
            do_cmd_travel(player_ptr);
        if (player_ptr->special_defense & KATA_MUSOU) {
            set_action(player_ptr, ACTION_NONE);
        }

        break;
    }
    default: {
        if (flush_failure)
            flush();
        if (one_in_(2)) {
            char error_m[1024];
            sound(SOUND_ILLEGAL);
            if (!get_rnd_line(_("error_j.txt", "error.txt"), 0, error_m))
                msg_print(error_m);
        } else {
            prt(_(" '?' でヘルプが表示されます。", "Type '?' for help."), 0, 0);
        }

        break;
    }
    }

    if (!player_ptr->energy_use && !now_message)
        now_message = old_now_message;
}
