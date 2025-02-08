/*!
 * @brief エルドリッチホラー処理
 * @date 2020/06/07
 * @author Hourier
 */

#include "player/eldritch-horror.h"
#include "core/stuff-handler.h"
#include "monster-floor/place-monster-types.h"
#include "monster/horror-descriptions.h"
#include "monster/monster-describer.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <string>
#include <string_view>
#ifdef JP
#else
#include "locale/english.h"
#endif

static bool process_mod_hallucination(PlayerType *player_ptr, std::string_view m_name, const MonraceDefinition &monrace)
{
    if (!player_ptr->effects()->hallucination().is_hallucinated()) {
        return false;
    }

    msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"), rand_choice(funny_desc).data(), m_name.data());
    if (one_in_(3)) {
        msg_print(rand_choice(funny_comments));
        BadStatusSetter(player_ptr).mod_hallucination(randnum1<short>(monrace.level));
    }

    return true;
}

/*!
 * @brief ELDRITCH_HORRORによるプレイヤーの精神破壊処理
 * @param m_idx ELDRITCH_HORRORを引き起こしたモンスターの参照ID。薬・罠・魔法の影響ならstd::nullopt。(デフォルト: std::nullopt)
 * @param necro 暗黒領域魔法の詠唱失敗によるものならばtrueを指定する (デフォルト: false)
 */
void sanity_blast(PlayerType *player_ptr, std::optional<short> m_idx, bool necro)
{
    const auto &world = AngbandWorld::get_instance();
    if (AngbandSystem::get_instance().is_phase_out() || !world.character_dungeon) {
        return;
    }

    auto &monraces = MonraceList::get_instance();
    auto power = 100;
    if (!necro && m_idx) {
        auto &monster = player_ptr->current_floor_ptr->m_list[*m_idx];
        auto &monrace = monster.get_appearance_monrace();
        const auto m_name = monster_desc(player_ptr, &monster, 0);
        power = monrace.level / 2;
        if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            if (monrace.misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
                power /= 2;
            }
        } else {
            power *= 2;
        }

        if (!world.is_loading_now) {
            return;
        }

        if (!monster.ml) {
            return;
        }

        if (monrace.misc_flags.has_not(MonsterMiscType::ELDRITCH_HORROR)) {
            return;
        }

        if (monster.is_pet()) {
            return;
        }

        if (randint1(100) > power) {
            return;
        }

        if (evaluate_percent(player_ptr->skill_sav - power)) {
            return;
        }

        if (process_mod_hallucination(player_ptr, m_name, monrace)) {
            return;
        }

        msg_print(monrace.build_eldritch_horror_message(m_name));
        monrace.r_misc_flags.set(MonsterMiscType::ELDRITCH_HORROR);
        switch (PlayerRace(player_ptr).life()) {
        case PlayerRaceLifeType::DEMON:
            return;
        case PlayerRaceLifeType::UNDEAD:
            if (evaluate_percent(25 + player_ptr->lev)) {
                return;
            }
            break;
        default:
            break;
        }
    } else if (!necro) {
        get_mon_num_prep_enum(player_ptr, MonraceHook::NIGHTMARE);
        const auto monrace_id = get_mon_num(player_ptr, 0, MAX_DEPTH, PM_NONE);
        auto &monrace = monraces.get_monrace(monrace_id);
        power = monrace.level + 10;
        const auto &desc = monrace.name;
        get_mon_num_prep_enum(player_ptr);
        std::string m_name;
#ifdef JP
#else

        if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            m_name = (is_a_vowel(desc.data()[0])) ? "an " : "a ";
        }
#endif
        m_name.append(desc);

        if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            if (monrace.misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
                power /= 2;
            }
        } else {
            power *= 2;
        }

        if (evaluate_percent(player_ptr->skill_sav * 100 / power)) {
            msg_format(_("夢の中で%sに追いかけられた。", "%s^ chases you through your dreams."), m_name.data());
            return;
        }

        if (process_mod_hallucination(player_ptr, m_name, monrace)) {
            return;
        }

        msg_print(monrace.build_eldritch_horror_message(desc));
        monrace.r_misc_flags.set(MonsterMiscType::ELDRITCH_HORROR);
        switch (PlayerRace(player_ptr).life()) {
        case PlayerRaceLifeType::DEMON:
            if (evaluate_percent(20 + player_ptr->lev)) {
                return;
            }
            break;
        case PlayerRaceLifeType::UNDEAD:
            if (evaluate_percent(10 + player_ptr->lev)) {
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
    auto save = true;
    for (auto i = 0; i < 5; i++) {
        save &= evaluate_percent(player_ptr->skill_sav - power);
    }

    if (save) {
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
        } while (!player_ptr->try_resist_eldritch_horror());

        do {
            (void)do_dec_stat(player_ptr, A_WIS);
        } while (!player_ptr->try_resist_eldritch_horror());

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

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
}
