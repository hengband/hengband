﻿#include "birth/birth-select-personality.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player/player-personality.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"

static std::string birth_personality_label(int cs, concptr sym)
{
    const char p2 = ')';
    std::string result;

    if (cs < 0 || cs >= MAX_PERSONALITIES) {
        result.append(1, '*').append(1, p2).append(_("ランダム", "Random"));
    } else {
        result.append(1, sym[cs]).append(1, p2).append(personality_info[cs].title);
    }
    return result;
}

static void enumerate_personality_list(PlayerType *player_ptr, char *sym)
{
    for (int n = 0; n < MAX_PERSONALITIES; n++) {
        if (personality_info[n].sex && (personality_info[n].sex != (player_ptr->psex + 1))) {
            continue;
        }

        ap_ptr = &personality_info[n];
        if (n < 26) {
            sym[n] = I2A(n);
        } else {
            sym[n] = ('A' + n - 26);
        }

        put_str(birth_personality_label(n, sym).data(), 12 + (n / 4), 2 + 18 * (n % 4));
    }
}

static std::string display_personality_stat(int cs, int *os, const std::string &cur, concptr sym)
{
    if (cs == *os) {
        return cur;
    }

    c_put_str(TERM_WHITE, cur.data(), 12 + (*os / 4), 2 + 18 * (*os % 4));
    put_str("                                   ", 3, 40);
    auto result = birth_personality_label(cs, sym);
    if (cs == MAX_PERSONALITIES) {
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);
        put_str("                                   ", 6, 40);
    } else {
        ap_ptr = &personality_info[cs];
        c_put_str(TERM_L_BLUE, ap_ptr->title, 3, 40);
        put_str(_("の性格修正", ": Personality modification"), 3, 40 + strlen(ap_ptr->title));
        put_str(_("腕力 知能 賢さ 器用 耐久 魅力      ", "Str  Int  Wis  Dex  Con  Chr       "), 4, 40);
        char buf[80];
        strnfmt(buf, sizeof(buf), "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d       ", ap_ptr->a_adj[0], ap_ptr->a_adj[1], ap_ptr->a_adj[2], ap_ptr->a_adj[3], ap_ptr->a_adj[4], ap_ptr->a_adj[5]);
        c_put_str(TERM_L_BLUE, buf, 5, 40);

        put_str("HD", 6, 40);
        strnfmt(buf, sizeof(buf), "%+3d", ap_ptr->a_mhp);
        c_put_str(TERM_L_BLUE, buf, 6, 42);

        put_str(_("隠密", "Stealth"), 6, 47);
        strnfmt(buf, sizeof(buf), "%+3d", ap_ptr->a_stl);
        c_put_str(TERM_L_BLUE, buf, 6, _(51, 54));
    }

    c_put_str(TERM_YELLOW, result.data(), 12 + (cs / 4), 2 + 18 * (cs % 4));
    *os = cs;
    return result;
}

static void interpret_personality_select_key_move(PlayerType *player_ptr, char c, int *cs)
{
    if (c == '8') {
        if (*cs >= 4) {
            *cs -= 4;
        }
        if ((*cs != MAX_PERSONALITIES) && personality_info[*cs].sex && (personality_info[*cs].sex != (player_ptr->psex + 1))) {
            if ((*cs - 4) > 0) {
                *cs -= 4;
            } else {
                *cs += 4;
            }
        }
    }

    if (c == '4') {
        if (*cs > 0) {
            (*cs)--;
        }
        if ((*cs != MAX_PERSONALITIES) && personality_info[*cs].sex && (personality_info[*cs].sex != (player_ptr->psex + 1))) {
            if ((*cs - 1) > 0) {
                (*cs)--;
            } else {
                (*cs)++;
            }
        }
    }

    if (c == '6') {
        if (*cs < MAX_PERSONALITIES) {
            (*cs)++;
        }
        if ((*cs != MAX_PERSONALITIES) && personality_info[*cs].sex && (personality_info[*cs].sex != (player_ptr->psex + 1))) {
            if ((*cs + 1) <= MAX_PERSONALITIES) {
                (*cs)++;
            } else {
                (*cs)--;
            }
        }
    }

    if (c == '2') {
        if ((*cs + 4) <= MAX_PERSONALITIES) {
            *cs += 4;
        }
        if ((*cs != MAX_PERSONALITIES) && personality_info[*cs].sex && (personality_info[*cs].sex != (player_ptr->psex + 1))) {
            if ((*cs + 4) <= MAX_PERSONALITIES) {
                *cs += 4;
            } else {
                *cs -= 4;
            }
        }
    }
}

static bool select_personality(PlayerType *player_ptr, int *k, concptr sym)
{
    int cs = player_ptr->ppersonality;
    int os = MAX_PERSONALITIES;
    std::string cur = birth_personality_label(os, sym);
    while (true) {
        cur = display_personality_stat(cs, &os, cur, sym);
        if (*k >= 0) {
            break;
        }

        char buf[80];
        strnfmt(buf, sizeof(buf), _("性格を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a personality (%c-%c) ('=' for options): "), sym[0], sym[MAX_PERSONALITIES - 1]);
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q') {
            birth_quit();
        }

        if (c == 'S') {
            return false;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_PERSONALITIES) {
                do {
                    *k = randint0(MAX_PERSONALITIES);
                } while (personality_info[*k].sex && (personality_info[*k].sex != (player_ptr->psex + 1)));

                cs = *k;
                continue;
            } else {
                *k = cs;
                break;
            }
        }

        interpret_personality_select_key_move(player_ptr, c, &cs);
        if (c == '*') {
            do {
                *k = randint0(MAX_PERSONALITIES);
            } while (personality_info[*k].sex && (personality_info[*k].sex != (player_ptr->psex + 1)));

            cs = *k;
            continue;
        }

        *k = (islower(c) ? A2I(c) : -1);
        if ((*k >= 0) && (*k < MAX_PERSONALITIES)) {
            if ((personality_info[*k].sex == 0) || (personality_info[*k].sex == (player_ptr->psex + 1))) {
                cs = *k;
                continue;
            }
        }

        *k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((*k >= 26) && (*k < MAX_PERSONALITIES)) {
            if ((personality_info[*k].sex == 0) || (personality_info[*k].sex == (player_ptr->psex + 1))) {
                cs = *k;
                continue;
            }
        } else {
            *k = -1;
        }

        birth_help_option(player_ptr, c, BK_PERSONALITY);
    }

    return true;
}

/*!
 * @brief プレイヤーの性格選択を行う / Select player's personality
 */
bool get_player_personality(PlayerType *player_ptr)
{
    clear_from(10);
    put_str(_("注意：《性格》によってキャラクターの能力やボーナスが変化します。", "Note: Your personality determines various intrinsic abilities and bonuses."),
        23, 5);
    put_str("                                   ", 6, 40);

    char sym[MAX_PERSONALITIES];
    enumerate_personality_list(player_ptr, sym);
    int k = -1;
    if (!select_personality(player_ptr, &k, sym)) {
        return false;
    }

    player_ptr->ppersonality = (player_personality_type)k;
    ap_ptr = &personality_info[player_ptr->ppersonality];
    char tmp[64];
#ifdef JP
    strcpy(tmp, ap_ptr->title);
    if (ap_ptr->no == 1) {
        strcat(tmp, "の");
    }
#else
    strcpy(tmp, ap_ptr->title);
    strcat(tmp, " ");
#endif
    strcat(tmp, player_ptr->name);
    c_put_str(TERM_L_BLUE, tmp, 1, 34);
    return true;
}
