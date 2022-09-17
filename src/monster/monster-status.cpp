#include "monster/monster-status.h"
#include "autopick/autopick-pref-processor.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h" //!< @todo 相互依存. 後で何とかする.
#include "monster/monster-update.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

#if JP
#else
#include "monster/monster-description-types.h"
#endif

static uint32_t csleep_noise;

/*!
 * @brief モンスターIDからPOWERFULフラグの有無を取得する /
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスターID
 * @return POWERFULフラグがあればTRUE、なければFALSEを返す。
 */
bool monster_is_powerful(floor_type *floor_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    return any_bits(r_ptr->flags2, RF2_POWERFUL);
}

/*!
 * @brief モンスターIDからモンスターのレベルを取得する(ただし最低1を保証する) /
 * @param m_idx モンスターID
 * @return モンスターのレベル
 */
DEPTH monster_level_idx(floor_type *floor_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
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
int mon_damage_mod(PlayerType *player_ptr, monster_type *m_ptr, int dam, bool is_psy_spear)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL) && dam > 0) {
        dam /= 100;
        if ((dam == 0) && one_in_(3)) {
            dam = 1;
        }
    }

    if (!m_ptr->is_invulnerable()) {
        return dam;
    }

    if (is_psy_spear) {
        if (!player_ptr->effects()->blindness()->is_blind() && is_seen(player_ptr, m_ptr)) {
            msg_print(_("バリアを切り裂いた！", "The barrier is penetrated!"));
        }

        return dam;
    }

    return one_in_(PENETRATE_INVULNERABILITY) ? dam : 0;
}

/*!
 * @brief モンスターの時限ステータスを取得する
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return m_idx モンスターの参照ID
 * @return mproc_type モンスターの時限ステータスID
 * @return 残りターン値
 */
int get_mproc_idx(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type)
{
    const auto &cur_mproc_list = floor_ptr->mproc_list[mproc_type];
    for (int i = floor_ptr->mproc_max[mproc_type] - 1; i >= 0; i--) {
        if (cur_mproc_list[i] == m_idx) {
            return i;
        }
    }

    return -1;
}

/*!
 * @brief モンスターの時限ステータスリストを追加する
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return m_idx モンスターの参照ID
 * @return mproc_type 追加したいモンスターの時限ステータスID
 */
void mproc_add(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type)
{
    if (floor_ptr->mproc_max[mproc_type] < w_ptr->max_m_idx) {
        floor_ptr->mproc_list[mproc_type][floor_ptr->mproc_max[mproc_type]++] = (int16_t)m_idx;
    }
}

/*!
 * @brief モンスターの時限ステータスリストを初期化する / Initialize monster process
 * @param floor_ptr 現在フロアへの参照ポインタ
 */
void mproc_init(floor_type *floor_ptr)
{
    /* Reset "player_ptr->current_floor_ptr->mproc_max[]" */
    for (int i = 0; i < MAX_MTIMED; i++) {
        floor_ptr->mproc_max[i] = 0;
    }

    /* Process the monsters (backwards) */
    for (MONSTER_IDX i = floor_ptr->m_max - 1; i >= 1; i--) {
        auto *m_ptr = &floor_ptr->m_list[i];

        /* Ignore "dead" monsters */
        if (!m_ptr->is_valid()) {
            continue;
        }

        for (int cmi = 0; cmi < MAX_MTIMED; cmi++) {
            if (m_ptr->mtimed[cmi]) {
                mproc_add(floor_ptr, i, cmi);
            }
        }
    }
}

/*!
 * @brief モンスターの各種状態値を時間経過により更新するサブルーチン
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param mtimed_idx 更新するモンスターの時限ステータスID
 */
static void process_monsters_mtimed_aux(PlayerType *player_ptr, MONSTER_IDX m_idx, int mtimed_idx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    switch (mtimed_idx) {
    case MTIMED_CSLEEP: {
        auto *r_ptr = &r_info[m_ptr->r_idx];
        auto is_wakeup = false;
        if (m_ptr->cdis < AAF_LIMIT) {
            /* Handle "sensing radius" */
            if (m_ptr->cdis <= (m_ptr->is_pet() ? ((r_ptr->aaf > MAX_SIGHT) ? MAX_SIGHT : r_ptr->aaf) : r_ptr->aaf)) {
                is_wakeup = true;
            }

            /* Handle "sight" and "aggravation" */
            else if ((m_ptr->cdis <= MAX_SIGHT) && (player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx))) {
                is_wakeup = true;
            }
        }

        if (!is_wakeup) {
            break;
        }

        auto notice = (uint32_t)randint0(1024);

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
        auto d = (m_ptr->cdis < AAF_LIMIT / 2) ? (AAF_LIMIT / m_ptr->cdis) : 1;

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
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
        }

        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            /* Hack -- Count the wakings */
            if (r_ptr->r_wake < MAX_UCHAR) {
                r_ptr->r_wake++;
            }
        }

        break;
    }

    case MTIMED_FAST:
        /* Reduce by one, note if expires */
        if (set_monster_fast(player_ptr, m_idx, m_ptr->get_remaining_acceleration() - 1)) {
            if (is_seen(player_ptr, m_ptr)) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(player_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sはもう加速されていない。", "%^s is no longer fast."), m_name);
            }
        }

        break;

    case MTIMED_SLOW:
        /* Reduce by one, note if expires */
        if (set_monster_slow(player_ptr, m_idx, m_ptr->get_remaining_deceleration() - 1)) {
            if (is_seen(player_ptr, m_ptr)) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(player_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sはもう減速されていない。", "%^s is no longer slow."), m_name);
            }
        }

        break;

    case MTIMED_STUNNED: {
        int rlev = r_info[m_ptr->r_idx].level;

        /* Recover from stun */
        if (set_monster_stunned(player_ptr, m_idx, (randint0(10000) <= rlev * rlev) ? 0 : (m_ptr->get_remaining_stun() - 1))) {
            /* Message if visible */
            if (is_seen(player_ptr, m_ptr)) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(player_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), m_name);
            }
        }

        break;
    }

    case MTIMED_CONFUSED: {
        /* Reduce the confusion */
        if (!set_monster_confused(player_ptr, m_idx, m_ptr->get_remaining_confusion() - randint1(r_info[m_ptr->r_idx].level / 20 + 1))) {
            break;
        }

        /* Message if visible */
        if (is_seen(player_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), m_name);
        }

        break;
    }

    case MTIMED_MONFEAR: {
        /* Reduce the fear */
        if (!set_monster_monfear(player_ptr, m_idx, m_ptr->get_remaining_fear() - randint1(r_info[m_ptr->r_idx].level / 20 + 1))) {
            break;
        }

        /* Visual note */
        if (is_seen(player_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
#ifdef JP
#else
            char m_poss[80];

            /* Acquire the monster possessive */
            monster_desc(player_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif
            monster_desc(player_ptr, m_name, m_ptr, 0);
#ifdef JP
            msg_format("%^sは勇気を取り戻した。", m_name);
#else
            msg_format("%^s recovers %s courage.", m_name, m_poss);
#endif
        }

        break;
    }

    case MTIMED_INVULNER: {
        /* Reduce by one, note if expires */
        if (!set_monster_invulner(player_ptr, m_idx, m_ptr->get_remaining_invulnerability() - 1, true)) {
            break;
        }

        if (is_seen(player_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("%^sはもう無敵でない。", "%^s is no longer invulnerable."), m_name);
        }

        break;
    }
    }
}

/*!
 * @brief 全モンスターの各種状態値を時間経過により更新するメインルーチン
 * @param mtimed_idx 更新するモンスターの時限ステータスID
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Process the counters of monsters (once per 10 game turns)\n
 * These functions are to process monsters' counters same as player's.
 */
void process_monsters_mtimed(PlayerType *player_ptr, int mtimed_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto &cur_mproc_list = floor_ptr->mproc_list[mtimed_idx];

    /* Hack -- calculate the "player noise" */
    if (mtimed_idx == MTIMED_CSLEEP) {
        csleep_noise = (1U << (30 - player_ptr->skill_stl));
    }

    /* Process the monsters (backwards) */
    for (auto i = floor_ptr->mproc_max[mtimed_idx] - 1; i >= 0; i--) {
        process_monsters_mtimed_aux(player_ptr, cur_mproc_list[i], mtimed_idx);
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
    GAME_TEXT m_name[MAX_NLEN];

    monster_desc(player_ptr, m_name, m_ptr, 0);
    if (set_monster_invulner(player_ptr, m_idx, 0, true)) {
        if (m_ptr->ml) {
            msg_format(_("%sはもう無敵ではない。", "%^s is no longer invulnerable."), m_name);
        }
    }

    if (set_monster_fast(player_ptr, m_idx, 0)) {
        if (m_ptr->ml) {
            msg_format(_("%sはもう加速されていない。", "%^s is no longer fast."), m_name);
        }
    }

    if (set_monster_slow(player_ptr, m_idx, 0)) {
        if (m_ptr->ml) {
            msg_format(_("%sはもう減速されていない。", "%^s is no longer slow."), m_name);
        }
    }
}

/*!
 * @brief モンスターの経験値取得処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 経験値を得るモンスターの参照ID
 * @param s_idx 撃破されたモンスター種族の参照ID
 */
void monster_gain_exp(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterRaceId s_idx)
{
    if (m_idx <= 0 || !MonsterRace(s_idx).is_valid()) {
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];

    if (!m_ptr->is_valid()) {
        return;
    }

    auto *r_ptr = &r_info[m_ptr->r_idx];
    auto *s_ptr = &r_info[s_idx];

    if (player_ptr->phase_out || (r_ptr->next_exp == 0)) {
        return;
    }

    auto new_exp = s_ptr->mexp * s_ptr->level / (r_ptr->level + 2);
    if (m_idx == player_ptr->riding) {
        new_exp = (new_exp + 1) / 2;
    }

    if (!floor_ptr->dun_level) {
        new_exp /= 5;
    }

    m_ptr->exp += new_exp;
    if (m_ptr->mflag2.has(MonsterConstantFlagType::CHAMELEON)) {
        return;
    }

    if (m_ptr->exp < r_ptr->next_exp) {
        if (m_idx == player_ptr->riding) {
            player_ptr->update |= PU_BONUS;
        }

        return;
    }

    GAME_TEXT m_name[MAX_NLEN];
    auto old_hp = m_ptr->hp;
    auto old_maxhp = m_ptr->max_maxhp;
    auto old_r_idx = m_ptr->r_idx;
    auto old_sub_align = m_ptr->sub_align;

    /* Hack -- Reduce the racial counter of previous monster */
    m_ptr->get_real_r_ref().cur_num--;

    monster_desc(player_ptr, m_name, m_ptr, 0);
    m_ptr->r_idx = r_ptr->next_r_idx;

    /* Count the monsters on the level */
    m_ptr->get_real_r_ref().cur_num++;

    m_ptr->ap_r_idx = m_ptr->r_idx;
    r_ptr = &r_info[m_ptr->r_idx];

    m_ptr->max_maxhp = any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) ? maxroll(r_ptr->hdice, r_ptr->hside) : damroll(r_ptr->hdice, r_ptr->hside);
    if (ironman_nightmare) {
        auto hp = m_ptr->max_maxhp * 2;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    m_ptr->maxhp = m_ptr->max_maxhp;
    m_ptr->hp = old_hp * m_ptr->maxhp / old_maxhp;

    /* dealt damage is 0 at initial*/
    m_ptr->dealt_damage = 0;

    /* Extract the monster base speed */
    m_ptr->mspeed = get_mspeed(floor_ptr, r_ptr);

    /* Sub-alignment of a monster */
    if (!m_ptr->is_pet() && r_ptr->kind_flags.has_none_of(alignment_mask)) {
        m_ptr->sub_align = old_sub_align;
    } else {
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
        if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            m_ptr->sub_align |= SUB_ALIGN_EVIL;
        }

        if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            m_ptr->sub_align |= SUB_ALIGN_GOOD;
        }
    }

    m_ptr->exp = 0;
    if (m_ptr->is_pet() || m_ptr->ml) {
        auto is_hallucinated = player_ptr->effects()->hallucination()->is_hallucinated();
        if (!ignore_unview || player_can_see_bold(player_ptr, m_ptr->fy, m_ptr->fx)) {
            if (is_hallucinated) {
                monster_race *hallucinated_race = nullptr;
                do {
                    auto r_idx = MonsterRace::pick_one_at_random();
                    hallucinated_race = &r_info[r_idx];
                } while (hallucinated_race->name.empty() || hallucinated_race->kind_flags.has(MonsterKindType::UNIQUE));
                auto mes_evolution = _("%sは%sに進化した。", "%^s evolved into %s.");
                auto mes_degeneration = _("%sは%sに退化した。", "%^s degenerated into %s.");
                auto mes = randint0(2) == 0 ? mes_evolution : mes_degeneration;
                msg_format(mes, m_name, hallucinated_race->name.c_str());
            } else {
                msg_format(_("%sは%sに進化した。", "%^s evolved into %s."), m_name, r_ptr->name.c_str());
            }
        }

        if (!is_hallucinated) {
            r_info[old_r_idx].r_can_evolve = true;
        }

        /* Now you feel very close to this pet. */
        m_ptr->parent_m_idx = 0;
    }

    update_monster(player_ptr, m_idx, false);
    lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);

    if (m_idx == player_ptr->riding) {
        player_ptr->update |= PU_BONUS;
    }
}
