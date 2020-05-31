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
#include "combat/monster-attack-switcher.h"
#include "combat/monster-attack-util.h"
#include "combat/aura-counterattack.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "main/sound-definitions-table.h"
#include "monster/monster-status.h"
#include "object/object-hook.h"
#include "player/player-damage.h"
#include "player/player-effects.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "realm/realm-hex.h"
#include "spell/process-effect.h"
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

/*!
 * @brief 切り傷と朦朧が同時に発生した時、片方を無効にする
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void select_cut_stun(monap_type *monap_ptr)
{
    if ((monap_ptr->do_cut == 0) || (monap_ptr->do_stun == 0))
        return;

    if (randint0(100) < 50)
        monap_ptr->do_cut = 0;
    else
        monap_ptr->do_stun = 0;
}

static void calc_player_cut(player_type *target_ptr, monap_type *monap_ptr)
{
    if (monap_ptr->do_cut == 0)
        return;

    int cut_plus = 0;
    int criticality = calc_monster_critical(monap_ptr->d_dice, monap_ptr->d_side, monap_ptr->damage);
    switch (criticality) {
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

    if (cut_plus > 0)
        (void)set_cut(target_ptr, target_ptr->cut + cut_plus);
}

static void calc_player_stun(player_type *target_ptr, monap_type *monap_ptr)
{
    if (monap_ptr->do_stun == 0)
        return;

    int stun_plus = 0;
    int criticality = calc_monster_critical(monap_ptr->d_dice, monap_ptr->d_side, monap_ptr->damage);
    switch (criticality) {
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

    if (stun_plus > 0)
        (void)set_stun(target_ptr, target_ptr->stun + stun_plus);
}

static void monster_explode(player_type *target_ptr, monap_type *monap_ptr)
{
    if (!monap_ptr->explode)
        return;

    sound(SOUND_EXPLODE);
    if (mon_take_hit(target_ptr, monap_ptr->m_idx, monap_ptr->m_ptr->hp + 1, &monap_ptr->fear, NULL)) {
        monap_ptr->blinked = FALSE;
        monap_ptr->alive = FALSE;
    }
}

static void describe_attack_evasion(player_type *target_ptr, monap_type *monap_ptr)
{
    if (!monap_ptr->m_ptr->ml)
        return;

    disturb(target_ptr, TRUE, TRUE);
#ifdef JP
    if (monap_ptr->abbreviate)
        msg_format("%sかわした。", (target_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "");
    else
        msg_format("%s%^sの攻撃をかわした。", (target_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "", monap_ptr->m_name);

    monap_ptr->abbreviate = 1; /* 2回目以降は省略 */
#else
    msg_format("%^s misses you.", monap_ptr->m_name);
#endif
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
        if ((monap_ptr->effect == RBE_NONE) || check_hit_from_monster_to_player(target_ptr, power, monap_ptr->rlev, MON_STUNNED(monap_ptr->m_ptr))) {
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

            switch_monster_blow_to_player(target_ptr, monap_ptr);
            select_cut_stun(monap_ptr);
            calc_player_cut(target_ptr, monap_ptr);
            calc_player_stun(target_ptr, monap_ptr);
            monster_explode(target_ptr, monap_ptr);
            process_aura_counterattack(target_ptr, monap_ptr);
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
                describe_attack_evasion(target_ptr, monap_ptr);
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

    if ((target_ptr->counter || (target_ptr->special_defense & KATA_MUSOU)) && monap_ptr->alive && !target_ptr->is_dead && monap_ptr->m_ptr->ml && (target_ptr->csp > 7)) {
        char m_target_name[MAX_NLEN];
        monster_desc(target_ptr, m_target_name, monap_ptr->m_ptr, 0);
        target_ptr->csp -= 7;
        msg_format(_("%^sに反撃した！", "You counterattacked %s!"), m_target_name);
        do_cmd_attack(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_COUNTER);
        monap_ptr->fear = FALSE;
        target_ptr->redraw |= (PR_MANA);
    }

    if (monap_ptr->blinked && monap_ptr->alive && !target_ptr->is_dead) {
        if (teleport_barrier(target_ptr, m_idx)) {
            msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
        } else {
            msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
            teleport_away(target_ptr, m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
        }
    }

    if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !floor_ptr->inside_arena)
        r_ptr->r_deaths++;

    if (monap_ptr->m_ptr->ml && monap_ptr->fear && monap_ptr->alive && !target_ptr->is_dead) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖で逃げ出した！", "%^s flees in terror!"), monap_ptr->m_name);
    }

    if (target_ptr->special_defense & KATA_IAI) {
        set_action(target_ptr, ACTION_NONE);
    }

    return TRUE;
}
