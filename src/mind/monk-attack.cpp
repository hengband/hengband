/*!
 * @brief 素手で攻撃することに補正のある職業 (修行僧、狂戦士、練気術師)の打撃処理
 * @date 2020/05/23
 * @author Hourier
 * @details 練気術師は騎乗していない時
 */

#include "mind/monk-attack.h"
#include "cmd-action/cmd-attack.h"
#include "combat/attack-criticality.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "floor/geometry.h"
#include "game-option/cheat-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-force-trainer.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "player-attack/player-attack-util.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 朦朧への抵抗値を計算する
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 朦朧への抵抗値
 */
static int calc_stun_resistance(player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    int resist_stun = 0;
    if (r_ptr->flags1 & RF1_UNIQUE)
        resist_stun += 88;

    if (r_ptr->flags3 & RF3_NO_STUN)
        resist_stun += 66;

    if (r_ptr->flags3 & RF3_NO_CONF)
        resist_stun += 33;

    if (r_ptr->flags3 & RF3_NO_SLEEP)
        resist_stun += 33;

    if ((r_ptr->flags3 & RF3_UNDEAD) || (r_ptr->flags3 & RF3_NONLIVING))
        resist_stun += 66;

    return resist_stun;
}

/*!
 * @brief 技のランダム選択回数を決定する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @return 技のランダム選択回数
 * @details ランダム選択は一番強い技が最終的に選択されるので、回数が多いほど有利
 */
static int calc_max_blow_selection_times(player_type *attacker_ptr)
{
    if (attacker_ptr->special_defense & KAMAE_BYAKKO)
        return (attacker_ptr->lev < 3 ? 1 : attacker_ptr->lev / 3);

    if (attacker_ptr->special_defense & KAMAE_SUZAKU)
        return 1;

    if (attacker_ptr->special_defense & KAMAE_GENBU)
        return 1;

    return attacker_ptr->lev < 7 ? 1 : attacker_ptr->lev / 7;
}

/*!
 * @brief プレーヤーのレベルと技の難度を加味しつつ、確率で一番強い技を選ぶ
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @return 技のランダム選択回数
 * @return 技の行使に必要な最低レベル
 */
static int select_blow(player_type *attacker_ptr, player_attack_type *pa_ptr, int max_blow_selection_times)
{
    int min_level = 1;
    const martial_arts *old_ptr = &ma_blows[0];
    for (int times = 0; times < max_blow_selection_times; times++) {
        do {
            pa_ptr->ma_ptr = &ma_blows[randint0(MAX_MA)];
            if ((attacker_ptr->pclass == CLASS_FORCETRAINER) && (pa_ptr->ma_ptr->min_level > 1))
                min_level = pa_ptr->ma_ptr->min_level + 3;
            else
                min_level = pa_ptr->ma_ptr->min_level;
        } while ((min_level > attacker_ptr->lev) || (randint1(attacker_ptr->lev) < pa_ptr->ma_ptr->chance));

        if ((pa_ptr->ma_ptr->min_level <= old_ptr->min_level) || attacker_ptr->stun || attacker_ptr->confused) {
            pa_ptr->ma_ptr = old_ptr;
            continue;
        }

        old_ptr = pa_ptr->ma_ptr;
        if (current_world_ptr->wizard && cheat_xtra)
            msg_print(_("攻撃を再選択しました。", "Attack re-selected."));
    }

    if (attacker_ptr->pclass == CLASS_FORCETRAINER)
        min_level = MAX(1, pa_ptr->ma_ptr->min_level - 3);
    else
        min_level = pa_ptr->ma_ptr->min_level;

    return min_level;
}

static int process_monk_additional_effect(player_attack_type *pa_ptr, int *stun_effect)
{
    int special_effect = 0;
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (pa_ptr->ma_ptr->effect == MA_KNEE) {
        if (r_ptr->flags1 & RF1_MALE) {
            msg_format(_("%sに金的膝蹴りをくらわした！", "You hit %s in the groin with your knee!"), pa_ptr->m_name);
            sound(SOUND_PAIN);
            special_effect = MA_KNEE;
        } else
            msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
    }

    else if (pa_ptr->ma_ptr->effect == MA_SLOW) {
        if (!((r_ptr->flags1 & RF1_NEVER_MOVE) || angband_strchr("~#{}.UjmeEv$,DdsbBFIJQSXclnw!=?", r_ptr->d_char))) {
            msg_format(_("%sの足首に関節蹴りをくらわした！", "You kick %s in the ankle."), pa_ptr->m_name);
            special_effect = MA_SLOW;
        } else
            msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
    } else {
        if (pa_ptr->ma_ptr->effect) {
            *stun_effect = (pa_ptr->ma_ptr->effect / 2) + randint1(pa_ptr->ma_ptr->effect / 2);
        }

        msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
    }

    return special_effect;
}

/*!
 * @brief 攻撃の重さ (修行僧と練気術師における武器重量)を決定する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @return 重さ
 */
static WEIGHT calc_monk_attack_weight(player_type *attacker_ptr)
{
    WEIGHT weight = 8;
    if (attacker_ptr->special_defense & KAMAE_SUZAKU)
        weight = 4;

    if ((attacker_ptr->pclass == CLASS_FORCETRAINER) && (get_current_ki(attacker_ptr) != 0)) {
        weight += (get_current_ki(attacker_ptr) / 30);
        if (weight > 20)
            weight = 20;
    }

    return weight;
}

/*!
 * @brief 急所攻撃による追加効果を与える
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param stun_effect 朦朧の残りターン
 * @param resist_stun 朦朧への抵抗値
 * @param special_effect 技を繰り出した時の追加効果
 */
static void process_attack_vital_spot(player_type *attacker_ptr, player_attack_type *pa_ptr, int *stun_effect, int *resist_stun, const int special_effect)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if ((special_effect == MA_KNEE) && ((pa_ptr->attack_damage + attacker_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        msg_format(_("%^sは苦痛にうめいている！", "%^s moans in agony!"), pa_ptr->m_name);
        *stun_effect = 7 + randint1(13);
        *resist_stun /= 3;
        return;
    }

    if ((special_effect == MA_SLOW) && ((pa_ptr->attack_damage + attacker_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        if (!(r_ptr->flags1 & RF1_UNIQUE) && (randint1(attacker_ptr->lev) > r_ptr->level) && pa_ptr->m_ptr->mspeed > 60) {
            msg_format(_("%^sは足をひきずり始めた。", "You've hobbled %s."), pa_ptr->m_name);
            pa_ptr->m_ptr->mspeed -= 10;
        }
    }
}

/*!
 * @brief 朦朧効果を受けたモンスターのステータス表示
 * @param attacker_ptr プレーヤーの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 * @param stun_effect 朦朧の残りターン
 * @param resist_stun 朦朧への抵抗値
 */
static void print_stun_effect(player_type *attacker_ptr, player_attack_type *pa_ptr, const int stun_effect, const int resist_stun)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (stun_effect && ((pa_ptr->attack_damage + attacker_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        if (attacker_ptr->lev > randint1(r_ptr->level + resist_stun + 10)) {
            if (set_monster_stunned(attacker_ptr, pa_ptr->g_ptr->m_idx, stun_effect + monster_stunned_remaining(pa_ptr->m_ptr))) {
                msg_format(_("%^sはフラフラになった。", "%^s is stunned."), pa_ptr->m_name);
            } else {
                msg_format(_("%^sはさらにフラフラになった。", "%^s is more stunned."), pa_ptr->m_name);
            }
        }
    }
}

/*!
 * @brief 強力な素手攻撃ができる職業 (修行僧、狂戦士、練気術師)の素手攻撃処理メインルーチン
 * @param attacker_ptr プレーヤーの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 */
void process_monk_attack(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    int resist_stun = calc_stun_resistance(pa_ptr);
    int max_blow_selection_times = calc_max_blow_selection_times(attacker_ptr);
    int min_level = select_blow(attacker_ptr, pa_ptr, max_blow_selection_times);

    pa_ptr->attack_damage = damroll(pa_ptr->ma_ptr->dd + attacker_ptr->to_dd[pa_ptr->hand], pa_ptr->ma_ptr->ds + attacker_ptr->to_ds[pa_ptr->hand]);
    if (attacker_ptr->special_attack & ATTACK_SUIKEN)
        pa_ptr->attack_damage *= 2;

    int stun_effect = 0;
    int special_effect = process_monk_additional_effect(pa_ptr, &stun_effect);
    WEIGHT weight = calc_monk_attack_weight(attacker_ptr);
    pa_ptr->attack_damage = critical_norm(attacker_ptr, attacker_ptr->lev * weight, min_level, pa_ptr->attack_damage, attacker_ptr->to_h[0], HISSATSU_NONE);
    process_attack_vital_spot(attacker_ptr, pa_ptr, &stun_effect, &resist_stun, special_effect);
    print_stun_effect(attacker_ptr, pa_ptr, stun_effect, resist_stun);
}

bool double_attack(player_type *creature_ptr)
{
    DIRECTION dir;
    if (!get_rep_dir(creature_ptr, &dir, false))
        return false;
    POSITION y = creature_ptr->y + ddy[dir];
    POSITION x = creature_ptr->x + ddx[dir];
    if (!creature_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
        msg_print(nullptr);
        return true;
    }

    if (one_in_(3))
        msg_print(_("あーたたたたたたたたたたたたたたたたたたたたたた！！！", "Ahhhtatatatatatatatatatatatatatataatatatatattaaaaa!!!!"));
    else if (one_in_(2))
        msg_print(_("無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄！！！", "Mudamudamudamudamudamudamudamudamudamudamudamudamuda!!!!"));
    else
        msg_print(_("オラオラオラオラオラオラオラオラオラオラオラオラ！！！", "Oraoraoraoraoraoraoraoraoraoraoraoraoraoraoraoraora!!!!"));

    do_cmd_attack(creature_ptr, y, x, HISSATSU_NONE);
    if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        handle_stuff(creature_ptr);
        do_cmd_attack(creature_ptr, y, x, HISSATSU_NONE);
    }

    creature_ptr->energy_need += ENERGY_NEED();
    return true;
}
