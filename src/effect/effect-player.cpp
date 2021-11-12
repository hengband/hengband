/*!
 * @brief 魔法によるプレイヤーへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-player.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-player-switcher.h"
#include "effect/effect-player-util.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-hex.h"
#include "effect/attribute-types.h"
#include "spell/spells-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <string>

/*!
 * @brief effect_player_type構造体を初期化する
 * @param ep_ptr 初期化前の構造体
 * @param who 魔法を唱えたモンスター (0ならプレイヤー自身)
 * @param dam 基本威力
 * @param attribute 効果属性
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 初期化後の構造体ポインタ
 */
static effect_player_type *initialize_effect_player(effect_player_type *ep_ptr, MONSTER_IDX who, HIT_POINT dam, AttributeType attribute, BIT_FLAGS flag)
{
    ep_ptr->rlev = 0;
    ep_ptr->m_ptr = nullptr;
    ep_ptr->get_damage = 0;
    ep_ptr->who = who;
    ep_ptr->dam = dam;
    ep_ptr->attribute = attribute;
    ep_ptr->flag = flag;
    return ep_ptr;
}

/*!
 * @brief ボルト魔法を反射する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @return 当たったらFALSE、反射したらTRUE
 */
static bool process_bolt_reflection(PlayerType *player_ptr, effect_player_type *ep_ptr, project_func project)
{
    auto can_reflect = (has_reflect(player_ptr) != 0);
    can_reflect &= any_bits(ep_ptr->flag, PROJECT_REFLECTABLE);
    can_reflect &= !one_in_(10);
    if (!can_reflect) {
        return false;
    }

    auto max_attempts = 10;
    sound(SOUND_REFLECT);

    std::string mes;
    if (player_ptr->blind) {
        mes = _("何かが跳ね返った！", "Something bounces!");
    } else if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::FUUJIN)) {
        mes = _("風の如く武器を振るって弾き返した！", "The attack bounces!");
    } else {
        mes = _("攻撃が跳ね返った！", "The attack bounces!");
    }

    msg_print(mes.data());
    POSITION t_y;
    POSITION t_x;
    if (ep_ptr->who > 0) {
        auto *floor_ptr = player_ptr->current_floor_ptr;
        auto *m_ptr = &floor_ptr->m_list[ep_ptr->who];
        do {
            t_y = m_ptr->fy - 1 + randint1(3);
            t_x = m_ptr->fx - 1 + randint1(3);
            max_attempts--;
        } while (max_attempts && in_bounds2u(floor_ptr, t_y, t_x) && !projectable(player_ptr, player_ptr->y, player_ptr->x, t_y, t_x));

        if (max_attempts < 1) {
            t_y = m_ptr->fy;
            t_x = m_ptr->fx;
        }
    } else {
        t_y = player_ptr->y - 1 + randint1(3);
        t_x = player_ptr->x - 1 + randint1(3);
    }

    (*project)(player_ptr, 0, 0, t_y, t_x, ep_ptr->dam, ep_ptr->attribute, (PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE));
    disturb(player_ptr, true, true);
    return true;
}

/*!
 * @brief 反射・忍者の変わり身などでそもそも当たらない状況を判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @param y 目標Y座標
 * @param x 目標X座標
 * @return 当たらなかったらFALSE、反射したらTRUE、当たったらCONTINUE
 */
static process_result check_continue_player_effect(PlayerType *player_ptr, effect_player_type *ep_ptr, POSITION y, POSITION x, project_func project)
{
    if (!player_bold(player_ptr, y, x)) {
        return PROCESS_FALSE;
    }

    auto is_effective = ep_ptr->dam > 0;
    is_effective &= randint0(55) < (player_ptr->lev * 3 / 5 + 20);
    is_effective &= ep_ptr->who > 0;
    is_effective &= ep_ptr->who != player_ptr->riding;
    if (is_effective && kawarimi(player_ptr, true)) {
        return PROCESS_FALSE;
    }

    if ((ep_ptr->who == 0) || (ep_ptr->who == player_ptr->riding)) {
        return PROCESS_FALSE;
    }

    if (process_bolt_reflection(player_ptr, ep_ptr, project)) {
        return PROCESS_TRUE;
    }

    return PROCESS_CONTINUE;
}

/*!
 * @brief 魔法を発したモンスター名を記述する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @param who_name モンスター名
 */
static void describe_effect_source(PlayerType *player_ptr, effect_player_type *ep_ptr, concptr who_name)
{
    if (ep_ptr->who > 0) {
        ep_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[ep_ptr->who];
        ep_ptr->rlev = (&r_info[ep_ptr->m_ptr->r_idx])->level >= 1 ? (&r_info[ep_ptr->m_ptr->r_idx])->level : 1;
        monster_desc(player_ptr, ep_ptr->m_name, ep_ptr->m_ptr, 0);
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
 * @param attribute 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_player(MONSTER_IDX who, PlayerType *player_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, AttributeType attribute,
    BIT_FLAGS flag, project_func project)
{
    effect_player_type tmp_effect;
    auto *ep_ptr = initialize_effect_player(&tmp_effect, who, dam, attribute, flag);
    auto check_result = check_continue_player_effect(player_ptr, ep_ptr, y, x, project);
    if (check_result != PROCESS_CONTINUE) {
        return check_result == PROCESS_TRUE;
    }

    if (ep_ptr->dam > 1600) {
        ep_ptr->dam = 1600;
    }

    ep_ptr->dam = (ep_ptr->dam + r) / (r + 1);
    describe_effect_source(player_ptr, ep_ptr, who_name);
    switch_effects_player(player_ptr, ep_ptr);

    SpellHex(player_ptr).store_vengeful_damage(ep_ptr->get_damage);
    if ((player_ptr->tim_eyeeye || SpellHex(player_ptr).is_spelling_specific(HEX_EYE_FOR_EYE)) && (ep_ptr->get_damage > 0) && !player_ptr->is_dead && (ep_ptr->who > 0)) {
        GAME_TEXT m_name_self[MAX_MONSTER_NAME];
        monster_desc(player_ptr, m_name_self, ep_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
        msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), ep_ptr->m_name, m_name_self);
        (*project)(player_ptr, 0, 0, ep_ptr->m_ptr->fy, ep_ptr->m_ptr->fx, ep_ptr->get_damage, AttributeType::MISSILE, PROJECT_KILL);
        if (player_ptr->tim_eyeeye) {
            set_tim_eyeeye(player_ptr, player_ptr->tim_eyeeye - 5, true);
        }
    }

    if (player_ptr->riding && ep_ptr->dam > 0) {
        rakubadam_p = (ep_ptr->dam > 200) ? 200 : ep_ptr->dam;
    }

    disturb(player_ptr, true, true);
    if (ep_ptr->dam && ep_ptr->who && (ep_ptr->who != player_ptr->riding)) {
        (void)kawarimi(player_ptr, false);
    }

    return true;
}
