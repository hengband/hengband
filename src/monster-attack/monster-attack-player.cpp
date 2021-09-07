/*!
 * @brief モンスターからプレーヤーへの直接攻撃処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "monster-attack/monster-attack-player.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-pet.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "combat/aura-counterattack.h"
#include "combat/combat-options-type.h"
#include "combat/hallucination-attacks-table.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/geometry.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "mind/mind-samurai.h"
#include "monster-attack/monster-attack-describer.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-switcher.h"
#include "monster-attack/monster-attack-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object-hook/hook-armor.h"
#include "object/item-tester-hooker.h"
#include "pet/pet-fall-off.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

static bool check_no_blow(player_type *target_ptr, monap_type *monap_ptr)
{
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (r_ptr->flags1 & (RF1_NEVER_BLOW))
        return false;

    if (d_info[target_ptr->dungeon_idx].flags.has(DF::NO_MELEE))
        return false;

    if (!is_hostile(monap_ptr->m_ptr))
        return false;

    return true;
}

/*!
 * @brief プレーヤー死亡等でモンスターからプレーヤーへの直接攻撃処理を途中で打ち切るかどうかを判定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return 攻撃続行ならばTRUE、打ち切りになったらFALSE
 */
static bool check_monster_continuous_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (!monster_is_valid(monap_ptr->m_ptr))
        return false;

    if (monap_ptr->method == RBM_NONE)
        return false;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (is_pet(monap_ptr->m_ptr) && (r_ptr->flags1 & RF1_UNIQUE) && (monap_ptr->method == RBM_EXPLODE)) {
        monap_ptr->method = RBM_HIT;
        monap_ptr->d_dice /= 10;
    }

    if (!target_ptr->playing || target_ptr->is_dead || (distance(target_ptr->y, target_ptr->x, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx) > 1)
        || target_ptr->leaving)
        return false;

    return true;
}

/*!
 * @brief 対邪悪結界が効いている状態で邪悪なモンスターから直接攻撃を受けた時の処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return briefに書いた条件＋確率が満たされたらTRUE、それ以外はFALSE
 */
static bool effect_protecion_from_evil(player_type *target_ptr, monap_type *monap_ptr)
{
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((target_ptr->protevil <= 0) || ((r_ptr->flags3 & RF3_EVIL) == 0) || (target_ptr->lev < monap_ptr->rlev) || ((randint0(100) + target_ptr->lev) <= 50))
        return false;

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

    return true;
}

static void describe_silly_attacks(monap_type *monap_ptr)
{
    if (monap_ptr->act == nullptr)
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
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
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
    MonsterDamageProcessor mdp(target_ptr, monap_ptr->m_idx, monap_ptr->m_ptr->hp + 1, &monap_ptr->fear);
    if (mdp.mon_take_hit(nullptr)) {
        monap_ptr->blinked = false;
        monap_ptr->alive = false;
    }
}

static void describe_attack_evasion(player_type *target_ptr, monap_type *monap_ptr)
{
    if (!monap_ptr->m_ptr->ml)
        return;

    disturb(target_ptr, true, true);
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

static void gain_armor_exp(player_type *target_ptr, monap_type *monap_ptr)
{
    const auto o_ptr_mh = &target_ptr->inventory_list[INVEN_MAIN_HAND];
    const auto o_ptr_sh = &target_ptr->inventory_list[INVEN_SUB_HAND];
    if (!o_ptr_mh->is_armour() && !o_ptr_sh->is_armour())
        return;

    int cur = target_ptr->skill_exp[SKILL_SHIELD];
    int max = s_info[target_ptr->pclass].s_max[SKILL_SHIELD];
    if (cur >= max)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    DEPTH targetlevel = r_ptr->level;
    int inc = 0;
    if ((cur / 100) < targetlevel) {
        if ((cur / 100 + 15) < targetlevel)
            inc += 1 + (targetlevel - (cur / 100 + 15));
        else
            inc += 1;
    }

    target_ptr->skill_exp[SKILL_SHIELD] = MIN(max, cur + inc);
    target_ptr->update |= (PU_BONUS);
}

/*!
 * @brief モンスターから直接攻撃を1回受けた時の処理
 * @return 対邪悪結界により攻撃が当たらなかったらFALSE、それ以外はTRUE
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @details 最大4 回/モンスター/ターン、このルーチンを通る
 */
static bool process_monster_attack_hit(player_type *target_ptr, monap_type *monap_ptr)
{
    disturb(target_ptr, true, true);
    if (effect_protecion_from_evil(target_ptr, monap_ptr))
        return false;

    monap_ptr->do_cut = 0;
    monap_ptr->do_stun = 0;
    describe_monster_attack_method(monap_ptr);
    describe_silly_attacks(monap_ptr);
    monap_ptr->obvious = true;
    monap_ptr->damage = damroll(monap_ptr->d_dice, monap_ptr->d_side);
    if (monap_ptr->explode)
        monap_ptr->damage = 0;

    switch_monster_blow_to_player(target_ptr, monap_ptr);
    select_cut_stun(monap_ptr);
    calc_player_cut(target_ptr, monap_ptr);
    calc_player_stun(target_ptr, monap_ptr);
    monster_explode(target_ptr, monap_ptr);
    process_aura_counterattack(target_ptr, monap_ptr);
    return true;
}

/*!
 * @brief 一部の打撃種別の場合のみ、避けた旨のメッセージ表示と盾技能スキル向上を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
static void process_monster_attack_evasion(player_type *target_ptr, monap_type *monap_ptr)
{
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
        gain_armor_exp(target_ptr, monap_ptr);
        monap_ptr->damage = 0;
        return;
    default:
        return;
    }
}

/*!
 * @brief モンスターの打撃情報を蓄積させる
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @param ap_cnt モンスターの打撃 N回目
 */
static void increase_blow_type_seen(player_type *target_ptr, monap_type *monap_ptr, const int ap_cnt)
{
    // どの敵が何をしてきたか正しく認識できていなければならない。
    if (!is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr) || monap_ptr->do_silly_attack)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];

    // 非自明な類の打撃については、そのダメージが 0 ならば基本的に知識が増えない。
    // ただし、既に一定以上の知識があれば常に知識が増える(何をされたのか察知できる)。
    if (!monap_ptr->obvious && (monap_ptr->damage == 0) && (r_ptr->r_blows[ap_cnt] <= 10))
        return;

    if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
        r_ptr->r_blows[ap_cnt]++;
}

/*!
 * @brief モンスターからプレイヤーへの打撃処理本体
 * @return 打撃に反応してプレイヤーがその場から離脱したかどうか
 */
static bool process_monster_blows(player_type *target_ptr, monap_type *monap_ptr)
{
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];

    for (int ap_cnt = 0; ap_cnt < MAX_NUM_BLOWS; ap_cnt++) {
        monap_ptr->obvious = false;
        monap_ptr->damage = 0;
        monap_ptr->act = nullptr;
        monap_ptr->effect = r_ptr->blow[ap_cnt].effect;
        monap_ptr->method = r_ptr->blow[ap_cnt].method;
        monap_ptr->d_dice = r_ptr->blow[ap_cnt].d_dice;
        monap_ptr->d_side = r_ptr->blow[ap_cnt].d_side;

        if (!check_monster_continuous_attack(target_ptr, monap_ptr))
            break;

        // effect が RBE_NONE (無効値)になることはあり得ないはずだが、万一そう
        // なっていたら単に攻撃を打ち切る。
        // r_info.txt の "B:" トークンに effect 以降を書き忘れた場合が該当する。
        if (monap_ptr->effect == RBE_NONE) {
            plog("unexpected: monap_ptr->effect == RBE_NONE");
            break;
        }

        if (monap_ptr->method == RBM_SHOOT)
            continue;

        // フレーバーの打撃は必中扱い。それ以外は通常の命中判定を行う。
        monap_ptr->ac = target_ptr->ac + target_ptr->to_a;
        bool hit;
        if (monap_ptr->effect == RBE_FLAVOR) {
            hit = true;
        } else {
            const int power = mbe_info[monap_ptr->effect].power;
            hit = check_hit_from_monster_to_player(target_ptr, power, monap_ptr->rlev, monster_stunned_remaining(monap_ptr->m_ptr));
        }

        if (hit) {
            // 命中した。命中処理と思い出処理を行う。
            // 打撃そのものは対邪悪結界で撃退した可能性がある。
            const bool protect = !process_monster_attack_hit(target_ptr, monap_ptr);
            increase_blow_type_seen(target_ptr, monap_ptr, ap_cnt);

            // 撃退成功時はそのまま次の打撃へ移行。
            if (protect)
                continue;

            // 撃退失敗時は落馬処理、変わり身のテレポート処理を行う。
            check_fall_off_horse(target_ptr, monap_ptr);
            if (target_ptr->special_defense & NINJA_KAWARIMI) {
                // 変わり身のテレポートが成功したら攻撃を打ち切り、プレイヤーが離脱した旨を返す。
                if (kawarimi(target_ptr, false))
                    return true;
            }
        } else {
            // 命中しなかった。回避時の処理、思い出処理を行う。
            process_monster_attack_evasion(target_ptr, monap_ptr);
            increase_blow_type_seen(target_ptr, monap_ptr, ap_cnt);
        }
    }

    // 通常はプレイヤーはその場にとどまる。
    return false;
}

/*!
 * @brief 呪術「目には目を」の効果処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 */
static void eyes_on_eyes(player_type *target_ptr, monap_type *monap_ptr)
{
    if (((target_ptr->tim_eyeeye == 0) && !RealmHex(target_ptr).is_spelling_specific(HEX_EYE_FOR_EYE)) || (monap_ptr->get_damage == 0) || target_ptr->is_dead)
        return;

#ifdef JP
    msg_format("攻撃が%s自身を傷つけた！", monap_ptr->m_name);
#else
    GAME_TEXT m_name_self[MAX_MONSTER_NAME];
    monster_desc(target_ptr, m_name_self, monap_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
    msg_format("The attack of %s has wounded %s!", monap_ptr->m_name, m_name_self);
#endif
    project(target_ptr, 0, 0, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, monap_ptr->get_damage, GF_MISSILE, PROJECT_KILL);
    if (target_ptr->tim_eyeeye)
        set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, true);
}

static void thief_teleport(player_type *target_ptr, monap_type *monap_ptr)
{
    if (!monap_ptr->blinked || !monap_ptr->alive || target_ptr->is_dead)
        return;

    if (RealmHex(target_ptr).check_hex_barrier(monap_ptr->m_idx, HEX_ANTI_TELE)) {
        msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
    } else {
        msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
        teleport_away(target_ptr, monap_ptr->m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
    }
}

static void postprocess_monster_blows(player_type *target_ptr, monap_type *monap_ptr)
{
    RealmHex(target_ptr).store_vengeful_damage(monap_ptr->get_damage);
    eyes_on_eyes(target_ptr, monap_ptr);
    musou_counterattack(target_ptr, monap_ptr);
    thief_teleport(target_ptr, monap_ptr);
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !target_ptr->current_floor_ptr->inside_arena)
        r_ptr->r_deaths++;

    if (monap_ptr->m_ptr->ml && monap_ptr->fear && monap_ptr->alive && !target_ptr->is_dead) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖で逃げ出した！", "%^s flees in terror!"), monap_ptr->m_name);
    }

    if (target_ptr->special_defense & KATA_IAI)
        set_action(target_ptr, ACTION_NONE);
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
    if (!check_no_blow(target_ptr, monap_ptr))
        return false;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    monap_ptr->rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    monster_desc(target_ptr, monap_ptr->m_name, monap_ptr->m_ptr, 0);
    monster_desc(target_ptr, monap_ptr->ddesc, monap_ptr->m_ptr, MD_WRONGDOER_NAME);
    if (target_ptr->special_defense & KATA_IAI) {
        msg_format(_("相手が襲いかかる前に素早く武器を振るった。", "You took sen, drew and cut in one motion before %s moved."), monap_ptr->m_name);
        if (do_cmd_attack(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_IAI))
            return true;
    }

    if ((target_ptr->special_defense & NINJA_KAWARIMI) && (randint0(55) < (target_ptr->lev * 3 / 5 + 20))) {
        if (kawarimi(target_ptr, true))
            return true;
    }

    monap_ptr->blinked = false;
    if (process_monster_blows(target_ptr, monap_ptr))
        return true;

    postprocess_monster_blows(target_ptr, monap_ptr);
    return true;
}
