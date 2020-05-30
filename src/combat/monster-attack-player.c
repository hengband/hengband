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
#include "combat/insults-moans.h"
#include "combat/monster-attack-effect.h"
#include "combat/monster-attack-util.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "main/sound-definitions-table.h"
#include "mind/racial-mirror-master.h"
#include "monster/monster-status.h"
#include "object/object2.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "object/object-mark-types.h"
#include "player/avatar.h"
#include "player/mimic-info-table.h"
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

static void show_jaian_song(monap_type *monap_ptr)
{
#ifdef JP
    switch (randint1(15)) {
    case 1:
    case 6:
    case 11:
        monap_ptr->act = "「♪お～れはジャイアン～～ガ～キだいしょう～」";
        break;
    case 2:
        monap_ptr->act = "「♪て～んかむ～てきのお～とこだぜ～～」";
        break;
    case 3:
        monap_ptr->act = "「♪の～び太スネ夫はメじゃないよ～～」";
        break;
    case 4:
        monap_ptr->act = "「♪け～んかスポ～ツ～どんとこい～」";
        break;
    case 5:
        monap_ptr->act = "「♪うた～も～～う～まいぜ～まかしとけ～」";
        break;
    case 7:
        monap_ptr->act = "「♪ま～ちいちば～んのに～んきもの～～」";
        break;
    case 8:
        monap_ptr->act = "「♪べんきょうしゅくだいメじゃないよ～～」";
        break;
    case 9:
        monap_ptr->act = "「♪きはやさし～くて～ち～からもち～」";
        break;
    case 10:
        monap_ptr->act = "「♪かお～も～～スタイルも～バツグンさ～」";
        break;
    case 12:
        monap_ptr->act = "「♪がっこうい～ちの～あ～ばれんぼう～～」";
        break;
    case 13:
        monap_ptr->act = "「♪ド～ラもドラミもメじゃないよ～～」";
        break;
    case 14:
        monap_ptr->act = "「♪よじげんぽけっと～な～くたって～」";
        break;
    case 15:
        monap_ptr->act = "「♪あし～の～～ながさ～は～まけないぜ～」";
        break;
    }
#else
    monap_ptr->act = "horribly sings 'I AM GIAAAAAN. THE BOOOSS OF THE KIIIIDS.'";
#endif
}

static void monster_attack_show(monap_type *monap_ptr)
{
#ifdef JP
    monap_ptr->abbreviate = -1;
#endif
    if (monap_ptr->m_ptr->r_idx == MON_JAIAN) {
        show_jaian_song(monap_ptr);
    } else {
        if (one_in_(3))
            monap_ptr->act = _("は♪僕らは楽しい家族♪と歌っている。", "sings 'We are a happy family.'");
        else
            monap_ptr->act = _("は♪アイ ラブ ユー、ユー ラブ ミー♪と歌っている。", "sings 'I love you, you love me.'");
    }

    sound(SOUND_SHOW);
}

static void describe_monster_attack_method(monap_type *monap_ptr)
{
    switch (monap_ptr->method) {
    case RBM_HIT: {
        monap_ptr->act = _("殴られた。", "hits you.");
        monap_ptr->do_cut = monap_ptr->do_stun = 1;
        monap_ptr->touched = TRUE;
        sound(SOUND_HIT);
        break;
    }
    case RBM_TOUCH: {
        monap_ptr->act = _("触られた。", "touches you.");
        monap_ptr->touched = TRUE;
        sound(SOUND_TOUCH);
        break;
    }
    case RBM_PUNCH: {
        monap_ptr->act = _("パンチされた。", "punches you.");
        monap_ptr->touched = TRUE;
        monap_ptr->do_stun = 1;
        sound(SOUND_HIT);
        break;
    }
    case RBM_KICK: {
        monap_ptr->act = _("蹴られた。", "kicks you.");
        monap_ptr->touched = TRUE;
        monap_ptr->do_stun = 1;
        sound(SOUND_HIT);
        break;
    }
    case RBM_CLAW: {
        monap_ptr->act = _("ひっかかれた。", "claws you.");
        monap_ptr->touched = TRUE;
        monap_ptr->do_cut = 1;
        sound(SOUND_CLAW);
        break;
    }
    case RBM_BITE: {
        monap_ptr->act = _("噛まれた。", "bites you.");
        monap_ptr->do_cut = 1;
        monap_ptr->touched = TRUE;
        sound(SOUND_BITE);
        break;
    }
    case RBM_STING: {
        monap_ptr->act = _("刺された。", "stings you.");
        monap_ptr->touched = TRUE;
        sound(SOUND_STING);
        break;
    }
    case RBM_SLASH: {
        monap_ptr->act = _("斬られた。", "slashes you.");
        monap_ptr->touched = TRUE;
        monap_ptr->do_cut = 1;
        sound(SOUND_CLAW);
        break;
    }
    case RBM_BUTT: {
        monap_ptr->act = _("角で突かれた。", "butts you.");
        monap_ptr->do_stun = 1;
        monap_ptr->touched = TRUE;
        sound(SOUND_HIT);
        break;
    }
    case RBM_CRUSH: {
        monap_ptr->act = _("体当たりされた。", "crushes you.");
        monap_ptr->do_stun = 1;
        monap_ptr->touched = TRUE;
        sound(SOUND_CRUSH);
        break;
    }
    case RBM_ENGULF: {
        monap_ptr->act = _("飲み込まれた。", "engulfs you.");
        monap_ptr->touched = TRUE;
        sound(SOUND_CRUSH);
        break;
    }
    case RBM_CHARGE: {
        monap_ptr->abbreviate = -1;
        monap_ptr->act = _("は請求書をよこした。", "charges you.");
        monap_ptr->touched = TRUE;

        /* このコメントはジョークが効いているので残しておく / Note! This is "charges", not "charges at". */
        sound(SOUND_BUY);
        break;
    }
    case RBM_CRAWL: {
        monap_ptr->abbreviate = -1;
        monap_ptr->act = _("が体の上を這い回った。", "crawls on you.");
        monap_ptr->touched = TRUE;
        sound(SOUND_SLIME);
        break;
    }
    case RBM_DROOL: {
        monap_ptr->act = _("よだれをたらされた。", "drools on you.");
        sound(SOUND_SLIME);
        break;
    }
    case RBM_SPIT: {
        monap_ptr->act = _("唾を吐かれた。", "spits on you.");
        sound(SOUND_SLIME);
        break;
    }
    case RBM_EXPLODE: {
        monap_ptr->abbreviate = -1;
        monap_ptr->act = _("は爆発した。", "explodes.");
        monap_ptr->explode = TRUE;
        break;
    }
    case RBM_GAZE: {
        monap_ptr->act = _("にらまれた。", "gazes at you.");
        break;
    }
    case RBM_WAIL: {
        monap_ptr->act = _("泣き叫ばれた。", "wails at you.");
        sound(SOUND_WAIL);
        break;
    }
    case RBM_SPORE: {
        monap_ptr->act = _("胞子を飛ばされた。", "releases spores at you.");
        sound(SOUND_SLIME);
        break;
    }
    case RBM_XXX4: {
        monap_ptr->abbreviate = -1;
        monap_ptr->act = _("が XXX4 を発射した。", "projects XXX4's at you.");
        break;
    }
    case RBM_BEG: {
        monap_ptr->act = _("金をせがまれた。", "begs you for money.");
        sound(SOUND_MOAN);
        break;
    }
    case RBM_INSULT: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = desc_insult[randint0(monap_ptr->m_ptr->r_idx == MON_DEBBY ? 10 : 8)];
        sound(SOUND_MOAN);
        break;
    }
    case RBM_MOAN: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = desc_moan[randint0(4)];
        sound(SOUND_MOAN);
        break;
    }
    case RBM_SHOW: {
        monster_attack_show(monap_ptr);
        break;
    }
    }
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
 * @brief モンスターからプレイヤーへの打撃処理 / Attack the player via physical attacks.
 * @param m_idx 打撃を行うモンスターのID
 * @return 実際に攻撃処理を行った場合TRUEを返す
 */
bool make_attack_normal(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monap_type tmp_monap;
    monap_type *monap_ptr = initialize_monap_type(target_ptr, &tmp_monap, m_idx);

    int k, tmp;
    ARMOUR_CLASS ac;
    PRICE gold;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    GAME_TEXT ddesc[80];

    bool blinked;
    bool fear = FALSE;
    bool alive = TRUE;
    HIT_POINT get_damage = 0;
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (r_ptr->flags1 & (RF1_NEVER_BLOW))
        return FALSE;

    if (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
        return FALSE;

    if (!is_hostile(monap_ptr->m_ptr))
        return FALSE;

    monap_ptr->rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    monster_desc(target_ptr, monap_ptr->m_name, monap_ptr->m_ptr, 0);
    monster_desc(target_ptr, ddesc, monap_ptr->m_ptr, MD_WRONGDOER_NAME);
    if (target_ptr->special_defense & KATA_IAI) {
        msg_format(_("相手が襲いかかる前に素早く武器を振るった。", "You took sen, drew and cut in one motion before %s moved."), monap_ptr->m_name);
        if (do_cmd_attack(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_IAI))
            return TRUE;
    }

    if ((target_ptr->special_defense & NINJA_KAWARIMI) && (randint0(55) < (target_ptr->lev * 3 / 5 + 20))) {
        if (kawarimi(target_ptr, TRUE))
            return TRUE;
    }

    blinked = FALSE;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (int ap_cnt = 0; ap_cnt < 4; ap_cnt++) {
        bool obvious = FALSE;
        HIT_POINT power = 0;
        HIT_POINT damage = 0;
        int effect = r_ptr->blow[ap_cnt].effect;
        monap_ptr->method = r_ptr->blow[ap_cnt].method;
        int d_dice = r_ptr->blow[ap_cnt].d_dice;
        int d_side = r_ptr->blow[ap_cnt].d_side;

        if (!monster_is_valid(monap_ptr->m_ptr))
            break;

        if (!monap_ptr->method)
            break;

        if (is_pet(monap_ptr->m_ptr) && (r_ptr->flags1 & RF1_UNIQUE) && (monap_ptr->method == RBM_EXPLODE)) {
            monap_ptr->method = RBM_HIT;
            d_dice /= 10;
        }

        if (!target_ptr->playing || target_ptr->is_dead || (distance(target_ptr->y, target_ptr->x, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx) > 1)
            || target_ptr->leaving)
            break;

        if (monap_ptr->method == RBM_SHOOT)
            continue;

        power = mbe_info[effect].power;
        ac = target_ptr->ac + target_ptr->to_a;
        if (!effect || check_hit_from_monster_to_player(target_ptr, power, monap_ptr->rlev, MON_STUNNED(monap_ptr->m_ptr))) {
            disturb(target_ptr, TRUE, TRUE);
            if (effect_protecion_from_evil(target_ptr, monap_ptr))
                continue;

            monap_ptr->do_cut = 0;
            monap_ptr->do_stun = 0;
            describe_monster_attack_method(monap_ptr);
            describe_silly_attacks(monap_ptr);
            obvious = TRUE;
            damage = damroll(d_dice, d_side);
            if (monap_ptr->explode)
                damage = 0;

            switch (effect) {
            case 0: {
                obvious = TRUE;
                damage = 0;
                break;
            }
            case RBE_SUPERHURT: /* AC軽減あり / Player armor reduces total damage */
            {
                if (((randint1(monap_ptr->rlev * 2 + 300) > (ac + 200)) || one_in_(13)) && !check_multishadow(target_ptr)) {
                    int tmp_damage = damage - (damage * ((ac < 150) ? ac : 150) / 250);
                    msg_print(_("痛恨の一撃！", "It was a critical hit!"));
                    tmp_damage = MAX(damage, tmp_damage * 2);
                    get_damage += take_hit(target_ptr, DAMAGE_ATTACK, tmp_damage, ddesc, -1);
                    break;
                }
            }
                /* Fall through */
            case RBE_HURT: /* AC軽減あり / Player armor reduces total damage */
            {
                obvious = TRUE;
                damage -= (damage * ((ac < 150) ? ac : 150) / 250);
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                break;
            }
            case RBE_POISON: {
                if (monap_ptr->explode)
                    break;

                if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr)) && !check_multishadow(target_ptr)) {
                    if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5)) {
                        obvious = TRUE;
                    }
                }

                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                update_smart_learn(target_ptr, m_idx, DRS_POIS);
                break;
            }
            case RBE_UN_BONUS: {
                if (monap_ptr->explode)
                    break;

                if (!target_ptr->resist_disen && !check_multishadow(target_ptr)) {
                    if (apply_disenchant(target_ptr, 0)) {
                        update_creature(target_ptr);
                        obvious = TRUE;
                    }
                }

                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                update_smart_learn(target_ptr, m_idx, DRS_DISEN);
                break;
            }
            case RBE_UN_POWER: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                for (k = 0; k < 10; k++) {
                    INVENTORY_IDX i = (INVENTORY_IDX)randint0(INVEN_PACK);
                    o_ptr = &target_ptr->inventory_list[i];
                    if (!o_ptr->k_idx)
                        continue;

                    if (((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND)) && (o_ptr->pval)) {
                        int heal = monap_ptr->rlev * o_ptr->pval;
                        if (o_ptr->tval == TV_STAFF)
                            heal *= o_ptr->number;

                        heal = MIN(heal, monap_ptr->m_ptr->maxhp - monap_ptr->m_ptr->hp);
                        msg_print(_("ザックからエネルギーが吸い取られた！", "Energy drains from your pack!"));
                        obvious = TRUE;
                        monap_ptr->m_ptr->hp += (HIT_POINT)heal;
                        if (target_ptr->health_who == m_idx)
                            target_ptr->redraw |= (PR_HEALTH);

                        if (target_ptr->riding == m_idx)
                            target_ptr->redraw |= (PR_UHEALTH);

                        o_ptr->pval = 0;
                        target_ptr->update |= (PU_COMBINE | PU_REORDER);
                        target_ptr->window |= (PW_INVEN);
                        break;
                    }
                }

                break;
            }
            case RBE_EAT_GOLD: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (MON_CONFUSED(monap_ptr->m_ptr))
                    break;

                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                obvious = TRUE;
                if (!target_ptr->paralyzed && (randint0(100) < (adj_dex_safe[target_ptr->stat_ind[A_DEX]] + target_ptr->lev))) {
                    msg_print(_("しかし素早く財布を守った！", "You quickly protect your money pouch!"));
                    if (randint0(3))
                        blinked = TRUE;
                }
                else {
                    gold = (target_ptr->au / 10) + randint1(25);
                    if (gold < 2)
                        gold = 2;
                    if (gold > 5000)
                        gold = (target_ptr->au / 20) + randint1(3000);
                    if (gold > target_ptr->au)
                        gold = target_ptr->au;
                    target_ptr->au -= gold;
                    if (gold <= 0) {
                        msg_print(_("しかし何も盗まれなかった。", "Nothing was stolen."));
                    } else if (target_ptr->au) {
                        msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
                        msg_format(_("$%ld のお金が盗まれた！", "%ld coins were stolen!"), (long)gold);
                        chg_virtue(target_ptr, V_SACRIFICE, 1);
                    } else {
                        msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
                        msg_print(_("お金が全部盗まれた！", "All of your coins were stolen!"));
                        chg_virtue(target_ptr, V_SACRIFICE, 2);
                    }

                    target_ptr->redraw |= (PR_GOLD);
                    target_ptr->window |= (PW_PLAYER);
                    blinked = TRUE;
                }

                break;
            }
            case RBE_EAT_ITEM: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (MON_CONFUSED(monap_ptr->m_ptr))
                    break;

                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (!target_ptr->paralyzed && (randint0(100) < (adj_dex_safe[target_ptr->stat_ind[A_DEX]] + target_ptr->lev))) {
                    msg_print(_("しかしあわててザックを取り返した！", "You grab hold of your backpack!"));
                    blinked = TRUE;
                    obvious = TRUE;
                    break;
                }

                for (k = 0; k < 10; k++) {
                    OBJECT_IDX o_idx;
                    INVENTORY_IDX i = (INVENTORY_IDX)randint0(INVEN_PACK);
                    o_ptr = &target_ptr->inventory_list[i];
                    if (!o_ptr->k_idx)
                        continue;

                    if (object_is_artifact(o_ptr))
                        continue;

                    object_desc(target_ptr, o_name, o_ptr, OD_OMIT_PREFIX);
#ifdef JP
                    msg_format("%s(%c)を%s盗まれた！", o_name, index_to_label(i), ((o_ptr->number > 1) ? "一つ" : ""));
#else
                    msg_format("%sour %s (%c) was stolen!", ((o_ptr->number > 1) ? "One of y" : "Y"), o_name, index_to_label(i));
#endif
                    chg_virtue(target_ptr, V_SACRIFICE, 1);
                    o_idx = o_pop(floor_ptr);
                    if (o_idx) {
                        object_type *j_ptr;
                        j_ptr = &floor_ptr->o_list[o_idx];
                        object_copy(j_ptr, o_ptr);
                        j_ptr->number = 1;
                        if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND)) {
                            j_ptr->pval = o_ptr->pval / o_ptr->number;
                            o_ptr->pval -= j_ptr->pval;
                        }

                        j_ptr->marked = OM_TOUCHED;
                        j_ptr->held_m_idx = m_idx;
                        j_ptr->next_o_idx = monap_ptr->m_ptr->hold_o_idx;
                        monap_ptr->m_ptr->hold_o_idx = o_idx;
                    }

                    inven_item_increase(target_ptr, i, -1);
                    inven_item_optimize(target_ptr, i);
                    obvious = TRUE;
                    blinked = TRUE;
                    break;
                }

                break;
            }

            case RBE_EAT_FOOD: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                for (k = 0; k < 10; k++) {
                    INVENTORY_IDX i = (INVENTORY_IDX)randint0(INVEN_PACK);
                    o_ptr = &target_ptr->inventory_list[i];
                    if (!o_ptr->k_idx)
                        continue;

                    if ((o_ptr->tval != TV_FOOD) && !((o_ptr->tval == TV_CORPSE) && (o_ptr->sval)))
                        continue;

                    object_desc(target_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
                    msg_format("%s(%c)を%s食べられてしまった！", o_name, index_to_label(i), ((o_ptr->number > 1) ? "一つ" : ""));
#else
                    msg_format("%sour %s (%c) was eaten!", ((o_ptr->number > 1) ? "One of y" : "Y"), o_name, index_to_label(i));
#endif
                    inven_item_increase(target_ptr, i, -1);
                    inven_item_optimize(target_ptr, i);
                    obvious = TRUE;
                    break;
                }

                break;
            }
            case RBE_EAT_LITE: {
                o_ptr = &target_ptr->inventory_list[INVEN_LITE];
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if ((o_ptr->xtra4 > 0) && (!object_is_fixed_artifact(o_ptr))) {
                    o_ptr->xtra4 -= (s16b)(250 + randint1(250));
                    if (o_ptr->xtra4 < 1)
                        o_ptr->xtra4 = 1;

                    if (!target_ptr->blind) {
                        msg_print(_("明かりが暗くなってしまった。", "Your light dims."));
                        obvious = TRUE;
                    }

                    target_ptr->window |= (PW_EQUIP);
                }

                break;
            }
            case RBE_ACID: {
                if (monap_ptr->explode)
                    break;

                obvious = TRUE;
                msg_print(_("酸を浴びせられた！", "You are covered in acid!"));
                get_damage += acid_dam(target_ptr, damage, ddesc, -1, FALSE);
                update_creature(target_ptr);
                update_smart_learn(target_ptr, m_idx, DRS_ACID);
                break;
            }
            case RBE_ELEC: {
                if (monap_ptr->explode)
                    break;
                obvious = TRUE;
                msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));
                get_damage += elec_dam(target_ptr, damage, ddesc, -1, FALSE);
                update_smart_learn(target_ptr, m_idx, DRS_ELEC);
                break;
            }
            case RBE_FIRE: {
                if (monap_ptr->explode)
                    break;
                obvious = TRUE;
                msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));
                get_damage += fire_dam(target_ptr, damage, ddesc, -1, FALSE);
                update_smart_learn(target_ptr, m_idx, DRS_FIRE);
                break;
            }
            case RBE_COLD: {
                if (monap_ptr->explode)
                    break;
                obvious = TRUE;
                msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));
                get_damage += cold_dam(target_ptr, damage, ddesc, -1, FALSE);
                update_smart_learn(target_ptr, m_idx, DRS_COLD);
                break;
            }
            case RBE_BLIND: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (!target_ptr->resist_blind && !check_multishadow(target_ptr)) {
                    if (set_blind(target_ptr, target_ptr->blind + 10 + randint1(monap_ptr->rlev))) {
#ifdef JP
                        if (monap_ptr->m_ptr->r_idx == MON_DIO)
                            msg_print("どうだッ！この血の目潰しはッ！");
#else
                        /* nanka */
#endif
                        obvious = TRUE;
                    }
                }

                update_smart_learn(target_ptr, m_idx, DRS_BLIND);
                break;
            }
            case RBE_CONFUSE: {
                if (monap_ptr->explode)
                    break;

                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (!target_ptr->resist_conf && !check_multishadow(target_ptr)) {
                    if (set_confused(target_ptr, target_ptr->confused + 3 + randint1(monap_ptr->rlev))) {
                        obvious = TRUE;
                    }
                }

                update_smart_learn(target_ptr, m_idx, DRS_CONF);
                break;
            }
            case RBE_TERRIFY: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (check_multishadow(target_ptr)) {
                    /* Do nothing */
                } else if (target_ptr->resist_fear) {
                    msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
                    obvious = TRUE;
                } else if (randint0(100 + r_ptr->level / 2) < target_ptr->skill_sav) {
                    msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
                    obvious = TRUE;
                } else {
                    if (set_afraid(target_ptr, target_ptr->afraid + 3 + randint1(monap_ptr->rlev))) {
                        obvious = TRUE;
                    }
                }

                update_smart_learn(target_ptr, m_idx, DRS_FEAR);
                break;
            }
            case RBE_PARALYZE: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

                if (target_ptr->is_dead)
                    break;

                if (check_multishadow(target_ptr)) {
                    /* Do nothing */
                } else if (target_ptr->free_act) {
                    msg_print(_("しかし効果がなかった！", "You are unaffected!"));
                    obvious = TRUE;
                } else if (randint0(100 + r_ptr->level / 2) < target_ptr->skill_sav) {
                    msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
                    obvious = TRUE;
                } else {
                    if (!target_ptr->paralyzed) {
                        if (set_paralyzed(target_ptr, 3 + randint1(monap_ptr->rlev))) {
                            obvious = TRUE;
                        }
                    }
                }

                update_smart_learn(target_ptr, m_idx, DRS_FREE);
                break;
            }
            case RBE_LOSE_STR: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_STR))
                    obvious = TRUE;

                break;
            }
            case RBE_LOSE_INT: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_INT))
                    obvious = TRUE;

                break;
            }
            case RBE_LOSE_WIS: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_WIS))
                    obvious = TRUE;

                break;
            }
            case RBE_LOSE_DEX: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_DEX))
                    obvious = TRUE;

                break;
            }
            case RBE_LOSE_CON: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_CON))
                    obvious = TRUE;

                break;
            }
            case RBE_LOSE_CHR: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_CHR))
                    obvious = TRUE;

                break;
            }
            case RBE_LOSE_ALL: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (do_dec_stat(target_ptr, A_STR))
                    obvious = TRUE;

                if (do_dec_stat(target_ptr, A_DEX))
                    obvious = TRUE;

                if (do_dec_stat(target_ptr, A_CON))
                    obvious = TRUE;

                if (do_dec_stat(target_ptr, A_INT))
                    obvious = TRUE;

                if (do_dec_stat(target_ptr, A_WIS))
                    obvious = TRUE;

                if (do_dec_stat(target_ptr, A_CHR))
                    obvious = TRUE;

                break;
            }
            case RBE_SHATTER: {
                obvious = TRUE;
                damage -= (damage * ((ac < 150) ? ac : 150) / 250);
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (damage > 23 || monap_ptr->explode)
                    earthquake(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, 8, m_idx);

                break;
            }
            case RBE_EXP_10: {
                s32b d = damroll(10, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                obvious = TRUE;
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 95);
                break;
            }
            case RBE_EXP_20: {
                s32b d = damroll(20, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                obvious = TRUE;
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 90);
                break;
            }
            case RBE_EXP_40: {
                s32b d = damroll(40, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                obvious = TRUE;
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 75);
                break;
            }
            case RBE_EXP_80: {
                s32b d = damroll(80, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                obvious = TRUE;
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                (void)drain_exp(target_ptr, d, d / 10, 50);
                break;
            }
            case RBE_DISEASE: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr))) {
                    if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5)) {
                        obvious = TRUE;
                    }
                }

                if ((randint1(100) < 11) && (target_ptr->prace != RACE_ANDROID)) {
                    bool perm = one_in_(10);
                    if (dec_stat(target_ptr, A_CON, randint1(10), perm)) {
                        msg_print(_("病があなたを蝕んでいる気がする。", "You feel sickly."));
                        obvious = TRUE;
                    }
                }

                break;
            }
            case RBE_TIME: {
                if (monap_ptr->explode)
                    break;

                process_monster_attack_time(target_ptr, monap_ptr);
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                break;
            }
            case RBE_DR_LIFE: {
                s32b d = damroll(60, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
                bool resist_drain;
                obvious = TRUE;
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead || check_multishadow(target_ptr))
                    break;

                resist_drain = !drain_exp(target_ptr, d, d / 10, 50);
                if (target_ptr->mimic_form) {
                    if (mimic_info[target_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING)
                        resist_drain = TRUE;
                } else {
                    switch (target_ptr->prace) {
                    case RACE_ZOMBIE:
                    case RACE_VAMPIRE:
                    case RACE_SPECTRE:
                    case RACE_SKELETON:
                    case RACE_BALROG:
                    case RACE_GOLEM:
                    case RACE_ANDROID:
                        resist_drain = TRUE;
                        break;
                    }
                }

                if ((damage > 5) && !resist_drain) {
                    bool did_heal = FALSE;
                    if (monap_ptr->m_ptr->hp < monap_ptr->m_ptr->maxhp)
                        did_heal = TRUE;

                    monap_ptr->m_ptr->hp += damroll(4, damage / 6);
                    if (monap_ptr->m_ptr->hp > monap_ptr->m_ptr->maxhp)
                        monap_ptr->m_ptr->hp = monap_ptr->m_ptr->maxhp;

                    if (target_ptr->health_who == m_idx)
                        target_ptr->redraw |= (PR_HEALTH);

                    if (target_ptr->riding == m_idx)
                        target_ptr->redraw |= (PR_UHEALTH);

                    if (monap_ptr->m_ptr->ml && did_heal) {
                        msg_format(_("%sは体力を回復したようだ。", "%^s appears healthier."), monap_ptr->m_name);
                    }
                }

                break;
            }
            case RBE_DR_MANA: {
                obvious = TRUE;
                if (check_multishadow(target_ptr)) {
                    msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
                } else {
                    monap_ptr->do_cut = 0;
                    target_ptr->csp -= damage;
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
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (check_multishadow(target_ptr)) {
                    /* Do nothing */
                } else {
                    if (set_slow(target_ptr, (target_ptr->slow + 4 + randint0(monap_ptr->rlev / 10)), FALSE)) {
                        obvious = TRUE;
                    }
                }

                break;
            }
            case RBE_STUN: {
                get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
                if (target_ptr->is_dead)
                    break;

                if (target_ptr->resist_sound || check_multishadow(target_ptr)) {
                    /* Do nothing */
                } else {
                    if (set_stun(target_ptr, target_ptr->stun + 10 + randint1(r_ptr->level / 4))) {
                        obvious = TRUE;
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
                tmp = calc_monster_critical(d_dice, d_side, damage);
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
                tmp = calc_monster_critical(d_dice, d_side, damage);
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
                    blinked = FALSE;
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
                            blinked = FALSE;
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
                            blinked = FALSE;
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
                            blinked = FALSE;
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
                            blinked = FALSE;
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
                                blinked = FALSE;
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
                            blinked = FALSE;
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
                            blinked = FALSE;
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

                damage = 0;
                break;
            }
        }

        if (is_original_ap_and_seen(target_ptr, monap_ptr->m_ptr) && !monap_ptr->do_silly_attack) {
            if (obvious || damage || (r_ptr->r_blows[ap_cnt] > 10)) {
                if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR) {
                    r_ptr->r_blows[ap_cnt]++;
                }
            }
        }

        if (target_ptr->riding && damage) {
            char m_steed_name[MAX_NLEN];
            monster_desc(target_ptr, m_steed_name, &floor_ptr->m_list[target_ptr->riding], 0);
            if (process_fall_off_horse(target_ptr, (damage > 200) ? 200 : damage, FALSE)) {
                msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_steed_name);
            }
        }

        if (target_ptr->special_defense & NINJA_KAWARIMI) {
            if (kawarimi(target_ptr, FALSE))
                return TRUE;
        }
    }

    revenge_store(target_ptr, get_damage);
    if ((target_ptr->tim_eyeeye || hex_spelling(target_ptr, HEX_EYE_FOR_EYE)) && get_damage > 0 && !target_ptr->is_dead) {
#ifdef JP
        msg_format("攻撃が%s自身を傷つけた！", monap_ptr->m_name);
#else
        GAME_TEXT m_name_self[80];
        monster_desc(target_ptr, m_name_self, monap_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
        msg_format("The attack of %s has wounded %s!", monap_ptr->m_name, m_name_self);
#endif
        project(target_ptr, 0, 0, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
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

    if (blinked && alive && !target_ptr->is_dead) {
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
