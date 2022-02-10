#include "combat/attack-criticality.h"
#include "combat/combat-options-type.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "object/tval-types.h"
#include "player-attack/player-attack-util.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief クリティカルダメージを適用する
 *
 * @param k クリティカルの強度を決定する値
 * @param base_dam クリティカル適用前のダメージ
 * @return クリティカルを適用したダメージと、クリティカル発生時に表示するメッセージと、クリティカル効果音のタプルを返す
 */
std::tuple<HIT_POINT, concptr, sound_type> apply_critical_norm_damage(int k, HIT_POINT base_dam)
{
    if (k < 400) {
        return { 2 * base_dam + 5, _("手ごたえがあった！", "It was a good hit!"), SOUND_GOOD_HIT };
    }
    if (k < 700) {
        return { 2 * base_dam + 10, _("かなりの手ごたえがあった！", "It was a great hit!"), SOUND_GREAT_HIT };
    }
    if (k < 900) {
        return { 3 * base_dam + 15, _("会心の一撃だ！", "It was a superb hit!"), SOUND_SUPERB_HIT };
    }
    if (k < 1300) {
        return { 3 * base_dam + 20, _("最高の会心の一撃だ！", "It was a *GREAT* hit!"), SOUND_STAR_GREAT_HIT };
    }
    return { ((7 * base_dam) / 2) + 25, _("比類なき最高の会心の一撃だ！", "It was a *SUPERB* hit!"), SOUND_STAR_SUPERB_HIT };
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
HIT_POINT critical_norm(PlayerType *player_ptr, WEIGHT weight, int plus, HIT_POINT dam, int16_t meichuu, combat_options mode, bool impact)
{
    /* Extract "blow" power */
    int i = (weight + (meichuu * 3 + plus * 5) + player_ptr->skill_thn);

    /* Chance */
    auto pow = PlayerClass(player_ptr).equals(PlayerClassType::NINJA) ? 4444 : 5000;
    if (impact)
        pow /= 2;

    bool is_special_option = randint1(pow) <= i;
    is_special_option |= mode == HISSATSU_MAJIN;
    is_special_option |= mode == HISSATSU_3DAN;
    if (!is_special_option)
        return dam;

    int k = weight + randint1(650);
    if (impact || (mode == HISSATSU_MAJIN) || (mode == HISSATSU_3DAN))
        k += randint1(650);

    auto [critical_dam, msg, battle_sound] = apply_critical_norm_damage(k, dam);
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
    auto *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    int maxhp = pa_ptr->m_ptr->maxhp;
    if (one_in_(pa_ptr->backstab ? 13 : (pa_ptr->stab_fleeing || pa_ptr->surprise_attack) ? 15
                                                                                          : 27)) {
        pa_ptr->attack_damage *= 5;
        pa_ptr->drain_result *= 2;
        msg_format(_("刃が%sに深々と突き刺さった！", "You critically injured %s!"), pa_ptr->m_name);
        return;
    }

    bool is_weaken = pa_ptr->m_ptr->hp < maxhp / 2;
    bool is_unique = ((r_ptr->flags1 & RF1_UNIQUE) != 0) || ((r_ptr->flags7 & RF7_UNIQUE2) != 0);
    bool is_critical = (is_weaken && one_in_((player_ptr->num_blow[0] + player_ptr->num_blow[1] + 1) * 10)) || ((one_in_(666) || ((pa_ptr->backstab || pa_ptr->surprise_attack) && one_in_(11))) && !is_unique);
    if (!is_critical)
        return;

    if (is_unique || !is_weaken) {
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
    auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    auto *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (((o_ptr->tval == ItemKindType::SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) || (pa_ptr->mode == HISSATSU_KYUSHO)) {
        if ((randint1(randint1(r_ptr->level / 7) + 5) == 1) && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags7 & RF7_UNIQUE2)) {
            pa_ptr->attack_damage = pa_ptr->m_ptr->hp + 1;
            msg_format(_("%sの急所を突き刺した！", "You hit %s on a fatal spot!"), pa_ptr->m_name);
        } else
            pa_ptr->attack_damage = 1;

        return;
    }

    bool is_ninja_hit = PlayerClass(player_ptr).equals(PlayerClassType::NINJA) && has_melee_weapon(player_ptr, INVEN_MAIN_HAND + pa_ptr->hand) && !player_ptr->is_icky_wield[pa_ptr->hand] && ((player_ptr->cur_lite <= 0) || one_in_(7));
    if (is_ninja_hit)
        ninja_critical(player_ptr, pa_ptr);
}
