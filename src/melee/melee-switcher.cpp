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

void describe_melee_method(player_type *player_ptr, mam_type *mam_ptr)
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
            disturb(player_ptr, true, true);

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

void decide_monster_attack_effect(player_type *player_ptr, mam_type *mam_ptr)
{
    switch (mam_ptr->effect) {
    case RaceBlowEffectType::NONE:
    case RaceBlowEffectType::DR_MANA:
        mam_ptr->damage = 0;
        mam_ptr->pt = GF_NONE;
        break;
    case RaceBlowEffectType::SUPERHURT:
        if ((randint1(mam_ptr->rlev * 2 + 250) > (mam_ptr->ac + 200)) || one_in_(13)) {
            int tmp_damage = mam_ptr->damage - (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
            mam_ptr->damage = std::max(mam_ptr->damage, tmp_damage * 2);
            break;
        }

        /* Fall through */
    case RaceBlowEffectType::HURT:
        mam_ptr->damage -= (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
        break;
    case RaceBlowEffectType::POISON:
    case RaceBlowEffectType::DISEASE:
        mam_ptr->pt = GF_POIS;
        break;
    case RaceBlowEffectType::UN_BONUS:
    case RaceBlowEffectType::UN_POWER:
        mam_ptr->pt = GF_DISENCHANT;
        break;
    case RaceBlowEffectType::EAT_ITEM:
    case RaceBlowEffectType::EAT_GOLD:
        if ((player_ptr->riding != mam_ptr->m_idx) && one_in_(2))
            mam_ptr->blinked = true;

        break;
    case RaceBlowEffectType::EAT_FOOD:
    case RaceBlowEffectType::EAT_LITE:
    case RaceBlowEffectType::BLIND:
    case RaceBlowEffectType::LOSE_STR:
    case RaceBlowEffectType::LOSE_INT:
    case RaceBlowEffectType::LOSE_WIS:
    case RaceBlowEffectType::LOSE_DEX:
    case RaceBlowEffectType::LOSE_CON:
    case RaceBlowEffectType::LOSE_CHR:
    case RaceBlowEffectType::LOSE_ALL:
        break;
    case RaceBlowEffectType::ACID:
        mam_ptr->pt = GF_ACID;
        break;
    case RaceBlowEffectType::ELEC:
        mam_ptr->pt = GF_ELEC;
        break;
    case RaceBlowEffectType::FIRE:
        mam_ptr->pt = GF_FIRE;
        break;
    case RaceBlowEffectType::COLD:
        mam_ptr->pt = GF_COLD;
        break;
    case RaceBlowEffectType::CONFUSE:
        mam_ptr->pt = GF_CONFUSION;
        break;
    case RaceBlowEffectType::TERRIFY:
        mam_ptr->effect_type = BLOW_EFFECT_TYPE_FEAR;
        break;
    case RaceBlowEffectType::PARALYZE:
        mam_ptr->effect_type = BLOW_EFFECT_TYPE_SLEEP;
        break;
    case RaceBlowEffectType::SHATTER:
        mam_ptr->damage -= (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
        if (mam_ptr->damage > 23)
            earthquake(player_ptr, mam_ptr->m_ptr->fy, mam_ptr->m_ptr->fx, 8, mam_ptr->m_idx);

        break;
    case RaceBlowEffectType::EXP_10:
    case RaceBlowEffectType::EXP_20:
    case RaceBlowEffectType::EXP_40:
    case RaceBlowEffectType::EXP_80:
        mam_ptr->pt = GF_NETHER;
        break;
    case RaceBlowEffectType::TIME:
        mam_ptr->pt = GF_TIME;
        break;
    case RaceBlowEffectType::DR_LIFE:
        mam_ptr->pt = GF_HYPODYNAMIA;
        mam_ptr->effect_type = BLOW_EFFECT_TYPE_HEAL;
        break;
    case RaceBlowEffectType::INERTIA:
        mam_ptr->pt = GF_INERTIAL;
        break;
    case RaceBlowEffectType::STUN:
        mam_ptr->pt = GF_SOUND;
        break;
    case RaceBlowEffectType::HUNGRY:
        mam_ptr->pt = GF_HUNGRY;
        break;
    case RaceBlowEffectType::FLAVOR:
        // フレーバー打撃には何の効果もない。
        mam_ptr->pt = GF_NONE;
        break;
    default:
        mam_ptr->pt = GF_NONE;
        break;
    }
}

void describe_monster_missed_monster(player_type *player_ptr, mam_type *mam_ptr)
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
        (void)set_monster_csleep(player_ptr, mam_ptr->t_idx, 0);
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
