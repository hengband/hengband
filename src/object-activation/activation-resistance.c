#include "object-activation/activation-resistance.h"
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
 * @brief アシッド・ボールを放ち、更に酸耐性を得る
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 発動対象アイテムへの参照ポインタ
 * @param name アイテム名
 * @return ボールを当てられるならばTRUE
 */
bool activate_resistance_acid(player_type *user_ptr, object_type *o_ptr, concptr name)
{
    msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);
    if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_ACID))
        return TRUE;

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_ACID, dir, 100, 2);
    (void)set_oppose_acid(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * @brief サンダー・ボールを放ち、更に電撃耐性を得る
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 発動対象アイテムへの参照ポインタ
 * @param name アイテム名
 * @return ボールを当てられるならばTRUE
 */
bool activate_resistance_elec(player_type *user_ptr, object_type *o_ptr, concptr name)
{
    msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);
    if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_ELEC))
        return TRUE;

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_ELEC, dir, 100, 2);
    (void)set_oppose_elec(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * @brief ファイア・ボールを放ち、更に火炎耐性を得る
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 発動対象アイテムへの参照ポインタ
 * @param name アイテム名
 * @return ボールを当てられるならばTRUE
 */
bool activate_resistance_fire(player_type *user_ptr, object_type *o_ptr, concptr name)
{
    msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);
    if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_FLAMES))
        return TRUE;

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_FIRE, dir, 100, 2);
    (void)set_oppose_fire(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * @brief アイス・ボールを放ち、更に冷気耐性を得る
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 発動対象アイテムへの参照ポインタ
 * @param name アイテム名
 * @return ボールを当てられるならばTRUE
 */
bool activate_resistance_cold(player_type *user_ptr, object_type *o_ptr, concptr name)
{
    msg_format(_("%sが白く輝いた...", "The %s grows white."), name);
    if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_ICE))
        return TRUE;

    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_COLD, dir, 100, 2);
    (void)set_oppose_cold(user_ptr, randint1(20) + 20, FALSE);
    return TRUE;
}

/*!
 * todo 何か追加効果が欲しい
 * @brief 毒耐性を得る
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
