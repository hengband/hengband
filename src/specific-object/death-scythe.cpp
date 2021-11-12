/*!
 * @brief 死の大鎌に特有の処理
 * @date 2020/05/23
 * @author Hourier
 * @details 現時点ではダメージ反射のみ。行数に注意して関数を追加しても良い.
 */

#include "specific-object/death-scythe.h"
#include "combat/attack-criticality.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player-attack/player-attack-util.h"
#include "player-info/race-info.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "status/element-resistance.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 死の大鎌ダメージが跳ね返ってきた時の、種族ごとのダメージ倍率を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 倍率 (実際は1/10になる)
 */
static int calc_death_scythe_reflection_magnification_mimic_none(PlayerType *player_ptr)
{
    switch (player_ptr->prace) {
    case PlayerRaceType::YEEK:
    case PlayerRaceType::KLACKON:
    case PlayerRaceType::HUMAN:
    case PlayerRaceType::AMBERITE:
    case PlayerRaceType::DUNADAN:
    case PlayerRaceType::BARBARIAN:
    case PlayerRaceType::BEASTMAN:
        return 25;
    case PlayerRaceType::HALF_ORC:
    case PlayerRaceType::HALF_TROLL:
    case PlayerRaceType::HALF_OGRE:
    case PlayerRaceType::HALF_GIANT:
    case PlayerRaceType::HALF_TITAN:
    case PlayerRaceType::CYCLOPS:
    case PlayerRaceType::IMP:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SPECTRE:
    case PlayerRaceType::BALROG:
    case PlayerRaceType::DRACONIAN:
        return 30;
    default:
        return 10;
    }
}

/*!
 * @brief 死の大鎌ダメージが跳ね返ってきた時の、変身中の種族も考慮したダメージ倍率を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 倍率 (実際は1/10になる)
 */
static int calc_death_scythe_reflection_magnification(PlayerType *player_ptr)
{
    switch (player_ptr->mimic_form) {
    case MIMIC_NONE:
        return calc_death_scythe_reflection_magnification_mimic_none(player_ptr);
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
    case MIMIC_VAMPIRE:
        return 30;
    default:
        return 10;
    }
}

/*!
 * @brief 耐性等に応じて死の大鎌による反射ダメージ倍率を補正する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param magnification ダメージ倍率
 * @param death_scythe_flags 死の大鎌に関するオブジェクトフラグ配列
 */
static void compensate_death_scythe_reflection_magnification(PlayerType *player_ptr, int *magnification, const TrFlags &death_scythe_flags)
{
    if ((player_ptr->alignment < 0) && (*magnification < 20))
        *magnification = 20;

    if (!(has_resist_acid(player_ptr) || is_oppose_acid(player_ptr) || has_immune_acid(player_ptr)) && (*magnification < 25))
        *magnification = 25;

    if (!(has_resist_elec(player_ptr) || is_oppose_elec(player_ptr) || has_immune_elec(player_ptr)) && (*magnification < 25))
        *magnification = 25;

    if (!(has_resist_fire(player_ptr) || is_oppose_fire(player_ptr) || has_immune_fire(player_ptr)) && (*magnification < 25))
        *magnification = 25;

    if (!(has_resist_cold(player_ptr) || is_oppose_cold(player_ptr) || has_immune_cold(player_ptr)) && (*magnification < 25))
        *magnification = 25;

    if (!(has_resist_pois(player_ptr) || is_oppose_pois(player_ptr)) && (*magnification < 25))
        *magnification = 25;

    if ((player_ptr->pclass != PlayerClassType::SAMURAI) && (death_scythe_flags.has(TR_FORCE_WEAPON)) && (player_ptr->csp > (player_ptr->msp / 30))) {
        player_ptr->csp -= (1 + (player_ptr->msp / 30));
        player_ptr->redraw |= (PR_MANA);
        *magnification = *magnification * 3 / 2 + 20;
    }
}

/*!
 * @brief 死の大鎌による反射ダメージ倍率を更に上げる
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void death_scythe_reflection_critial_hit(player_attack_type *pa_ptr)
{
    if (!one_in_(6))
        return;

    int more_magnification = 2;
    msg_format(_("グッサリ切り裂かれた！", "Your weapon cuts deep into yourself!"));
    while (one_in_(4))
        more_magnification++;

    pa_ptr->attack_damage *= (HIT_POINT)more_magnification;
}

/*!
 * @brief 死の大鎌によるダメージ反射のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void process_death_scythe_reflection(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    sound(SOUND_HIT);
    msg_format(_("ミス！ %sにかわされた。", "You miss %s."), pa_ptr->m_name);
    msg_print(_("振り回した大鎌が自分自身に返ってきた！", "Your scythe returns to you!"));

    object_type *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    auto death_scythe_flags = object_flags(o_ptr);
    pa_ptr->attack_damage = damroll(o_ptr->dd + player_ptr->to_dd[pa_ptr->hand], o_ptr->ds + player_ptr->to_ds[pa_ptr->hand]);
    int magnification = calc_death_scythe_reflection_magnification(player_ptr);
    compensate_death_scythe_reflection_magnification(player_ptr, &magnification, death_scythe_flags);
    pa_ptr->attack_damage *= (HIT_POINT)magnification;
    pa_ptr->attack_damage /= 10;
    pa_ptr->attack_damage = critical_norm(player_ptr, o_ptr->weight, o_ptr->to_h, pa_ptr->attack_damage, player_ptr->to_h[pa_ptr->hand], pa_ptr->mode);
    death_scythe_reflection_critial_hit(pa_ptr);
    pa_ptr->attack_damage += (player_ptr->to_d[pa_ptr->hand] + o_ptr->to_d);
    if (pa_ptr->attack_damage < 0)
        pa_ptr->attack_damage = 0;

    take_hit(player_ptr, DAMAGE_FORCE, pa_ptr->attack_damage, _("死の大鎌", "Death scythe"));
    handle_stuff(player_ptr);
}
