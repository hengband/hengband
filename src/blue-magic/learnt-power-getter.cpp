/*!
 * @file learnt-power-getter.cpp
 * @brief 青魔法の処理実行定義
 */

#include "blue-magic/learnt-power-getter.h"
#include "blue-magic/blue-magic-checker.h"
#include "blue-magic/learnt-info.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mind/mind-blue-mage.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/monster-power-table.h"
#include "player/player-status-table.h"
#include "realm/realm-types.h"
#include "spell/spell-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"
#include "util/flag-group.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

#include <vector>

typedef struct learnt_magic_type {
    int blue_magic_num;
    int count;
    TERM_LEN y;
    TERM_LEN x;
    PLAYER_LEVEL plev;
    PERCENTAGE chance;
    int ask;
    int mode;
    std::vector<int> blue_magics;
    char choice;
    char out_val[160];
    char comment[80];
    EnumClassFlagGroup<RF_ABILITY> ability_flags;
    monster_power spell;
    int menu_line;
    bool flag;
    bool redraw;
    int need_mana;
    char psi_desc[80];
} learnt_magic_type;

static learnt_magic_type *initialize_lenat_magic_type(player_type *player_ptr, learnt_magic_type *lm_ptr)
{
    lm_ptr->blue_magic_num = 0;
    lm_ptr->count = 0;
    lm_ptr->y = 1;
    lm_ptr->x = 18;
    lm_ptr->plev = player_ptr->lev;
    lm_ptr->chance = 0;
    lm_ptr->ask = true;
    lm_ptr->mode = 0;
    lm_ptr->blue_magics.clear();
    lm_ptr->ability_flags.clear();
    lm_ptr->menu_line = use_menu ? 1 : 0;
    lm_ptr->flag = false;
    lm_ptr->redraw = false;
    return lm_ptr;
}

/*!
 * @brief コマンド反復チェック
 * @param sn 選択したモンスター攻撃ID
 * @return 発動可能な魔法を選択した場合TRUE、処理続行の場合FALSE
 */
static bool check_blue_magic_cancel(SPELL_IDX *sn)
{
    *sn = -1;
    COMMAND_CODE code;
    if (!repeat_pull(&code))
        return false;

    *sn = (SPELL_IDX)code;
    return true;
}

static bool select_blue_magic_kind_menu(learnt_magic_type *lm_ptr)
{
    while (lm_ptr->mode == 0) {
        prt(format(_(" %s ボルト", " %s bolt"), (lm_ptr->menu_line == 1) ? _("》", "> ") : "  "), 2, 14);
        prt(format(_(" %s ボール", " %s ball"), (lm_ptr->menu_line == 2) ? _("》", "> ") : "  "), 3, 14);
        prt(format(_(" %s ブレス", " %s breath"), (lm_ptr->menu_line == 3) ? _("》", "> ") : "  "), 4, 14);
        prt(format(_(" %s 召喚", " %s sommoning"), (lm_ptr->menu_line == 4) ? _("》", "> ") : "  "), 5, 14);
        prt(format(_(" %s その他", " %s others"), (lm_ptr->menu_line == 5) ? _("》", "> ") : "  "), 6, 14);
        prt(_("どの種類の魔法を使いますか？", "use which type of magic? "), 0, 0);

        lm_ptr->choice = inkey();
        switch (lm_ptr->choice) {
        case ESCAPE:
        case 'z':
        case 'Z':
            screen_load();
            return false;
        case '2':
        case 'j':
        case 'J':
            lm_ptr->menu_line++;
            break;
        case '8':
        case 'k':
        case 'K':
            lm_ptr->menu_line += 4;
            break;
        case '\r':
        case 'x':
        case 'X':
            lm_ptr->mode = lm_ptr->menu_line;
            break;
        }

        if (lm_ptr->menu_line > 5)
            lm_ptr->menu_line -= 5;
    }

    return true;
}

static bool select_blue_magic_kind_command(learnt_magic_type *lm_ptr)
{
    sprintf(lm_ptr->comment, _("[A]ボルト, [B]ボール, [C]ブレス, [D]召喚, [E]その他:", "[A] bolt, [B] ball, [C] breath, [D] summoning, [E] others:"));
    while (true) {
        char ch;
        if (!get_com(lm_ptr->comment, &ch, true))
            return false;

        if (ch == 'A' || ch == 'a') {
            lm_ptr->mode = 1;
            break;
        }

        if (ch == 'B' || ch == 'b') {
            lm_ptr->mode = 2;
            break;
        }

        if (ch == 'C' || ch == 'c') {
            lm_ptr->mode = 3;
            break;
        }

        if (ch == 'D' || ch == 'd') {
            lm_ptr->mode = 4;
            break;
        }

        if (ch == 'E' || ch == 'e') {
            lm_ptr->mode = 5;
            break;
        }
    }

    return true;
}

static bool check_blue_magic_kind(learnt_magic_type *lm_ptr)
{
    if (!use_menu)
        return select_blue_magic_kind_command(lm_ptr);

    screen_save();
    if (!select_blue_magic_kind_menu(lm_ptr))
        return false;

    screen_load();
    return true;
}

static bool sweep_learnt_spells(player_type *player_ptr, learnt_magic_type *lm_ptr)
{
    set_rf_masks(lm_ptr->ability_flags, static_cast<blue_magic_type>(lm_ptr->mode));

    std::vector<RF_ABILITY> spells;
    EnumClassFlagGroup<RF_ABILITY>::get_flags(lm_ptr->ability_flags, std::back_inserter(spells));
    std::transform(spells.begin(), spells.end(), std::back_inserter(lm_ptr->blue_magics), [](RF_ABILITY ability) { return enum2i(ability); });
    lm_ptr->count = lm_ptr->ability_flags.count();

    for (lm_ptr->blue_magic_num = 0; lm_ptr->blue_magic_num < lm_ptr->count; lm_ptr->blue_magic_num++) {
        if (player_ptr->magic_num2[lm_ptr->blue_magics[lm_ptr->blue_magic_num]] == 0)
            continue;

        if (use_menu)
            lm_ptr->menu_line = lm_ptr->blue_magic_num + 1;

        break;
    }

    if (lm_ptr->blue_magic_num == lm_ptr->count) {
        msg_print(_("その種類の魔法は覚えていない！", "You don't know any spell of this type."));
        return false;
    }

    (void)strnfmt(lm_ptr->out_val, 78, _("(%c-%c, '*'で一覧, ESC) どの%sを唱えますか？", "(%c-%c, *=List, ESC=exit) Use which %s? "), I2A(0),
        I2A(lm_ptr->count - 1), _("魔法", "magic"));
    return true;
}

static bool switch_blue_magic_choice(player_type *player_ptr, learnt_magic_type *lm_ptr)
{
    switch (lm_ptr->choice) {
    case '0':
        screen_load();
        return false;
    case '8':
    case 'k':
    case 'K':
        do {
            lm_ptr->menu_line += (lm_ptr->count - 1);
            if (lm_ptr->menu_line > lm_ptr->count)
                lm_ptr->menu_line -= lm_ptr->count;
        } while (!player_ptr->magic_num2[lm_ptr->blue_magics[lm_ptr->menu_line - 1]]);
        return true;
    case '2':
    case 'j':
    case 'J':
        do {
            lm_ptr->menu_line++;
            if (lm_ptr->menu_line > lm_ptr->count)
                lm_ptr->menu_line -= lm_ptr->count;
        } while (!player_ptr->magic_num2[lm_ptr->blue_magics[lm_ptr->menu_line - 1]]);
        return true;
    case '6':
    case 'l':
    case 'L':
        lm_ptr->menu_line = lm_ptr->count;
        while (!player_ptr->magic_num2[lm_ptr->blue_magics[lm_ptr->menu_line - 1]])
            lm_ptr->menu_line--;

        return true;
    case '4':
    case 'h':
    case 'H':
        lm_ptr->menu_line = 1;
        while (!player_ptr->magic_num2[lm_ptr->blue_magics[lm_ptr->menu_line - 1]])
            lm_ptr->menu_line++;

        return true;
    case 'x':
    case 'X':
    case '\r':
        lm_ptr->blue_magic_num = lm_ptr->menu_line - 1;
        lm_ptr->ask = false;
        return true;
    default:
        return true;
    }
}

static void calculate_blue_magic_success_probability(player_type *player_ptr, learnt_magic_type *lm_ptr)
{
    lm_ptr->chance = lm_ptr->spell.fail;
    if (lm_ptr->plev > lm_ptr->spell.level)
        lm_ptr->chance -= 3 * (lm_ptr->plev - lm_ptr->spell.level);
    else
        lm_ptr->chance += (lm_ptr->spell.level - lm_ptr->plev);

    lm_ptr->chance -= 3 * (adj_mag_stat[player_ptr->stat_index[A_INT]] - 1);
    lm_ptr->chance = mod_spell_chance_1(player_ptr, lm_ptr->chance);
    lm_ptr->need_mana = mod_need_mana(player_ptr, monster_powers[lm_ptr->blue_magics[lm_ptr->blue_magic_num]].smana, 0, REALM_NONE);
    if (lm_ptr->need_mana > player_ptr->csp)
        lm_ptr->chance += 5 * (lm_ptr->need_mana - player_ptr->csp);

    PERCENTAGE minfail = adj_mag_fail[player_ptr->stat_index[A_INT]];
    if (lm_ptr->chance < minfail)
        lm_ptr->chance = minfail;

    if (player_ptr->stun > 50)
        lm_ptr->chance += 25;
    else if (player_ptr->stun)
        lm_ptr->chance += 15;

    if (lm_ptr->chance > 95)
        lm_ptr->chance = 95;

    lm_ptr->chance = mod_spell_chance_2(player_ptr, lm_ptr->chance);
}

static void close_blue_magic_name(learnt_magic_type *lm_ptr)
{
    if (!use_menu) {
        sprintf(lm_ptr->psi_desc, "  %c)", I2A(lm_ptr->blue_magic_num));
        return;
    }

    if (lm_ptr->blue_magic_num == (lm_ptr->menu_line - 1))
        strcpy(lm_ptr->psi_desc, _("  》", "  > "));
    else
        strcpy(lm_ptr->psi_desc, "    ");
}

static void describe_blue_magic_name(player_type *player_ptr, learnt_magic_type *lm_ptr)
{
    prt("", lm_ptr->y, lm_ptr->x);
    put_str(_("名前", "Name"), lm_ptr->y, lm_ptr->x + 5);
    put_str(_("MP 失率 効果", "SP Fail Info"), lm_ptr->y, lm_ptr->x + 33);
    for (lm_ptr->blue_magic_num = 0; lm_ptr->blue_magic_num < lm_ptr->count; lm_ptr->blue_magic_num++) {
        prt("", lm_ptr->y + lm_ptr->blue_magic_num + 1, lm_ptr->x);
        if (!player_ptr->magic_num2[lm_ptr->blue_magics[lm_ptr->blue_magic_num]])
            continue;

        lm_ptr->spell = monster_powers[lm_ptr->blue_magics[lm_ptr->blue_magic_num]];
        calculate_blue_magic_success_probability(player_ptr, lm_ptr);
        learnt_info(player_ptr, lm_ptr->comment, static_cast<RF_ABILITY>(lm_ptr->blue_magics[lm_ptr->blue_magic_num]));
        close_blue_magic_name(lm_ptr);
        strcat(lm_ptr->psi_desc, format(" %-26s %3d %3d%%%s", lm_ptr->spell.name, lm_ptr->need_mana, lm_ptr->chance, lm_ptr->comment));
        prt(lm_ptr->psi_desc, lm_ptr->y + lm_ptr->blue_magic_num + 1, lm_ptr->x);
    }
}

static bool blue_magic_key_input(player_type *player_ptr, learnt_magic_type *lm_ptr)
{
    if ((lm_ptr->choice != ' ') && (lm_ptr->choice != '*') && (lm_ptr->choice != '?') && (!use_menu || (lm_ptr->ask == 0)))
        return false;

    if (lm_ptr->redraw && !use_menu) {
        lm_ptr->redraw = false;
        screen_load();
        return true;
    }

    lm_ptr->redraw = true;
    if (!use_menu)
        screen_save();

    describe_blue_magic_name(player_ptr, lm_ptr);
    if (lm_ptr->y < 22)
        prt("", lm_ptr->y + lm_ptr->blue_magic_num + 1, lm_ptr->x);

    return true;
}

static void convert_lower_blue_magic_selection(learnt_magic_type *lm_ptr)
{
    if (use_menu)
        return;

    lm_ptr->ask = isupper(lm_ptr->choice);
    if (lm_ptr->ask)
        lm_ptr->choice = (char)tolower(lm_ptr->choice);

    lm_ptr->blue_magic_num = islower(lm_ptr->choice) ? A2I(lm_ptr->choice) : -1;
}

static bool ask_cast_blue_magic(learnt_magic_type *lm_ptr)
{
    if (lm_ptr->ask == 0)
        return true;

    char tmp_val[160];
    (void)strnfmt(tmp_val, 78, _("%sの魔法を唱えますか？", "Use %s? "), monster_powers[lm_ptr->blue_magics[lm_ptr->blue_magic_num]].name);
    return get_check(tmp_val);
}

static bool select_learnt_spells(player_type *player_ptr, learnt_magic_type *lm_ptr)
{
    while (!lm_ptr->flag) {
        if (lm_ptr->choice == ESCAPE)
            lm_ptr->choice = ' ';
        else if (!get_com(lm_ptr->out_val, &lm_ptr->choice, true))
            break;

        if (use_menu && (lm_ptr->choice != ' ') && !switch_blue_magic_choice(player_ptr, lm_ptr))
            return false;

        if (blue_magic_key_input(player_ptr, lm_ptr))
            continue;

        convert_lower_blue_magic_selection(lm_ptr);
        if ((lm_ptr->blue_magic_num < 0) || (lm_ptr->blue_magic_num >= lm_ptr->count) || !player_ptr->magic_num2[lm_ptr->blue_magics[lm_ptr->blue_magic_num]]) {
            bell();
            continue;
        }

        lm_ptr->spell = monster_powers[lm_ptr->blue_magics[lm_ptr->blue_magic_num]];
        if (!ask_cast_blue_magic(lm_ptr))
            continue;

        lm_ptr->flag = true;
    }

    return true;
}

/*!
 * @brief 使用可能な青魔法を選択する /
 * Allow user to choose a imitation.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sn 選択したモンスター攻撃ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
bool get_learned_power(player_type *player_ptr, SPELL_IDX *sn)
{
    learnt_magic_type tmp_magic;
    learnt_magic_type *lm_ptr = initialize_lenat_magic_type(player_ptr, &tmp_magic);
    if (check_blue_magic_cancel(sn))
        return true;

    if (!check_blue_magic_kind(lm_ptr) || !sweep_learnt_spells(player_ptr, lm_ptr))
        return false;

    if (use_menu)
        screen_save();

    lm_ptr->choice = (always_show_list || use_menu) ? ESCAPE : 1;
    if (!select_learnt_spells(player_ptr, lm_ptr))
        return false;

    if (lm_ptr->redraw)
        screen_load();

    player_ptr->window_flags |= PW_SPELL;
    handle_stuff(player_ptr);

    if (!lm_ptr->flag)
        return false;

    *sn = lm_ptr->blue_magics[lm_ptr->blue_magic_num];
    repeat_push((COMMAND_CODE)lm_ptr->blue_magics[lm_ptr->blue_magic_num]);
    return true;
}
