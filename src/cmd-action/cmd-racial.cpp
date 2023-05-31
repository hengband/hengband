/*
 * @brief クラス、種族、突然変異に関するコマンド処理
 * @author Hourier
 * @date 2022/02/24
 */

#include "cmd-action/cmd-racial.h"
#include "action/action-limited.h"
#include "action/mutation-execution.h"
#include "action/racial-execution.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mutation/mutation-flag-types.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/special-defense-types.h"
#include "racial/class-racial-switcher.h"
#include "racial/mutation-racial-selector.h"
#include "racial/race-racial-command-setter.h"
#include "racial/racial-util.h"
#include "status/action-setter.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-util.h"
#include <string>

#define RC_PAGE_SIZE 18

#define RC_CANCEL true
#define RC_CONTINUE false

static void racial_power_display_cursor(rc_type *rc_ptr)
{
    auto y = rc_ptr->menu_line % RC_PAGE_SIZE;
    put_str(_(" 》 ", " >  "), 2 + y, 11);
}

static void racial_power_erase_cursor(rc_type *rc_ptr)
{
    auto y = rc_ptr->menu_line % RC_PAGE_SIZE;
    put_str(_("    ", "    "), 2 + y, 11);
}

/*!
 * @brief レイシャルパワー一覧を表示
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return キャンセルしたらRC_CANCEL、それ以外ならRC_CONTINUE
 */
static void racial_power_display_list(PlayerType *player_ptr, rc_type *rc_ptr)
{
    TERM_LEN x = 11;

    prt(_("                                   Lv   MP 失率 効果", "                                   Lv   MP Fail Effect"), 1, x);

    auto y = 0;
    for (; y < RC_PAGE_SIZE; y++) {
        auto ctr = RC_PAGE_SIZE * rc_ptr->page + y;
        if (ctr >= rc_ptr->power_count()) {
            break;
        }

        std::string dummy;
        if (use_menu) {
            dummy = "    ";
        } else {
            char letter;
            if (ctr < 26) {
                letter = I2A(ctr);
            } else {
                letter = '0' + ctr - 26;
            }

            dummy = format(" %c) ", letter);
        }

        auto &rpi = rc_ptr->power_desc[ctr];
        dummy.append(
            format("%-30.30s %2d %4d %3d%% %s", rpi.racial_name.data(), rpi.min_level, rpi.cost, 100 - racial_chance(player_ptr, &rc_ptr->power_desc[ctr]),
                rpi.info.data())
                .data());

        prt(dummy, 2 + y, x);
    }

    prt("", 2 + y, x);

    if (use_menu) {
        racial_power_display_cursor(rc_ptr);
    }
}

/*!
 * @brief レイシャルパワー選択用のプロンプトを作成する
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 */
static void racial_power_make_prompt(rc_type *rc_ptr)
{
    concptr fmt;

    if (rc_ptr->browse_mode) {
        fmt = _(
            "(特殊能力 %c-%c, '*':一覧, '/'で使用, ESCで中断) どの能力について知りますか？", "(Powers %c-%c, *=List. /=Use, ESC=exit) Browse which power? ");
    } else {
        fmt = _("(特殊能力 %c-%c, '*'で一覧, '/'で閲覧, ESCで中断) どの能力を使いますか？", "(Powers %c-%c, *=List, /=Browse, ESC=exit) Use which power? ");
    }

    (void)strnfmt(rc_ptr->out_val, 78, fmt, I2A(0), (rc_ptr->power_count() <= 26) ? I2A(rc_ptr->power_count() - 1) : '0' + rc_ptr->power_count() - 27);
}

/*!
 * @brief レイシャルパワー選択用のカーソル位置を進める
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @param i カーソル増分
 */
static void racial_power_add_index(PlayerType *player_ptr, rc_type *rc_ptr, int i)
{
    auto n = rc_ptr->menu_line + i;
    if (i < -1 || i > 1) {
        if (n < 0 || n >= rc_ptr->power_count()) {
            return;
        }
    }
    if (n < 0) {
        n = rc_ptr->power_count() - 1;
    }
    if (n >= rc_ptr->power_count()) {
        n = 0;
    }

    auto p = n / RC_PAGE_SIZE;
    racial_power_erase_cursor(rc_ptr);
    rc_ptr->menu_line = n;
    if (rc_ptr->page != p) {
        rc_ptr->page = p;
        screen_load();
        screen_save();
        racial_power_display_list(player_ptr, rc_ptr);
    } else {
        racial_power_display_cursor(rc_ptr);
    }
}

/*!
 * @brief メニューによる選択のキーを処理する
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return キャンセルならRC_CANCEL、そうでないならRC_CONTINUE
 */
static bool racial_power_interpret_menu_keys(PlayerType *player_ptr, rc_type *rc_ptr)
{
    switch (rc_ptr->choice) {
    case '0':
        return RC_CANCEL;
    case '8':
    case 'k':
    case 'K':
        racial_power_add_index(player_ptr, rc_ptr, -1);
        return RC_CONTINUE;
    case '2':
    case 'j':
    case 'J':
        racial_power_add_index(player_ptr, rc_ptr, 1);
        return RC_CONTINUE;
    case '6':
    case 'l':
    case 'L':
        racial_power_add_index(player_ptr, rc_ptr, RC_PAGE_SIZE);
        return RC_CONTINUE;
    case '4':
    case 'h':
    case 'H':
        racial_power_add_index(player_ptr, rc_ptr, 0 - RC_PAGE_SIZE);
        return RC_CONTINUE;
    case 'x':
    case 'X':
    case '\r':
        rc_ptr->command_code = (COMMAND_CODE)rc_ptr->menu_line;
        rc_ptr->is_chosen = true;
        rc_ptr->ask = false;
        return RC_CONTINUE;
    case '/':
        rc_ptr->browse_mode = !rc_ptr->browse_mode;
        racial_power_make_prompt(rc_ptr);
        return RC_CONTINUE;
    default:
        return RC_CONTINUE;
    }
}

/*!
 * @brief メニューからの選択決定を処理
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return キャンセルしたらRC_CANCEL、それ以外ならRC_CONTINUE
 */
static bool racial_power_select_by_menu(PlayerType *player_ptr, rc_type *rc_ptr)
{
    if (!use_menu || rc_ptr->choice == ' ') {
        return RC_CONTINUE;
    }

    if (racial_power_interpret_menu_keys(player_ptr, rc_ptr)) {
        return RC_CANCEL;
    }

    if (rc_ptr->menu_line > rc_ptr->power_count()) {
        rc_ptr->menu_line -= rc_ptr->power_count();
    }

    return RC_CONTINUE;
}

/*!
 * @brief レイシャルパワーの選択を解釈
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コマンド選択していたらtrue、していなかったらfalse
 */
static bool racial_power_interpret_choise(PlayerType *player_ptr, rc_type *rc_ptr)
{
    if (use_menu) {
        return false;
    }

    if (rc_ptr->choice == ' ' || rc_ptr->choice == '*') {
        rc_ptr->page++;
        if (rc_ptr->page > rc_ptr->max_page) {
            rc_ptr->page = 0;
        }
        screen_load();
        screen_save();
        racial_power_display_list(player_ptr, rc_ptr);
        return false;
    }

    if (rc_ptr->choice == '/') {
        rc_ptr->browse_mode = !rc_ptr->browse_mode;
        racial_power_make_prompt(rc_ptr);
        return false;
    }

    if (rc_ptr->choice == '?') {
        return true;
    }

    return true;
}

static void decide_racial_command(rc_type *rc_ptr)
{
    if (use_menu) {
        return;
    }

    if (rc_ptr->choice == '\r' && rc_ptr->power_count() == 1) {
        rc_ptr->choice = 'a';
    }

    if (!isalpha(rc_ptr->choice)) {
        rc_ptr->ask = false;
        rc_ptr->command_code = rc_ptr->choice - '0' + 26;
        return;
    }

    rc_ptr->ask = (isupper(rc_ptr->choice));
    if (rc_ptr->ask) {
        rc_ptr->choice = (char)tolower(rc_ptr->choice);
    }

    rc_ptr->command_code = (islower(rc_ptr->choice) ? A2I(rc_ptr->choice) : -1);
}

static bool ask_invoke_racial_power(rc_type *rc_ptr)
{
    if ((rc_ptr->command_code < 0) || (rc_ptr->command_code >= rc_ptr->power_count())) {
        bell();
        return false;
    }

    if (!rc_ptr->ask) {
        return true;
    }

    char tmp_val[160];
    (void)strnfmt(tmp_val, 78, _("%sを使いますか？ ", "Use %s? "), rc_ptr->power_desc[rc_ptr->command_code].racial_name.data());
    return get_check(tmp_val);
}

static void racial_power_display_explanation(PlayerType *player_ptr, rc_type *rc_ptr)
{
    auto &rpi = rc_ptr->power_desc[rc_ptr->command_code];

    term_erase(12, 21, 255);
    term_erase(12, 20, 255);
    term_erase(12, 19, 255);
    term_erase(12, 18, 255);
    term_erase(12, 17, 255);
    term_erase(12, 16, 255);
    display_wrap_around(rpi.text, 62, 17, 15);

    prt(_("何かキーを押して下さい。", "Hit any key."), 0, 0);
    (void)inkey();

    screen_load();
    screen_save();
    racial_power_display_list(player_ptr, rc_ptr);
    rc_ptr->is_chosen = false;
}

/*!
 * @brief レイシャルパワー選択処理のメインループ
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コマンド選択したらRC_CONTINUE、キャンセルしたらRC_CANCEL
 */
static bool racial_power_process_input(PlayerType *player_ptr, rc_type *rc_ptr)
{
    rc_ptr->choice = (always_show_list || use_menu) ? ESCAPE : 1;

    while (true) {
        if (rc_ptr->choice == ESCAPE) {
            rc_ptr->choice = ' ';
        } else if (!get_com(rc_ptr->out_val, &rc_ptr->choice, false)) {
            return RC_CANCEL;
        }

        if (racial_power_select_by_menu(player_ptr, rc_ptr) == RC_CANCEL) {
            return RC_CANCEL;
        }

        if (!rc_ptr->is_chosen && racial_power_interpret_choise(player_ptr, rc_ptr)) {
            decide_racial_command(rc_ptr);
            if (ask_invoke_racial_power(rc_ptr)) {
                rc_ptr->is_chosen = true;
            }
        }

        if (rc_ptr->is_chosen) {
            if (rc_ptr->browse_mode) {
                racial_power_display_explanation(player_ptr, rc_ptr);
            } else {
                break;
            }
        }
    }

    return RC_CONTINUE;
}

/*!
 * @brief レイシャル/クラスパワー選択を処理
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コマンド選択したらRC_CONTINUE、キャンセルしたらRC_CANCEL
 */
static bool racial_power_select_power(PlayerType *player_ptr, rc_type *rc_ptr)
{
    if (repeat_pull(&rc_ptr->command_code) && rc_ptr->command_code >= 0 && rc_ptr->command_code < rc_ptr->power_count()) {
        return RC_CONTINUE;
    }

    screen_save();

    if (use_menu) {
        racial_power_display_list(player_ptr, rc_ptr);
    }

    auto canceled = racial_power_process_input(player_ptr, rc_ptr) == RC_CANCEL;

    screen_load();

    if (canceled) {
        return RC_CANCEL;
    }

    repeat_push(rc_ptr->command_code);
    return RC_CONTINUE;
}

/*!
 * @brief レイシャルパワーの使用を試みる
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @details
 * 戻り値の代わりにrc_ptr->castに使用の有無を入れる。
 */
static void racial_power_cast_power(PlayerType *player_ptr, rc_type *rc_ptr)
{
    auto *rpi_ptr = &rc_ptr->power_desc[rc_ptr->command_code];

    switch (check_racial_level(player_ptr, rpi_ptr)) {
    case RACIAL_SUCCESS:
        if (rpi_ptr->number < 0) {
            rc_ptr->cast = exe_racial_power(player_ptr, rpi_ptr->number);
        } else {
            rc_ptr->cast = exe_mutation_power(player_ptr, i2enum<PlayerMutationType>(rpi_ptr->number));
        }
        break;
    case RACIAL_FAILURE:
        rc_ptr->cast = true;
        break;
    case RACIAL_CANCEL:
        rc_ptr->cast = false;
        break;
    }
}

/*!
 * @brief レイシャルパワーのコストを減らす
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 * @return コストを減らしたらtrue、減らさなかったらfalse
 * @details
 * MPが足りない場合はHPを減らす。
 * 戻り値はHP/MPの再描画が必要か判定するのに使用。
 */
static bool racial_power_reduce_mana(PlayerType *player_ptr, rc_type *rc_ptr)
{
    int racial_cost = rc_ptr->power_desc[rc_ptr->command_code].racial_cost;
    if (racial_cost == 0) {
        return false;
    }

    int actual_racial_cost = racial_cost / 2 + randint1(racial_cost / 2);

    if (player_ptr->csp >= actual_racial_cost) {
        player_ptr->csp -= actual_racial_cost;
    } else {
        actual_racial_cost -= player_ptr->csp;
        player_ptr->csp = 0;
        take_hit(player_ptr, DAMAGE_USELIFE, actual_racial_cost, _("過度の集中", "concentrating too hard"));
    }

    return true;
}

/*!
 * @brief レイシャル・パワーコマンドのメインルーチン / Allow user to choose a power (racial / mutation) to activate
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_racial_power(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode) {
        return;
    }

    PlayerEnergy energy(player_ptr);
    if (cmd_limit_confused(player_ptr)) {
        energy.reset_player_turn();
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    auto tmp_r = rc_type(player_ptr);
    auto *rc_ptr = &tmp_r;

    switch_class_racial(player_ptr, rc_ptr);

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        set_mimic_racial_command(player_ptr, rc_ptr);
    } else {
        set_race_racial_command(player_ptr, rc_ptr);
    }

    select_mutation_racial(player_ptr, rc_ptr);

    if (rc_ptr->power_count() == 0) {
        msg_print(_("特殊能力はありません。", "You have no special powers."));
        return;
    }

    rc_ptr->max_page = 1 + (rc_ptr->power_count() - 1) / RC_PAGE_SIZE;
    rc_ptr->page = use_menu ? 0 : -1;
    racial_power_make_prompt(rc_ptr);

    if (racial_power_select_power(player_ptr, rc_ptr) == RC_CONTINUE) {
        racial_power_cast_power(player_ptr, rc_ptr);
    }

    if (!rc_ptr->cast) {
        energy.reset_player_turn();
        return;
    }

    if (!racial_power_reduce_mana(player_ptr, rc_ptr)) {
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto &flags_mwrf = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    rfu.set_flags(flags_mwrf);
    const auto flags_swrf = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags_swrf);
}
