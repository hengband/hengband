#include "timed-effect/player-stun.h"
#include "system/angband.h"

enum class PlayerStunRank {
    NONE = 0,
    SLIGHT = 1,
    NORMAL = 2,
    HARD = 3,
    UNCONSCIOUS = 4,
    KNOCKED = 5,
};

PlayerStunRank PlayerStun::get_rank(short value)
{
    if (value > 200) {
        return PlayerStunRank::KNOCKED;
    }

    if (value > 150) {
        return PlayerStunRank::UNCONSCIOUS;
    }

    if (value > 100) {
        return PlayerStunRank::HARD;
    }

    if (value > 50) {
        return PlayerStunRank::NORMAL;
    }

    if (value > 0) {
        return PlayerStunRank::SLIGHT;
    }

    return PlayerStunRank::NONE;
}

std::string_view PlayerStun::get_stun_mes(PlayerStunRank stun_rank)
{
    switch (stun_rank) {
    case PlayerStunRank::NONE:
        return "";
    case PlayerStunRank::SLIGHT:
        return _("意識が少しもうろうとしてきた。", "You have been slightly stunned.");
    case PlayerStunRank::NORMAL:
        return _("意識がもうろうとしてきた。", "You have been stunned.");
    case PlayerStunRank::HARD:
        return _("意識がひどくもうろうとしてきた。", "You have been heavily stunned.");
    case PlayerStunRank::UNCONSCIOUS:
        return _("頭がクラクラして意識が遠のいてきた。", "You have been unconcious.");
    case PlayerStunRank::KNOCKED:
        return _("あなたはぶっ倒れた！", "You are knocked out!!");
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
        return randint1(10);
    case 2:
        return randint1(10) + 10;
    case 3:
        return randint1(10) + 20;
    case 4:
        return randint1(10) + 30;
    case 5:
        return randint1(10) + 40;
    case 6:
        return randint1(10) + 50;
    case 7:
        return randint1(10) + 60;
    default: // 8 or more.
        return randint1(10) + 70;
    }
}

/*!
 * @brief モンスター打撃の朦朧蓄積ランクを返す.
 * @param total 痛恨の一撃でない場合の最大ダメージ
 * @param dam プレイヤーに与えた実際のダメージ
 * @return 朦朧蓄積ランク
 * @details
 * totalは、ダイスXdY に対し、X*Y
 * damageは、痛恨 かつ AC < 125 ならばtotalを超える可能性あり
 */
int PlayerStun::get_accumulation_rank(int total, int damage)
{
    auto is_no_stun = damage < total * 19 / 20;
    is_no_stun |= damage <= 20;
    if (is_no_stun) {
        return 0;
    }

    if (damage > 256) {
        return 8;
    }

    if (damage > 111) {
        return 7;
    }

    if (damage > 96) {
        return 6;
    }

    if (damage > 81) {
        return 5;
    }

    if (damage > 66) {
        return 4;
    }

    if (damage > 51) {
        return 3;
    }

    if (damage > 36) {
        return 2;
    }

    // damage > 21.
    return 1;
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
 * @brief 朦朧ランクに応じて魔法系の失率を上げる.
 * @return 失率
 * @details
 * 昏倒ならばそもそも動けないのでこのメソッドを通らない.
 * しかし今後の拡張を考慮して100%としておく.
 */
int PlayerStun::get_magic_chance_penalty() const
{
    switch (this->get_rank()) {
    case PlayerStunRank::NONE:
        return 0;
    case PlayerStunRank::SLIGHT:
        return 10;
    case PlayerStunRank::NORMAL:
        return 20;
    case PlayerStunRank::HARD:
        return 30;
    case PlayerStunRank::UNCONSCIOUS:
        return 50;
    case PlayerStunRank::KNOCKED:
        return 100;
    default:
        throw("Invalid stun rank is specified!");
    }
}

/*!
 * @brief 朦朧ランクに応じてアイテム使用の失率を上げる.
 * @return リファクタリング中の暫定的な値 (意識不明瞭ならば100%、それ以外は常に0%)
 * @details
 * 昏倒ならばそもそも動けないのでこのメソッドを通らない.
 * しかし今後の拡張を考慮して100%としておく.
 */
int PlayerStun::get_item_chance_penalty() const
{
    switch (this->get_rank()) {
    case PlayerStunRank::NONE:
    case PlayerStunRank::SLIGHT:
    case PlayerStunRank::NORMAL:
        return 0;
    case PlayerStunRank::HARD:
        return 5;
    case PlayerStunRank::UNCONSCIOUS:
        return 10;
    case PlayerStunRank::KNOCKED:
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
    case PlayerStunRank::SLIGHT:
        return 5;
    case PlayerStunRank::NORMAL:
        return 10;
    case PlayerStunRank::HARD:
        return 20;
    case PlayerStunRank::UNCONSCIOUS:
        return 40;
    case PlayerStunRank::KNOCKED:
        return 100;
    default:
        throw("Invalid stun rank is specified!");
    }
}

/*!
 * @brief プレイヤーが朦朧しているかを返す
 * @return 朦朧状態ならばtrue、頭がハッキリしているならばfalse
 */
bool PlayerStun::is_stunned() const
{
    return this->get_rank() > PlayerStunRank::NONE;
}

/*!
 * @brief プレイヤーが朦朧で行動不能かを返す
 * @return 昏倒状態ならばtrue、それ以外ならばfalse
 */
bool PlayerStun::is_knocked_out() const
{
    return this->get_rank() == PlayerStunRank::KNOCKED;
}

std::tuple<term_color_type, std::string_view> PlayerStun::get_expr() const
{
    switch (this->get_rank()) {
    case PlayerStunRank::NONE: // dummy.
        return std::make_tuple(TERM_WHITE, "            ");
    case PlayerStunRank::SLIGHT:
        return std::make_tuple(TERM_WHITE, _("やや朦朧    ", "Slight stun "));
    case PlayerStunRank::NORMAL:
        return std::make_tuple(TERM_YELLOW, _("朦朧        ", "Stun        "));
    case PlayerStunRank::HARD:
        return std::make_tuple(TERM_ORANGE, _("ひどく朦朧  ", "Heavy stun  "));
    case PlayerStunRank::UNCONSCIOUS:
        return std::make_tuple(TERM_RED, _("意識不明瞭  ", "Unconcious  "));
    case PlayerStunRank::KNOCKED:
        return std::make_tuple(TERM_VIOLET, _("昏倒        ", "Knocked out "));
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
