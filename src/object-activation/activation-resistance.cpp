﻿#include "object-activation/activation-resistance.h"
#include "core/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/temporary-resistance.h"
#include "sv-definition/sv-ring-types.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_resistance_elements(player_type *user_ptr)
{
    msg_print(_("様々な色に輝いている...", "It glows many colours..."));
    (void)set_oppose_acid(user_ptr, randint1(40) + 40, FALSE);
    (void)set_oppose_elec(user_ptr, randint1(40) + 40, FALSE);
    (void)set_oppose_fire(user_ptr, randint1(40) + 40, FALSE);
    (void)set_oppose_cold(user_ptr, randint1(40) + 40, FALSE);
    (void)set_oppose_pois(user_ptr, randint1(40) + 40, FALSE);
    return TRUE;
}

/*!
 * @brief 酸属性のボールを放ち、酸の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_acid_ball_and_resistance(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_ACID, dir, 100, 2);
    (void)set_oppose_acid(user_ptr, randint1(20) + 20, FALSE);

    return TRUE;
}

/*!
 * @brief 電撃属性のボールを放ち、電撃の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_elec_ball_and_resistance(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_ELEC, dir, 100, 2);
    (void)set_oppose_elec(user_ptr, randint1(20) + 20, FALSE);

    return TRUE;
}

/*!
 * @brief 火炎属性のボールを放ち、火炎の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_fire_ball_and_resistance(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_FIRE, dir, 100, 2);
    (void)set_oppose_fire(user_ptr, randint1(20) + 20, FALSE);

    return TRUE;
}

/*!
 * @brief 冷気属性のボールを放ち、冷気の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_cold_ball_and_resistance(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが白く輝いた...", "The %s grows white."), name);

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_COLD, dir, 100, 2);
    (void)set_oppose_cold(user_ptr, randint1(20) + 20, FALSE);

    return TRUE;
}

/*!
 * @brief 毒属性のボールを放ち、毒の一時耐性を得る
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_pois_ball_and_resistance(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_POIS, dir, 100, 2);
    (void)set_oppose_pois(user_ptr, randint1(20) + 20, FALSE);

    return TRUE;
}

/*!
 * @brief 酸の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_acid(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);
    (void)set_oppose_acid(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * @brief 電撃の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_elec(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);
    (void)set_oppose_elec(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * @brief 火炎の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_fire(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);
    (void)set_oppose_fire(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * @brief 冷気の一時耐性を得る。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_cold(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが白く輝いた...", "The %s grows white."), name);
    (void)set_oppose_cold(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * @brief 毒の一時耐性を得る
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_pois(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);
    (void)set_oppose_pois(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

bool activate_ultimate_resistance(player_type *user_ptr)
{
    TIME_EFFECT v = randint1(25) + 25;
    (void)set_afraid(user_ptr, 0);
    (void)set_hero(user_ptr, v, FALSE);
    (void)hp_player(user_ptr, 10);
    (void)set_blessed(user_ptr, v, FALSE);
    (void)set_oppose_acid(user_ptr, v, FALSE);
    (void)set_oppose_elec(user_ptr, v, FALSE);
    (void)set_oppose_fire(user_ptr, v, FALSE);
    (void)set_oppose_cold(user_ptr, v, FALSE);
    (void)set_oppose_pois(user_ptr, v, FALSE);
    (void)set_ultimate_res(user_ptr, v, FALSE);
    return TRUE;
}