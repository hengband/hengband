/*!
 * @brief モンスターからプレーヤーへの直接攻撃処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "combat/monster-attack-player.h"
#include "cmd/cmd-attack.h"
#include "cmd/cmd-pet.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "combat/combat-options-type.h"
#include "combat/hallucination-attacks-table.h"
#include "combat/monster-attack-describer.h"
#include "combat/monster-attack-effect.h"
#include "combat/monster-attack-util.h"
#include "combat/monster-eating.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "main/sound-definitions-table.h"
#include "mind/racial-mirror-master.h"
#include "monster/monster-status.h"
#include "object/object-hook.h"
#include "player/player-damage.h"
#include "player/player-effects.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "realm/realm-hex.h"
#include "spell/process-effect.h"
#include "spell/spells-floor.h"
#include "spell/spells-type.h"
#include "spell/spells2.h"
#include "spell/spells3.h"

static bool check_no_blow(player_type *target_ptr, monap_type *monap_ptr)
{
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (r_ptr->flags1 & (RF1_NEVER_BLOW))
        return FALSE;

    if (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
        return FALSE;

    if (!is_hostile(monap_ptr->m_ptr))
        return FALSE;

    return TRUE;
}

/*!
 * @brief 時間逆転攻撃による能力低下
 * @param target_ptr プレーヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void describe_disability(player_type *target_ptr, monap_type *monap_ptr)
{
    int stat = randint0(6);
    switch (stat) {
    case A_STR:
        monap_ptr->act = _("強く", "strong");
        break;
    case A_INT:
        monap_ptr->act = _("聡明で", "bright");
        break;
    case A_WIS:
        monap_ptr->act = _("賢明で", "wise");
        break;
    case A_DEX:
        monap_ptr->act = _("器用で", "agile");
        break;
    case A_CON:
        monap_ptr->act = _("健康で", "hale");
        break;
    case A_CHR:
        monap_ptr->act = _("美しく", "beautiful");
        break;
    }

    msg_format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), monap_ptr->act);
    target_ptr->stat_cur[stat] = (target_ptr->stat_cur[stat] * 3) / 4;
    if (target_ptr->stat_cur[stat] < 3)
        target_ptr->stat_cur[stat] = 3;
}

static void process_monster_attack_time(player_type *target_ptr, monap_type *monap_ptr)
{
    if (target_ptr->resist_time || check_multishadow(target_ptr))
        return;

    switch (randint1(10)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5: {
        if (target_ptr->prace == RACE_ANDROID)
            break;

        msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
        lose_exp(target_ptr, 100 + (target_ptr->exp / 100) * MON_DRAIN_LIFE);
        break;
    }
    case 6:
    case 7:
    case 8:
    case 9: {
        describe_disability(target_ptr, monap_ptr);
        target_ptr->update |= (PU_BONUS);
        break;
    }
    case 10: {
        msg_print(_("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be..."));
        for (int i = 0; i < A_MAX; i++) {
            target_ptr->stat_cur[i] = (target_ptr->stat_cur[i] * 7) / 8;
            if (target_ptr->stat_cur[i] < 3)
                target_ptr->stat_cur[i] = 3;
        }

        target_ptr->update |= (PU_BONUS);
        break;
    }
    }
}

/*!
 * @brief プレーヤー死亡等でモンスターからプレーヤーへの直接攻撃処理を途中で打ち切るかどうかを判定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return 攻撃続行ならばTRUE、打ち切りになったらFALSE
 */
static bool check_monster_attack_terminated(player_type *target_ptr, monap_type *monap_ptr)
{
    if (!monster_is_valid(monap_ptr->m_ptr))
        return FALSE;

    if (monap_ptr->method == RBM_NONE)
        return FALSE;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (is_pet(monap_ptr->m_ptr) && (r_ptr->flags1 & RF1_UNIQUE) && (monap_ptr->method == RBM_EXPLODE)) {
        monap_ptr->method = RBM_HIT;
        monap_ptr->d_dice /= 10;
    }

    if (!target_ptr->playing || target_ptr->is_dead || (distance(target_ptr->y, target_ptr->x, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx) > 1)
        || target_ptr->leaving)
        return FALSE;

    return TRUE;
}

/*!
 * @brief 対邪悪結界が効いている状態で邪悪なモンスターから直接攻撃を受けた時の処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return briefに書いた条件＋確率が満たされたらTRUE、それ以外はFALSE
 */
static bool effect_protecion_from_evil(player_type *target_ptr, monap_type *monap_ptr)
{
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((target_ptr->protevil <= 0) || ((r_ptr->flags3 & RF3_EVIL) == 0) || (target_ptr->lev < monap_ptr->rlev) || ((randint0(100) + target_ptr->lev) <= 50))
        return FALSE;

    if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
        r_ptr->r_flags3 |= RF3_EVIL;

#ifdef JP
    if (monap_ptr->abbreviate)
        msg_format("撃退した。");
    else
        msg_format("%^sは撃退された。", monap_ptr->m_name);

    monap_ptr->abbreviate = 1; /* 2回目以降は省略 */
#else
    msg_format("%^s is repelled.", monap_ptr->m_name);
#endif

    return TRUE;
}

static void describe_silly_attacks(monap_type *monap_ptr)
{
    if (monap_ptr->act == NULL)
        return;

    if (monap_ptr->do_silly_attack) {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
    }

#ifdef JP
    if (monap_ptr->abbreviate == 0)
        msg_format("%^sに%s", monap_ptr->m_name, monap_ptr->act);
    else if (monap_ptr->abbreviate == 1)
        msg_format("%s", monap_ptr->act);
    else /* if (monap_ptr->abbreviate == -1) */
        msg_format("%^s%s", monap_ptr->m_name, monap_ptr->act);

    monap_ptr->abbreviate = 1; /*2回目以降は省略 */
#else
    msg_format("%^s %s%s", monap_ptr->m_name, monap_ptr->act, monap_ptr->do_silly_attack ? " you." : "");
#endif
}

static void process_blind_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (target_ptr->resist_blind || check_multishadow(target_ptr))
        return;

    if (!set_blind(target_ptr, target_ptr->blind + 10 + randint1(monap_ptr->rlev)))
        return;

    if (monap_ptr->m_ptr->r_idx == MON_DIO)
        msg_print(_("どうだッ！この血の目潰しはッ！", "How is it! This blood-blinding!"));

    monap_ptr->obvious = TRUE;
}

static void process_terrify_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (check_multishadow(target_ptr))
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (target_ptr->resist_fear) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = TRUE;
        return;
    }

    if (randint0(100 + r_ptr->level / 2) < target_ptr->skill_sav) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = TRUE;
        return;
    }

    if (set_afraid(target_ptr, target_ptr->afraid + 3 + randint1(monap_ptr->rlev)))
        monap_ptr->obvious = TRUE;
}

static void process_paralyze_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (check_multishadow(target_ptr))
        return;
    
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (target_ptr->free_act) {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
        monap_ptr->obvious = TRUE;
        return;
    }

    if (randint0(100 + r_ptr->level / 2) < target_ptr->skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        monap_ptr->obvious = TRUE;
        return;
    }

    if (!target_ptr->paralyzed && set_paralyzed(target_ptr, 3 + randint1(monap_ptr->rlev)))
        monap_ptr->obvious = TRUE;
}

static void process_lose_all_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (do_dec_stat(target_ptr, A_STR))
        monap_ptr->obvious = TRUE;

    if (do_dec_stat(target_ptr, A_DEX))
        monap_ptr->obvious = TRUE;

    if (do_dec_stat(target_ptr, A_CON))
        monap_ptr->obvious = TRUE;

    if (do_dec_stat(target_ptr, A_INT))
        monap_ptr->obvious = TRUE;

    if (do_dec_stat(target_ptr, A_WIS))
        monap_ptr->obvious = TRUE;

    if (do_dec_stat(target_ptr, A_CHR))
        monap_ptr->obvious = TRUE;
}

/*!
 * @brief モンスターからプレイヤーへの打撃処理 / Attack the player via physical attacks.
 * @param m_idx 打撃を行うモンスターのID
 * @return 実際に攻撃処理を行った場合TRUEを返す
 */
bool make_attack_normal(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monap_type tmp_monap;
    monap_type *monap_ptr = initialize_monap_type(target_ptr, &tmp_monap, m_idx);

    int tmp;
    bool fear = FALSE;
    bool alive = TRUE;
    check_no_blow(target_ptr, monap_ptr);
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    monap_ptr->rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    monster_desc(target_ptr, monap_ptr->m_name, monap_ptr->m_ptr, 0);
    monster_desc(target_ptr, monap_ptr->ddesc, monap_ptr->m_ptr, MD_WRONGDOER_NAME);
    if (target_ptr->special_defense & KATA_IAI) {
        msg_format(_("相手が襲いかかる前に素早く武器を振るった。", "You took sen, drew and cut in one motion before %s moved."), monap_ptr->m_name);
        if (do_cmd_attack(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_IAI))
            return TRUE;
    }

    if ((target_ptr->special_defense & NINJA_KAWARIMI) && (randint0(55) < (target_ptr->lev * 3 / 5 + 20))) {
        if (kawarimi(target_ptr, TRUE))
            return TRUE;
    }

    monap_ptr->blinked = FALSE;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (int ap_cnt = 0; ap_cnt < 4; ap_cnt++) {
        monap_ptr->obvious = FALSE;
        HIT_POINT power = 0;
        monap_ptr->damage = 0;
        monap_ptr->act = NULL;
        monap_ptr->effect = r_ptr->blow[ap_cnt].effect;
        monap_ptr->method = r_ptr->blow[ap_cnt].method;
        monap_ptr->d_dice = r_ptr->blow[ap_cnt].d_dice;
        monap_ptr->d_side = r_ptr->blow[ap_cnt].d_side;

        if (!check_monster_attack_terminated(target_ptr, monap_ptr))
            break;

        if (monap_ptr->method == RBM_SHOOT)
            continue;

        power = mbe_info[monap_ptr->effect].power;
        monap_ptr->ac = target_ptr->ac + target_ptr->to_a;
        if (!monap_ptr->effect || check_hit_from_monster_to_player(target_ptr, power, monap_ptr->rlev, MON_STUNNED(monap_ptr->m_ptr))) {
            disturb(target_ptr, TRUE, TRUE);
            if (effect_protecion_from_evil(target_ptr, monap_ptr))
                continue;

            monap_ptr->do_cut = 0;
            monap_ptr->do_stun = 0;
            describe_monster_attack_method(monap_ptr);
            describe_silly_attacks(monap_ptr);
            monap_ptr->obvious = TRUE;
            monap_ptr->damage = damroll(monap_ptr->d_dice, monap_ptr->d_side);
            if (monap_ptr->explode)
                monap_ptr->damage = 0;

            switch (monap_ptr->effect) {
            case RBE_NONE: {
                monap_ptr->obvious = TRUE;
                monap_ptr->damage = 0;
                break;
            }
            case RBE_SUPERHURT: { /* AC軽減あり / Player armor reduces total damage */
                if (((randint1(monap_ptr->rlev * 2 + 300) > (monap_ptr->ac + 200)) || one_in_(13)) && !check_multishadow(target_ptr)) {
                    int tmp_damage = monap_ptr->damage - (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
                    msg_print(_("痛恨の一撃！", "It was a critical hit!"));
                    tmp_damage = MAX(monap_ptr->damage, tmp_damage * 2);
                    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, tmp_damage, monap_ptr->ddesc, -1);
                    break;
                }
            }
                /* Fall through */
            case RBE_HURT: {/* AC軽減あり / Player armor reduces total damage */
                monap_ptr->obvious = TRUE;
                monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                break;
            }
            case RBE_POISON: {
                if (monap_ptr->explode)
                    break;

                if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr)) && !check_multishadow(target_ptr)) {
                    if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5)) {
                        monap_ptr->obvious = TRUE;
                    }
                }

                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                update_smart_learn(target_ptr, m_idx, DRS_POIS);
                break;
            }
            case RBE_UN_BONUS: {
                if (monap_ptr->explode)
                    break;

                if (!target_ptr->resist_disen && !check_multishadow(target_ptr)) {
                    if (apply_disenchant(target_ptr, 0)) {
                        update_creature(target_ptr);
                        monap_ptr->obvious = TRUE;
                    }
                }

                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                update_smart_learn(target_ptr, m_idx, DRS_DISEN);
                break;
            }
            case RBE_UN_POWER: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                for (int i = 0; i < 10; i++) {
                    INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
                    monap_ptr->o_ptr = &target_ptr->inventory_list[i_idx];
                    if (monap_ptr->o_ptr->k_idx == 0)
                        continue;

                    if (process_un_power(target_ptr, monap_ptr))
                        break;
                }

                break;
            }
            case RBE_EAT_GOLD: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (MON_CONFUSED(monap_ptr->m_ptr))
                    break;

                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                monap_ptr->obvious = TRUE;
                process_eat_gold(target_ptr, monap_ptr);
                break;
            }
            case RBE_EAT_ITEM: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (!check_eat_item(target_ptr, monap_ptr))
                    break;

                process_eat_item(target_ptr, monap_ptr);
                break;
            }

            case RBE_EAT_FOOD: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                process_eat_food(target_ptr, monap_ptr);
                break;
            }
            case RBE_EAT_LITE: {
                monap_ptr->o_ptr = &target_ptr->inventory_list[INVEN_LITE];
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                process_eat_lite(target_ptr, monap_ptr);
                break;
            }
            case RBE_ACID: {
                if (monap_ptr->explode)
                    break;

                monap_ptr->obvious = TRUE;
                msg_print(_("酸を浴びせられた！", "You are covered in acid!"));
                monap_ptr->get_damage += acid_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
                update_creature(target_ptr);
                update_smart_learn(target_ptr, m_idx, DRS_ACID);
                break;
            }
            case RBE_ELEC: {
                if (monap_ptr->explode)
                    break;
                monap_ptr->obvious = TRUE;
                msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));
                monap_ptr->get_damage += elec_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
                update_smart_learn(target_ptr, m_idx, DRS_ELEC);
                break;
            }
            case RBE_FIRE: {
                if (monap_ptr->explode)
                    break;
                monap_ptr->obvious = TRUE;
                msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));
                monap_ptr->get_damage += fire_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
                update_smart_learn(target_ptr, m_idx, DRS_FIRE);
                break;
            }
            case RBE_COLD: {
                if (monap_ptr->explode)
                    break;
                monap_ptr->obvious = TRUE;
                msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));
                monap_ptr->get_damage += cold_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
                update_smart_learn(target_ptr, m_idx, DRS_COLD);
                break;
            }
            case RBE_BLIND: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                process_blind_attack(target_ptr, monap_ptr);
                update_smart_learn(target_ptr, m_idx, DRS_BLIND);
                break;
            }
            case RBE_CONFUSE: {
                if (monap_ptr->explode)
                    break;

                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (!target_ptr->resist_conf && !check_multishadow(target_ptr)) {
                    if (set_confused(target_ptr, target_ptr->confused + 3 + randint1(monap_ptr->rlev))) {
                        monap_ptr->obvious = TRUE;
                    }
                }

                update_smart_learn(target_ptr, m_idx, DRS_CONF);
                break;
            }
            case RBE_TERRIFY: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                process_terrify_attack(target_ptr, monap_ptr);
                update_smart_learn(target_ptr, m_idx, DRS_FEAR);
                break;
            }
            case RBE_PARALYZE: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);

                if (target_ptr->is_dead)
                    break;

                process_paralyze_attack(target_ptr, monap_ptr);
                update_smart_learn(target_ptr, m_idx, DRS_FREE);
                break;
            }
            case RBE_LOSE_STR: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_STR))
                    monap_ptr->obvious = TRUE;

                break;
            }
            case RBE_LOSE_INT: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_INT))
                    monap_ptr->obvious = TRUE;

                break;
            }
            case RBE_LOSE_WIS: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_WIS))
                    monap_ptr->obvious = TRUE;

                break;
            }
            case RBE_LOSE_DEX: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_DEX))
                    monap_ptr->obvious = TRUE;

                break;
            }
            case RBE_LOSE_CON: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_CON))
                    monap_ptr->obvious = TRUE;

                break;
            }
            case RBE_LOSE_CHR: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_CHR))
                    monap_ptr->obvious = TRUE;

                break;
            }
            case RBE_LOSE_ALL: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                process_lose_all_attack(target_ptr, monap_ptr);
                break;
            }
            case RBE_SHATTER: { /* AC軽減あり / Player armor reduces total damage */
                monap_ptr->obvious = TRUE;
                monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (monap_ptr->damage > 23 || monap_ptr->explode)
                    earthquake(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, 8, m_idx);

                break;
            }
            case RBE_EXP_10: {
                s32b d = damroll(10, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                monap_ptr->obvious = TRUE;
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 95);
                break;
            }
            case RBE_EXP_20: {
                s32b d = damroll(20, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                monap_ptr->obvious = TRUE;
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 90);
                break;
            }
            case RBE_EXP_40: {
                s32b d = damroll(40, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                monap_ptr->obvious = TRUE;
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 75);
                break;
            }
            case RBE_EXP_80: {
                s32b d = damroll(80, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                monap_ptr->obvious = TRUE;
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 50);
                break;
            }
            case RBE_DISEASE: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr))) {
                    if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5)) {
                        monap_ptr->obvious = TRUE;
                    }
                }

                if ((randint1(100) < 11) && (target_ptr->prace != RACE_ANDROID)) {
                    bool perm = one_in_(10);
                    if (dec_stat(target_ptr, A_CON, randint1(10), perm)) {
                        msg_print(_("病があなたを蝕んでいる気がする。", "You feel sickly."));
                        monap_ptr->obvious = TRUE;
                    }
                }

                break;
            }
            case RBE_TIME: {
                if (monap_ptr->explode)
                    break;

                process_monster_attack_time(target_ptr, monap_ptr);
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                break;
            }
            case RBE_DR_LIFE: {
                s32b d = damroll(60, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                monap_ptr->obvious = TRUE;
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                bool resist_drain = check_drain_hp(target_ptr, d);
                process_drain_life(target_ptr, monap_ptr, resist_drain);
                break;
            }
            case RBE_DR_MANA: {
                monap_ptr->obvious = TRUE;
                if (check_multishadow(target_ptr)) {
                    msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
                } else {
                    monap_ptr->do_cut = 0;
                    target_ptr->csp -= monap_ptr->damage;
                    if (target_ptr->csp < 0) {
                        target_ptr->csp = 0;
                        target_ptr->csp_frac = 0;
                    }

                    target_ptr->redraw |= (PR_MANA);
                }

                update_smart_learn(target_ptr, m_idx, DRS_MANA);
                break;
            }
            case RBE_INERTIA: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (check_multishadow(target_ptr)) {
                    /* Do nothing */
                } else {
                    if (set_slow(target_ptr, (target_ptr->slow + 4 + randint0(monap_ptr->rlev / 10)), FALSE)) {
                        monap_ptr->obvious = TRUE;
                    }
                }

                break;
            }
            case RBE_STUN: {
                monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (target_ptr->resist_sound || check_multishadow(target_ptr)) {
                    /* Do nothing */
                } else {
                    if (set_stun(target_ptr, target_ptr->stun + 10 + randint1(r_ptr->level / 4))) {
                        monap_ptr->obvious = TRUE;
                    }
                }

                break;
            }
            }

            // TODO 三項演算子に差し替え.
            if (monap_ptr->do_cut && monap_ptr->do_stun) {
                if (randint0(100) < 50) {
                    monap_ptr->do_cut = 0;
                }
                else {
                    monap_ptr->do_stun = 0;
                }
            }

            if (monap_ptr->do_cut) {
                int cut_plus = 0;
                tmp = calc_monster_critical(monap_ptr->d_dice, monap_ptr->d_side, monap_ptr->damage);
                switch (tmp) {
                case 0:
                    cut_plus = 0;
                    break;
                case 1:
                    cut_plus = randint1(5);
                    break;
                case 2:
                    cut_plus = randint1(5) + 5;
                    break;
                case 3:
                    cut_plus = randint1(20) + 20;
                    break;
                case 4:
                    cut_plus = randint1(50) + 50;
                    break;
                case 5:
                    cut_plus = randint1(100) + 100;
                    break;
                case 6:
                    cut_plus = 300;
                    break;
                default:
                    cut_plus = 500;
                    break;
                }

                if (cut_plus)
                    (void)set_cut(target_ptr, target_ptr->cut + cut_plus);
            }

            if (monap_ptr->do_stun) {
                int stun_plus = 0;
                tmp = calc_monster_critical(monap_ptr->d_dice, monap_ptr->d_side, monap_ptr->damage);
                switch (tmp) {
                case 0:
                    stun_plus = 0;
                    break;
                case 1:
                    stun_plus = randint1(5);
                    break;
                case 2:
                    stun_plus = randint1(5) + 10;
                    break;
                case 3:
                    stun_plus = randint1(10) + 20;
                    break;
                case 4:
                    stun_plus = randint1(15) + 30;
                    break;
                case 5:
                    stun_plus = randint1(20) + 40;
                    break;
                case 6:
                    stun_plus = 80;
                    break;
                default:
                    stun_plus = 150;
                    break;
                }

                if (stun_plus)
                    (void)set_stun(target_ptr, target_ptr->stun + stun_plus);
            }

            if (monap_ptr->explode) {
                sound(SOUND_EXPLODE);

                if (mon_take_hit(target_ptr, m_idx, monap_ptr->m_ptr->hp + 1, &fear, NULL)) {
                    monap_ptr->blinked = FALSE;
                    alive = FALSE;
                }
            }

            if (monap_ptr->touched) {
                if (target_ptr->sh_fire && alive && !target_ptr->is_dead) {
                    if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)) {
                        HIT_POINT dam = damroll(2, 6);
                        dam = mon_damage_mod(target_ptr, monap_ptr->m_ptr, dam, FALSE);
                        msg_format(_("%^sは突然熱くなった！", "%^s is suddenly very hot!"), monap_ptr->m_name);
                        if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は灰の山になった。", " turns into a pile of ash."))) {
                            monap_ptr->blinked = FALSE;
                            alive = FALSE;
                        }

                    } else {
                        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
                    }
                }

                if (target_ptr->sh_elec && alive && !target_ptr->is_dead) {
                    if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK)) {
                        HIT_POINT dam = damroll(2, 6);
                        dam = mon_damage_mod(target_ptr, monap_ptr->m_ptr, dam, FALSE);
                        msg_format(_("%^sは電撃をくらった！", "%^s gets zapped!"), monap_ptr->m_name);
                        if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は燃え殻の山になった。", " turns into a pile of cinder."))) {
                            monap_ptr->blinked = FALSE;
                            alive = FALSE;
                        }
                    } else {
                        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
                    }
                }

                if (target_ptr->sh_cold && alive && !target_ptr->is_dead) {
                    if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK)) {
                        HIT_POINT dam = damroll(2, 6);
                        dam = mon_damage_mod(target_ptr, monap_ptr->m_ptr, dam, FALSE);
                        msg_format(_("%^sは冷気をくらった！", "%^s is very cold!"), monap_ptr->m_name);
                        if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は凍りついた。", " was frozen."))) {
                            monap_ptr->blinked = FALSE;
                            alive = FALSE;
                        }
                    } else {
                        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
                    }
                }

                if (target_ptr->dustrobe && alive && !target_ptr->is_dead) {
                    if (!(r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK)) {
                        HIT_POINT dam = damroll(2, 6);
                        dam = mon_damage_mod(target_ptr, monap_ptr->m_ptr, dam, FALSE);
                        msg_format(_("%^sは鏡の破片をくらった！", "%^s gets zapped!"), monap_ptr->m_name);
                        if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("はズタズタになった。", " had torn to pieces."))) {
                            monap_ptr->blinked = FALSE;
                            alive = FALSE;
                        }
                    } else {
                        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK);
                    }

                    if (is_mirror_grid(&floor_ptr->grid_array[target_ptr->y][target_ptr->x])) {
                        teleport_player(target_ptr, 10, TELEPORT_SPONTANEOUS);
                    }
                }

                if (target_ptr->tim_sh_holy && alive && !target_ptr->is_dead) {
                    if (r_ptr->flags3 & RF3_EVIL) {
                        if (!(r_ptr->flagsr & RFR_RES_ALL)) {
                            HIT_POINT dam = damroll(2, 6);
                            dam = mon_damage_mod(target_ptr, monap_ptr->m_ptr, dam, FALSE);
                            msg_format(_("%^sは聖なるオーラで傷ついた！", "%^s is injured by holy power!"), monap_ptr->m_name);
                            if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は倒れた。", " is destroyed."))) {
                                monap_ptr->blinked = FALSE;
                                alive = FALSE;
                            }
                            if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                                r_ptr->r_flags3 |= RF3_EVIL;
                        } else {
                            if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                                r_ptr->r_flagsr |= RFR_RES_ALL;
                        }
                    }
                }

                if (target_ptr->tim_sh_touki && alive && !target_ptr->is_dead) {
                    if (!(r_ptr->flagsr & RFR_RES_ALL)) {
                        HIT_POINT dam = damroll(2, 6);
                        dam = mon_damage_mod(target_ptr, monap_ptr->m_ptr, dam, FALSE);
                        msg_format(_("%^sが鋭い闘気のオーラで傷ついた！", "%^s is injured by the Force"), monap_ptr->m_name);
                        if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は倒れた。", " is destroyed."))) {
                            monap_ptr->blinked = FALSE;
                            alive = FALSE;
                        }
                    } else {
                        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                            r_ptr->r_flagsr |= RFR_RES_ALL;
                    }
                }

                if (hex_spelling(target_ptr, HEX_SHADOW_CLOAK) && alive && !target_ptr->is_dead) {
                    HIT_POINT dam = 1;
                    object_type *o_armed_ptr = &target_ptr->inventory_list[INVEN_RARM];
                    if (!(r_ptr->flagsr & RFR_RES_ALL || r_ptr->flagsr & RFR_RES_DARK)) {
                        if (o_armed_ptr->k_idx) {
                            int basedam = ((o_armed_ptr->dd + target_ptr->to_dd[0]) * (o_armed_ptr->ds + target_ptr->to_ds[0] + 1));
                            dam = basedam / 2 + o_armed_ptr->to_d + target_ptr->to_d[0];
                        }

                        o_armed_ptr = &target_ptr->inventory_list[INVEN_BODY];
                        if ((o_armed_ptr->k_idx) && object_is_cursed(o_armed_ptr))
                            dam *= 2;

                        dam = mon_damage_mod(target_ptr, monap_ptr->m_ptr, dam, FALSE);
                        msg_format(_("影のオーラが%^sに反撃した！", "Enveloping shadows attack %^s."), monap_ptr->m_name);
                        if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は倒れた。", " is destroyed."))) {
                            monap_ptr->blinked = FALSE;
                            alive = FALSE;
                        } else /* monster does not dead */
                        {
                            int j;
                            BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
                            EFFECT_ID typ[4][2]
                                = { { INVEN_HEAD, GF_OLD_CONF }, { INVEN_LARM, GF_OLD_SLEEP }, { INVEN_HANDS, GF_TURN_ALL }, { INVEN_FEET, GF_OLD_SLOW } };

                            /* Some cursed armours gives an extra effect */
                            for (j = 0; j < 4; j++) {
                                o_armed_ptr = &target_ptr->inventory_list[typ[j][0]];
                                if ((o_armed_ptr->k_idx) && object_is_cursed(o_armed_ptr) && object_is_armour(o_armed_ptr))
                                    project(target_ptr, 0, 0, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, (target_ptr->lev * 2), typ[j][1], flg, -1);
                            }
                        }
                    } else {
                        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr))
                            r_ptr->r_flagsr |= (RFR_RES_ALL | RFR_RES_DARK);
                    }
                }
            }
        }
        else {
            switch (monap_ptr->method) {
            case RBM_HIT:
            case RBM_TOUCH:
            case RBM_PUNCH:
            case RBM_KICK:
            case RBM_CLAW:
            case RBM_BITE:
            case RBM_STING:
            case RBM_SLASH:
            case RBM_BUTT:
            case RBM_CRUSH:
            case RBM_ENGULF:
            case RBM_CHARGE:
                if (monap_ptr->m_ptr->ml) {
                    disturb(target_ptr, TRUE, TRUE);
#ifdef JP
                    if (monap_ptr->abbreviate)
                        msg_format("%sかわした。", (target_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "");
                    else
                        msg_format("%s%^sの攻撃をかわした。", (target_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "", monap_ptr->m_name);

                    monap_ptr->abbreviate = 1; /*2回目以降は省略 */
#else
                    msg_format("%^s misses you.", monap_ptr->m_name);
#endif
                }

                if (object_is_armour(&target_ptr->inventory_list[INVEN_RARM]) || object_is_armour(&target_ptr->inventory_list[INVEN_LARM])) {
                    int cur = target_ptr->skill_exp[GINOU_SHIELD];
                    int max = s_info[target_ptr->pclass].s_max[GINOU_SHIELD];
                    if (cur < max) {
                        DEPTH targetlevel = r_ptr->level;
                        int inc = 0;
                        if ((cur / 100) < targetlevel) {
                            if ((cur / 100 + 15) < targetlevel)
                                inc += 1 + (targetlevel - (cur / 100 + 15));
                            else
                                inc += 1;
                        }

                        target_ptr->skill_exp[GINOU_SHIELD] = MIN(max, cur + inc);
                        target_ptr->update |= (PU_BONUS);
                    }
                }

                monap_ptr->damage = 0;
                break;
            }
        }

        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr) && !monap_ptr->do_silly_attack) {
            if (monap_ptr->obvious || monap_ptr->damage || (r_ptr->r_blows[ap_cnt] > 10)) {
                if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR) {
                    r_ptr->r_blows[ap_cnt]++;
                }
            }
        }

        if (target_ptr->riding && monap_ptr->damage) {
            char m_steed_name[MAX_NLEN];
            monster_desc(target_ptr, m_steed_name, &floor_ptr->m_list[target_ptr->riding], 0);
            if (process_fall_off_horse(target_ptr, (monap_ptr->damage > 200) ? 200 : monap_ptr->damage, FALSE)) {
                msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_steed_name);
            }
        }

        if (target_ptr->special_defense & NINJA_KAWARIMI) {
            if (kawarimi(target_ptr, FALSE))
                return TRUE;
        }
    }

    revenge_store(target_ptr, monap_ptr->get_damage);
    if ((target_ptr->tim_eyeeye || hex_spelling(target_ptr, HEX_EYE_FOR_EYE)) && monap_ptr->get_damage > 0 && !target_ptr->is_dead) {
#ifdef JP
        msg_format("攻撃が%s自身を傷つけた！", monap_ptr->m_name);
#else
        GAME_TEXT m_name_self[80];
        monster_desc(target_ptr, m_name_self, monap_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
        msg_format("The attack of %s has wounded %s!", monap_ptr->m_name, m_name_self);
#endif
        project(target_ptr, 0, 0, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, monap_ptr->get_damage, GF_MISSILE, PROJECT_KILL, -1);
        if (target_ptr->tim_eyeeye)
            set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, TRUE);
    }

    if ((target_ptr->counter || (target_ptr->special_defense & KATA_MUSOU)) && alive && !target_ptr->is_dead && monap_ptr->m_ptr->ml && (target_ptr->csp > 7)) {
        char m_target_name[MAX_NLEN];
        monster_desc(target_ptr, m_target_name, monap_ptr->m_ptr, 0);
        target_ptr->csp -= 7;
        msg_format(_("%^sに反撃した！", "You counterattacked %s!"), m_target_name);
        do_cmd_attack(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_COUNTER);
        fear = FALSE;
        target_ptr->redraw |= (PR_MANA);
    }

    if (monap_ptr->blinked && alive && !target_ptr->is_dead) {
        if (teleport_barrier(target_ptr, m_idx)) {
            msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
        } else {
            msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
            teleport_away(target_ptr, m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
        }
    }

    if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !floor_ptr->inside_arena)
        r_ptr->r_deaths++;

    if (monap_ptr->m_ptr->ml && fear && alive && !target_ptr->is_dead) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖で逃げ出した！", "%^s flees in terror!"), monap_ptr->m_name);
    }

    if (target_ptr->special_defense & KATA_IAI) {
        set_action(target_ptr, ACTION_NONE);
    }

    return TRUE;
}
