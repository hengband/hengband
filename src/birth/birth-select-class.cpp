#include "birth/birth-select-class.h"
#include "birth/birth-util.h"
#include "io/input-key-acceptor.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <sstream>

static std::string birth_class_label(int cs, concptr sym)
{
    constexpr auto p2 = ')';
    std::stringstream ss;
    if (cs < 0 || cs >= PLAYER_CLASS_TYPE_MAX) {
        ss << '*' << p2 << _("ランダム", "Random");
        return ss.str();
    }

    ss << sym[cs] << p2;
    const auto pclass = i2enum<PlayerClassType>(cs);
    const auto &title = class_info.at(pclass).title;
    if (!(rp_ptr->choice & (1UL << cs))) {
        ss << '(' << title << ')';
    } else {
        ss << title;
    }

    return ss.str();
}

static void enumerate_class_list(char *sym)
{
    for (auto n = 0; n < PLAYER_CLASS_TYPE_MAX; n++) {
        cp_ptr = &class_info.at(i2enum<PlayerClassType>(n));
        mp_ptr = &class_magics_info[n];
        if (n < 26) {
            sym[n] = I2A(n);
        } else {
            sym[n] = ('A' + n - 26);
        }

        auto cs = i2enum<PlayerClassType>(n);
        c_put_str(w_ptr->get_birth_class_color(cs), birth_class_label(n, sym), 13 + (n / 4), 2 + 19 * (n % 4));
    }
}

static std::string display_class_stat(int cs, int *os, const std::string &cur, concptr sym)
{
    if (cs == *os) {
        return cur;
    }

    auto pclass = i2enum<PlayerClassType>(*os);
    c_put_str(w_ptr->get_birth_class_color(pclass), cur, 13 + (*os / 4), 2 + 19 * (*os % 4));
    put_str("                                   ", 3, 40);
    auto result = birth_class_label(cs, sym);
    if (cs == PLAYER_CLASS_TYPE_MAX) {
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);
        put_str("                                   ", 6, 40);
    } else {
        cp_ptr = &class_info.at(i2enum<PlayerClassType>(cs));
        mp_ptr = &class_magics_info[cs];

        c_put_str(TERM_L_BLUE, cp_ptr->title, 3, 40);
        put_str(_("の職業修正", ": Class modification"), 3, 40 + cp_ptr->title.length());
        put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
        const auto stats = format("%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ", cp_ptr->c_adj[0], cp_ptr->c_adj[1], cp_ptr->c_adj[2], cp_ptr->c_adj[3], cp_ptr->c_adj[4], cp_ptr->c_adj[5], cp_ptr->c_exp);
        c_put_str(TERM_L_BLUE, stats, 5, 40);

        put_str("HD", 6, 40);
        const auto hd = format("%+3d", cp_ptr->c_mhp);
        c_put_str(TERM_L_BLUE, hd, 6, 42);

        put_str(_("隠密", "Stealth"), 6, 47);
        std::string stealth;
        if (i2enum<PlayerClassType>(cs) == PlayerClassType::BERSERKER) {
            stealth = " xx";
        } else {
            stealth = format(" %+2d", cp_ptr->c_stl);
        }
        c_put_str(TERM_L_BLUE, stealth, 6, _(51, 54));
    }

    c_put_str(TERM_YELLOW, result, 13 + (cs / 4), 2 + 19 * (cs % 4));
    *os = cs;
    return result;
}

static bool interpret_class_select_key_move(char c, int *cs)
{
    if (c == '8') {
        if (*cs >= 4) {
            *cs -= 4;
        }
        return true;
    }

    if (c == '4') {
        if (*cs > 0) {
            (*cs)--;
        }
        return true;
    }

    if (c == '6') {
        if (*cs < PLAYER_CLASS_TYPE_MAX) {
            (*cs)++;
        }
        return true;
    }

    if (c == '2') {
        if (*cs + 4 <= PLAYER_CLASS_TYPE_MAX) {
            *cs += 4;
        }
        return true;
    }

    return false;
}

static bool select_class(PlayerType *player_ptr, concptr sym, int *k)
{
    auto cs = player_ptr->pclass;
    auto os = PlayerClassType::MAX;
    int int_os = enum2i(os);
    auto cur = birth_class_label(int_os, sym);
    while (true) {
        int int_cs = enum2i(cs);
        cur = display_class_stat(int_cs, &int_os, cur, sym);
        if (*k >= 0) {
            break;
        }

        const auto buf = format(_("職業を選んで下さい (%c-%c) ('='初期オプション設定, 灰色:勝利済): ", "Choose a class (%c-%c) ('=' for options, Gray is winner): "), sym[0], sym[PLAYER_CLASS_TYPE_MAX - 1]);

        put_str(buf, 10, 6);
        char c = inkey();
        if (c == 'Q') {
            birth_quit();
        }

        if (c == 'S') {
            return false;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (int_cs == enum2i(PlayerClassType::MAX)) {
                *k = randint0(PLAYER_CLASS_TYPE_MAX);
                cs = i2enum<PlayerClassType>(*k);
                continue;
            } else {
                *k = int_cs;
                break;
            }
        }

        if (interpret_class_select_key_move(c, &int_cs)) {
            cs = i2enum<PlayerClassType>(int_cs);
            continue;
        }

        if (c == '*') {
            *k = randint0(PLAYER_CLASS_TYPE_MAX);
            cs = i2enum<PlayerClassType>(*k);
            continue;
        }

        *k = (islower(c) ? A2I(c) : -1);
        if ((*k >= 0) && (*k < PLAYER_CLASS_TYPE_MAX)) {
            cs = i2enum<PlayerClassType>(*k);
            continue;
        }

        *k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((*k >= 26) && (*k < PLAYER_CLASS_TYPE_MAX)) {
            cs = i2enum<PlayerClassType>(*k);
            continue;
        } else {
            *k = -1;
        }

        birth_help_option(player_ptr, c, BirthKind::CLASS);
    }

    return true;
}

/*!
 * @brief プレイヤーの職業選択を行う / Player class
 */
bool get_player_class(PlayerType *player_ptr)
{
    clear_from(10);
    put_str(
        _("注意：《職業》によってキャラクターの先天的な能力やボーナスが変化します。", "Note: Your 'class' determines various intrinsic abilities and bonuses."),
        23, 5);
    put_str(_("()で囲まれた選択肢はこの種族には似合わない職業です。", "Any entries in parentheses should only be used by advanced players."), 11, 5);
    put_str("                                   ", 6, 40);

    char sym[PLAYER_CLASS_TYPE_MAX];
    enumerate_class_list(sym);

    int k = -1;
    if (!select_class(player_ptr, sym, &k)) {
        return false;
    }

    player_ptr->pclass = i2enum<PlayerClassType>(k);
    cp_ptr = &class_info.at(player_ptr->pclass);
    mp_ptr = &class_magics_info[enum2i(player_ptr->pclass)];
    c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 15);
    return true;
}
