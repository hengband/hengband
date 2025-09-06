#include "combat/attack-criticality.h"
#include "combat/combat-options-type.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-of-music.h"
#include "object/tval-types.h"
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief クリティカルダメージを適用する
 *
 * @param k クリティカルの強度を決定する値
 * @param base_dam クリティカル適用前のダメージ
 * @param mult 期待値計算時のdam倍率
 * @return クリティカルを適用したダメージと、クリティカル発生時に表示するメッセージと、クリティカル効果音のタプルを返す
 */
std::tuple<int, std::string, SoundKind> apply_critical_norm_damage(int k, int base_dam, int mult)
{
    if (k < 400) {
        return { 2 * base_dam + 5 * mult, _("手ごたえがあった！", "It was a good hit!"), SoundKind::GOOD_HIT };
    }
    if (k < 700) {
        return { 2 * base_dam + 10 * mult, _("かなりの手ごたえがあった！", "It was a great hit!"), SoundKind::GREAT_HIT };
    }
    if (k < 900) {
        return { 3 * base_dam + 15 * mult, _("会心の一撃だ！", "It was a superb hit!"), SoundKind::SUPERB_HIT };
    }
    if (k < 1300) {
        return { 3 * base_dam + 20 * mult, _("最高の会心の一撃だ！", "It was a *GREAT* hit!"), SoundKind::STAR_GREAT_HIT };
    }
    return { ((7 * base_dam) / 2) + 25 * mult, _("比類なき最高の会心の一撃だ！", "It was a *SUPERB* hit!"), SoundKind::STAR_SUPERB_HIT };
}

/*!
 * @brief プレイヤーからモンスターへの打撃クリティカル判定 /
 * Critical hits (by player) Factor in weapon weight, total plusses, player melee bonus
 * @param weight 矢弾の重量
 * @param plus 武器の命中修正
 * @param dam 現在算出中のダメージ値
 * @param meichuu 打撃の基本命中力
 * @param mode オプションフラグ
 * @return クリティカル修正が入ったダメージ値
 */
int critical_norm(PlayerType *player_ptr, WEIGHT weight, int plus, int dam, int16_t meichuu, combat_options mode, bool impact)
{
    /* Extract "blow" power */
    int i = (weight + (meichuu * 3 + plus * 5) + player_ptr->skill_thn);

    /* Chance */
    auto pow = PlayerClass(player_ptr).equals(PlayerClassType::NINJA) ? 4444 : 5000;
    if (impact) {
        pow /= 2;
    }

    bool is_special_option = randint1(pow) <= i;
    is_special_option |= mode == HISSATSU_MAJIN;
    is_special_option |= mode == HISSATSU_3DAN;
    if (!is_special_option) {
        return dam;
    }

    auto k = weight + randint1(CRITICAL_DIE_SIDES);
    if (impact || (mode == HISSATSU_MAJIN) || (mode == HISSATSU_3DAN)) {
        k += randint1(CRITICAL_DIE_SIDES);
    }

    const auto &[critical_dam, msg, battle_sound] = apply_critical_norm_damage(k, dam);
    sound(battle_sound);
    msg_print(msg);
    return critical_dam;
}

/*!
 * @brief 忍者ヒットで急所を突く
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @details 闇討ち＆追討ちを実施した後に致命傷チェックを行う
 * チェックを通ったら、ユニークならば2倍ダメージ、それ以外は一撃死
 * @todo 3つの処理をdetailsに書くよりは関数自体を分割すべきだが、一旦後回しにする。他の項目と一緒に処理する
 */
static void ninja_critical(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    auto &monrace = pa_ptr->m_ptr->get_monrace();
    int maxhp = pa_ptr->m_ptr->maxhp;
    if (one_in_(pa_ptr->backstab ? 13 : (pa_ptr->stab_fleeing || pa_ptr->surprise_attack) ? 15
                                                                                          : 27)) {
        pa_ptr->attack_damage *= 5;
        pa_ptr->drain_result *= 2;
        msg_format(_("刃が%sに深々と突き刺さった！", "You critically injured %s!"), pa_ptr->m_name);
        return;
    }

    const auto no_instantly_death = monrace.resistance_flags.has(MonsterResistanceType::NO_INSTANTLY_DEATH);
    bool is_weaken = pa_ptr->m_ptr->hp < maxhp / 2;
    bool is_unique = monrace.kind_flags.has(MonsterKindType::UNIQUE) || no_instantly_death;
    bool is_critical = (is_weaken && one_in_((player_ptr->num_blow[0] + player_ptr->num_blow[1] + 1) * 10)) || ((one_in_(666) || ((pa_ptr->backstab || pa_ptr->surprise_attack) && one_in_(11))) && !is_unique);
    if (!is_critical) {
        return;
    }

    if (is_unique || !is_weaken) {
        if (no_instantly_death) {
            monrace.r_resistance_flags.set(MonsterResistanceType::NO_INSTANTLY_DEATH);
        }
        pa_ptr->attack_damage = std::max(pa_ptr->attack_damage * 5, pa_ptr->m_ptr->hp / 2);
        pa_ptr->drain_result *= 2;
        msg_format(_("%sに致命傷を負わせた！", "You fatally injured %s!"), pa_ptr->m_name);
    } else {
        pa_ptr->attack_damage = pa_ptr->m_ptr->hp + 1;
        msg_format(_("刃が%sの急所を貫いた！", "You hit %s on a fatal spot!"), pa_ptr->m_name);
    }
}

/*!
 * @brief 急所を突く
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void critical_attack(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    auto *o_ptr = player_ptr->inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    auto &monrace = pa_ptr->m_ptr->get_monrace();
    const auto no_instantly_death = monrace.resistance_flags.has(MonsterResistanceType::NO_INSTANTLY_DEATH);
    if ((o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) || (pa_ptr->mode == HISSATSU_KYUSHO)) {
        if ((randint1(randint1(monrace.level / 7) + 5) == 1) && monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && !no_instantly_death) {
            pa_ptr->attack_damage = pa_ptr->m_ptr->hp + 1;
            msg_format(_("%sの急所を突き刺した！", "You hit %s on a fatal spot!"), pa_ptr->m_name);
        } else {
            if (no_instantly_death) {
                monrace.r_resistance_flags.set(MonsterResistanceType::NO_INSTANTLY_DEATH);
            }
            pa_ptr->attack_damage = 1;
        }

        return;
    }

    if (!PlayerClass(player_ptr).equals(PlayerClassType::NINJA)) {
        return;
    }

    const auto has_weapon = has_melee_weapon(player_ptr, enum2i(INVEN_MAIN_HAND) + pa_ptr->hand);
    const auto is_ninja_hit = has_weapon && !player_ptr->is_icky_wield[pa_ptr->hand] && ((player_ptr->cur_lite <= 0) || one_in_(7));
    if (is_ninja_hit) {
        ninja_critical(player_ptr, pa_ptr);
    }
}
