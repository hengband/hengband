/*!
 * @brief モンスター処理 / misc code for monsters
 * @date 2014/07/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "monster/monster-list.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "grid/grid.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "pet/pet-fall-off.h"
#include "player/player-status.h"
#include "system/alloc-entries.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/probability-table.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <iterator>

#define HORDE_NOGOOD 0x01 /*!< (未実装フラグ)HORDE生成でGOODなモンスターの生成を禁止する？ */
#define HORDE_NOEVIL 0x02 /*!< (未実装フラグ)HORDE生成でEVILなモンスターの生成を禁止する？ */

/*!
 * @brief モンスター配列の空きを探す / Acquires and returns the index of a "free" monster.
 * @return 利用可能なモンスター配列の添字
 * @details
 * This routine should almost never fail, but it *can* happen.
 */
MONSTER_IDX m_pop(floor_type *floor_ptr)
{
    /* Normal allocation */
    if (floor_ptr->m_max < w_ptr->max_m_idx) {
        MONSTER_IDX i = floor_ptr->m_max;
        floor_ptr->m_max++;
        floor_ptr->m_cnt++;
        return i;
    }

    /* Recycle dead monsters */
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        monster_type *m_ptr;
        m_ptr = &floor_ptr->m_list[i];
        if (m_ptr->r_idx)
            continue;
        floor_ptr->m_cnt++;
        return i;
    }

    if (w_ptr->character_dungeon)
        msg_print(_("モンスターが多すぎる！", "Too many monsters!"));
    return 0;
}

/*!
 * @brief 生成モンスター種族を1種生成テーブルから選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param min_level 最小生成階
 * @param max_level 最大生成階
 * @return 選択されたモンスター生成種族
 */
MONRACE_IDX get_mon_num(player_type *player_ptr, DEPTH min_level, DEPTH max_level, BIT_FLAGS option)
{
    int r_idx;
    monster_race *r_ptr;

    int pls_kakuritu, pls_max_level, over_days;
    int delay = mysqrt(max_level * 10000L) + (max_level * 5);

    /* town max_level : same delay as 10F, no nasty mons till day18 */
    if (!max_level)
        delay = 360;

    if (max_level > MAX_DEPTH - 1)
        max_level = MAX_DEPTH - 1;

    /* +1 per day after the base date */
    /* base dates : day5(1F), day18(10F,0F), day34(30F), day53(60F), day69(90F) */
    over_days = std::max<int>(0, w_ptr->dungeon_turn / (TURNS_PER_TICK * 10000L) - delay / 20);

    /* starts from 1/25, reaches 1/3 after 44days from a max_level dependent base date */
    pls_kakuritu = std::max(NASTY_MON_MAX, NASTY_MON_BASE - over_days / 2);
    /* starts from 0, reaches +25lv after 75days from a max_level dependent base date */
    pls_max_level = std::min(NASTY_MON_PLUS_MAX, over_days / 3);

    if (d_info[player_ptr->dungeon_idx].flags.has(DF::MAZE)) {
        pls_kakuritu = std::min(pls_kakuritu / 2, pls_kakuritu - 10);
        if (pls_kakuritu < 2)
            pls_kakuritu = 2;
        pls_max_level += 2;
        max_level += 3;
    }

    /* Boost the max_level */
    if ((option & GMN_ARENA) || d_info[player_ptr->dungeon_idx].flags.has_not(DF::BEGINNER)) {
        /* Nightmare mode allows more out-of depth monsters */
        if (ironman_nightmare && !randint0(pls_kakuritu)) {
            /* What a bizarre calculation */
            max_level = 1 + (max_level * MAX_DEPTH / randint1(MAX_DEPTH));
        } else {
            /* Occasional "nasty" monster */
            if (!randint0(pls_kakuritu)) {
                /* Pick a max_level bonus */
                max_level += pls_max_level;
            }
        }
    }

    ProbabilityTable<int> prob_table;

    /* Process probabilities */
    for (auto i = 0U; i < alloc_race_table.size(); i++) {
        const auto &entry = alloc_race_table[i];
        if (entry.level < min_level)
            continue;
        if (max_level < entry.level)
            break; // sorted by depth array,
        r_idx = entry.index;
        r_ptr = &r_info[r_idx];
        if (!(option & GMN_ARENA) && !chameleon_change_m_idx) {
            if (((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags7 & (RF7_NAZGUL))) && (r_ptr->cur_num >= r_ptr->max_num)) {
                continue;
            }

            if ((r_ptr->flags7 & (RF7_UNIQUE2)) && (r_ptr->cur_num >= 1)) {
                continue;
            }

            if (r_idx == MON_BANORLUPART) {
                if (r_info[MON_BANOR].cur_num > 0)
                    continue;
                if (r_info[MON_LUPART].cur_num > 0)
                    continue;
            }
        }

        prob_table.entry_item(i, entry.prob2);
    }

    if (cheat_hear) {
        msg_format(_("モンスター第3次候補数:%d(%d-%dF)%d ", "monster third selection:%d(%d-%dF)%d "), prob_table.item_count(), min_level, max_level,
            prob_table.total_prob());
    }

    if (prob_table.empty())
        return 0;

    // 40%で1回、50%で2回、10%で3回抽選し、その中で一番レベルが高いモンスターを選択する
    int n = 1;

    const int p = randint0(100);
    if (p < 60)
        n++;
    if (p < 10)
        n++;

    std::vector<int> result;
    ProbabilityTable<int>::lottery(std::back_inserter(result), prob_table, n);

    auto it = std::max_element(result.begin(), result.end(), [](int a, int b) { return alloc_race_table[a].level < alloc_race_table[b].level; });

    return alloc_race_table[*it].index;
}

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(player_type *player_ptr, MONRACE_IDX r_idx)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_race *r_ptr = &r_info[r_idx];
    monster_type *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
    monster_race *old_r_ptr = &r_info[m_ptr->r_idx];

    if (!(r_ptr->flags1 & (RF1_UNIQUE)))
        return false;
    if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON))
        return false;

    if (std::abs(r_ptr->level - r_info[MON_CHAMELEON_K].level) > 5)
        return false;

    if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE)
        || (r_ptr->blow[3].method == RBM_EXPLODE))
        return false;

    if (!monster_can_cross_terrain(player_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0))
        return false;

    if (!(old_r_ptr->flags7 & RF7_CHAMELEON)) {
        if (monster_has_hostile_align(player_ptr, m_ptr, 0, 0, r_ptr))
            return false;
    } else if (summon_specific_who > 0) {
        if (monster_has_hostile_align(player_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr))
            return false;
    }

    return true;
}

/*!
 * @brief カメレオンの変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(player_type *player_ptr, MONRACE_IDX r_idx)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_race *r_ptr = &r_info[r_idx];
    monster_type *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
    monster_race *old_r_ptr = &r_info[m_ptr->r_idx];

    if (r_ptr->flags1 & (RF1_UNIQUE))
        return false;
    if (r_ptr->flags2 & RF2_MULTIPLY)
        return false;
    if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON))
        return false;

    if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE)
        || (r_ptr->blow[3].method == RBM_EXPLODE))
        return false;

    if (!monster_can_cross_terrain(player_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0))
        return false;

    if (!(old_r_ptr->flags7 & RF7_CHAMELEON)) {
        if ((old_r_ptr->flags3 & RF3_GOOD) && !(r_ptr->flags3 & RF3_GOOD))
            return false;
        if ((old_r_ptr->flags3 & RF3_EVIL) && !(r_ptr->flags3 & RF3_EVIL))
            return false;
        if (!(old_r_ptr->flags3 & (RF3_GOOD | RF3_EVIL)) && (r_ptr->flags3 & (RF3_GOOD | RF3_EVIL)))
            return false;
    } else if (summon_specific_who > 0) {
        if (monster_has_hostile_align(player_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr))
            return false;
    }

    return (*(get_monster_hook(player_ptr)))(player_ptr, r_idx);
}

/*!
 * @brief モンスターの変身処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 変身処理を受けるモンスター情報のID
 * @param born 生成時の初変身先指定ならばtrue
 * @param r_idx 旧モンスター種族のID
 */
void choose_new_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr;

    bool old_unique = false;
    if (r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE)
        old_unique = true;
    if (old_unique && (r_idx == MON_CHAMELEON))
        r_idx = MON_CHAMELEON_K;
    r_ptr = &r_info[r_idx];

    char old_m_name[MAX_NLEN];
    monster_desc(player_ptr, old_m_name, m_ptr, 0);

    if (!r_idx) {
        DEPTH level;

        chameleon_change_m_idx = m_idx;
        if (old_unique)
            get_mon_num_prep(player_ptr, monster_hook_chameleon_lord, nullptr);
        else
            get_mon_num_prep(player_ptr, monster_hook_chameleon, nullptr);

        if (old_unique)
            level = r_info[MON_CHAMELEON_K].level;
        else if (!floor_ptr->dun_level)
            level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
        else
            level = floor_ptr->dun_level;

        if (d_info[player_ptr->dungeon_idx].flags.has(DF::CHAMELEON))
            level += 2 + randint1(3);

        r_idx = get_mon_num(player_ptr, 0, level, 0);
        r_ptr = &r_info[r_idx];

        chameleon_change_m_idx = 0;
        if (!r_idx)
            return;
    }

    m_ptr->r_idx = r_idx;
    m_ptr->ap_r_idx = r_idx;
    update_monster(player_ptr, m_idx, false);
    lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);

    int old_r_idx = m_ptr->r_idx;
    if ((r_info[old_r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) || (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)))
        player_ptr->update |= (PU_MON_LITE);

    if (born) {
        if (r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)) {
            m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
            if (r_ptr->flags3 & RF3_EVIL)
                m_ptr->sub_align |= SUB_ALIGN_EVIL;
            if (r_ptr->flags3 & RF3_GOOD)
                m_ptr->sub_align |= SUB_ALIGN_GOOD;
        }

        return;
    }

    if (m_idx == player_ptr->riding) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, m_ptr, 0);
        msg_format(_("突然%sが変身した。", "Suddenly, %s transforms!"), old_m_name);
        if (!(r_ptr->flags7 & RF7_RIDING))
            if (process_fall_off_horse(player_ptr, 0, true))
                msg_format(_("地面に落とされた。", "You have fallen from %s."), m_name);
    }

    m_ptr->mspeed = get_mspeed(floor_ptr, r_ptr);

    int oldmaxhp = m_ptr->max_maxhp;
    if (r_ptr->flags1 & RF1_FORCE_MAXHP) {
        m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
    } else {
        m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
    }

    if (ironman_nightmare) {
        auto hp = m_ptr->max_maxhp * 2;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    m_ptr->maxhp = (long)(m_ptr->maxhp * m_ptr->max_maxhp) / oldmaxhp;
    if (m_ptr->maxhp < 1)
        m_ptr->maxhp = 1;
    m_ptr->hp = (long)(m_ptr->hp * m_ptr->max_maxhp) / oldmaxhp;
    m_ptr->dealt_damage = 0;
}

/*!
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param r_ptr モンスター種族の参照ポインタ
 * @return 加速値
 */
SPEED get_mspeed(floor_type *floor_ptr, monster_race *r_ptr)
{
    SPEED mspeed = r_ptr->speed;
    if (!(r_ptr->flags1 & RF1_UNIQUE) && !floor_ptr->inside_arena) {
        /* Allow some small variation per monster */
        int i = SPEED_TO_ENERGY(r_ptr->speed) / (one_in_(4) ? 3 : 10);
        if (i)
            mspeed += rand_spread(0, i);
    }

    if (mspeed > 199)
        mspeed = 199;

    return mspeed;
}

/*!
 * @brief 指定したモンスターに隣接しているモンスターの数を返す。
 * / Count number of adjacent monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 隣接数を調べたいモンスターのID
 * @return 隣接しているモンスターの数
 */
int get_monster_crowd_number(floor_type *floor_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    POSITION my = m_ptr->fy;
    POSITION mx = m_ptr->fx;
    int count = 0;
    for (int i = 0; i < 7; i++) {
        int ay = my + ddy_ddd[i];
        int ax = mx + ddx_ddd[i];

        if (!in_bounds(floor_ptr, ay, ax))
            continue;
        if (floor_ptr->grid_array[ay][ax].m_idx > 0)
            count++;
    }

    return count;
}
