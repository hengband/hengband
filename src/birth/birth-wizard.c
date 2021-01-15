#include "birth/birth-wizard.h"
#include "birth/auto-roller.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-explanations-table.h"
#include "birth/birth-select-class.h"
#include "birth/birth-select-personality.h"
#include "birth/birth-select-race.h"
#include "birth/birth-select-realm.h"
#include "birth/birth-stat.h"
#include "birth/birth-util.h"
#include "birth/game-play-initializer.h"
#include "birth/history-editor.h"
#include "birth/history-generator.h"
#include "birth/quick-start.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/cmd-help.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "game-option/birth-options.h"
#include "io/input-key-acceptor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-info/avatar.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-race.h"
#include "player/player-sex.h"
#include "player/process-name.h"
#include "player/player-status-table.h"
#include "system/game-option-types.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-birth.h" // 暫定。後で消す予定。
#include "view/display-player.h" // 暫定。後で消す.
#include "world/world.h"

/*!
 * オートローラーの内容を描画する間隔 /
 * How often the autoroller will update the display and pause
 * to check for user interuptions.
 * Bigger values will make the autoroller faster, but slower
 * system may have problems because the user can't stop the
 * autoroller for this number of rolls.
 */
#define AUTOROLLER_STEP 54321L

static void display_initial_birth_message(player_type *creature_ptr)
{
    term_clear();
    put_str(_("名前  :", "Name  :"), 1, 26);
    put_str(_("性別        :", "Sex         :"), 3, 1);
    put_str(_("種族        :", "Race        :"), 4, 1);
    put_str(_("職業        :", "Class       :"), 5, 1);
    c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
    put_str(_("キャラクターを作成します。('S'やり直す, 'Q'終了, '?'ヘルプ)", "Make your character. ('S' Restart, 'Q' Quit, '?' Help)"), 8, 10);
    put_str(_("注意：《性別》の違いはゲーム上ほとんど影響を及ぼしません。", "Note: Your 'sex' does not have any significant gameplay effects."), 23, 5);
}

/*!
 * @prief 性別選択画面でヘルプを表示させる
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param c 入力したコマンド
 * @return なし
 * @details 他の関数名と被りそうだったので少し眺め
 */
static void display_help_on_sex_select(player_type *creature_ptr, char c)
{
    if (c == '?')
        do_cmd_help(creature_ptr);
    else if (c == '=') {
        screen_save();
        do_cmd_options_aux(creature_ptr, OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
        screen_load();
    } else if (c != '4' && c != '6')
        bell();
}

/*!
 * @brief プレイヤーの性別選択を行う / Player sex
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @buf 表示用バッファ
 * @return やり直すならFALSE、それ以外はTRUE
 */
static bool get_player_sex(player_type *creature_ptr, char *buf)
{
    const char p2 = ')';
    char cur[80];
    sprintf(cur, _("%c%c%s", "%c%c %s"), '*', p2, _("ランダム", "Random"));
    int k = -1;
    int cs = 0;
    int os = MAX_SEXES;
    while (TRUE) {
        if (cs != os) {
            put_str(cur, 12 + (os / 5), 2 + 15 * (os % 5));
            if (cs == MAX_SEXES)
                sprintf(cur, _("%c%c%s", "%c%c %s"), '*', p2, _("ランダム", "Random"));
            else {
                sp_ptr = &sex_info[cs];
                concptr str = sp_ptr->title;
                sprintf(cur, _("%c%c%s", "%c%c %s"), I2A(cs), p2, str);
            }

            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("性別を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a sex (%c-%c) ('=' for options): "), I2A(0), I2A(1));
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();

        if (c == 'S')
            return FALSE;

        if (c == ' ' || c == '\r' || c == '\n') {
            k = cs == MAX_SEXES ? randint0(MAX_SEXES) : cs;
            break;
        }

        if (c == '*') {
            k = randint0(MAX_SEXES);
            break;
        }

        if (c == '4') {
            if (cs > 0)
                cs--;
        }

        if (c == '6') {
            if (cs < MAX_SEXES)
                cs++;
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_SEXES)) {
            cs = k;
            continue;
        } else
            k = -1;

        display_help_on_sex_select(creature_ptr, c);
    }

    creature_ptr->psex = (byte)k;
    sp_ptr = &sex_info[creature_ptr->psex];
    c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 15);
    return TRUE;
}

static bool let_player_select_race(player_type *creature_ptr)
{
    clear_from(10);
    creature_ptr->prace = 0;
    while (TRUE) {
        char temp[80 * 10];
        if (!get_player_race(creature_ptr))
            return FALSE;

        clear_from(10);
        shape_buffer(race_explanations[creature_ptr->prace], 74, temp, sizeof(temp));
        concptr t = temp;
        for (int i = 0; i < 10; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }
        if (get_check_strict(creature_ptr, _("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        clear_from(10);
        c_put_str(TERM_WHITE, "              ", 4, 15);
    }

    return TRUE;
}

static bool let_player_select_class(player_type *creature_ptr)
{
    clear_from(10);
    creature_ptr->pclass = 0;
    while (TRUE) {
        char temp[80 * 9];
        if (!get_player_class(creature_ptr))
            return FALSE;

        clear_from(10);
        shape_buffer(class_explanations[creature_ptr->pclass], 74, temp, sizeof(temp));
        concptr t = temp;
        for (int i = 0; i < 9; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (get_check_strict(creature_ptr, _("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        c_put_str(TERM_WHITE, "              ", 5, 15);
    }

    return TRUE;
}

static bool let_player_select_personality(player_type *creature_ptr)
{
    creature_ptr->pseikaku = 0;
    while (TRUE) {
        char temp[80 * 8];
        if (!get_player_personality(creature_ptr))
            return FALSE;

        clear_from(10);
        shape_buffer(personality_explanations[creature_ptr->pseikaku], 74, temp, sizeof(temp));
        concptr t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (get_check_strict(creature_ptr, _("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
        prt("", 1, 34 + strlen(creature_ptr->name));
    }

    return TRUE;
}

static bool let_player_build_character(player_type *creature_ptr)
{
    char buf[80];
    if (!get_player_sex(creature_ptr, buf))
        return FALSE;

    if (!let_player_select_race(creature_ptr))
        return FALSE;

    if (!let_player_select_class(creature_ptr))
        return FALSE;

    if (!get_player_realms(creature_ptr))
        return FALSE;

    if (!let_player_select_personality(creature_ptr))
        return FALSE;

    return TRUE;
}

static void display_initial_options(player_type *creature_ptr)
{
    clear_from(10);
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);
    screen_save();
    do_cmd_options_aux(creature_ptr, OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
    screen_load();
}

static void display_auto_roller_success_rate(const int col)
{
    if (!autoroller)
        return;

    put_str(_("最小値", " Limit"), 2, col + 13);
    put_str(_("現在値", "  Roll"), 2, col + 24);

    char buf[32];

    if (autoroll_chance >= 1)
        sprintf(buf, _("確率 :  1/%8d00", "Prob :  1/%8d00"), autoroll_chance);
    else if (autoroll_chance == -999)
        sprintf(buf, _("確率 :     不可能", "Prob :     Impossible"));
    else
        sprintf(buf, _("確率 :     1/10000以上", "Prob :     >1/10000"));
    put_str(buf, 11, col + 10);

    put_str(_(
        "注意 : 体格等のオートローラを併用時は、上記確率より困難です。",
        "Note : Prob may be lower when you use the 'autochara' option."
        ), 22, 5);

    for (int i = 0; i < A_MAX; i++) {
        put_str(stat_names[i], 3 + i, col + 8);
        int j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];
        int m = adjust_stat(stat_limit[i], j);
        cnv_stat(m, buf);
        c_put_str(TERM_L_BLUE, buf, 3 + i, col + 13);
    }
}

static void auto_roller_count(void)
{
    if (auto_round < 1000000000L)
        return;

    auto_round = 1;
    if (!autoroller)
        return;

    auto_upper_round++;
}

static bool decide_initial_stat(player_type *creature_ptr)
{
    if (!autoroller)
        return TRUE;

    bool accept = TRUE;
    for (int i = 0; i < A_MAX; i++) {
        if (creature_ptr->stat_max[i] < stat_limit[i]) {
            accept = FALSE;
            break;
        }
    }

    return accept;
}

static bool decide_body_spec(player_type *creature_ptr, chara_limit_type chara_limit, bool *accept)
{
    if (!*accept)
        return FALSE;

    get_ahw(creature_ptr);
    get_history(creature_ptr);

    if (autochara) {
        if ((creature_ptr->age < chara_limit.agemin) || (creature_ptr->age > chara_limit.agemax))
            *accept = FALSE;
        if ((creature_ptr->ht < chara_limit.htmin) || (creature_ptr->ht > chara_limit.htmax))
            *accept = FALSE;
        if ((creature_ptr->wt < chara_limit.wtmin) || (creature_ptr->wt > chara_limit.wtmax))
            *accept = FALSE;
        if ((creature_ptr->sc < chara_limit.scmin) || (creature_ptr->sc > chara_limit.scmax))
            *accept = FALSE;
    }

    return *accept;
}

static bool display_auto_roller_count(player_type *creature_ptr, const int col)
{
    if ((auto_round % AUTOROLLER_STEP) != 0)
        return FALSE;

    birth_put_stats(creature_ptr);
    if (auto_upper_round)
        put_str(format("%ld%09ld", auto_upper_round, auto_round), 10, col + 20);
    else
        put_str(format("%10ld", auto_round), 10, col + 20);
    term_fresh();
    inkey_scan = TRUE;
    if (inkey()) {
        get_ahw(creature_ptr);
        get_history(creature_ptr);
        return TRUE;
    }

    return FALSE;
}

static void exe_auto_roller(player_type *creature_ptr, chara_limit_type chara_limit, const int col)
{
    while (autoroller || autochara) {
        get_stats(creature_ptr);
        auto_round++;
        auto_roller_count();
        bool accept = decide_initial_stat(creature_ptr);
        if (decide_body_spec(creature_ptr, chara_limit, &accept))
            return;

        if (display_auto_roller_count(creature_ptr, col))
            return;
    }
}

static bool display_auto_roller_result(player_type *creature_ptr, bool prev, char *c)
{
    BIT_FLAGS mode = 0;
    while (TRUE) {
        creature_ptr->update |= (PU_BONUS | PU_HP);
        update_creature(creature_ptr);
        creature_ptr->chp = creature_ptr->mhp;
        creature_ptr->csp = creature_ptr->msp;
        display_player(creature_ptr, mode);
        term_gotoxy(2, 23);
        const char b1 = '[';
        term_addch(TERM_WHITE, b1);
        term_addstr(-1, TERM_WHITE, _("'r' 次の数値", "'r'eroll"));
        if (prev)
            term_addstr(-1, TERM_WHITE, _(", 'p' 前の数値", "'p'previous"));

        if (mode)
            term_addstr(-1, TERM_WHITE, _(", 'h' その他の情報", ", 'h' Misc."));
        else
            term_addstr(-1, TERM_WHITE, _(", 'h' 生い立ちを表示", ", 'h'istory"));

        term_addstr(-1, TERM_WHITE, _(", Enter この数値に決定", ", or Enter to accept"));
        const char b2 = ']';
        term_addch(TERM_WHITE, b2);
        *c = inkey();
        if (*c == 'Q')
            birth_quit();

        if (*c == 'S')
            return FALSE;

        if (*c == '\r' || *c == '\n' || *c == ESCAPE)
            break;

        if ((*c == ' ') || (*c == 'r'))
            break;

        if (prev && (*c == 'p')) {
            load_prev_data(creature_ptr, TRUE);
            continue;
        }

        if ((*c == 'H') || (*c == 'h')) {
            mode = ((mode != 0) ? 0 : 1);
            continue;
        }

        birth_help_option(creature_ptr, *c, BK_AUTO_ROLLER);
        bell();
    }

    return TRUE;
}

/*
 * @brief オートロールを回して結果を表示し、その数値に決めるかさらに回すか確認する。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param chara_limit 社会的地位の要求水準
 * @detail 2つめの結果以降は、'p'キーで1つ前のロール結果に戻せる。
 */
static bool display_auto_roller(player_type *creature_ptr, chara_limit_type chara_limit)
{
    bool prev = FALSE;

    while (TRUE) {
        int col = 22;
        if (autoroller || autochara) {
            term_clear();
            put_str(_("回数 :", "Round:"), 10, col + 10);
            put_str(_("(ESCで停止)", "(Hit ESC to stop)"), 13, col + 13);
        } else {
            get_stats(creature_ptr);
            get_ahw(creature_ptr);
            get_history(creature_ptr);
        }

        display_auto_roller_success_rate(col);
        exe_auto_roller(creature_ptr, chara_limit, col);
        if (autoroller || autochara)
            sound(SOUND_LEVEL);

        flush();

        get_extra(creature_ptr, TRUE);
        get_money(creature_ptr);
        creature_ptr->chaos_patron = (s16b)randint0(MAX_PATRON);

        char c;
        if (!display_auto_roller_result(creature_ptr, prev, &c))
            return FALSE;

        if (c == '\r' || c == '\n' || c == ESCAPE)
            break;

        save_prev_data(creature_ptr, &previous_char);
        previous_char.quick_ok = FALSE;
        prev = TRUE;
    }

    return TRUE;
}

/*!
 * @brief 名前と生い立ちを設定する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param process_autopick_file_command 自動拾いコマンドへの関数ポインタ
 * @return なし
 * @details ついでにステータス限界もここで決めている
 */
static void set_name_history(player_type *creature_ptr, void (*process_autopick_file_command)(char *))
{
    clear_from(23);
    get_name(creature_ptr);
    process_player_name(creature_ptr, current_world_ptr->creating_savefile);
    edit_history(creature_ptr, process_autopick_file_command);
    get_max_stats(creature_ptr);
    get_virtues(creature_ptr);
    prt(_("[ 'Q' 中断, 'S' 初めから, Enter ゲーム開始 ]", "['Q'uit, 'S'tart over, or Enter to continue]"), 23, _(14, 10));
}

/*!
 * @brief プレーヤーキャラ作成ウィザード
 * @details
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 * @return なし
 */
bool player_birth_wizard(player_type *creature_ptr, void (*process_autopick_file_command)(char *))
{
    display_initial_birth_message(creature_ptr);
    const char p2 = ')';
    char buf[80];
    for (int n = 0; n < MAX_SEXES; n++) {
        sp_ptr = &sex_info[n];
        sprintf(buf, _("%c%c%s", "%c%c %s"), I2A(n), p2, sp_ptr->title);
        put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
    }

    if (!let_player_build_character(creature_ptr))
        return FALSE;

    display_initial_options(creature_ptr);
    if (autoroller || autochara) {
        auto_round = 0L;
        auto_upper_round = 0L;
        autoroll_chance = 0L;
    }

    if (autoroller)
        if (!get_stat_limits(creature_ptr))
            return FALSE;

    chara_limit_type chara_limit;
    initialize_chara_limit(&chara_limit);
    if (autochara)
        if (!get_chara_limits(creature_ptr, &chara_limit))
            return FALSE;

    clear_from(10);
    init_turn(creature_ptr);
    if (!display_auto_roller(creature_ptr, chara_limit))
        return FALSE;

    set_name_history(creature_ptr, process_autopick_file_command);
    char c = inkey();
    if (c == 'Q')
        birth_quit();

    if (c == 'S')
        return FALSE;

    init_dungeon_quests(creature_ptr);
    save_prev_data(creature_ptr, &previous_char);
    previous_char.quick_ok = TRUE;
    return TRUE;
}
