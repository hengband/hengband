#include "monster/monster-status.h"
#include "autopick/autopick-pref-processor.h"
#include "core/speed-table.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-special-flags.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h" //!< @todo 相互依存. 後で何とかする.
#include "monster/monster-update.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "tracking/health-bar-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#if JP
#else
#include "monster/monster-description-types.h"
#endif

static uint32_t csleep_noise;

/*!
 * @brief モンスターIDからPOWERFULフラグの有無を取得する /
 * @param floor フロアへの参照
 * @param m_idx モンスターID
 * @return POWERFULフラグがあればTRUE、なければFALSEを返す。
 */
bool monster_is_powerful(const FloorType &floor, MONSTER_IDX m_idx)
{
    auto *m_ptr = &floor.m_list[m_idx];
    auto *r_ptr = &m_ptr->get_monrace();
    return r_ptr->misc_flags.has(MonsterMiscType::POWERFUL);
}

/*!
 * @brief モンスターIDからモンスターのレベルを取得する(ただし最低1を保証する) /
 * @param m_idx モンスターID
 * @return モンスターのレベル
 */
DEPTH monster_level_idx(const FloorType &floor, MONSTER_IDX m_idx)
{
    auto *m_ptr = &floor.m_list[m_idx];
    auto *r_ptr = &m_ptr->get_monrace();
    return (r_ptr->level >= 1) ? r_ptr->level : 1;
}

/*!
 * @brief モンスターに与えたダメージの修正処理 /
 * Modify the physical damage done to the monster.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr ダメージを受けるモンスターの構造体参照ポインタ
 * @param dam ダメージ基本値
 * @param is_psy_spear 攻撃手段が光の剣ならばTRUE
 * @return 修正を行った結果のダメージ量
 * @details RES_ALL持ちはAC軽減後のダメージを1/100に補正する. 光の剣は無敵を無効化する. 一定確率で無敵は貫通できる.
 */
int mon_damage_mod(PlayerType *player_ptr, MonsterEntity *m_ptr, int dam, bool is_psy_spear)
{
    auto *r_ptr = &m_ptr->get_monrace();
    if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL) && dam > 0) {
        dam /= 100;
        if ((dam == 0) && one_in_(3)) {
            dam = 1;
        }
    }

    auto &race_info = m_ptr->get_monrace();

    if (race_info.special_flags.has(MonsterSpecialType::DIMINISH_MAX_DAMAGE)) {
        race_info.r_special_flags.set(MonsterSpecialType::DIMINISH_MAX_DAMAGE);
        if (dam > m_ptr->hp / 10) {
            dam = std::max(m_ptr->hp / 10, m_ptr->maxhp * 7 / 500);
            msg_format(_("%s^は致命的なダメージを抑えた！", "%s^ resisted a critical damage!"), monster_desc(player_ptr, m_ptr, 0).data());
        }
    }

    if (!m_ptr->is_invulnerable()) {
        return dam;
    }

    if (is_psy_spear) {
        if (!player_ptr->effects()->blindness().is_blind() && is_seen(player_ptr, m_ptr)) {
            msg_print(_("バリアを切り裂いた！", "The barrier is penetrated!"));
        }

        return dam;
    }

    return one_in_(PENETRATE_INVULNERABILITY) ? dam : 0;
}

/*!
 * @brief モンスターの各種状態値を時間経過により更新するサブルーチン
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param mte 更新するモンスターの時限ステータスID
 */
static void process_monsters_mtimed_aux(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterTimedEffect mte)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto *m_ptr = &floor.m_list[m_idx];
    switch (mte) {
    case MonsterTimedEffect::SLEEP: {
        auto *r_ptr = &m_ptr->get_monrace();
        auto is_wakeup = false;
        if (m_ptr->cdis < MAX_MONSTER_SENSING) {
            /* Handle "sensing radius" */
            if (m_ptr->cdis <= (m_ptr->is_pet() ? ((r_ptr->aaf > MAX_PLAYER_SIGHT) ? MAX_PLAYER_SIGHT : r_ptr->aaf) : r_ptr->aaf)) {
                is_wakeup = true;
            }

            /* Handle "sight" and "aggravation" */
            else if ((m_ptr->cdis <= MAX_PLAYER_SIGHT) && floor.has_los({ m_ptr->fy, m_ptr->fx })) {
                is_wakeup = true;
            }
        }

        if (!is_wakeup) {
            break;
        }

        auto notice = randnum0<uint32_t>(1024);

        /* Nightmare monsters are more alert */
        if (ironman_nightmare) {
            notice /= 2;
        }

        /* Hack -- See if monster "notices" player */
        if ((notice * notice * notice) > csleep_noise) {
            break;
        }

        /* Hack -- amount of "waking" */
        /* Wake up faster near the player */
        auto d = (m_ptr->cdis < MAX_MONSTER_SENSING / 2) ? (MAX_MONSTER_SENSING / m_ptr->cdis) : 1;

        /* Hack -- amount of "waking" is affected by speed of player */
        d = (d * speed_to_energy(player_ptr->pspeed)) / 10;
        if (d < 0) {
            d = 1;
        }

        /* Monster wakes up "a little bit" */

        /* Still asleep */
        if (!set_monster_csleep(player_ptr, m_idx, m_ptr->get_remaining_sleep() - d)) {
            /* Notice the "not waking up" */
            if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                /* Hack -- Count the ignores */
                if (r_ptr->r_ignore < MAX_UCHAR) {
                    r_ptr->r_ignore++;
                }
            }

            break;
        }

        /* Notice the "waking up" */
        if (m_ptr->ml) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            msg_format(_("%s^が目を覚ました。", "%s^ wakes up."), m_name.data());
        }

        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            /* Hack -- Count the wakings */
            if (r_ptr->r_wake < MAX_UCHAR) {
                r_ptr->r_wake++;
            }
        }

        break;
    }
    case MonsterTimedEffect::FAST:
        /* Reduce by one, note if expires */
        if (set_monster_fast(player_ptr, m_idx, m_ptr->get_remaining_acceleration() - 1)) {
            if (is_seen(player_ptr, m_ptr)) {
                const auto m_name = monster_desc(player_ptr, m_ptr, 0);
                msg_format(_("%s^はもう加速されていない。", "%s^ is no longer fast."), m_name.data());
            }
        }

        break;
    case MonsterTimedEffect::SLOW:
        /* Reduce by one, note if expires */
        if (set_monster_slow(player_ptr, m_idx, m_ptr->get_remaining_deceleration() - 1)) {
            if (is_seen(player_ptr, m_ptr)) {
                const auto m_name = monster_desc(player_ptr, m_ptr, 0);
                msg_format(_("%s^はもう減速されていない。", "%s^ is no longer slow."), m_name.data());
            }
        }

        break;
    case MonsterTimedEffect::STUN: {
        int rlev = m_ptr->get_monrace().level;

        /* Recover from stun */
        if (set_monster_stunned(player_ptr, m_idx, (randint0(10000) <= rlev * rlev) ? 0 : (m_ptr->get_remaining_stun() - 1))) {
            /* Message if visible */
            if (is_seen(player_ptr, m_ptr)) {
                const auto m_name = monster_desc(player_ptr, m_ptr, 0);
                msg_format(_("%s^は朦朧状態から立ち直った。", "%s^ is no longer stunned."), m_name.data());
            }
        }

        break;
    }
    case MonsterTimedEffect::CONFUSION: {
        /* Reduce the confusion */
        if (!set_monster_confused(player_ptr, m_idx, m_ptr->get_remaining_confusion() - randint1(m_ptr->get_monrace().level / 20 + 1))) {
            break;
        }

        /* Message if visible */
        if (is_seen(player_ptr, m_ptr)) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            msg_format(_("%s^は混乱から立ち直った。", "%s^ is no longer confused."), m_name.data());
        }

        break;
    }
    case MonsterTimedEffect::FEAR: {
        /* Reduce the fear */
        if (!set_monster_monfear(player_ptr, m_idx, m_ptr->get_remaining_fear() - randint1(m_ptr->get_monrace().level / 20 + 1))) {
            break;
        }

        /* Visual note */
        if (is_seen(player_ptr, m_ptr)) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
#ifdef JP
#else
            /* Acquire the monster possessive */
            const auto m_poss = monster_desc(player_ptr, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif
#ifdef JP
            msg_format("%s^は勇気を取り戻した。", m_name.data());
#else
            msg_format("%s^ recovers %s courage.", m_name.data(), m_poss.data());
#endif
        }

        break;
    }
    case MonsterTimedEffect::INVULNERABILITY: {
        /* Reduce by one, note if expires */
        if (!set_monster_invulner(player_ptr, m_idx, m_ptr->get_remaining_invulnerability() - 1, true)) {
            break;
        }

        if (is_seen(player_ptr, m_ptr)) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            msg_format(_("%s^はもう無敵でない。", "%s^ is no longer invulnerable."), m_name.data());
        }

        break;
    }
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid MonsterTimedEffect is specified! %d", enum2i(mte)));
    }
}

/*!
 * @brief 全モンスターの各種状態値を時間経過により更新するメインルーチン
 * @param mte 更新するモンスターの時限ステータスID
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Process the counters of monsters (once per 10 game turns)\n
 * These functions are to process monsters' counters same as player's.
 */
void process_monsters_mtimed(PlayerType *player_ptr, MonsterTimedEffect mte)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &cur_mproc_list = floor.mproc_list.at(mte);

    /* Hack -- calculate the "player noise" */
    if (mte == MonsterTimedEffect::SLEEP) {
        csleep_noise = (1U << (30 - player_ptr->skill_stl));
    }

    /* Process the monsters (backwards) */
    for (auto i = floor.mproc_max.at(mte) - 1; i >= 0; i--) {
        const auto m_idx = cur_mproc_list[i];
        process_monsters_mtimed_aux(player_ptr, m_idx, mte);
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
    }
}

/*!
 * @brief モンスターへの魔力消去処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 魔力消去を受けるモンスターの参照ID
 */
void dispel_monster_status(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    const auto m_name = monster_desc(player_ptr, m_ptr, 0);
    if (set_monster_invulner(player_ptr, m_idx, 0, true)) {
        if (m_ptr->ml) {
            msg_format(_("%sはもう無敵ではない。", "%s^ is no longer invulnerable."), m_name.data());
        }
    }

    if (set_monster_fast(player_ptr, m_idx, 0)) {
        if (m_ptr->ml) {
            msg_format(_("%sはもう加速されていない。", "%s^ is no longer fast."), m_name.data());
        }
    }

    if (set_monster_slow(player_ptr, m_idx, 0)) {
        if (m_ptr->ml) {
            msg_format(_("%sはもう減速されていない。", "%s^ is no longer slow."), m_name.data());
        }
    }
}

/*!
 * @brief モンスターの経験値取得処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 経験値を得るモンスターの参照ID
 * @param monrace_id 撃破されたモンスター種族ID
 */
void monster_gain_exp(PlayerType *player_ptr, MONSTER_IDX m_idx, MonraceId monrace_id)
{
    if (m_idx <= 0 || !MonraceList::is_valid(monrace_id)) {
        return;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    if (!monster.is_valid()) {
        return;
    }

    auto &old_monrace = monster.get_monrace(); //!< @details 現在 (進化前)の種族.
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace_defeated = monraces.get_monrace(monrace_id);
    if (AngbandSystem::get_instance().is_phase_out() || (old_monrace.next_exp == 0)) {
        return;
    }

    auto new_exp = monrace_defeated.mexp * monrace_defeated.level / (old_monrace.level + 2);
    if (monster.is_riding()) {
        new_exp = (new_exp + 1) / 2;
    }

    if (!floor.is_underground()) {
        new_exp /= 5;
    }

    monster.exp += new_exp;
    if (monster.mflag2.has(MonsterConstantFlagType::CHAMELEON)) {
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (monster.exp < old_monrace.next_exp) {
        if (monster.is_riding()) {
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
        }

        return;
    }

    const auto old_hp = monster.hp;
    const auto old_maxhp = monster.max_maxhp;
    const auto old_sub_align = monster.sub_align;

    /* Hack -- Reduce the racial counter of previous monster */
    monster.get_real_monrace().decrement_current_numbers();

    const auto m_name = monster_desc(player_ptr, &monster, 0);
    monster.r_idx = old_monrace.next_r_idx;

    /* Count the monsters on the level */
    monster.get_real_monrace().increment_current_numbers();
    monster.ap_r_idx = monster.r_idx;

    const auto &new_monrace = monster.get_monrace(); //!< @details 進化後の種族.
    monster.max_maxhp = new_monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP) ? new_monrace.hit_dice.maxroll() : new_monrace.hit_dice.roll();
    if (ironman_nightmare) {
        const auto hp = monster.max_maxhp * 2;
        monster.max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    monster.maxhp = monster.max_maxhp;
    monster.hp = old_hp * monster.maxhp / old_maxhp;

    /* dealt damage is 0 at initial*/
    monster.dealt_damage = 0;

    /* Extract the monster base speed */
    monster.set_individual_speed(floor.inside_arena);

    /* Sub-alignment of a monster */
    if (!monster.is_pet() && new_monrace.kind_flags.has_none_of(alignment_mask)) {
        monster.sub_align = old_sub_align;
    } else {
        monster.sub_align = SUB_ALIGN_NEUTRAL;
        if (new_monrace.kind_flags.has(MonsterKindType::EVIL)) {
            monster.sub_align |= SUB_ALIGN_EVIL;
        }

        if (new_monrace.kind_flags.has(MonsterKindType::GOOD)) {
            monster.sub_align |= SUB_ALIGN_GOOD;
        }
    }

    monster.exp = 0;
    if (monster.is_pet() || monster.ml) {
        const auto is_hallucinated = player_ptr->effects()->hallucination().is_hallucinated();
        if (!ignore_unview || player_can_see_bold(player_ptr, monster.fy, monster.fx)) {
            if (is_hallucinated) {
                const auto ids = monraces.search([](const auto &monrace) { return monrace.kind_flags.has_not(MonsterKindType::UNIQUE); });
                const auto &monrace_hallucinated = monraces.get_monrace(rand_choice(ids));
                auto mes_evolution = _("%sは%sに進化した。", "%s^ evolved into %s.");
                auto mes_degeneration = _("%sは%sに退化した。", "%s^ degenerated into %s.");
                auto mes = randint0(2) == 0 ? mes_evolution : mes_degeneration;
                msg_format(mes, m_name.data(), monrace_hallucinated.name.data());
            } else {
                msg_format(_("%sは%sに進化した。", "%s^ evolved into %s."), m_name.data(), new_monrace.name.data());
            }
        }

        if (!is_hallucinated) {
            old_monrace.r_can_evolve = true;
        }

        /* Now you feel very close to this pet. */
        monster.parent_m_idx = 0;
    }

    update_monster(player_ptr, m_idx, false);
    lite_spot(player_ptr, monster.fy, monster.fx);

    if (monster.is_riding()) {
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
    }
}
