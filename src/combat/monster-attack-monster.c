#include "combat/monster-attack-monster.h"
#include "dungeon/dungeon.h"
#include "combat/monster-attack-effect.h"
#include "combat/hallucination-attacks-table.h"
#include "floor/floor.h"
#include "spell/spells-type.h"
#include "effect/effect-characteristics.h"
#include "main/sound-definitions-table.h"
#include "player/player-move.h"
#include "monster/monster-status.h"
#include "combat/attack-accuracy.h"
#include "spell/spells3.h"
#include "spell/spells-floor.h"
#include "spell/process-effect.h"
#include "monster/monster-race-hook.h"
#include "realm/realm-hex.h"
#include "combat/melee-postprocess.h"

#define BLOW_EFFECT_TYPE_NONE 0
#define BLOW_EFFECT_TYPE_FEAR 1
#define BLOW_EFFECT_TYPE_SLEEP 2
#define BLOW_EFFECT_TYPE_HEAL 3

/* monster-attack-monster type*/
typedef struct mam_type {
    int effect_type;
    int m_idx;
    int t_idx;
    monster_type *m_ptr;
    monster_type *t_ptr;
    GAME_TEXT m_name[MAX_NLEN];
    GAME_TEXT t_name[MAX_NLEN];
    HIT_POINT damage;
    bool see_m;
    bool see_t;
    bool see_either;
} mam_type;

mam_type *initialize_mam_type(player_type *subject_ptr, mam_type *mam_ptr, int m_idx, int t_idx, monster_type *m_ptr, monster_type *t_ptr) 
{
    mam_ptr->effect_type = 0;
    mam_ptr->m_idx = m_idx;
    mam_ptr->t_idx = t_idx;
    mam_ptr->m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
    mam_ptr->t_ptr = &subject_ptr->current_floor_ptr->m_list[t_idx];
    mam_ptr->damage = 0;
    mam_ptr->see_m = is_seen(m_ptr);
    mam_ptr->see_t = is_seen(t_ptr);
    mam_ptr->see_either = mam_ptr->see_m || mam_ptr->see_t;
    return mam_ptr;
}

static void process_blow_effect(player_type *subject_ptr, mam_type *mam_ptr)
{
    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    switch (mam_ptr->effect_type) {
    case BLOW_EFFECT_TYPE_FEAR:
        project(subject_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, mam_ptr->damage, GF_TURN_ALL,
            PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
        break;

    case BLOW_EFFECT_TYPE_SLEEP:
        project(subject_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, r_ptr->level, GF_OLD_SLEEP,
            PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED,
            -1);
        break;

    case BLOW_EFFECT_TYPE_HEAL:
        if ((monster_living(mam_ptr->m_idx)) && (mam_ptr->damage > 2)) {
            bool did_heal = FALSE;

            if (mam_ptr->m_ptr->hp < mam_ptr->m_ptr->maxhp)
                did_heal = TRUE;

            /* Heal */
            mam_ptr->m_ptr->hp += damroll(4, mam_ptr->damage / 6);
            if (mam_ptr->m_ptr->hp > mam_ptr->m_ptr->maxhp)
                mam_ptr->m_ptr->hp = mam_ptr->m_ptr->maxhp;

            /* Redraw (later) if needed */
            if (subject_ptr->health_who == mam_ptr->m_idx)
                subject_ptr->redraw |= (PR_HEALTH);
            if (subject_ptr->riding == mam_ptr->m_idx)
                subject_ptr->redraw |= (PR_UHEALTH);

            /* Special message */
            if (mam_ptr->see_m && did_heal) {
                msg_format(_("%sは体力を回復したようだ。", "%^s appears healthier."), mam_ptr->m_name);
            }
        }
        break;
    }
}

/*!
 * @brief モンスターから敵モンスターへの打撃攻撃処理
 * @param m_idx 攻撃側モンスターの参照ID
 * @param t_idx 目標側モンスターの参照ID
 * @return 実際に打撃処理が行われた場合TRUEを返す
 */
bool monst_attack_monst(player_type *subject_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    monster_type *m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
    monster_type *t_ptr = &subject_ptr->current_floor_ptr->m_list[t_idx];
    mam_type tmp_mam;
    mam_type *mam_ptr = initialize_mam_type(subject_ptr, mam_ptr, m_idx, t_idx, m_ptr, t_ptr);

    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];

    int pt;
    char temp[MAX_NLEN];
    bool explode = FALSE, touched = FALSE, fear = FALSE, dead = FALSE;
    POSITION y_saver = t_ptr->fy;
    POSITION x_saver = t_ptr->fx;
    int effect_type;


    /* Can the player be aware of this attack? */
    bool known = (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);
    bool do_silly_attack = (one_in_(2) && subject_ptr->image);

    if (m_idx == t_idx)
        return FALSE;
    if (r_ptr->flags1 & RF1_NEVER_BLOW)
        return FALSE;
    if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
        return FALSE;

    /* Total armor */
    ARMOUR_CLASS ac = tr_ptr->ac;

    /* Extract the effective monster level */
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

    monster_desc(subject_ptr, mam_ptr->m_name, m_ptr, 0);
    monster_desc(subject_ptr, mam_ptr->t_name, t_ptr, 0);

    /* Assume no blink */
    bool blinked = FALSE;

    if (!mam_ptr->see_either && known) {
        subject_ptr->current_floor_ptr->monster_noise = TRUE;
    }

    if (subject_ptr->riding && (m_idx == subject_ptr->riding))
        disturb(subject_ptr, TRUE, TRUE);

    /* Scan through all four blows */
    for (ARMOUR_CLASS ap_cnt = 0; ap_cnt < 4; ap_cnt++) {
        bool obvious = FALSE;

        HIT_POINT power = 0;

        concptr act = NULL;

        /* Extract the attack infomation */
        int effect = r_ptr->blow[ap_cnt].effect;
        rbm_type method = r_ptr->blow[ap_cnt].method;
        int d_dice = r_ptr->blow[ap_cnt].d_dice;
        int d_side = r_ptr->blow[ap_cnt].d_side;

        if (!monster_is_valid(m_ptr))
            break;

        /* Stop attacking if the target dies! */
        if (t_ptr->fx != x_saver || t_ptr->fy != y_saver)
            break;

        /* Hack -- no more attacks */
        if (!method)
            break;

        if (method == RBM_SHOOT)
            continue;

        /* Extract the attack "power" */
        power = mbe_info[effect].power;

        /* Monster hits */
        if (!effect || check_hit_from_monster_to_monster(power, rlev, ac, MON_STUNNED(m_ptr))) {
            (void)set_monster_csleep(subject_ptr, t_idx, 0);

            if (t_ptr->ml) {
                /* Redraw the health bar */
                if (subject_ptr->health_who == t_idx)
                    subject_ptr->redraw |= (PR_HEALTH);
                if (subject_ptr->riding == t_idx)
                    subject_ptr->redraw |= (PR_UHEALTH);
            }

            /* Describe the attack method */
            switch (method) {
            case RBM_HIT: {
                act = _("%sを殴った。", "hits %s.");
                touched = TRUE;
                break;
            }

            case RBM_TOUCH: {
                act = _("%sを触った。", "touches %s.");
                touched = TRUE;
                break;
            }

            case RBM_PUNCH: {
                act = _("%sをパンチした。", "punches %s.");
                touched = TRUE;
                break;
            }

            case RBM_KICK: {
                act = _("%sを蹴った。", "kicks %s.");
                touched = TRUE;
                break;
            }

            case RBM_CLAW: {
                act = _("%sをひっかいた。", "claws %s.");
                touched = TRUE;
                break;
            }

            case RBM_BITE: {
                act = _("%sを噛んだ。", "bites %s.");
                touched = TRUE;
                break;
            }

            case RBM_STING: {
                act = _("%sを刺した。", "stings %s.");
                touched = TRUE;
                break;
            }

            case RBM_SLASH: {
                act = _("%sを斬った。", "slashes %s.");
                break;
            }

            case RBM_BUTT: {
                act = _("%sを角で突いた。", "butts %s.");
                touched = TRUE;
                break;
            }

            case RBM_CRUSH: {
                act = _("%sに体当りした。", "crushes %s.");
                touched = TRUE;
                break;
            }

            case RBM_ENGULF: {
                act = _("%sを飲み込んだ。", "engulfs %s.");
                touched = TRUE;
                break;
            }

            case RBM_CHARGE: {
                act = _("%sに請求書をよこした。", "charges %s.");
                touched = TRUE;
                break;
            }

            case RBM_CRAWL: {
                act = _("%sの体の上を這い回った。", "crawls on %s.");
                touched = TRUE;
                break;
            }

            case RBM_DROOL: {
                act = _("%sによだれをたらした。", "drools on %s.");
                touched = FALSE;
                break;
            }

            case RBM_SPIT: {
                act = _("%sに唾を吐いた。", "spits on %s.");
                touched = FALSE;
                break;
            }

            case RBM_EXPLODE: {
                if (mam_ptr->see_either)
                    disturb(subject_ptr, TRUE, TRUE);
                act = _("爆発した。", "explodes.");
                explode = TRUE;
                touched = FALSE;
                break;
            }

            case RBM_GAZE: {
                act = _("%sをにらんだ。", "gazes at %s.");
                touched = FALSE;
                break;
            }

            case RBM_WAIL: {
                act = _("%sに泣きついた。", "wails at %s.");
                touched = FALSE;
                break;
            }

            case RBM_SPORE: {
                act = _("%sに胞子を飛ばした。", "releases spores at %s.");
                touched = FALSE;
                break;
            }

            case RBM_XXX4: {
                act = _("%sにXXX4を飛ばした。", "projects XXX4's at %s.");
                touched = FALSE;
                break;
            }

            case RBM_BEG: {
                act = _("%sに金をせがんだ。", "begs %s for money.");
                touched = FALSE;
                break;
            }

            case RBM_INSULT: {
                act = _("%sを侮辱した。", "insults %s.");
                touched = FALSE;
                break;
            }

            case RBM_MOAN: {
                act = _("%sにむかってうめいた。", "moans at %s.");
                touched = FALSE;
                break;
            }

            case RBM_SHOW: {
                act = _("%sにむかって歌った。", "sings to %s.");
                touched = FALSE;
                break;
            }
            }

            if (act && mam_ptr->see_either) {
#ifdef JP
                if (do_silly_attack)
                    act = silly_attacks2[randint0(MAX_SILLY_ATTACK)];
                strfmt(temp, act, mam_ptr->t_name);
                msg_format("%^sは%s", mam_ptr->m_name, temp);
#else
                if (do_silly_attack) {
                    act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
                    strfmt(temp, "%s %s.", act, t_name);
                } else
                    strfmt(temp, act, t_name);
                msg_format("%^s %s", m_name, temp);
#endif
            }

            /* Hack -- assume all attacks are obvious */
            obvious = TRUE;

            /* Roll out the damage */
            mam_ptr->damage = damroll(d_dice, d_side);

            /* Assume no effect */
            effect_type = BLOW_EFFECT_TYPE_NONE;

            pt = GF_MISSILE;

            /* Apply appropriate damage */
            switch (effect) {
            case 0:
            case RBE_DR_MANA:
                mam_ptr->damage = pt = 0;
                break;

            case RBE_SUPERHURT:
                if ((randint1(rlev * 2 + 250) > (ac + 200)) || one_in_(13)) {
                    int tmp_damage = mam_ptr->damage - (mam_ptr->damage * ((ac < 150) ? ac : 150) / 250);
                    mam_ptr->damage = MAX(mam_ptr->damage, tmp_damage * 2);
                    break;
                }

                /* Fall through */

            case RBE_HURT:
                mam_ptr->damage -= (mam_ptr->damage * ((ac < 150) ? ac : 150) / 250);
                break;

            case RBE_POISON:
            case RBE_DISEASE:
                pt = GF_POIS;
                break;

            case RBE_UN_BONUS:
            case RBE_UN_POWER:
                pt = GF_DISENCHANT;
                break;

            case RBE_EAT_ITEM:
            case RBE_EAT_GOLD:
                if ((subject_ptr->riding != m_idx) && one_in_(2))
                    blinked = TRUE;
                break;

            case RBE_EAT_FOOD:
            case RBE_EAT_LITE:
            case RBE_BLIND:
            case RBE_LOSE_STR:
            case RBE_LOSE_INT:
            case RBE_LOSE_WIS:
            case RBE_LOSE_DEX:
            case RBE_LOSE_CON:
            case RBE_LOSE_CHR:
            case RBE_LOSE_ALL:
                break;

            case RBE_ACID:
                pt = GF_ACID;
                break;

            case RBE_ELEC:
                pt = GF_ELEC;
                break;

            case RBE_FIRE:
                pt = GF_FIRE;
                break;

            case RBE_COLD:
                pt = GF_COLD;
                break;

            case RBE_CONFUSE:
                pt = GF_CONFUSION;
                break;

            case RBE_TERRIFY:
                effect_type = BLOW_EFFECT_TYPE_FEAR;
                break;

            case RBE_PARALYZE:
                effect_type = BLOW_EFFECT_TYPE_SLEEP;
                break;

            case RBE_SHATTER:
                mam_ptr->damage -= (mam_ptr->damage * ((ac < 150) ? ac : 150) / 250);
                if (mam_ptr->damage > 23)
                    earthquake(subject_ptr, m_ptr->fy, m_ptr->fx, 8, m_idx);
                break;

            case RBE_EXP_10:
            case RBE_EXP_20:
            case RBE_EXP_40:
            case RBE_EXP_80:
                pt = GF_NETHER;
                break;

            case RBE_TIME:
                pt = GF_TIME;
                break;

            case RBE_DR_LIFE:
                pt = GF_HYPODYNAMIA;
                effect_type = BLOW_EFFECT_TYPE_HEAL;
                break;

            case RBE_INERTIA:
                pt = GF_INERTIAL;
                break;

            case RBE_STUN:
                pt = GF_SOUND;
                break;

            default:
                pt = 0;
                break;
            }

            if (pt) {
                /* Do damage if not exploding */
                if (!explode) {
                    project(subject_ptr, m_idx, 0, t_ptr->fy, t_ptr->fx, mam_ptr->damage, pt, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
                }

                process_blow_effect(subject_ptr, mam_ptr);
                if (touched) {
                    /* Aura fire */
                    if ((tr_ptr->flags2 & RF2_AURA_FIRE) && m_ptr->r_idx) {
                        if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)) {
                            if (mam_ptr->see_either) {
                                msg_format(_("%^sは突然熱くなった！", "%^s is suddenly very hot!"), mam_ptr->m_name);
                            }
                            if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr))
                                tr_ptr->r_flags2 |= RF2_AURA_FIRE;
                            project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx, damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17)), GF_FIRE,
                                PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
                        } else {
                            if (is_original_ap_and_seen(subject_ptr, m_ptr))
                                r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
                        }
                    }

                    /* Aura cold */
                    if ((tr_ptr->flags3 & RF3_AURA_COLD) && m_ptr->r_idx) {
                        if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK)) {
                            if (mam_ptr->see_either) {
                                msg_format(_("%^sは突然寒くなった！", "%^s is suddenly very cold!"), mam_ptr->m_name);
                            }
                            if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr))
                                tr_ptr->r_flags3 |= RF3_AURA_COLD;
                            project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx, damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17)), GF_COLD,
                                PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
                        } else {
                            if (is_original_ap_and_seen(subject_ptr, m_ptr))
                                r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
                        }
                    }

                    /* Aura elec */
                    if ((tr_ptr->flags2 & RF2_AURA_ELEC) && m_ptr->r_idx) {
                        if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK)) {
                            if (mam_ptr->see_either) {
                                msg_format(_("%^sは電撃を食らった！", "%^s gets zapped!"), mam_ptr->m_name);
                            }
                            if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr))
                                tr_ptr->r_flags2 |= RF2_AURA_ELEC;
                            project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx, damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17)), GF_ELEC,
                                PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
                        } else {
                            if (is_original_ap_and_seen(subject_ptr, m_ptr))
                                r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
                        }
                    }
                }
            }
        }

        /* Monster missed player */
        else {
            /* Analyze failed attacks */
            switch (method) {
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
            case RBM_CHARGE: {
                (void)set_monster_csleep(subject_ptr, t_idx, 0);

                /* Visible monsters */
                if (mam_ptr->see_m) {
#ifdef JP
                    msg_format("%sは%^sの攻撃をかわした。", mam_ptr->t_name, mam_ptr->m_name);
#else
                    msg_format("%^s misses %s.", m_name, t_name);
#endif
                }

                break;
            }
            }
        }

        /* Analyze "visible" monsters only */
        if (is_original_ap_and_seen(subject_ptr, m_ptr) && !do_silly_attack) {
            /* Count "obvious" attacks (and ones that cause damage) */
            if (obvious || mam_ptr->damage || (r_ptr->r_blows[ap_cnt] > 10)) {
                /* Count attacks of this type */
                if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR) {
                    r_ptr->r_blows[ap_cnt]++;
                }
            }
        }
    }

    if (explode) {
        sound(SOUND_EXPLODE);

        /* Cancel Invulnerability */
        (void)set_monster_invulner(subject_ptr, m_idx, 0, FALSE);
        mon_take_hit_mon(subject_ptr, m_idx, m_ptr->hp + 1, &dead, &fear, _("は爆発して粉々になった。", " explodes into tiny shreds."), m_idx);
        blinked = FALSE;
    }

    if (!blinked || m_ptr->r_idx == 0)
        return TRUE;

    if (teleport_barrier(subject_ptr, m_idx)) {
        if (mam_ptr->see_m) {
            msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
        } else if (known) {
            subject_ptr->current_floor_ptr->monster_noise = TRUE;
        }
    } else {
        if (mam_ptr->see_m) {
            msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
        } else if (known) {
            subject_ptr->current_floor_ptr->monster_noise = TRUE;
        }

        teleport_away(subject_ptr, m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
    }

    return TRUE;
}
