#include "birth/birth-select-race.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player-info/race-info.h"
#include "player/race-info-table.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"

static const char p2 = ')';

static void enumerate_race_list(char *sym)
{
    char buf[80];
    for (int n = 0; n < MAX_RACES; n++) {
        rp_ptr = &race_info[n];
        concptr str = rp_ptr->title;
        if (n < 26)
            sym[n] = I2A(n);
        else
            sym[n] = ('A' + n - 26);

        sprintf(buf, "%c%c%s", sym[n], p2, str);
        put_str(buf, 12 + (n / 5), 1 + 16 * (n % 5));
    }
}

static void display_race_stat(int cs, int *os, char *cur, char *sym)
{
    char buf[80];
    if (cs == *os)
        return;

    c_put_str(TERM_WHITE, cur, 12 + (*os / 5), 1 + 16 * (*os % 5));
    put_str("                                   ", 3, 40);
    if (cs == MAX_RACES) {
        sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);
        put_str("                                   ", 6, 40);
    } else {
        rp_ptr = &race_info[cs];
        concptr str = rp_ptr->title;
        sprintf(cur, "%c%c%s", sym[cs], p2, str);
        c_put_str(TERM_L_BLUE, rp_ptr->title, 3, 40);
        put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
        put_str(_("の種族修正", ": Race modification"), 3, 40 + strlen(rp_ptr->title));

        sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ", rp_ptr->r_adj[0], rp_ptr->r_adj[1], rp_ptr->r_adj[2], rp_ptr->r_adj[3], rp_ptr->r_adj[4],
            rp_ptr->r_adj[5], (rp_ptr->r_exp - 100));
        c_put_str(TERM_L_BLUE, buf, 5, 40);

        put_str("HD ", 6, 40);
        sprintf(buf, "%2d", rp_ptr->r_mhp);
        c_put_str(TERM_L_BLUE, buf, 6, 43);

        put_str(_("隠密", "Stealth"), 6, 47);
        sprintf(buf, "%+2d", rp_ptr->r_stl);
        c_put_str(TERM_L_BLUE, buf, 6, _(52, 55));

        put_str(_("赤外線視力", "Infra"), 6, _(56, 59));
        sprintf(buf, _("%2dft", "%2dft"), 10 * rp_ptr->infra);
        c_put_str(TERM_L_BLUE, buf, 6, _(67, 65));
    }

    c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 1 + 16 * (cs % 5));
    *os = cs;
}

static void interpret_race_select_key_move(char c, int *cs)
{
    if (c == '8') {
        if (*cs >= 5)
            *cs -= 5;
    }

    if (c == '4') {
        if (*cs > 0)
            (*cs)--;
    }

    if (c == '6') {
        if (*cs < MAX_RACES)
            (*cs)++;
    }

    if (c == '2') {
        if ((*cs + 5) <= MAX_RACES)
            *cs += 5;
    }
}

static bool select_race(PlayerType *player_ptr, char *sym, int *k)
{
    char cur[80];
    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
    auto cs = enum2i(player_ptr->prace);
    int os = MAX_RACES;
    while (true) {
        display_race_stat(cs, &os, cur, sym);
        if (*k >= 0)
            break;

        char buf[80];
        sprintf(buf, _("種族を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a race (%c-%c) ('=' for options): "), sym[0], sym[MAX_RACES - 1]);
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();

        if (c == 'S')
            return false;

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_RACES) {
                *k = randint0(MAX_RACES);
                cs = *k;
                continue;
            } else {
                *k = cs;
                break;
            }
        }

        if (c == '*') {
            *k = randint0(MAX_RACES);
            cs = *k;
            continue;
        }

        interpret_race_select_key_move(c, &cs);
        *k = (islower(c) ? A2I(c) : -1);
        if ((*k >= 0) && (*k < MAX_RACES)) {
            cs = *k;
            continue;
        }

        *k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((*k >= 26) && (*k < MAX_RACES)) {
            cs = *k;
            continue;
        } else
            *k = -1;

        birth_help_option(player_ptr, c, BK_RACE);
    }

    return true;
}

/*!
 * @brief プレイヤーの種族選択を行う / Player race
 */
bool get_player_race(PlayerType *player_ptr)
{
    clear_from(10);
    put_str(
        _("注意：《種族》によってキャラクターの先天的な資質やボーナスが変化します。", "Note: Your 'race' determines various intrinsic factors and bonuses."),
        23, 5);

    char sym[MAX_RACES];
    enumerate_race_list(sym);
    int k = -1;
    if (!select_race(player_ptr, sym, &k))
        return false;

    player_ptr->prace = i2enum<PlayerRaceType>(k);
    rp_ptr = &race_info[k];
    c_put_str(TERM_L_BLUE, rp_ptr->title, 4, 15);
    return true;
}
