#include "timed-effect/player-stun.h"
#include "system/angband.h"

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

short PlayerStun::get_accumulation(int rank)
{
    switch (rank) {
    case 0:
        return 0;
    case 1:
        return randint1(5);
    case 2:
        return randint1(5) + 10;
    case 3:
        return randint1(10) + 20;
    case 4:
        return randint1(15) + 30;
    case 5:
        return randint1(20) + 40;
    case 6:
        return 80;
    default:
        return 150;
    }
}

/*!
 * @brief モンスター打撃の朦朧蓄積ランクを返す.
 * @param total 痛恨の一撃でない場合の最大ダメージ (ダイスXdY に対し、X*Y)
 * @param dam プレイヤーに与えた実際のダメージ
 * @return 朦朧蓄積ランク
 */
int PlayerStun::get_accumulation_rank(int total, int damage)
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

short PlayerStun::current() const
{
    return this->stun;
}

PlayerStunRank PlayerStun::get_rank() const
{
    return this->get_rank(this->stun);
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
