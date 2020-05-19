#include "system/angband.h"
#include "birth/birth-select-race.h"
#include "term/gameterm.h"
#include "birth/birth-util.h"

/*!
 * @brief プレイヤーの種族選択を行う / Player race
 * @return なし
 */
bool get_player_race(player_type *creature_ptr)
{
    char p2 = ')';
    char buf[80];
    char cur[80];

    clear_from(10);
    put_str(_("注意：《種族》によってキャラクターの先天的な資質やボーナスが変化します。",
                "Note: Your 'race' determines various intrinsic factors and bonuses."),
        23, 5);

    char sym[MAX_RACES];
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

    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
    int k = -1;
    int cs = creature_ptr->prace;
    int os = MAX_RACES;
    while (TRUE) {
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 12 + (os / 5), 1 + 16 * (os % 5));
            put_str("                                   ", 3, 40);
            if (cs == MAX_RACES) {
                sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
                put_str("                                   ", 4, 40);
                put_str("                                   ", 5, 40);
            } else {
                rp_ptr = &race_info[cs];
                concptr str = rp_ptr->title;
                sprintf(cur, "%c%c%s", sym[cs], p2, str);
                c_put_str(TERM_L_BLUE, rp_ptr->title, 3, 40);
                put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
                put_str(_("の種族修正", ": Race modification"), 3, 40 + strlen(rp_ptr->title));

                sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ",
                    rp_ptr->r_adj[0], rp_ptr->r_adj[1], rp_ptr->r_adj[2], rp_ptr->r_adj[3],
                    rp_ptr->r_adj[4], rp_ptr->r_adj[5], (rp_ptr->r_exp - 100));
                c_put_str(TERM_L_BLUE, buf, 5, 40);
            }
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 1 + 16 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("種族を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a race (%c-%c) ('=' for options): "), sym[0], sym[MAX_RACES - 1]);

        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_RACES) {
                k = randint0(MAX_RACES);
                cs = k;
                continue;
            } else {
                k = cs;
                break;
            }
        }

        if (c == '*') {
            k = randint0(MAX_RACES);
            cs = k;
            continue;
        }

        if (c == '8') {
            if (cs >= 5)
                cs -= 5;
        }

        if (c == '4') {
            if (cs > 0)
                cs--;
        }

        if (c == '6') {
            if (cs < MAX_RACES)
                cs++;
        }

        if (c == '2') {
            if ((cs + 5) <= MAX_RACES)
                cs += 5;
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_RACES)) {
            cs = k;
            continue;
        }

        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < MAX_RACES)) {
            cs = k;
            continue;
        } else
            k = -1;

        birth_help_option(creature_ptr, c, BK_RACE);
    }

    creature_ptr->prace = (byte)k;
    rp_ptr = &race_info[creature_ptr->prace];
    c_put_str(TERM_L_BLUE, rp_ptr->title, 4, 15);
    return TRUE;
}
