﻿/*!
 * @brief 攻撃コマンド処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "cmd-action/cmd-attack.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/cheat-types.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "mutation/mutation-flag-types.h"
#include "object/item-use-flags.h"
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "spell/spell-types.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief プレイヤーの変異要素による打撃処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 攻撃目標となったモンスターの参照ID
 * @param attack 変異要素による攻撃要素の種類
 * @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
 * @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
 */
static void natural_attack(PlayerType *player_ptr, MONSTER_IDX m_idx, MUTA attack, bool *fear, bool *mdeath)
{
    WEIGHT n_weight = 0;
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    int dice_num, dice_side;
    concptr atk_desc;
    switch (attack) {
    case MUTA::SCOR_TAIL:
        dice_num = 3;
        dice_side = 7;
        n_weight = 5;
        atk_desc = _("尻尾", "tail");
        break;
    case MUTA::HORNS:
        dice_num = 2;
        dice_side = 6;
        n_weight = 15;
        atk_desc = _("角", "horns");
        break;
    case MUTA::BEAK:
        dice_num = 2;
        dice_side = 4;
        n_weight = 5;
        atk_desc = _("クチバシ", "beak");
        break;
    case MUTA::TRUNK:
        dice_num = 1;
        dice_side = 4;
        n_weight = 35;
        atk_desc = _("象の鼻", "trunk");
        break;
    case MUTA::TENTACLES:
        dice_num = 2;
        dice_side = 5;
        n_weight = 5;
        atk_desc = _("触手", "tentacles");
        break;
    default:
        dice_num = dice_side = n_weight = 1;
        atk_desc = _("未定義の部位", "undefined body part");
    }

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, m_ptr, 0);

    int bonus = player_ptr->to_h_m + (player_ptr->lev * 6 / 5);
    int chance = (player_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

    bool is_hit = ((r_ptr->flags2 & RF2_QUANTUM) == 0) || !randint0(2);
    is_hit &= test_hit_norm(player_ptr, chance, r_ptr->ac, m_ptr->ml);
    if (!is_hit) {
        sound(SOUND_MISS);
        msg_format(_("ミス！ %sにかわされた。", "You miss %s."), m_name);
        return;
    }

    sound(SOUND_HIT);
    msg_format(_("%sを%sで攻撃した。", "You hit %s with your %s."), m_name, atk_desc);

    HIT_POINT k = damroll(dice_num, dice_side);
    k = critical_norm(player_ptr, n_weight, bonus, k, (int16_t)bonus, HISSATSU_NONE);
    k += player_ptr->to_d_m;
    if (k < 0)
        k = 0;

    k = mon_damage_mod(player_ptr, m_ptr, k, false);
    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), k, m_ptr->hp - k,
        m_ptr->maxhp, m_ptr->max_maxhp);
    if (k > 0)
        anger_monster(player_ptr, m_ptr);

    switch (attack) {
    case MUTA::SCOR_TAIL:
        project(player_ptr, 0, 0, m_ptr->fy, m_ptr->fx, k, GF_POIS, PROJECT_KILL);
        *mdeath = (m_ptr->r_idx == 0);
        break;
    case MUTA::HORNS:
    case MUTA::BEAK:
    case MUTA::TRUNK:
    case MUTA::TENTACLES:
    default: {
        MonsterDamageProcessor mdp(player_ptr, m_idx, k, fear);
        *mdeath = mdp.mon_take_hit(nullptr);
        break;
    }
    }

    touch_zap_player(m_ptr, player_ptr);
}

/*!
 * @brief プレイヤーの打撃処理メインルーチン
 * @param y 攻撃目標のY座標
 * @param x 攻撃目標のX座標
 * @param mode 発動中の剣術ID
 * @return 実際に攻撃処理が行われた場合TRUEを返す。
 * @details
 * If no "weapon" is available, then "punch" the monster one time.
 */
bool do_cmd_attack(PlayerType *player_ptr, POSITION y, POSITION x, combat_options mode)
{
    grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    GAME_TEXT m_name[MAX_NLEN];

    const std::initializer_list<MUTA> mutation_attack_methods = { MUTA::HORNS, MUTA::BEAK, MUTA::SCOR_TAIL, MUTA::TRUNK, MUTA::TENTACLES };

    disturb(player_ptr, false, true);

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (!can_attack_with_main_hand(player_ptr) && !can_attack_with_sub_hand(player_ptr) && player_ptr->muta.has_none_of(mutation_attack_methods)) {
        msg_format(_("%s攻撃できない。", "You cannot attack."), (empty_hands(player_ptr, false) == EMPTY_HAND_NONE) ? _("両手がふさがって", "") : "");
        return false;
    }

    monster_desc(player_ptr, m_name, m_ptr, 0);

    if (m_ptr->ml) {
        if (!player_ptr->hallucinated)
            monster_race_track(player_ptr, m_ptr->ap_r_idx);

        health_track(player_ptr, g_ptr->m_idx);
    }

    auto effects = player_ptr->effects();
    auto is_stunned = effects->stun()->is_stunned();
    if (any_bits(r_ptr->flags1, RF1_FEMALE) && !(is_stunned || player_ptr->confused || player_ptr->hallucinated || !m_ptr->ml)) {
        if ((player_ptr->inventory_list[INVEN_MAIN_HAND].name1 == ART_ZANTETSU) || (player_ptr->inventory_list[INVEN_SUB_HAND].name1 == ART_ZANTETSU)) {
            msg_print(_("拙者、おなごは斬れぬ！", "I can not attack women!"));
            return false;
        }
    }

    if (d_info[player_ptr->dungeon_idx].flags.has(DF::NO_MELEE)) {
        msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
        return false;
    }

    bool stormbringer = false;
    if (!is_hostile(m_ptr) && !(is_stunned || player_ptr->confused || player_ptr->hallucinated || is_shero(player_ptr) || !m_ptr->ml)) {
        if (player_ptr->inventory_list[INVEN_MAIN_HAND].name1 == ART_STORMBRINGER)
            stormbringer = true;
        if (player_ptr->inventory_list[INVEN_SUB_HAND].name1 == ART_STORMBRINGER)
            stormbringer = true;
        if (stormbringer) {
            msg_format(_("黒い刃は強欲に%sを攻撃した！", "Your black blade greedily attacks %s!"), m_name);
            chg_virtue(player_ptr, V_INDIVIDUALISM, 1);
            chg_virtue(player_ptr, V_HONOUR, -1);
            chg_virtue(player_ptr, V_JUSTICE, -1);
            chg_virtue(player_ptr, V_COMPASSION, -1);
        } else if (player_ptr->pclass != PlayerClassType::BERSERKER) {
            if (get_check(_("本当に攻撃しますか？", "Really hit it? "))) {
                chg_virtue(player_ptr, V_INDIVIDUALISM, 1);
                chg_virtue(player_ptr, V_HONOUR, -1);
                chg_virtue(player_ptr, V_JUSTICE, -1);
                chg_virtue(player_ptr, V_COMPASSION, -1);
            } else {
                msg_format(_("%sを攻撃するのを止めた。", "You stop to avoid hitting %s."), m_name);
                return false;
            }
        }
    }

    if (player_ptr->afraid) {
        if (m_ptr->ml)
            msg_format(_("恐くて%sを攻撃できない！", "You are too afraid to attack %s!"), m_name);
        else
            msg_format(_("そっちには何か恐いものがいる！", "There is something scary in your way!"));

        (void)set_monster_csleep(player_ptr, g_ptr->m_idx, 0);
        return false;
    }

    if (monster_csleep_remaining(m_ptr)) {
        if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5))
            chg_virtue(player_ptr, V_COMPASSION, -1);
        if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5))
            chg_virtue(player_ptr, V_HONOUR, -1);
    }

    if (can_attack_with_main_hand(player_ptr) && can_attack_with_sub_hand(player_ptr)) {
        if (((player_ptr->skill_exp[PlayerSkillKindType::TWO_WEAPON] - 1000) / 200) < r_ptr->level) {
            PlayerSkill(player_ptr).gain_two_weapon_skill_exp();
        }
    }

    if (player_ptr->riding) {
        PlayerSkill(player_ptr).gain_riding_skill_exp_on_melee_attack(r_ptr);
    }

    player_ptr->riding_t_m_idx = g_ptr->m_idx;
    bool fear = false;
    bool mdeath = false;
    if (can_attack_with_main_hand(player_ptr))
        exe_player_attack_to_monster(player_ptr, y, x, &fear, &mdeath, 0, mode);
    if (can_attack_with_sub_hand(player_ptr) && !mdeath)
        exe_player_attack_to_monster(player_ptr, y, x, &fear, &mdeath, 1, mode);

    if (!mdeath) {
        for (auto m : mutation_attack_methods) {
            if (player_ptr->muta.has(m) && !mdeath) {
                natural_attack(player_ptr, g_ptr->m_idx, m, &fear, &mdeath);
            }
        }
    }

    if (fear && m_ptr->ml && !mdeath) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
    }

    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStance::IAI) && ((mode != HISSATSU_IAI) || mdeath)) {
        set_action(player_ptr, ACTION_NONE);
    }

    return mdeath;
}
