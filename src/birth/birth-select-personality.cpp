#include "birth/birth-select-personality.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player/player-personality.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-player-misc-info.h"
#include <sstream>

static std::string birth_personality_label(int cs, concptr sym)
{
    const char p2 = ')';
    std::stringstream ss;

    if (cs < 0 || cs >= MAX_PERSONALITIES) {
        ss << '*' << p2 << _("ランダム", "Random");
    } else {
        ss << sym[cs] << p2 << personality_info[cs].title;
    }
    return ss.str();
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

        put_str(birth_personality_label(n, sym), 12 + (n / 4), 2 + 18 * (n % 4));
    }
}

static std::string display_personality_stat(int cs, int *os, const std::string &cur, concptr sym)
{
    if (cs == *os) {
        return cur;
    }

    c_put_str(TERM_WHITE, cur, 12 + (*os / 4), 2 + 18 * (*os % 4));
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

    c_put_str(TERM_YELLOW, result, 12 + (cs / 4), 2 + 18 * (cs % 4));
    *os = cs;
    return result;
}

static bool check_selected_sex(int pp_idx, player_sex psex)
{
    const auto ppersonality = personality_info[pp_idx];
    return (ppersonality.sex != 0) && (ppersonality.sex != (psex + 1));
}

static int interpret_personality_select_key_move(PlayerType *player_ptr, char key, int initial_personality)
{
    auto pp_idx = initial_personality;
    switch (key) {
    case '8':
        if (pp_idx >= 4) {
            pp_idx -= 4;
        }

        if ((pp_idx >= MAX_PERSONALITIES) || !check_selected_sex(pp_idx, player_ptr->psex)) {
            return pp_idx;
        }

        if ((pp_idx - 4) > 0) {
            pp_idx -= 4;
        } else {
            pp_idx += 4;
        }

        return pp_idx;
    case '4':
        if (pp_idx > 0) {
            (pp_idx)--;
        }

        if ((pp_idx >= MAX_PERSONALITIES) || !check_selected_sex(pp_idx, player_ptr->psex)) {
            return pp_idx;
        }

        if ((pp_idx - 1) > 0) {
            (pp_idx)--;
        } else {
            (pp_idx)++;
        }

        return pp_idx;
    case '6':
        if (pp_idx < MAX_PERSONALITIES) {
            (pp_idx)++;
        }

        if ((pp_idx >= MAX_PERSONALITIES) || !check_selected_sex(pp_idx, player_ptr->psex)) {
            return pp_idx;
        }

        if ((pp_idx + 1) <= MAX_PERSONALITIES) {
            (pp_idx)++;
        } else {
            (pp_idx)--;
        }

        return pp_idx;
    case '2':
        if ((pp_idx + 4) <= MAX_PERSONALITIES) {
            pp_idx += 4;
        }

        if ((pp_idx >= MAX_PERSONALITIES) || !check_selected_sex(pp_idx, player_ptr->psex)) {
            return pp_idx;
        }

        if ((pp_idx + 4) <= MAX_PERSONALITIES) {
            pp_idx += 4;
        } else {
            pp_idx -= 4;
        }

        return pp_idx;
    default:
        return pp_idx;
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

        cs = interpret_personality_select_key_move(player_ptr, c, cs);
        if (c == '*') {
            player_personality ppersonality{};
            do {
                *k = randint0(MAX_PERSONALITIES);
                if (*k < 0) {
                    continue; // 静的解析対応.
                }

                ppersonality = personality_info[*k];
            } while (ppersonality.sex && (ppersonality.sex != (player_ptr->psex + 1)));

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

        birth_help_option(player_ptr, c, BirthKind::PERSONALITY);
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
    display_player_name(player_ptr);
    return true;
}
