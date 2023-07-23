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
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/monk-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
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
    auto *r_ptr = &monraces_info[pa_ptr->m_ptr->r_idx];
    int resist_stun = 0;
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        resist_stun += 88;
    }

    if (r_ptr->flags3 & RF3_NO_STUN) {
        resist_stun += 66;
    }

    if (r_ptr->flags3 & RF3_NO_CONF) {
        resist_stun += 33;
    }

    if (r_ptr->flags3 & RF3_NO_SLEEP) {
        resist_stun += 33;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNDEAD) || r_ptr->kind_flags.has(MonsterKindType::NONLIVING)) {
        resist_stun += 66;
    }

    return resist_stun;
}

/*!
 * @brief 技のランダム選択回数を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 技のランダム選択回数
 * @details ランダム選択は一番強い技が最終的に選択されるので、回数が多いほど有利
 */
static int calc_max_blow_selection_times(PlayerType *player_ptr)
{
    PlayerClass pc(player_ptr);
    if (pc.monk_stance_is(MonkStanceType::BYAKKO)) {
        return player_ptr->lev < 3 ? 1 : player_ptr->lev / 3;
    }

    if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        return 1;
    }

    if (pc.monk_stance_is(MonkStanceType::GENBU)) {
        return 1;
    }

    return player_ptr->lev < 7 ? 1 : player_ptr->lev / 7;
}

/*!
 * @brief プレイヤーのレベルと技の難度を加味しつつ、確率で一番強い技を選ぶ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 技のランダム選択回数
 * @return 技の行使に必要な最低レベル
 */
static int select_blow(PlayerType *player_ptr, player_attack_type *pa_ptr, int max_blow_selection_times)
{
    int min_level = 1;
    const martial_arts *old_ptr = &ma_blows[0];
    for (int times = 0; times < max_blow_selection_times; times++) {
        do {
            pa_ptr->ma_ptr = &rand_choice(ma_blows);
            if (PlayerClass(player_ptr).equals(PlayerClassType::FORCETRAINER) && (pa_ptr->ma_ptr->min_level > 1)) {
                min_level = pa_ptr->ma_ptr->min_level + 3;
            } else {
                min_level = pa_ptr->ma_ptr->min_level;
            }
        } while ((min_level > player_ptr->lev) || (randint1(player_ptr->lev) < pa_ptr->ma_ptr->chance));

        auto effects = player_ptr->effects();
        auto is_stunned = effects->stun()->is_stunned();
        auto is_confused = effects->confusion()->is_confused();
        if ((pa_ptr->ma_ptr->min_level <= old_ptr->min_level) || is_stunned || is_confused) {
            pa_ptr->ma_ptr = old_ptr;
            continue;
        }

        old_ptr = pa_ptr->ma_ptr;
        if (w_ptr->wizard && cheat_xtra) {
            msg_print(_("攻撃を再選択しました。", "Attack re-selected."));
        }
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::FORCETRAINER)) {
        min_level = std::max(1, pa_ptr->ma_ptr->min_level - 3);
    } else {
        min_level = pa_ptr->ma_ptr->min_level;
    }

    return min_level;
}

static int process_monk_additional_effect(player_attack_type *pa_ptr, int *stun_effect)
{
    int special_effect = 0;
    auto *r_ptr = &monraces_info[pa_ptr->m_ptr->r_idx];
    if (pa_ptr->ma_ptr->effect == MA_KNEE) {
        if (r_ptr->flags1 & RF1_MALE) {
            msg_format(_("%sに金的膝蹴りをくらわした！", "You hit %s in the groin with your knee!"), pa_ptr->m_name);
            sound(SOUND_PAIN);
            special_effect = MA_KNEE;
        } else {
            msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
        }
    }

    else if (pa_ptr->ma_ptr->effect == MA_SLOW) {
        if (!(r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) || angband_strchr("~#{}.UjmeEv$,DdsbBFIJQSXclnw!=?", r_ptr->d_char))) {
            msg_format(_("%sの足首に関節蹴りをくらわした！", "You kick %s in the ankle."), pa_ptr->m_name);
            special_effect = MA_SLOW;
        } else {
            msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
        }
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 重さ
 */
static WEIGHT calc_monk_attack_weight(PlayerType *player_ptr)
{
    WEIGHT weight = 8;
    PlayerClass pc(player_ptr);
    if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        weight = 4;
    }

    if (pc.equals(PlayerClassType::FORCETRAINER) && (get_current_ki(player_ptr) != 0)) {
        weight += (get_current_ki(player_ptr) / 30);
        if (weight > 20) {
            weight = 20;
        }
    }

    return weight;
}

/*!
 * @brief 急所攻撃による追加効果を与える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param stun_effect 朦朧の残りターン
 * @param resist_stun 朦朧への抵抗値
 * @param special_effect 技を繰り出した時の追加効果
 */
static void process_attack_vital_spot(PlayerType *player_ptr, player_attack_type *pa_ptr, int *stun_effect, int *resist_stun, const int special_effect)
{
    auto *r_ptr = &monraces_info[pa_ptr->m_ptr->r_idx];
    if ((special_effect == MA_KNEE) && ((pa_ptr->attack_damage + player_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        msg_format(_("%s^は苦痛にうめいている！", "%s^ moans in agony!"), pa_ptr->m_name);
        *stun_effect = 7 + randint1(13);
        *resist_stun /= 3;
        return;
    }

    if ((special_effect == MA_SLOW) && ((pa_ptr->attack_damage + player_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        const auto is_unique = r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE);
        if (is_unique && (randint1(player_ptr->lev) > r_ptr->level) && (pa_ptr->m_ptr->mspeed > STANDARD_SPEED - 50)) {
            msg_format(_("%s^は足をひきずり始めた。", "You've hobbled %s."), pa_ptr->m_name);
            pa_ptr->m_ptr->mspeed -= 10;
        }
    }
}

/*!
 * @brief 朦朧効果を受けたモンスターのステータス表示
 * @param player_ptr プレイヤーの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 * @param stun_effect 朦朧の残りターン
 * @param resist_stun 朦朧への抵抗値
 */
static void print_stun_effect(PlayerType *player_ptr, player_attack_type *pa_ptr, const int stun_effect, const int resist_stun)
{
    auto *r_ptr = &monraces_info[pa_ptr->m_ptr->r_idx];
    if (stun_effect && ((pa_ptr->attack_damage + player_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        if (player_ptr->lev > randint1(r_ptr->level + resist_stun + 10)) {
            if (set_monster_stunned(player_ptr, pa_ptr->g_ptr->m_idx, stun_effect + pa_ptr->m_ptr->get_remaining_stun())) {
                msg_format(_("%s^はフラフラになった。", "%s^ is stunned."), pa_ptr->m_name);
            } else {
                msg_format(_("%s^はさらにフラフラになった。", "%s^ is more stunned."), pa_ptr->m_name);
            }
        }
    }
}

/*!
 * @brief 強力な素手攻撃ができる職業 (修行僧、狂戦士、練気術師)の素手攻撃処理メインルーチン
 * @param player_ptr プレイヤーの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 */
void process_monk_attack(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    int resist_stun = calc_stun_resistance(pa_ptr);
    int max_blow_selection_times = calc_max_blow_selection_times(player_ptr);
    int min_level = select_blow(player_ptr, pa_ptr, max_blow_selection_times);

    pa_ptr->attack_damage = damroll(pa_ptr->ma_ptr->dd + player_ptr->to_dd[pa_ptr->hand], pa_ptr->ma_ptr->ds + player_ptr->to_ds[pa_ptr->hand]);
    if (player_ptr->special_attack & ATTACK_SUIKEN) {
        pa_ptr->attack_damage *= 2;
    }

    int stun_effect = 0;
    int special_effect = process_monk_additional_effect(pa_ptr, &stun_effect);
    WEIGHT weight = calc_monk_attack_weight(player_ptr);
    pa_ptr->attack_damage = critical_norm(player_ptr, player_ptr->lev * weight, min_level, pa_ptr->attack_damage, player_ptr->to_h[0], HISSATSU_NONE);
    process_attack_vital_spot(player_ptr, pa_ptr, &stun_effect, &resist_stun, special_effect);
    print_stun_effect(player_ptr, pa_ptr, stun_effect, resist_stun);
}

bool double_attack(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_rep_dir(player_ptr, &dir)) {
        return false;
    }
    POSITION y = player_ptr->y + ddy[dir];
    POSITION x = player_ptr->x + ddx[dir];
    if (!player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
        msg_print(nullptr);
        return true;
    }

    if (one_in_(3)) {
        msg_print(_("あーたたたたたたたたたたたたたたたたたたたたたた！！！", "Ahhhtatatatatatatatatatatatatatataatatatatattaaaaa!!!!"));
    } else if (one_in_(2)) {
        msg_print(_("無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄！！！", "Mudamudamudamudamudamudamudamudamudamudamudamudamuda!!!!"));
    } else {
        msg_print(_("オラオラオラオラオラオラオラオラオラオラオラオラ！！！", "Oraoraoraoraoraoraoraoraoraoraoraoraoraoraoraoraora!!!!"));
    }

    do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
    if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        handle_stuff(player_ptr);
        do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
    }

    player_ptr->energy_need += ENERGY_NEED();
    return true;
}
