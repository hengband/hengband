/*!
 * @brief エルドリッチホラー処理
 * @date 2020/06/07
 * @author Hourier
 */

#include "player/eldritch-horror.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/horror-descriptions.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-flag-types.h"
#include "player/mimic-info-table.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

/*!
 * @brief エルドリッチホラーの形容詞種別を決める
 * @param r_ptr モンスター情報への参照ポインタ
 * @return
 */
static concptr decide_horror_message(monster_race *r_ptr)
{
    int horror_num = randint0(MAX_SAN_HORROR_SUM);
    if (horror_num < MAX_SAN_HORROR_COMMON) {
        return horror_desc_common[horror_num];
    }

    if ((r_ptr->flags3 & RF3_EVIL) != 0) {
        return horror_desc_evil[horror_num - MAX_SAN_HORROR_COMMON];
    }

    return horror_desc_neutral[horror_num - MAX_SAN_HORROR_COMMON];
}

/*!
 * @brief エルドリッチホラー持ちのモンスターを見た時の反応 (モンスター名版)
 * @param m_name モンスター名
 * @param r_ptr モンスター情報への参照ポインタ
 * @todo m_nameとdescで何が違うのかは良く分からない
 */
static void see_eldritch_horror(GAME_TEXT *m_name, monster_race *r_ptr)
{
    concptr horror_message = decide_horror_message(r_ptr);
    msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), horror_message, m_name);
    r_ptr->r_flags2 |= RF2_ELDRITCH_HORROR;
}

/*!
 * @brief エルドリッチホラー持ちのモンスターを見た時の反応 (モンスター名版)
 * @param desc モンスター名 (エルドリッチホラー持ちの全モンスターからランダム…のはず)
 * @param r_ptr モンスターへの参照ポインタ
 */
static void feel_eldritch_horror(concptr desc, monster_race *r_ptr)
{
    concptr horror_message = decide_horror_message(r_ptr);
    msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), horror_message, desc);
    r_ptr->r_flags2 |= RF2_ELDRITCH_HORROR;
}

/*!
 * @brief ELDRITCH_HORRORによるプレイヤーの精神破壊処理
 * @param m_ptr ELDRITCH_HORRORを引き起こしたモンスターの参照ポインタ。薬・罠・魔法の影響ならnullptr
 * @param necro 暗黒領域魔法の詠唱失敗によるものならばTRUEを返す
 */
void sanity_blast(player_type *creature_ptr, monster_type *m_ptr, bool necro)
{
    if (creature_ptr->phase_out || !current_world_ptr->character_dungeon)
        return;

    int power = 100;
    if (!necro && m_ptr) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_race *r_ptr = &r_info[m_ptr->ap_r_idx];
        power = r_ptr->level / 2;
        monster_desc(creature_ptr, m_name, m_ptr, 0);
        if (!(r_ptr->flags1 & RF1_UNIQUE)) {
            if (r_ptr->flags1 & RF1_FRIENDS)
                power /= 2;
        } else
            power *= 2;

        if (!current_world_ptr->is_loading_now)
            return;

        if (!m_ptr->ml)
            return;

        if (!(r_ptr->flags2 & RF2_ELDRITCH_HORROR))
            return;

        if (is_pet(m_ptr))
            return;

        if (randint1(100) > power)
            return;

        if (saving_throw(creature_ptr->skill_sav - power))
            return;

        if (creature_ptr->image) {
            msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), funny_desc[randint0(MAX_SAN_FUNNY)], m_name);

            if (one_in_(3)) {
                msg_print(funny_comments[randint0(MAX_SAN_COMMENT)]);
                creature_ptr->image = creature_ptr->image + randint1(r_ptr->level);
            }

            return;
        }

        see_eldritch_horror(m_name, r_ptr);
        switch (player_race_life(creature_ptr)) {
        case PlayerRaceLife::DEMON:
            return;
        case PlayerRaceLife::UNDEAD:
            if (saving_throw(25 + creature_ptr->lev))
                return;
            break;
        default:
            break;
        }
    } else if (!necro) {
        monster_race *r_ptr;
        GAME_TEXT m_name[MAX_NLEN];
        concptr desc;
        get_mon_num_prep(creature_ptr, get_nightmare, nullptr);
        r_ptr = &r_info[get_mon_num(creature_ptr, 0, MAX_DEPTH, 0)];
        power = r_ptr->level + 10;
        desc = r_ptr->name.c_str();
        get_mon_num_prep(creature_ptr, nullptr, nullptr);
#ifdef JP
#else

        if (!(r_ptr->flags1 & RF1_UNIQUE))
            sprintf(m_name, "%s %s", (is_a_vowel(desc[0]) ? "an" : "a"), desc);
        else
#endif
        sprintf(m_name, "%s", desc);

        if (!(r_ptr->flags1 & RF1_UNIQUE)) {
            if (r_ptr->flags1 & RF1_FRIENDS)
                power /= 2;
        } else
            power *= 2;

        if (saving_throw(creature_ptr->skill_sav * 100 / power)) {
            msg_format(_("夢の中で%sに追いかけられた。", "%^s chases you through your dreams."), m_name);
            return;
        }

        if (creature_ptr->image) {
            msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), funny_desc[randint0(MAX_SAN_FUNNY)], m_name);

            if (one_in_(3)) {
                msg_print(funny_comments[randint0(MAX_SAN_COMMENT)]);
                creature_ptr->image = creature_ptr->image + randint1(r_ptr->level);
            }

            return;
        }

        feel_eldritch_horror(desc, r_ptr);
        switch (player_race_life(creature_ptr)) {
        case PlayerRaceLife::DEMON:
            if (saving_throw(20 + creature_ptr->lev))
                return;
            break;
        case PlayerRaceLife::UNDEAD:
            if (saving_throw(10 + creature_ptr->lev))
                return;
            break;
        default:
            break;
        }
    } else {
        msg_print(_("ネクロノミコンを読んで正気を失った！", "Your sanity is shaken by reading the Necronomicon!"));
    }

    /* 過去の効果無効率再現のため5回saving_throw 実行 */
    if (saving_throw(creature_ptr->skill_sav - power) && saving_throw(creature_ptr->skill_sav - power) && saving_throw(creature_ptr->skill_sav - power)
        && saving_throw(creature_ptr->skill_sav - power) && saving_throw(creature_ptr->skill_sav - power)) {
        return;
    }

    switch (randint1(22)) {
    case 1: {
        if (creature_ptr->muta.has_not(MUTA::MORONIC)) {
            if ((creature_ptr->stat_use[A_INT] < 4) && (creature_ptr->stat_use[A_WIS] < 4)) {
                msg_print(_("あなたは完璧な馬鹿になったような気がした。しかしそれは元々だった。", "You turn into an utter moron!"));
            } else {
                msg_print(_("あなたは完璧な馬鹿になった！", "You turn into an utter moron!"));
            }

            if (creature_ptr->muta.has(MUTA::HYPER_INT)) {
                msg_print(_("あなたの脳は生体コンピュータではなくなった。", "Your brain is no longer a living computer."));
                creature_ptr->muta.reset(MUTA::HYPER_INT);
            }

            creature_ptr->muta.set(MUTA::MORONIC);
        }

        break;
    }
    case 2: {
        if (creature_ptr->muta.has_not(MUTA::COWARDICE) && !has_resist_fear(creature_ptr)) {
            msg_print(_("あなたはパラノイアになった！", "You become paranoid!"));
            if (creature_ptr->muta.has(MUTA::FEARLESS)) {
                msg_print(_("あなたはもう恐れ知らずではなくなった。", "You are no longer fearless."));
                creature_ptr->muta.reset(MUTA::FEARLESS);
            }

            creature_ptr->muta.set(MUTA::COWARDICE);
        }

        break;
    }
    case 3: {
        if (creature_ptr->muta.has_not(MUTA::HALLU) && !has_resist_chaos(creature_ptr)) {
            msg_print(_("幻覚をひき起こす精神錯乱に陥った！", "You are afflicted by a hallucinatory insanity!"));
            creature_ptr->muta.set(MUTA::HALLU);
        }

        break;
    }
    case 4: {
        if (creature_ptr->muta.has_not(MUTA::BERS_RAGE) && !has_resist_conf(creature_ptr)) {
            msg_print(_("激烈な感情の発作におそわれるようになった！", "You become subject to fits of berserk rage!"));
            creature_ptr->muta.set(MUTA::BERS_RAGE);
        }

        break;
    }
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12: {
        if (!has_resist_conf(creature_ptr)) {
            (void)set_confused(creature_ptr, creature_ptr->confused + randint0(4) + 4);
        }

        if (!has_resist_chaos(creature_ptr) && one_in_(3)) {
            (void)set_image(creature_ptr, creature_ptr->image + randint0(250) + 150);
        }

        /*!< @todo いつからかは不明だがreturnとbreakが同時に存在している。どちらがデッドコードか不明瞭なので保留 */
        return;
        break;
    }
    case 13:
    case 14:
    case 15: {
        if (!has_resist_conf(creature_ptr)) {
            (void)set_confused(creature_ptr, creature_ptr->confused + randint0(4) + 4);
        }
        if (!creature_ptr->free_act) {
            (void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + randint0(4) + 4);
        }
        if (!has_resist_chaos(creature_ptr)) {
            (void)set_image(creature_ptr, creature_ptr->image + randint0(250) + 150);
        }

        do {
            (void)do_dec_stat(creature_ptr, A_INT);
        } while (randint0(100) > creature_ptr->skill_sav && one_in_(2));

        do {
            (void)do_dec_stat(creature_ptr, A_WIS);
        } while (randint0(100) > creature_ptr->skill_sav && one_in_(2));

        break;
    }
    case 16:
    case 17: {
        if (lose_all_info(creature_ptr))
            msg_print(_("あまりの恐怖に全てのことを忘れてしまった！", "You forget everything in your utmost terror!"));
        break;
    }
    case 18:
    case 19:
    case 20:
    case 21:
    case 22: {
        do_dec_stat(creature_ptr, A_INT);
        do_dec_stat(creature_ptr, A_WIS);
        break;
    }
    default:
        break;
    }

    creature_ptr->update |= PU_BONUS;
    handle_stuff(creature_ptr);
}
