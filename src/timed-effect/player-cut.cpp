#include "timed-effect/player-cut.h"
#include "system/angband.h"

PlayerCutRank PlayerCut::get_rank(short value)
{
    if (value > 1000) {
        return PlayerCutRank::MORTAL;
    }

    if (value > 200) {
        return PlayerCutRank::DEEP;
    }

    if (value > 100) {
        return PlayerCutRank::SEVERE;
    }

    if (value > 50) {
        return PlayerCutRank::NASTY;
    }

    if (value > 25) {
        return PlayerCutRank::BAD;
    }

    if (value > 10) {
        return PlayerCutRank::LIGHT;
    }

    if (value > 0) {
        return PlayerCutRank::GRAZING;
    }

    return PlayerCutRank::NONE;
}

std::string_view PlayerCut::get_cut_mes(PlayerCutRank stun_rank)
{
    switch (stun_rank) {
    case PlayerCutRank::NONE:
        return "";
    case PlayerCutRank::GRAZING:
        return _("かすり傷を負ってしまった。", "You have been given a graze.");
    case PlayerCutRank::LIGHT:
        return _("軽い傷を負ってしまった。", "You have been given a light cut.");
    case PlayerCutRank::BAD:
        return _("ひどい傷を負ってしまった。", "You have been given a bad cut.");
    case PlayerCutRank::NASTY:
        return _("大変な傷を負ってしまった。", "You have been given a nasty cut.");
    case PlayerCutRank::SEVERE:
        return _("重大な傷を負ってしまった。", "You have been given a severe cut.");
    case PlayerCutRank::DEEP:
        return _("ひどい深手を負ってしまった。", "You have been given a deep gash.");
    case PlayerCutRank::MORTAL:
        return _("致命的な傷を負ってしまった。", "You have been given a mortal wound.");
    default:
        throw("Invalid StunRank was specified!");
    }
}

/*!
 * @brief モンスター打撃による切り傷値を返す.
 * @param total 痛恨の一撃でない場合の最大ダメージ (ダイスXdY に対し、X*Y)
 * @param dam プレイヤーに与えた実際のダメージ
 * @return 切り傷値
 */
short PlayerCut::get_accumulation(int total, int damage)
{
    auto rank = get_accumulation_rank(total, damage);
    switch (rank) {
    case 0:
        return 0;
    case 1:
        return randint1(5);
    case 2:
        return randint1(5) + 5;
    case 3:
        return randint1(20) + 20;
    case 4:
        return randint1(50) + 50;
    case 5:
        return randint1(100) + 100;
    case 6:
        return 300;
    default: // 7 or more.
        return 500;
    }
}

short PlayerCut::current() const
{
    return this->cut;
}

PlayerCutRank PlayerCut::get_rank() const
{
    return this->get_rank(this->cut);
}

bool PlayerCut::is_cut() const
{
    return this->cut > 0;
}

std::tuple<term_color_type, std::string_view> PlayerCut::get_expr() const
{
    switch (this->get_rank()) {
    case PlayerCutRank::NONE: // dummy.
        return std::make_tuple(TERM_WHITE, "");
    case PlayerCutRank::GRAZING:
        return std::make_tuple(TERM_YELLOW, _("かすり傷    ", "Graze       "));
    case PlayerCutRank::LIGHT:
        return std::make_tuple(TERM_YELLOW, _("軽傷        ", "Light cut   "));
    case PlayerCutRank::BAD:
        return std::make_tuple(TERM_ORANGE, _("ひどい傷    ", "Bad cut     "));
    case PlayerCutRank::NASTY:
        return std::make_tuple(TERM_ORANGE, _("大変な傷    ", "Nasty cut   "));
    case PlayerCutRank::SEVERE:
        return std::make_tuple(TERM_RED, _("重傷        ", "Severe cut  "));
    case PlayerCutRank::DEEP:
        return std::make_tuple(TERM_RED, _("ひどい深手  ", "Deep gash   "));
    case PlayerCutRank::MORTAL:
        return std::make_tuple(TERM_L_RED, _("致命傷      ", "Mortal wound"));
    default:
        throw("Invalid StunRank was specified!");
    }
}

int PlayerCut::get_damage() const
{
    switch (this->get_rank()) {
    case PlayerCutRank::NONE:
        return 0;
    case PlayerCutRank::GRAZING:
        return 1;
    case PlayerCutRank::LIGHT:
        return 3;
    case PlayerCutRank::BAD:
        return 7;
    case PlayerCutRank::NASTY:
        return 16;
    case PlayerCutRank::SEVERE:
        return 32;
    case PlayerCutRank::DEEP:
        return 80;
    case PlayerCutRank::MORTAL:
        return 200;
    default:
        throw("Invalid StunRank was specified!");
    }
}

void PlayerCut::set(short value)
{
    this->cut = value;
}

void PlayerCut::reset()
{
    this->set(0);
}

/*!
 * @brief モンスター打撃の切り傷蓄積ランクを返す.
 * @param total 痛恨の一撃でない場合の最大ダメージ (ダイスXdY に対し、X*Y)
 * @param dam プレイヤーに与えた実際のダメージ
 * @return 切り傷蓄積ランク
 * @details v3.0.0 Alpha40時点では朦朧と異なりv2.2.1までの仕様と同一
 */
int PlayerCut::get_accumulation_rank(int total, int damage)
{
    if (damage < total * 19 / 20) {
        return 0;
    }

    if ((damage < 20) && (damage <= randint0(100))) {
        return 0;
    }

    auto max = 0;
    if ((damage >= total) && (damage >= 40)) {
        max++;
    }

    if (damage >= 20) {
        while (randint0(100) < 2) {
            max++;
        }
    }

    if (damage > 45) {
        return (6 + max);
    }

    if (damage > 33) {
        return (5 + max);
    }

    if (damage > 25) {
        return (4 + max);
    }

    if (damage > 18) {
        return (3 + max);
    }

    if (damage > 11) {
        return (2 + max);
    }

    return (1 + max);
}
