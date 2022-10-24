#include "object-activation/activation-resistance.h"
#include "effect/attribute-types.h"
#include "hpmp/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/temporary-resistance.h"
#include "sv-definition/sv-ring-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_resistance_elements(PlayerType *player_ptr)
{
    msg_print(_("様々な色に輝いている...", "It glows many colours..."));
    (void)set_oppose_acid(player_ptr, randint1(40) + 40, false);
    (void)set_oppose_elec(player_ptr, randint1(40) + 40, false);
    (void)set_oppose_fire(player_ptr, randint1(40) + 40, false);
    (void)set_oppose_cold(player_ptr, randint1(40) + 40, false);
    (void)set_oppose_pois(player_ptr, randint1(40) + 40, false);
    return true;
}

/*!
 * @brief 酸属性のボールを放ち、酸の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_acid_ball_and_resistance(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);

    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::ACID, dir, 100, 2);
    (void)set_oppose_acid(player_ptr, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 電撃属性のボールを放ち、電撃の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_elec_ball_and_resistance(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);

    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::ELEC, dir, 100, 2);
    (void)set_oppose_elec(player_ptr, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 火炎属性のボールを放ち、火炎の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_fire_ball_and_resistance(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);

    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::FIRE, dir, 100, 2);
    (void)set_oppose_fire(player_ptr, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 冷気属性のボールを放ち、冷気の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_cold_ball_and_resistance(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが白く輝いた...", "The %s grows white."), name);

    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::COLD, dir, 100, 2);
    (void)set_oppose_cold(player_ptr, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 毒属性のボールを放ち、毒の一時耐性を得る
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_pois_ball_and_resistance(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);

    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::POIS, dir, 100, 2);
    (void)set_oppose_pois(player_ptr, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 酸の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_acid(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);
    (void)set_oppose_acid(player_ptr, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 電撃の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_elec(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);
    (void)set_oppose_elec(player_ptr, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 火炎の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_fire(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);
    (void)set_oppose_fire(player_ptr, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 冷気の一時耐性を得る。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_cold(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが白く輝いた...", "The %s grows white."), name);
    (void)set_oppose_cold(player_ptr, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 毒の一時耐性を得る
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_pois(PlayerType *player_ptr, concptr name)
{
    msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);
    (void)set_oppose_pois(player_ptr, randint1(20) + 20, false);
    return true;
}

bool activate_ultimate_resistance(PlayerType *player_ptr)
{
    TIME_EFFECT v = randint1(25) + 25;
    (void)BadStatusSetter(player_ptr).set_fear(0);
    (void)set_hero(player_ptr, v, false);
    (void)hp_player(player_ptr, 10);
    (void)set_blessed(player_ptr, v, false);
    (void)set_oppose_acid(player_ptr, v, false);
    (void)set_oppose_elec(player_ptr, v, false);
    (void)set_oppose_fire(player_ptr, v, false);
    (void)set_oppose_cold(player_ptr, v, false);
    (void)set_oppose_pois(player_ptr, v, false);
    (void)set_ultimate_res(player_ptr, v, false);
    return true;
}
