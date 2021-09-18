#include "timed-effect/player-stun.h"
#include "system/angband.h"

short PlayerStun::current() const
{
    return this->stun;
}

PlayerStunRank PlayerStun::get_rank() const
{
    return this->get_rank(this->stun);
}

PlayerStunRank PlayerStun::get_rank(short value)
{
    if (value > 100) {
        return PlayerStunRank::UNCONSCIOUS;
    }

    if (value > 50) {
        return PlayerStunRank::HARD;
    }

    if (value > 0) {
        return PlayerStunRank::NORMAL;
    }

    return PlayerStunRank::NONE;
}

std::string_view PlayerStun::get_stun_mes(PlayerStunRank stun_rank)
{
    switch (stun_rank) {
    case PlayerStunRank::NONE:
        return "";
    case PlayerStunRank::NORMAL:
        return _("意識がもうろうとしてきた。", "You have been stunned.");
    case PlayerStunRank::HARD:
        return _("意識がひどくもうろうとしてきた。", "You have been heavily stunned.");
    case PlayerStunRank::UNCONSCIOUS:
        return _("頭がクラクラして意識が遠のいてきた。", "You have been knocked out.");
    default:
        throw("Invalid StunRank was specified!");
    }
}

/*!
 * @brief 朦朧ランクに応じて各種失率を上げる.
 * @return 朦朧ならば15%、ひどく朦朧ならば25%.
 * @details
 * 意識不明瞭ならばそもそも動けないのでこのメソッドを通らない.
 * しかし今後の拡張を考慮して100%としておく.
 */
int PlayerStun::get_chance_penalty() const
{
    switch (this->get_rank()) {
    case PlayerStunRank::NONE:
        return 0;
    case PlayerStunRank::NORMAL:
        return 15;
    case PlayerStunRank::HARD:
        return 25;
    case PlayerStunRank::UNCONSCIOUS:
        return 100;
    default:
        throw("Invalid stun rank is specified!");
    }
}

/*!
 * @brief 朦朧ランクに応じてダメージ量 or 命中率を下げる.
 * @return 朦朧ならば5、ひどく朦朧ならば20.
 * @details
 * 呼び出し元で減算しているのでこのメソッドでは正値.
 * 意識不明瞭ならばそもそも動けないのでこのメソッドを通らない.
 * しかし今後の拡張を考慮して100としておく.
 */
short PlayerStun::get_damage_penalty() const
{
    switch (this->get_rank()) {
    case PlayerStunRank::NONE:
        return 0;
    case PlayerStunRank::NORMAL:
        return 5;
    case PlayerStunRank::HARD:
        return 20;
    case PlayerStunRank::UNCONSCIOUS:
        return 100;
    default:
        throw("Invalid stun rank is specified!");
    }
}

bool PlayerStun::is_stunned() const
{
    return this->get_rank() > PlayerStunRank::NONE;
}

std::tuple<term_color_type, std::string_view> PlayerStun::get_expr() const
{
    switch (this->get_rank()) {
    case PlayerStunRank::NONE: // dummy.
        return std::make_tuple(TERM_WHITE, "            ");
    case PlayerStunRank::NORMAL:
        return std::make_tuple(TERM_ORANGE, _("朦朧        ", "Stun        "));
    case PlayerStunRank::HARD:
        return std::make_tuple(TERM_ORANGE, _("ひどく朦朧  ", "Heavy stun  "));
    case PlayerStunRank::UNCONSCIOUS:
        return std::make_tuple(TERM_RED, _("意識不明瞭  ", "Knocked out "));
    default:
        throw("Invalid stun rank is specified!");
    }
}

void PlayerStun::set(short value)
{
    this->stun = value;
}

void PlayerStun::reset()
{
    this->set(0);
}
