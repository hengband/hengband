/*!
 * @brief エルドリッチホラー処理
 * @date 2020/06/07
 * @author Hourier
 */

#include "player/eldritch-horror.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "locale/english.h"
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
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief エルドリッチホラーの形容詞種別を決める
 * @param r_ptr モンスター情報への参照ポインタ
 * @return
 */
static concptr decide_horror_message(MonsterRaceInfo *r_ptr)
{
    int horror_num = randint0(MAX_SAN_HORROR_SUM);
    if (horror_num < MAX_SAN_HORROR_COMMON) {
        return horror_desc_common[horror_num];
    }

    if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
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
static void see_eldritch_horror(std::string_view m_name, MonsterRaceInfo *r_ptr)
{
    concptr horror_message = decide_horror_message(r_ptr);
    msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), horror_message, m_name.data());
    r_ptr->r_flags2 |= RF2_ELDRITCH_HORROR;
}

/*!
 * @brief エルドリッチホラー持ちのモンスターを見た時の反応 (モンスター名版)
 * @param desc モンスター名 (エルドリッチホラー持ちの全モンスターからランダム…のはず)
 * @param r_ptr モンスターへの参照ポインタ
 */
static void feel_eldritch_horror(concptr desc, MonsterRaceInfo *r_ptr)
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
void sanity_blast(PlayerType *player_ptr, MonsterEntity *m_ptr, bool necro)
{
    if (player_ptr->phase_out || !w_ptr->character_dungeon) {
        return;
    }

    int power = 100;
    if (!necro && m_ptr) {
        auto *r_ptr = &monraces_info[m_ptr->ap_r_idx];
        const auto m_name = monster_desc(player_ptr, m_ptr, 0);
        power = r_ptr->level / 2;
        if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
            if (r_ptr->flags1 & RF1_FRIENDS) {
                power /= 2;
            }
        } else {
            power *= 2;
        }

        if (!w_ptr->is_loading_now) {
            return;
        }

        if (!m_ptr->ml) {
            return;
        }

        if (!(r_ptr->flags2 & RF2_ELDRITCH_HORROR)) {
            return;
        }

        if (m_ptr->is_pet()) {
            return;
        }

        if (randint1(100) > power) {
            return;
        }

        if (saving_throw(player_ptr->skill_sav - power)) {
            return;
        }

        if (player_ptr->effects()->hallucination()->is_hallucinated()) {
            msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), funny_desc[randint0(MAX_SAN_FUNNY)], m_name.data());
            if (one_in_(3)) {
                msg_print(funny_comments[randint0(MAX_SAN_COMMENT)]);
                BadStatusSetter(player_ptr).mod_hallucination(randint1(r_ptr->level));
            }

            return;
        }

        see_eldritch_horror(m_name, r_ptr);
        switch (PlayerRace(player_ptr).life()) {
        case PlayerRaceLifeType::DEMON:
            return;
        case PlayerRaceLifeType::UNDEAD:
            if (saving_throw(25 + player_ptr->lev)) {
                return;
            }
            break;
        default:
            break;
        }
    } else if (!necro) {
        MonsterRaceInfo *r_ptr;
        std::string m_name;
        concptr desc;
        get_mon_num_prep(player_ptr, get_nightmare, nullptr);
        r_ptr = &monraces_info[get_mon_num(player_ptr, 0, MAX_DEPTH, 0)];
        power = r_ptr->level + 10;
        desc = r_ptr->name.data();
        get_mon_num_prep(player_ptr, nullptr, nullptr);
#ifdef JP
#else

        if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
            m_name = (is_a_vowel(desc[0])) ? "an " : "a ";
        }
#endif
        m_name.append(desc);

        if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
            if (r_ptr->flags1 & RF1_FRIENDS) {
                power /= 2;
            }
        } else {
            power *= 2;
        }

        if (saving_throw(player_ptr->skill_sav * 100 / power)) {
            msg_format(_("夢の中で%sに追いかけられた。", "%s^ chases you through your dreams."), m_name.data());
            return;
        }

        if (player_ptr->effects()->hallucination()->is_hallucinated()) {
            msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), funny_desc[randint0(MAX_SAN_FUNNY)], m_name.data());
            if (one_in_(3)) {
                msg_print(funny_comments[randint0(MAX_SAN_COMMENT)]);
                BadStatusSetter(player_ptr).mod_hallucination(randint1(r_ptr->level));
            }

            return;
        }

        feel_eldritch_horror(desc, r_ptr);
        switch (PlayerRace(player_ptr).life()) {
        case PlayerRaceLifeType::DEMON:
            if (saving_throw(20 + player_ptr->lev)) {
                return;
            }
            break;
        case PlayerRaceLifeType::UNDEAD:
            if (saving_throw(10 + player_ptr->lev)) {
                return;
            }
            break;
        default:
            break;
        }
    } else {
        msg_print(_("ネクロノミコンを読んで正気を失った！", "Your sanity is shaken by reading the Necronomicon!"));
    }

    /* 過去の効果無効率再現のため5回saving_throw 実行 */
    if (saving_throw(player_ptr->skill_sav - power) && saving_throw(player_ptr->skill_sav - power) && saving_throw(player_ptr->skill_sav - power) && saving_throw(player_ptr->skill_sav - power) && saving_throw(player_ptr->skill_sav - power)) {
        return;
    }

    switch (randint1(22)) {
    case 1: {
        if (player_ptr->muta.has_not(PlayerMutationType::MORONIC)) {
            if ((player_ptr->stat_use[A_INT] < 4) && (player_ptr->stat_use[A_WIS] < 4)) {
                msg_print(_("あなたは完璧な馬鹿になったような気がした。しかしそれは元々だった。", "You turn into an utter moron!"));
            } else {
                msg_print(_("あなたは完璧な馬鹿になった！", "You turn into an utter moron!"));
            }

            if (player_ptr->muta.has(PlayerMutationType::HYPER_INT)) {
                msg_print(_("あなたの脳は生体コンピュータではなくなった。", "Your brain is no longer a living computer."));
                player_ptr->muta.reset(PlayerMutationType::HYPER_INT);
            }

            player_ptr->muta.set(PlayerMutationType::MORONIC);
        }

        break;
    }
    case 2: {
        if (player_ptr->muta.has_not(PlayerMutationType::COWARDICE) && !has_resist_fear(player_ptr)) {
            msg_print(_("あなたはパラノイアになった！", "You become paranoid!"));
            if (player_ptr->muta.has(PlayerMutationType::FEARLESS)) {
                msg_print(_("あなたはもう恐れ知らずではなくなった。", "You are no longer fearless."));
                player_ptr->muta.reset(PlayerMutationType::FEARLESS);
            }

            player_ptr->muta.set(PlayerMutationType::COWARDICE);
        }

        break;
    }
    case 3: {
        if (player_ptr->muta.has_not(PlayerMutationType::HALLU) && !has_resist_chaos(player_ptr)) {
            msg_print(_("幻覚をひき起こす精神錯乱に陥った！", "You are afflicted by a hallucinatory insanity!"));
            player_ptr->muta.set(PlayerMutationType::HALLU);
        }

        break;
    }
    case 4: {
        if (player_ptr->muta.has_not(PlayerMutationType::BERS_RAGE) && !has_resist_conf(player_ptr)) {
            msg_print(_("激烈な感情の発作におそわれるようになった！", "You become subject to fits of berserk rage!"));
            player_ptr->muta.set(PlayerMutationType::BERS_RAGE);
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
        BadStatusSetter bss(player_ptr);
        if (!has_resist_conf(player_ptr)) {
            (void)bss.mod_confusion(randint0(4) + 4);
        }

        if (!has_resist_chaos(player_ptr) && one_in_(3)) {
            (void)bss.mod_hallucination(randint0(250) + 150);
        }

        /*!< @todo いつからかは不明だがreturnとbreakが同時に存在している。どちらがデッドコードか不明瞭なので保留 */
        return;
        break;
    }
    case 13:
    case 14:
    case 15: {
        BadStatusSetter bss(player_ptr);
        if (!has_resist_conf(player_ptr)) {
            (void)bss.mod_confusion(randint0(4) + 4);
        }
        if (!player_ptr->free_act) {
            (void)bss.mod_paralysis(randint0(4) + 4);
        }
        if (!has_resist_chaos(player_ptr)) {
            (void)bss.mod_hallucination(randint0(250) + 150);
        }

        do {
            (void)do_dec_stat(player_ptr, A_INT);
        } while (randint0(100) > player_ptr->skill_sav && one_in_(2));

        do {
            (void)do_dec_stat(player_ptr, A_WIS);
        } while (randint0(100) > player_ptr->skill_sav && one_in_(2));

        break;
    }
    case 16:
    case 17: {
        if (lose_all_info(player_ptr)) {
            msg_print(_("あまりの恐怖に全てのことを忘れてしまった！", "You forget everything in your utmost terror!"));
        }
        break;
    }
    case 18:
    case 19:
    case 20:
    case 21:
    case 22: {
        do_dec_stat(player_ptr, A_INT);
        do_dec_stat(player_ptr, A_WIS);
        break;
    }
    default:
        break;
    }

    player_ptr->update |= PU_BONUS;
    handle_stuff(player_ptr);
}
