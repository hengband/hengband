/*!
 * @brief モンスター同士が乱闘を起こした時の攻撃種別をスイッチングする
 * @date 2020/05/30
 * @author Hourier
 */

#include "melee/melee-switcher.h"
#include "core/disturbance.h"
#include "melee/melee-util.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster/monster-status-setter.h"
#include "spell-kind/earthquake.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

void describe_melee_method(player_type *subject_ptr, mam_type *mam_ptr)
{
    switch (mam_ptr->method) {
    case RBM_HIT: {
        mam_ptr->act = _("%sを殴った。", "hits %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_TOUCH: {
        mam_ptr->act = _("%sを触った。", "touches %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_PUNCH: {
        mam_ptr->act = _("%sをパンチした。", "punches %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_KICK: {
        mam_ptr->act = _("%sを蹴った。", "kicks %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_CLAW: {
        mam_ptr->act = _("%sをひっかいた。", "claws %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_BITE: {
        mam_ptr->act = _("%sを噛んだ。", "bites %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_STING: {
        mam_ptr->act = _("%sを刺した。", "stings %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_SLASH: {
        mam_ptr->act = _("%sを斬った。", "slashes %s.");
        break;
    }
    case RBM_BUTT: {
        mam_ptr->act = _("%sを角で突いた。", "butts %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_CRUSH: {
        mam_ptr->act = _("%sに体当りした。", "crushes %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_ENGULF: {
        mam_ptr->act = _("%sを飲み込んだ。", "engulfs %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_CHARGE: {
        mam_ptr->act = _("%sに請求書をよこした。", "charges %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_CRAWL: {
        mam_ptr->act = _("%sの体の上を這い回った。", "crawls on %s.");
        mam_ptr->touched = true;
        break;
    }
    case RBM_DROOL: {
        mam_ptr->act = _("%sによだれをたらした。", "drools on %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_SPIT: {
        mam_ptr->act = _("%sに唾を吐いた。", "spits on %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_EXPLODE: {
        if (mam_ptr->see_either)
            disturb(subject_ptr, true, true);

        mam_ptr->act = _("爆発した。", "explodes.");
        mam_ptr->explode = true;
        mam_ptr->touched = false;
        break;
    }
    case RBM_GAZE: {
        mam_ptr->act = _("%sをにらんだ。", "gazes at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_WAIL: {
        mam_ptr->act = _("%sに泣きついた。", "wails at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_SPORE: {
        mam_ptr->act = _("%sに胞子を飛ばした。", "releases spores at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_XXX4: {
        mam_ptr->act = _("%sにXXX4を飛ばした。", "projects XXX4's at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_BEG: {
        mam_ptr->act = _("%sに金をせがんだ。", "begs %s for money.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_INSULT: {
        mam_ptr->act = _("%sを侮辱した。", "insults %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_MOAN: {
        mam_ptr->act = _("%sにむかってうめいた。", "moans at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RBM_SHOW: {
        mam_ptr->act = _("%sにむかって歌った。", "sings to %s.");
        mam_ptr->touched = false;
        break;
    }

    case RBM_NONE:
    case RBM_SHOOT:
    case NB_RBM_TYPE:
        break;
    }
}

void decide_monster_attack_effect(player_type *subject_ptr, mam_type *mam_ptr)
{
    switch (mam_ptr->effect) {
    case 0:
    case RBE_DR_MANA:
        mam_ptr->damage = 0;
        mam_ptr->pt = GF_NONE;
        break;
    case RBE_SUPERHURT:
        if ((randint1(mam_ptr->rlev * 2 + 250) > (mam_ptr->ac + 200)) || one_in_(13)) {
            int tmp_damage = mam_ptr->damage - (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
            mam_ptr->damage = MAX(mam_ptr->damage, tmp_damage * 2);
            break;
        }

        /* Fall through */
    case RBE_HURT:
        mam_ptr->damage -= (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
        break;
    case RBE_POISON:
    case RBE_DISEASE:
        mam_ptr->pt = GF_POIS;
        break;
    case RBE_UN_BONUS:
    case RBE_UN_POWER:
        mam_ptr->pt = GF_DISENCHANT;
        break;
    case RBE_EAT_ITEM:
    case RBE_EAT_GOLD:
        if ((subject_ptr->riding != mam_ptr->m_idx) && one_in_(2))
            mam_ptr->blinked = true;

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
        mam_ptr->pt = GF_ACID;
        break;
    case RBE_ELEC:
        mam_ptr->pt = GF_ELEC;
        break;
    case RBE_FIRE:
        mam_ptr->pt = GF_FIRE;
        break;
    case RBE_COLD:
        mam_ptr->pt = GF_COLD;
        break;
    case RBE_CONFUSE:
        mam_ptr->pt = GF_CONFUSION;
        break;
    case RBE_TERRIFY:
        mam_ptr->effect_type = BLOW_EFFECT_TYPE_FEAR;
        break;
    case RBE_PARALYZE:
        mam_ptr->effect_type = BLOW_EFFECT_TYPE_SLEEP;
        break;
    case RBE_SHATTER:
        mam_ptr->damage -= (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
        if (mam_ptr->damage > 23)
            earthquake(subject_ptr, mam_ptr->m_ptr->fy, mam_ptr->m_ptr->fx, 8, mam_ptr->m_idx);

        break;
    case RBE_EXP_10:
    case RBE_EXP_20:
    case RBE_EXP_40:
    case RBE_EXP_80:
        mam_ptr->pt = GF_NETHER;
        break;
    case RBE_TIME:
        mam_ptr->pt = GF_TIME;
        break;
    case RBE_DR_LIFE:
        mam_ptr->pt = GF_HYPODYNAMIA;
        mam_ptr->effect_type = BLOW_EFFECT_TYPE_HEAL;
        break;
    case RBE_INERTIA:
        mam_ptr->pt = GF_INERTIAL;
        break;
    case RBE_STUN:
        mam_ptr->pt = GF_SOUND;
        break;
    case RBE_HUNGRY:
        mam_ptr->pt = GF_HUNGRY;
        break;
    case RBE_FLAVOR:
        // フレーバー打撃には何の効果もない。
        mam_ptr->pt = GF_NONE;
        break;
    default:
        mam_ptr->pt = GF_NONE;
        break;
    }
}

void describe_monster_missed_monster(player_type *subject_ptr, mam_type *mam_ptr)
{
    switch (mam_ptr->method) {
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
        (void)set_monster_csleep(subject_ptr, mam_ptr->t_idx, 0);
        if (mam_ptr->see_m) {
#ifdef JP
            msg_format("%sは%^sの攻撃をかわした。", mam_ptr->t_name, mam_ptr->m_name);
#else
            msg_format("%^s misses %s.", mam_ptr->m_name, mam_ptr->t_name);
#endif
        }

        return;
    }
    default:
        return;
    }
}
