/*!
 * @brief モンスター同士が乱闘を起こした時の攻撃種別をスイッチングする
 * @date 2020/05/30
 * @author Hourier
 */

#include "melee/melee-switcher.h"
#include "core/disturbance.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "melee/melee-util.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-polymorph.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-util.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief モンスターがカオス属性へ耐性を示すかどうか
 */
static bool monster_has_chaos_resist(PlayerType *player_ptr, const MonsterEntity &monster)
{
    auto &monrace = monster.get_monrace();
    if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_CHAOS)) {
        if (is_original_ap_and_seen(player_ptr, monster)) {
            monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_CHAOS);
        }
        return true;
    } else if (monrace.kind_flags.has(MonsterKindType::DEMON) && one_in_(3)) {
        if (is_original_ap_and_seen(player_ptr, monster)) {
            monrace.r_kind_flags.set(MonsterKindType::DEMON);
        }
        return true;
    }
    return false;
}

void describe_melee_method(PlayerType *player_ptr, mam_type *mam_ptr)
{
    switch (mam_ptr->method) {
    case RaceBlowMethodType::HIT: {
        mam_ptr->act = _("%sを殴った。", "hits %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::TOUCH: {
        mam_ptr->act = _("%sを触った。", "touches %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::PUNCH: {
        mam_ptr->act = _("%sをパンチした。", "punches %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::KICK: {
        mam_ptr->act = _("%sを蹴った。", "kicks %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::CLAW: {
        mam_ptr->act = _("%sをひっかいた。", "claws %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::BITE: {
        mam_ptr->act = _("%sを噛んだ。", "bites %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::STING: {
        mam_ptr->act = _("%sを刺した。", "stings %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::SLASH: {
        mam_ptr->act = _("%sを斬った。", "slashes %s.");
        break;
    }
    case RaceBlowMethodType::BUTT: {
        mam_ptr->act = _("%sを角で突いた。", "butts %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::CRUSH: {
        mam_ptr->act = _("%sを押し潰した", "crushes %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::ENGULF: {
        mam_ptr->act = _("%sを飲み込んだ。", "engulfs %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::CHARGE: {
        mam_ptr->act = _("%sに請求書をよこした。", "charges %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::CRAWL: {
        mam_ptr->act = _("%sの体の上を這い回った。", "crawls on %s.");
        mam_ptr->touched = true;
        break;
    }
    case RaceBlowMethodType::DROOL: {
        mam_ptr->act = _("%sによだれをたらした。", "drools on %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::SPIT: {
        mam_ptr->act = _("%sに唾を吐いた。", "spits on %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::EXPLODE: {
        if (mam_ptr->see_either) {
            disturb(player_ptr, true, true);
        }

        mam_ptr->act = _("爆発した。", "explodes.");
        mam_ptr->explode = true;
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::GAZE: {
        mam_ptr->act = _("%sをにらんだ。", "gazes at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::WAIL: {
        mam_ptr->act = _("%sに泣きついた。", "wails at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::SPORE: {
        mam_ptr->act = _("%sに胞子を飛ばした。", "releases spores at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::XXX4: {
        mam_ptr->act = _("%sにXXX4を飛ばした。", "projects XXX4's at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::BEG: {
        mam_ptr->act = _("%sに金をせがんだ。", "begs %s for money.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::INSULT: {
        mam_ptr->act = _("%sを侮辱した。", "insults %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::MOAN: {
        mam_ptr->act = _("%sにむかってうめいた。", "moans at %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::SHOW: {
        mam_ptr->act = _("%sにむかって歌った。", "sings to %s.");
        mam_ptr->touched = false;
        break;
    }
    case RaceBlowMethodType::DASH: {
        mam_ptr->act = _("%sに体当りした。", "dashes %s.");
        mam_ptr->touched = true;
        break;
    }

    case RaceBlowMethodType::NONE:
    case RaceBlowMethodType::MAX:
        break;
    }
}

void decide_monster_attack_effect(PlayerType *player_ptr, mam_type *mam_ptr)
{
    switch (mam_ptr->effect) {
    case RaceBlowEffectType::NONE:
    case RaceBlowEffectType::DR_MANA:
        mam_ptr->damage = 0;
        mam_ptr->pt = AttributeType::NONE;
        break;
    case RaceBlowEffectType::SUPERHURT:
        if ((randint1(mam_ptr->rlev * 2 + 250) > (mam_ptr->ac + 200)) || one_in_(13)) {
            int tmp_damage = mam_ptr->damage - (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
            mam_ptr->damage = std::max(mam_ptr->damage, tmp_damage * 2);
            break;
        }

        [[fallthrough]];
    case RaceBlowEffectType::HURT:
        mam_ptr->damage -= (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
        break;
    case RaceBlowEffectType::POISON:
    case RaceBlowEffectType::DISEASE:
        mam_ptr->pt = AttributeType::POIS;
        break;
    case RaceBlowEffectType::UN_BONUS:
    case RaceBlowEffectType::UN_POWER:
        mam_ptr->pt = AttributeType::DISENCHANT;
        break;
    case RaceBlowEffectType::EAT_ITEM:
    case RaceBlowEffectType::EAT_GOLD:
        if (!mam_ptr->m_ptr->is_riding() && one_in_(2)) {
            mam_ptr->blinked = true;
        }

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
        mam_ptr->pt = AttributeType::ACID;
        break;
    case RaceBlowEffectType::ELEC:
        mam_ptr->pt = AttributeType::ELEC;
        break;
    case RaceBlowEffectType::FIRE:
        mam_ptr->pt = AttributeType::FIRE;
        break;
    case RaceBlowEffectType::COLD:
        mam_ptr->pt = AttributeType::COLD;
        break;
    case RaceBlowEffectType::CONFUSE:
        mam_ptr->pt = AttributeType::CONFUSION;
        break;
    case RaceBlowEffectType::TERRIFY:
        mam_ptr->attribute = BlowEffectType::FEAR;
        break;
    case RaceBlowEffectType::PARALYZE:
        mam_ptr->attribute = BlowEffectType::SLEEP;
        break;
    case RaceBlowEffectType::SHATTER:
        mam_ptr->damage -= (mam_ptr->damage * ((mam_ptr->ac < 150) ? mam_ptr->ac : 150) / 250);
        if (mam_ptr->damage > 23) {
            earthquake(player_ptr, mam_ptr->m_ptr->get_position(), 8, mam_ptr->m_idx);
        }

        break;
    case RaceBlowEffectType::EXP_10:
    case RaceBlowEffectType::EXP_20:
    case RaceBlowEffectType::EXP_40:
    case RaceBlowEffectType::EXP_80:
        mam_ptr->pt = AttributeType::NETHER;
        break;
    case RaceBlowEffectType::TIME:
        mam_ptr->pt = AttributeType::TIME;
        break;
    case RaceBlowEffectType::DR_LIFE:
        mam_ptr->pt = AttributeType::HYPODYNAMIA;
        mam_ptr->attribute = BlowEffectType::HEAL;
        break;
    case RaceBlowEffectType::INERTIA:
        mam_ptr->pt = AttributeType::INERTIAL;
        break;
    case RaceBlowEffectType::STUN:
        mam_ptr->pt = AttributeType::SOUND;
        break;
    case RaceBlowEffectType::HUNGRY:
        mam_ptr->pt = AttributeType::HUNGRY;
        break;
    case RaceBlowEffectType::CHAOS: {
        const auto has_resist = monster_has_chaos_resist(player_ptr, *mam_ptr->t_ptr);
        if (has_resist) {
            mam_ptr->damage *= 3;
            mam_ptr->damage /= randint1(6) + 6;
        }

        if (randint1(5) < 3) {
            mam_ptr->pt = AttributeType::HYPODYNAMIA;
            mam_ptr->attribute = BlowEffectType::HEAL;
            break;
        }
        if (one_in_(250)) {
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.is_underground() && (!floor.is_in_quest() || !QuestType::is_fixed(floor.quest_number))) {
                if (mam_ptr->damage > 23) {
                    msg_print(_("カオスの力でダンジョンが崩れ始める！", "The dungeon tumbles by the chaotic power!"));
                    earthquake(player_ptr, mam_ptr->m_ptr->get_position(), 8, mam_ptr->m_idx);
                }
            }
        }
        if (!one_in_(10)) {
            if (!has_resist) {
                mam_ptr->pt = AttributeType::CONFUSION;
            }
            break;
        }

        if (!has_resist) {
            if (one_in_(2)) {
                msg_format(_(("%s^はどこかへ消えていった！"), ("%s^ disappears!")), mam_ptr->t_name);
                teleport_away(player_ptr, mam_ptr->t_idx, 50, TELEPORT_PASSIVE);
            } else {
                if (polymorph_monster(player_ptr, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx)) {
                    msg_format(_("%s^は変化した！", "%s^ changes!"), mam_ptr->t_name);
                }
            }
        }
    } break;
    case RaceBlowEffectType::FLAVOR:
        // フレーバー打撃には何の効果もない。
        mam_ptr->pt = AttributeType::NONE;
        break;
    default:
        mam_ptr->pt = AttributeType::NONE;
        break;
    }
}

void describe_monster_missed_monster(PlayerType *player_ptr, mam_type *mam_ptr)
{
    switch (mam_ptr->method) {
    case RaceBlowMethodType::HIT:
    case RaceBlowMethodType::TOUCH:
    case RaceBlowMethodType::PUNCH:
    case RaceBlowMethodType::KICK:
    case RaceBlowMethodType::CLAW:
    case RaceBlowMethodType::BITE:
    case RaceBlowMethodType::STING:
    case RaceBlowMethodType::SLASH:
    case RaceBlowMethodType::BUTT:
    case RaceBlowMethodType::CRUSH:
    case RaceBlowMethodType::ENGULF:
    case RaceBlowMethodType::CHARGE: {
        (void)set_monster_csleep(player_ptr, mam_ptr->t_idx, 0);
        if (mam_ptr->see_m) {
#ifdef JP
            msg_format("%sは%s^の攻撃をかわした。", mam_ptr->t_name, mam_ptr->m_name);
#else
            msg_format("%s^ misses %s.", mam_ptr->m_name, mam_ptr->t_name);
#endif
        }

        return;
    }
    default:
        return;
    }
}
