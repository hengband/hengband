/*!
 * @brief 魔法によるプレーヤーへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-player.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-player-switcher.h"
#include "effect/effect-player-util.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

typedef enum effect_player_check_result {
    EP_CHECK_FALSE = 0,
    EP_CHECK_TRUE = 1,
    EP_CHECK_CONTINUE = 2,
} ep_check_result;

/*!
 * @brief effect_player_type構造体を初期化する
 * @param ep_ptr 初期化前の構造体
 * @param who 魔法を唱えたモンスター (0ならプレーヤー自身)
 * @param dam 基本威力
 * @param effect_type 効果属性
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 初期化後の構造体ポインタ
 */
static effect_player_type *initialize_effect_player(
    effect_player_type *ep_ptr, MONSTER_IDX who, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, int monspell)
{
    ep_ptr->rlev = 0;
    ep_ptr->m_ptr = NULL;
    ep_ptr->get_damage = 0;
    ep_ptr->who = who;
    ep_ptr->dam = dam;
    ep_ptr->effect_type = effect_type;
    ep_ptr->flag = flag;
    ep_ptr->monspell = monspell;
    return ep_ptr;
}

/*!
 * @brief ボルト魔法を反射する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ep_ptr プレーヤー効果構造体への参照ポインタ
 * @return 当たったらFALSE、反射したらTRUE
 */
static bool process_bolt_reflection(player_type *target_ptr, effect_player_type *ep_ptr, project_func project)
{
    bool can_bolt_hit = target_ptr->reflect || (((target_ptr->special_defense & KATA_FUUJIN) != 0) && !target_ptr->blind);
    can_bolt_hit &= (ep_ptr->flag & PROJECT_REFLECTABLE) != 0;
    can_bolt_hit &= !one_in_(10);
    if (!can_bolt_hit)
        return FALSE;

    POSITION t_y, t_x;
    int max_attempts = 10;
    sound(SOUND_REFLECT);

    if (target_ptr->blind)
        msg_print(_("何かが跳ね返った！", "Something bounces!"));
    else if (target_ptr->special_defense & KATA_FUUJIN)
        msg_print(_("風の如く武器を振るって弾き返した！", "The attack bounces!"));
    else
        msg_print(_("攻撃が跳ね返った！", "The attack bounces!"));

    if (ep_ptr->who > 0) {
        floor_type *floor_ptr = target_ptr->current_floor_ptr;
        monster_type m_type = floor_ptr->m_list[ep_ptr->who];
        do {
            t_y = m_type.fy - 1 + randint1(3);
            t_x = m_type.fx - 1 + randint1(3);
            max_attempts--;
        } while (max_attempts && in_bounds2u(floor_ptr, t_y, t_x) && !projectable(target_ptr, target_ptr->y, target_ptr->x, t_y, t_x));

        if (max_attempts < 1) {
            t_y = m_type.fy;
            t_x = m_type.fx;
        }
    } else {
        t_y = target_ptr->y - 1 + randint1(3);
        t_x = target_ptr->x - 1 + randint1(3);
    }

    (*project)(target_ptr, 0, 0, t_y, t_x, ep_ptr->dam, ep_ptr->effect_type, (PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE), ep_ptr->monspell);
    disturb(target_ptr, TRUE, TRUE);
    return TRUE;
}

/*!
 * @brief 反射・忍者の変わり身などでそもそも当たらない状況を判定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ep_ptr プレーヤー効果構造体への参照ポインタ
 * @param y 目標Y座標
 * @param x 目標X座標
 * @return 当たらなかったらFALSE、反射したらTRUE、当たったらCONTINUE
 */
static ep_check_result check_continue_player_effect(player_type *target_ptr, effect_player_type *ep_ptr, POSITION y, POSITION x, project_func project)
{
    if (!player_bold(target_ptr, y, x))
        return EP_CHECK_FALSE;

    if (((target_ptr->special_defense & NINJA_KAWARIMI) != 0) && (ep_ptr->dam > 0) && (randint0(55) < (target_ptr->lev * 3 / 5 + 20)) && (ep_ptr->who > 0)
        && (ep_ptr->who != target_ptr->riding) && kawarimi(target_ptr, TRUE))
        return EP_CHECK_FALSE;

    if ((ep_ptr->who == 0) || (ep_ptr->who == target_ptr->riding))
        return EP_CHECK_FALSE;

    if (process_bolt_reflection(target_ptr, ep_ptr, project))
        return EP_CHECK_TRUE;

    return EP_CHECK_CONTINUE;
}

/*!
 * @brief 魔法を発したモンスター名を記述する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ep_ptr プレーヤー効果構造体への参照ポインタ
 * @param who_name モンスター名
 * @return なし
 */
static void describe_effect_source(player_type *target_ptr, effect_player_type *ep_ptr, concptr who_name)
{
    if (ep_ptr->who > 0) {
        ep_ptr->m_ptr = &target_ptr->current_floor_ptr->m_list[ep_ptr->who];
        ep_ptr->rlev = (&r_info[ep_ptr->m_ptr->r_idx])->level >= 1 ? (&r_info[ep_ptr->m_ptr->r_idx])->level : 1;
        monster_desc(target_ptr, ep_ptr->m_name, ep_ptr->m_ptr, 0);
        strcpy(ep_ptr->killer, who_name);
        return;
    }

    switch (ep_ptr->who) {
    case PROJECT_WHO_UNCTRL_POWER:
        strcpy(ep_ptr->killer, _("制御できない力の氾流", "uncontrollable power storm"));
        break;
    case PROJECT_WHO_GLASS_SHARDS:
        strcpy(ep_ptr->killer, _("ガラスの破片", "shards of glass"));
        break;
    default:
        strcpy(ep_ptr->killer, _("罠", "a trap"));
        break;
    }

    strcpy(ep_ptr->m_name, ep_ptr->killer);
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるプレイヤーへの効果処理 / Helper function for "project()" below.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー、負値ならば自然発生) / Index of "source" monster (zero for "player")
 * @param who_name 効果を起こしたモンスターの名前
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param effect_type 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_player(MONSTER_IDX who, player_type *target_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type,
    BIT_FLAGS flag, int monspell, project_func project)
{
    effect_player_type tmp_effect;
    effect_player_type *ep_ptr = initialize_effect_player(&tmp_effect, who, dam, effect_type, flag, monspell);
    ep_check_result check_result = check_continue_player_effect(target_ptr, ep_ptr, y, x, project);
    if (check_result != EP_CHECK_CONTINUE)
        return check_result;

    if (ep_ptr->dam > 1600)
        ep_ptr->dam = 1600;

    ep_ptr->dam = (ep_ptr->dam + r) / (r + 1);
    describe_effect_source(target_ptr, ep_ptr, who_name);
    switch_effects_player(target_ptr, ep_ptr);

    revenge_store(target_ptr, ep_ptr->get_damage);
    if ((target_ptr->tim_eyeeye || hex_spelling(target_ptr, HEX_EYE_FOR_EYE)) && (ep_ptr->get_damage > 0) && !target_ptr->is_dead && (ep_ptr->who > 0)) {
        GAME_TEXT m_name_self[80];
        monster_desc(target_ptr, m_name_self, ep_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
        msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), ep_ptr->m_name, m_name_self);
        (*project)(target_ptr, 0, 0, ep_ptr->m_ptr->fy, ep_ptr->m_ptr->fx, ep_ptr->get_damage, GF_MISSILE, PROJECT_KILL, -1);
        if (target_ptr->tim_eyeeye)
            set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, TRUE);
    }

    if (target_ptr->riding && ep_ptr->dam > 0) {
        rakubadam_p = (ep_ptr->dam > 200) ? 200 : ep_ptr->dam;
    }

    disturb(target_ptr, TRUE, TRUE);
    if ((target_ptr->special_defense & NINJA_KAWARIMI) && ep_ptr->dam && ep_ptr->who && (ep_ptr->who != target_ptr->riding)) {
        (void)kawarimi(target_ptr, FALSE);
    }

    return TRUE;
}
