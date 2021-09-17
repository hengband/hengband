#include "timed-effect/player-stun.h"
#include "locale/language-switcher.h"

short PlayerStun::current() const
{
    return this->stun;
}

StunRank PlayerStun::get_rank() const
{
    return this->get_rank(this->stun);
}

StunRank PlayerStun::get_rank(short value) const
{
    if (value > 100) {
        return StunRank::UNCONSCIOUS;
    }

    if (value > 50) {
        return StunRank::HARD;
    }

    if (value > 0) {
        return StunRank::NORMAL;
    }

    return StunRank::NONE;
}

std::string_view PlayerStun::get_stun_mes(StunRank stun_rank) const
{
    switch (stun_rank) {
    case StunRank::NONE:
        return "";
    case StunRank::NORMAL:
        return _("意識がもうろうとしてきた。", "You have been stunned.");
    case StunRank::HARD:
        return _("意識がひどくもうろうとしてきた。", "You have been heavily stunned.");
    case StunRank::UNCONSCIOUS:
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
int PlayerStun::decrease_chance() const
{
    switch (this->get_rank()) {
    case StunRank::NONE:
        return 0;
    case StunRank::NORMAL:
        return 15;
    case StunRank::HARD:
        return 25;
    case StunRank::UNCONSCIOUS:
        return 100;
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
