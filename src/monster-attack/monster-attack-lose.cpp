#include "monster-attack/monster-attack-lose.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-status.h"
#include "monster-attack/monster-attack-util.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 病気ダメージを計算する (毒耐性があれば、(1d4 + 4) / 9になる。二重耐性なら更に(1d4 + 4) / 9)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @details 10% (毒の一次耐性があれば4%、二重耐性ならば1.6%)の確率で耐久が低下し、更に1/10の確率で永久低下する
 */
void calc_blow_disease(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_resist_pois(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;

    if (is_oppose_pois(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (!(has_resist_pois(target_ptr) || is_oppose_pois(target_ptr)) && set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5))
        monap_ptr->obvious = true;

    bool disease_possibility = randint1(100) > calc_nuke_damage_rate(target_ptr);
    if (disease_possibility || (randint1(100) > 10) || (target_ptr->prace == player_race_type::ANDROID))
        return;

    bool perm = one_in_(10);
    if (dec_stat(target_ptr, A_CON, randint1(10), perm)) {
        msg_print(_("病があなたを蝕んでいる気がする。", "You feel sickly."));
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 腕力低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_strength(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_sustain_str(target_ptr))
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (do_dec_stat(target_ptr, A_STR))
        monap_ptr->obvious = true;
}

/*!
 * @brief 知能低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_intelligence(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_sustain_int(target_ptr))
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (do_dec_stat(target_ptr, A_INT))
        monap_ptr->obvious = true;
}

/*!
 * @brief 賢さ低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_wisdom(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_sustain_wis(target_ptr))
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (do_dec_stat(target_ptr, A_WIS))
        monap_ptr->obvious = true;
}

/*!
 * @brief 器用低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_dexterity(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_sustain_dex(target_ptr))
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (do_dec_stat(target_ptr, A_DEX))
        monap_ptr->obvious = true;
}

/*!
 * @brief 耐久低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_constitution(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_sustain_con(target_ptr))
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (do_dec_stat(target_ptr, A_CON))
        monap_ptr->obvious = true;
}

/*!
 * @brief 魅力低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_charisma(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_sustain_chr(target_ptr))
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (do_dec_stat(target_ptr, A_CHR))
        monap_ptr->obvious = true;
}

/*!
 * @brief 全能力低下ダメージを計算する (維持があれば、1つに付き-3%軽減する)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_all(player_type *target_ptr, monap_type *monap_ptr)
{
    int damage_ratio = 100;
    if (has_sustain_str(target_ptr))
        damage_ratio -= 3;

    if (has_sustain_int(target_ptr))
        damage_ratio -= 3;

    if (has_sustain_wis(target_ptr))
        damage_ratio -= 3;

    if (has_sustain_dex(target_ptr))
        damage_ratio -= 3;

    if (has_sustain_con(target_ptr))
        damage_ratio -= 3;

    if (has_sustain_chr(target_ptr))
        damage_ratio -= 3;

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 100;
    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    process_lose_all_attack(target_ptr, monap_ptr);
}
