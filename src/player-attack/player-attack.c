/*!
 * @brief プレーヤーからモンスターへの打撃処理
 * @date 2020/05/22
 * @author Hourier
 */

#include "player-attack/player-attack.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-attack.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "combat/martial-arts-table.h"
#include "combat/slaying.h"
#include "core/player-update-types.h"
#include "floor/cave.h"
#include "game-option/cheat-types.h"
#include "grid/feature-flag-types.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "mind/mind-samurai.h"
#include "mind/monk-attack.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/vorpal-weapon.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "player-attack/attack-chaos-effect.h"
#include "player-attack/blood-sucking-processor.h"
#include "player-attack/player-attack-util.h"
#include "player-info/avatar.h"
#include "player/player-damage.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/earthquake.h"
#include "spell-realm/spells-hex.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

static player_attack_type *initialize_player_attack_type(
    player_attack_type *pa_ptr, s16b hand, combat_options mode, monster_type *m_ptr, grid_type *g_ptr, bool *fear, bool *mdeath)
{
    pa_ptr->hand = hand;
    pa_ptr->mode = mode;
    pa_ptr->m_ptr = m_ptr;
    pa_ptr->backstab = FALSE;
    pa_ptr->surprise_attack = FALSE;
    pa_ptr->stab_fleeing = FALSE;
    pa_ptr->monk_attack = FALSE;
    pa_ptr->num_blow = 0;
    pa_ptr->attack_damage = 0;
    pa_ptr->can_drain = FALSE;
    pa_ptr->ma_ptr = &ma_blows[0];
    pa_ptr->drain_result = 0;
    pa_ptr->g_ptr = g_ptr;
    pa_ptr->fear = fear;
    pa_ptr->mdeath = mdeath;
    pa_ptr->drain_left = MAX_VAMPIRIC_DRAIN;
    pa_ptr->weak = FALSE;
    return pa_ptr;
}

/*!
 * @brief 一部職業で攻撃に倍率がかかったりすることの処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void attack_classify(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    switch (attacker_ptr->pclass) {
    case CLASS_ROGUE:
    case CLASS_NINJA:
        process_surprise_attack(attacker_ptr, pa_ptr);
        return;
    case CLASS_MONK:
    case CLASS_FORCETRAINER:
    case CLASS_BERSERKER:
        if ((empty_hands(attacker_ptr, TRUE) & EMPTY_HAND_RARM) && !attacker_ptr->riding)
            pa_ptr->monk_attack = TRUE;
        return;
    default:
        return;
    }
}

/*!
 * @brief マーシャルアーツの技能値を増加させる
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void get_bare_knuckle_exp(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if ((r_ptr->level + 10) <= attacker_ptr->lev || (attacker_ptr->skill_exp[GINOU_SUDE] >= s_info[attacker_ptr->pclass].s_max[GINOU_SUDE]))
        return;

    if (attacker_ptr->skill_exp[GINOU_SUDE] < WEAPON_EXP_BEGINNER)
        attacker_ptr->skill_exp[GINOU_SUDE] += 40;
    else if ((attacker_ptr->skill_exp[GINOU_SUDE] < WEAPON_EXP_SKILLED))
        attacker_ptr->skill_exp[GINOU_SUDE] += 5;
    else if ((attacker_ptr->skill_exp[GINOU_SUDE] < WEAPON_EXP_EXPERT) && (attacker_ptr->lev > 19))
        attacker_ptr->skill_exp[GINOU_SUDE] += 1;
    else if ((attacker_ptr->lev > 34))
        if (one_in_(3))
            attacker_ptr->skill_exp[GINOU_SUDE] += 1;

    attacker_ptr->update |= (PU_BONUS);
}

/*!
 * @brief 装備している武器の技能値を増加させる
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void get_weapon_exp(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    tval_type tval = attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand].tval - TV_WEAPON_BEGIN;
    OBJECT_SUBTYPE_VALUE sval = attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand].sval;
    int now_exp = attacker_ptr->weapon_exp[tval][sval];
    if (now_exp >= s_info[attacker_ptr->pclass].w_max[tval][sval])
        return;

    SUB_EXP amount = 0;
    if (now_exp < WEAPON_EXP_BEGINNER)
        amount = 80;
    else if (now_exp < WEAPON_EXP_SKILLED)
        amount = 10;
    else if ((now_exp < WEAPON_EXP_EXPERT) && (attacker_ptr->lev > 19))
        amount = 1;
    else if ((attacker_ptr->lev > 34) && one_in_(2))
        amount = 1;

    attacker_ptr->weapon_exp[tval][sval] += amount;
    attacker_ptr->update |= (PU_BONUS);
}

/*!
 * @brief 直接攻撃に伴う技能値の上昇処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void get_attack_exp(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    if (o_ptr->k_idx == 0) {
        get_bare_knuckle_exp(attacker_ptr, pa_ptr);
        return;
    }

    if (!object_is_melee_weapon(o_ptr) || ((r_ptr->level + 10) <= attacker_ptr->lev))
        return;

    get_weapon_exp(attacker_ptr, pa_ptr);
}

/*!
 * @brief 攻撃回数を決定する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 * @details 毒針は確定で1回
 */
static void calc_num_blow(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    if ((pa_ptr->mode == HISSATSU_KYUSHO) || (pa_ptr->mode == HISSATSU_MINEUCHI) || (pa_ptr->mode == HISSATSU_3DAN) || (pa_ptr->mode == HISSATSU_IAI))
        pa_ptr->num_blow = 1;
    else if (pa_ptr->mode == HISSATSU_COLD)
        pa_ptr->num_blow = attacker_ptr->num_blow[pa_ptr->hand] + 2;
    else
        pa_ptr->num_blow = attacker_ptr->num_blow[pa_ptr->hand];

    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
        pa_ptr->num_blow = 1;
}

/*!
 * @brief 混沌属性の武器におけるカオス効果を決定する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return カオス効果
 * @details
 * 吸血20%、地震0.12%、混乱26.892%、テレポート・アウェイ1.494%、変身1.494% /
 * Vampiric 20%, Quake 0.12%, Confusion 26.892%, Teleport away 1.494% and Polymorph 1.494%
 */
static chaotic_effect select_chaotic_effect(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    if (!(has_flag(pa_ptr->flags, TR_CHAOTIC)) || one_in_(2))
        return CE_NONE;

    if (one_in_(10))
        chg_virtue(attacker_ptr, V_CHANCE, 1);

    if (randint1(5) < 3)
        return CE_VAMPIRIC;

    if (one_in_(250))
        return CE_CONFUSION;

    if (!one_in_(10))
        return CE_QUAKE;

    return one_in_(2) ? CE_TELE_AWAY : CE_POLYMORPH;
}

/*!
 * @brief 武器による直接攻撃メインルーチン
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param vorpal_cut メッタ斬りにできるかどうか
 * @param vorpal_chance ヴォーパル倍率上昇の機会値
 * @return 攻撃の結果、地震を起こすことになったらTRUE、それ以外はFALSE
 */
static void process_weapon_attack(player_type *attacker_ptr, player_attack_type *pa_ptr, bool *do_quake, const bool vorpal_cut, const int vorpal_chance)
{
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    pa_ptr->attack_damage = damroll(o_ptr->dd + attacker_ptr->to_dd[pa_ptr->hand], o_ptr->ds + attacker_ptr->to_ds[pa_ptr->hand]);
    pa_ptr->attack_damage = calc_attack_damage_with_slay(attacker_ptr, o_ptr, pa_ptr->attack_damage, pa_ptr->m_ptr, pa_ptr->mode, FALSE);
    calc_surprise_attack_damage(attacker_ptr, pa_ptr);

    if (((attacker_ptr->impact & FLAG_CAUSE_INVEN_RARM) && ((pa_ptr->attack_damage > 50) || one_in_(7))) || (pa_ptr->chaos_effect == CE_QUAKE)
        || (pa_ptr->mode == HISSATSU_QUAKE))
        *do_quake = TRUE;

    if ((!(o_ptr->tval == TV_SWORD) || !(o_ptr->sval == SV_POISON_NEEDLE)) && !(pa_ptr->mode == HISSATSU_KYUSHO))
        pa_ptr->attack_damage = critical_norm(attacker_ptr, o_ptr->weight, o_ptr->to_h, pa_ptr->attack_damage, attacker_ptr->to_h[pa_ptr->hand], pa_ptr->mode);

    pa_ptr->drain_result = pa_ptr->attack_damage;
    process_vorpal_attack(attacker_ptr, pa_ptr, vorpal_cut, vorpal_chance);
    pa_ptr->attack_damage += o_ptr->to_d;
    pa_ptr->drain_result += o_ptr->to_d;
}

/*!
 * @brief 武器または素手による攻撃ダメージを計算する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param do_quake 攻撃の結果、地震を起こすことになったらTRUE、それ以外はFALSE
 * @param vorpal_cut メッタ斬りにできるかどうか
 * @param vorpal_change ヴォーパル倍率上昇の機会値
 * @return なし
 * @details 取り敢えず素手と仮定し1とする.
 */
static void calc_attack_damage(player_type *attacker_ptr, player_attack_type *pa_ptr, bool *do_quake, const bool vorpal_cut, const int vorpal_chance)
{
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    pa_ptr->attack_damage = 1;
    if (pa_ptr->monk_attack) {
        process_monk_attack(attacker_ptr, pa_ptr);
        return;
    }

    if (o_ptr->k_idx) {
        process_weapon_attack(attacker_ptr, pa_ptr, do_quake, vorpal_cut, vorpal_chance);
    }
}

/*!
 * @brief 武器のダメージボーナスや剣術家の技によってダメージにボーナスを与える
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void apply_damage_bonus(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    pa_ptr->attack_damage += attacker_ptr->to_d[pa_ptr->hand];
    pa_ptr->drain_result += attacker_ptr->to_d[pa_ptr->hand];

    if ((pa_ptr->mode == HISSATSU_SUTEMI) || (pa_ptr->mode == HISSATSU_3DAN))
        pa_ptr->attack_damage *= 2;

    if ((pa_ptr->mode == HISSATSU_SEKIRYUKA) && !monster_living(pa_ptr->m_ptr->r_idx))
        pa_ptr->attack_damage = 0;

    if ((pa_ptr->mode == HISSATSU_SEKIRYUKA) && !attacker_ptr->cut)
        pa_ptr->attack_damage /= 2;
}

/*!
 * todo かなりのレアケースだが、右手に混沌属性の武器を持ち、左手にエクスカリバー・ジュニアを持ち、
 * 右手の最終打撃で蜘蛛に変身したとしても、左手の攻撃でダメージが減らない気がする
 * モンスターへの参照ポインタは変身時に変わるのにis_ej_nullifiedはその前に代入されて参照されるだけであるため
 * @brief 特殊な条件でダメージが減ったり0になったりする処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param is_zantetsu_nullified 斬鉄剣で切れないならばTRUE
 * @param is_ej_nullified 蜘蛛相手ならばTRUE
 * @details ダメージが0未満なら0に補正する
 */
static void apply_damage_negative_effect(player_attack_type *pa_ptr, bool is_zantetsu_nullified, bool is_ej_nullified)
{
    if (pa_ptr->attack_damage < 0)
        pa_ptr->attack_damage = 0;

    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if ((pa_ptr->mode == HISSATSU_ZANMA) && !(!monster_living(pa_ptr->m_ptr->r_idx) && (r_ptr->flags3 & RF3_EVIL))) {
        pa_ptr->attack_damage = 0;
    }

    if (is_zantetsu_nullified) {
        msg_print(_("こんな軟らかいものは切れん！", "You cannot cut such a elastic thing!"));
        pa_ptr->attack_damage = 0;
    }

    if (is_ej_nullified) {
        msg_print(_("蜘蛛は苦手だ！", "Spiders are difficult for you to deal with!"));
        pa_ptr->attack_damage /= 2;
    }
}

/*!
 * @brief モンスターのHPを減らした後、恐怖させるか死なす (フロアから消滅させる)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 死んだらTRUE、生きていたらFALSE
 */
static bool check_fear_death(player_type *attacker_ptr, player_attack_type *pa_ptr, const int num, const bool is_lowlevel)
{
    if (!mon_take_hit(attacker_ptr, pa_ptr->g_ptr->m_idx, pa_ptr->attack_damage, pa_ptr->fear, NULL))
        return FALSE;

    *(pa_ptr->mdeath) = TRUE;
    if ((attacker_ptr->pclass == CLASS_BERSERKER) && attacker_ptr->energy_use) {
        if (has_right_hand_weapon(attacker_ptr) && has_left_hand_weapon(attacker_ptr)) {
            if (pa_ptr->hand)
                attacker_ptr->energy_use = attacker_ptr->energy_use * 3 / 5 + attacker_ptr->energy_use * num * 2 / (attacker_ptr->num_blow[pa_ptr->hand] * 5);
            else
                attacker_ptr->energy_use = attacker_ptr->energy_use * num * 3 / (attacker_ptr->num_blow[pa_ptr->hand] * 5);
        } else {
            attacker_ptr->energy_use = attacker_ptr->energy_use * num / attacker_ptr->num_blow[pa_ptr->hand];
        }
    }

    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    if ((o_ptr->name1 == ART_ZANTETSU) && is_lowlevel)
        msg_print(_("またつまらぬものを斬ってしまった．．．", "Sigh... Another trifling thing I've cut...."));

    return TRUE;
}

/*!
 * @brief 直接攻撃が当たった時の処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param do_quake 攻撃後に地震を起こすかどうか
 * @param is_zantetsu_nullified 斬鉄剣で切れないならばTRUE
 * @param is_ej_nullified 蜘蛛相手ならばTRUE
 * @return なし
 */
static void apply_actual_attack(
    player_type *attacker_ptr, player_attack_type *pa_ptr, bool *do_quake, const bool is_zantetsu_nullified, const bool is_ej_nullified)
{
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    int vorpal_chance = ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD)) ? 2 : 4;

    sound(SOUND_HIT);
    print_surprise_attack(pa_ptr);

    object_flags(attacker_ptr, o_ptr, pa_ptr->flags);
    pa_ptr->chaos_effect = select_chaotic_effect(attacker_ptr, pa_ptr);
    decide_blood_sucking(attacker_ptr, pa_ptr);

    // process_monk_attackの中でplayer_type->magic_num1[0] を書き換えているので、ここでhex_spelling() の判定をしないとダメ.
    bool vorpal_cut
        = (has_flag(pa_ptr->flags, TR_VORPAL) || hex_spelling(attacker_ptr, HEX_RUNESWORD)) && (randint1(vorpal_chance * 3 / 2) == 1) && !is_zantetsu_nullified;

    calc_attack_damage(attacker_ptr, pa_ptr, do_quake, vorpal_cut, vorpal_chance);
    apply_damage_bonus(attacker_ptr, pa_ptr);
    apply_damage_negative_effect(pa_ptr, is_zantetsu_nullified, is_ej_nullified);
    mineuchi(attacker_ptr, pa_ptr);
    pa_ptr->attack_damage = mon_damage_mod(attacker_ptr, pa_ptr->m_ptr, pa_ptr->attack_damage,
        (bool)(((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE)) || ((attacker_ptr->pclass == CLASS_BERSERKER) && one_in_(2))));
    critical_attack(attacker_ptr, pa_ptr);
    msg_format_wizard(attacker_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"),
        pa_ptr->attack_damage, pa_ptr->m_ptr->hp - pa_ptr->attack_damage, pa_ptr->m_ptr->maxhp, pa_ptr->m_ptr->max_maxhp);
}

/*!
 * @brief 地震を起こす
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param do_quake 攻撃後に地震を起こすかどうか
 * @param y モンスターのY座標
 * @param x モンスターのX座標
 * @return なし
 */
static void cause_earthquake(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool do_quake, const POSITION y, const POSITION x)
{
    if (!do_quake)
        return;

    earthquake(attacker_ptr, attacker_ptr->y, attacker_ptr->x, 10, 0);
    if (attacker_ptr->current_floor_ptr->grid_array[y][x].m_idx == 0)
        *(pa_ptr->mdeath) = TRUE;
}

/*!
 * @brief プレイヤーの打撃処理サブルーチン /
 * Player attacks a (poor, defenseless) creature        -RAK-
 * @param y 攻撃目標のY座標
 * @param x 攻撃目標のX座標
 * @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
 * @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
 * @param hand 攻撃を行うための武器を持つ手
 * @param mode 発動中の剣術ID
 * @return なし
 * @details
 * If no "weapon" is available, then "punch" the monster one time.
 */
void exe_player_attack_to_monster(player_type *attacker_ptr, POSITION y, POSITION x, bool *fear, bool *mdeath, s16b hand, combat_options mode)
{
    bool do_quake = FALSE;
    bool drain_msg = TRUE;

    floor_type *floor_ptr = attacker_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    player_attack_type tmp_attack;
    player_attack_type *pa_ptr = initialize_player_attack_type(&tmp_attack, hand, mode, m_ptr, g_ptr, fear, mdeath);
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    bool is_human = (r_ptr->d_char == 'p');
    bool is_lowlevel = (r_ptr->level < (attacker_ptr->lev - 15));

    attack_classify(attacker_ptr, pa_ptr);
    get_attack_exp(attacker_ptr, pa_ptr);

    /* Disturb the monster */
    (void)set_monster_csleep(attacker_ptr, g_ptr->m_idx, 0);
    monster_desc(attacker_ptr, pa_ptr->m_name, m_ptr, 0);

    int chance = calc_attack_quality(attacker_ptr, pa_ptr);
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    bool is_zantetsu_nullified = ((o_ptr->name1 == ART_ZANTETSU) && (r_ptr->d_char == 'j'));
    bool is_ej_nullified = ((o_ptr->name1 == ART_EXCALIBUR_J) && (r_ptr->d_char == 'S'));
    calc_num_blow(attacker_ptr, pa_ptr);

    /* Attack once for each legal blow */
    int num = 0;
    while ((num++ < pa_ptr->num_blow) && !attacker_ptr->is_dead) {
        if (!process_attack_hit(attacker_ptr, pa_ptr, chance))
            continue;

        apply_actual_attack(attacker_ptr, pa_ptr, &do_quake, is_zantetsu_nullified, is_ej_nullified);
        calc_drain(pa_ptr);
        if (check_fear_death(attacker_ptr, pa_ptr, num, is_lowlevel))
            break;

        /* Anger the monster */
        if (pa_ptr->attack_damage > 0)
            anger_monster(attacker_ptr, m_ptr);

        touch_zap_player(m_ptr, attacker_ptr);
        process_drain(attacker_ptr, pa_ptr, is_human, &drain_msg);
        pa_ptr->can_drain = FALSE;
        pa_ptr->drain_result = 0;
        change_monster_stat(attacker_ptr, pa_ptr, y, x, &num);
        pa_ptr->backstab = FALSE;
        pa_ptr->surprise_attack = FALSE;
    }

    if (pa_ptr->weak && !(*mdeath))
        msg_format(_("%sは弱くなったようだ。", "%^s seems weakened."), pa_ptr->m_name);

    if ((pa_ptr->drain_left != MAX_VAMPIRIC_DRAIN) && one_in_(4))
        chg_virtue(attacker_ptr, V_UNLIFE, 1);

    cause_earthquake(attacker_ptr, pa_ptr, do_quake, y, x);
}

/*!
 * @brief 皆殺し(全方向攻撃)処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void massacre(player_type *caster_ptr)
{
    grid_type *g_ptr;
    monster_type *m_ptr;
    for (DIRECTION dir = 0; dir < 8; dir++) {
        POSITION y = caster_ptr->y + ddy_ddd[dir];
        POSITION x = caster_ptr->x + ddx_ddd[dir];
        g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
        m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
        if (g_ptr->m_idx && (m_ptr->ml || cave_has_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT)))
            do_cmd_attack(caster_ptr, y, x, 0);
    }
}
