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
#include "core/speed-table.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "grid/grid.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-brightness-mask.h"
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
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/probability-table.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <cmath>
#include <iterator>

#define HORDE_NOGOOD 0x01 /*!< (未実装フラグ)HORDE生成でGOODなモンスターの生成を禁止する？ */
#define HORDE_NOEVIL 0x02 /*!< (未実装フラグ)HORDE生成でEVILなモンスターの生成を禁止する？ */

/*!
 * @brief モンスター配列の空きを探す / Acquires and returns the index of a "free" monster.
 * @return 利用可能なモンスター配列の添字
 * @details
 * This routine should almost never fail, but it *can* happen.
 */
MONSTER_IDX m_pop(FloorType *floor_ptr)
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
        MonsterEntity *m_ptr;
        m_ptr = &floor_ptr->m_list[i];
        if (MonsterRace(m_ptr->r_idx).is_valid()) {
            continue;
        }
        floor_ptr->m_cnt++;
        return i;
    }

    if (w_ptr->character_dungeon) {
        msg_print(_("モンスターが多すぎる！", "Too many monsters!"));
    }
    return 0;
}

/*!
 * @brief 生成モンスター種族を1種生成テーブルから選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param min_level 最小生成階
 * @param max_level 最大生成階
 * @return 選択されたモンスター生成種族
 * @details nasty生成 (ゲーム内経過日数に応じて、現在フロアより深いフロアのモンスターを出現させる仕様)は
 */
MonsterRaceId get_mon_num(PlayerType *player_ptr, DEPTH min_level, DEPTH max_level, BIT_FLAGS option)
{
    /* town max_level : same delay as 10F, no nasty mons till day18 */
    auto delay = static_cast<int>(std::sqrt(max_level * 10000)) + (max_level * 5);
    if (!max_level) {
        delay = 360;
    }

    if (max_level > MAX_DEPTH - 1) {
        max_level = MAX_DEPTH - 1;
    }

    /* +1 per day after the base date */
    /* base dates : day5(1F), day18(10F,0F), day34(30F), day53(60F), day69(90F) */
    const auto over_days = std::max<int>(0, w_ptr->dungeon_turn / (TURNS_PER_TICK * 10000L) - delay / 20);

    /* Probability starts from 1/25, reaches 1/3 after 44days from a max_level dependent base date */
    /* Boost level starts from 0, reaches +25lv after 75days from a max_level dependent base date */
    constexpr auto chance_nasty_monster = 25;
    constexpr auto max_num_nasty_monsters = 3;
    constexpr auto max_depth_nasty_monster = 25;
    auto chance_nasty = std::max(max_num_nasty_monsters, chance_nasty_monster - over_days / 2);
    auto nasty_level = std::min(max_depth_nasty_monster, over_days / 3);
    const auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::MAZE)) {
        chance_nasty = std::min(chance_nasty / 2, chance_nasty - 10);
        if (chance_nasty < 2) {
            chance_nasty = 2;
        }

        nasty_level += 2;
        max_level += 3;
    }

    /* Boost the max_level */
    if ((option & GMN_ARENA) || dungeons_info[floor.dungeon_idx].flags.has_not(DungeonFeatureType::BEGINNER)) {
        /* Nightmare mode allows more out-of depth monsters */
        if (ironman_nightmare && !randint0(chance_nasty)) {
            /* What a bizarre calculation */
            max_level = 1 + (max_level * MAX_DEPTH / randint1(MAX_DEPTH));
        } else {
            /* Occasional "nasty" monster */
            if (!randint0(chance_nasty)) {
                /* Pick a max_level bonus */
                max_level += nasty_level;
            }
        }
    }

    ProbabilityTable<int> prob_table;

    /* Process probabilities */
    for (auto i = 0U; i < alloc_race_table.size(); i++) {
        const auto &entry = alloc_race_table[i];
        if (entry.level < min_level) {
            continue;
        }
        if (max_level < entry.level) {
            break;
        } // sorted by depth array,
        auto r_idx = i2enum<MonsterRaceId>(entry.index);
        auto r_ptr = &monraces_info[r_idx];
        if (!(option & GMN_ARENA) && !chameleon_change_m_idx) {
            if ((r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->population_flags.has(MonsterPopulationType::NAZGUL)) && (r_ptr->cur_num >= r_ptr->max_num)) {
                continue;
            }

            if ((r_ptr->flags7 & (RF7_UNIQUE2)) && (r_ptr->cur_num >= 1)) {
                continue;
            }

            if (r_idx == MonsterRaceId::BANORLUPART) {
                if (monraces_info[MonsterRaceId::BANOR].cur_num > 0) {
                    continue;
                }
                if (monraces_info[MonsterRaceId::LUPART].cur_num > 0) {
                    continue;
                }
            }
        }

        prob_table.entry_item(i, entry.prob2);
    }

    if (cheat_hear) {
        msg_format(_("モンスター第3次候補数:%lu(%d-%dF)%d ", "monster third selection:%lu(%d-%dF)%d "), prob_table.item_count(), min_level, max_level,
            prob_table.total_prob());
    }

    if (prob_table.empty()) {
        return MonsterRace::empty_id();
    }

    // 40%で1回、50%で2回、10%で3回抽選し、その中で一番レベルが高いモンスターを選択する
    int n = 1;

    const int p = randint0(100);
    if (p < 60) {
        n++;
    }
    if (p < 10) {
        n++;
    }

    std::vector<int> result;
    ProbabilityTable<int>::lottery(std::back_inserter(result), prob_table, n);

    auto it = std::max_element(result.begin(), result.end(), [](int a, int b) { return alloc_race_table[a].level < alloc_race_table[b].level; });

    return i2enum<MonsterRaceId>(alloc_race_table[*it].index);
}

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *r_ptr = &monraces_info[r_idx];
    auto *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
    MonsterRaceInfo *old_r_ptr = &monraces_info[m_ptr->r_idx];

    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::FRIENDLY) || (r_ptr->flags7 & RF7_CHAMELEON)) {
        return false;
    }

    if (std::abs(r_ptr->level - monraces_info[MonsterRaceId::CHAMELEON_K].level) > 5) {
        return false;
    }

    if (m_ptr->is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0)) {
        return false;
    }

    if (!(old_r_ptr->flags7 & RF7_CHAMELEON)) {
        if (monster_has_hostile_align(player_ptr, m_ptr, 0, 0, r_ptr)) {
            return false;
        }
    } else if (summon_specific_who > 0) {
        if (monster_has_hostile_align(player_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief カメレオンの変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *r_ptr = &monraces_info[r_idx];
    auto *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
    MonsterRaceInfo *old_r_ptr = &monraces_info[m_ptr->r_idx];

    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }
    if (r_ptr->flags2 & RF2_MULTIPLY) {
        return false;
    }
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::FRIENDLY) || (r_ptr->flags7 & RF7_CHAMELEON)) {
        return false;
    }

    if (m_ptr->is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0)) {
        return false;
    }

    if (!(old_r_ptr->flags7 & RF7_CHAMELEON)) {
        if (old_r_ptr->kind_flags.has(MonsterKindType::GOOD) && r_ptr->kind_flags.has_not(MonsterKindType::GOOD)) {
            return false;
        }
        if (old_r_ptr->kind_flags.has(MonsterKindType::EVIL) && r_ptr->kind_flags.has_not(MonsterKindType::EVIL)) {
            return false;
        }
        if (old_r_ptr->kind_flags.has_none_of(alignment_mask)) {
            return false;
        }
    } else if (summon_specific_who > 0) {
        if (monster_has_hostile_align(player_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr)) {
            return false;
        }
    }

    auto hook_pf = get_monster_hook(player_ptr);
    return (*hook_pf)(player_ptr, r_idx);
}

/*!
 * @brief モンスターの変身処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 変身処理を受けるモンスター情報のID
 * @param born 生成時の初変身先指定ならばtrue
 * @param r_idx 旧モンスター種族のID
 */
void choose_new_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool born, MonsterRaceId r_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    MonsterRaceInfo *r_ptr;

    bool old_unique = false;
    if (monraces_info[m_ptr->r_idx].kind_flags.has(MonsterKindType::UNIQUE)) {
        old_unique = true;
    }
    if (old_unique && (r_idx == MonsterRaceId::CHAMELEON)) {
        r_idx = MonsterRaceId::CHAMELEON_K;
    }
    r_ptr = &monraces_info[r_idx];

    const auto old_m_name = monster_desc(player_ptr, m_ptr, 0);

    if (!MonsterRace(r_idx).is_valid()) {
        DEPTH level;

        chameleon_change_m_idx = m_idx;
        if (old_unique) {
            get_mon_num_prep(player_ptr, monster_hook_chameleon_lord, nullptr);
        } else {
            get_mon_num_prep(player_ptr, monster_hook_chameleon, nullptr);
        }

        if (old_unique) {
            level = monraces_info[MonsterRaceId::CHAMELEON_K].level;
        } else if (!floor_ptr->dun_level) {
            level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
        } else {
            level = floor_ptr->dun_level;
        }

        if (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CHAMELEON)) {
            level += 2 + randint1(3);
        }

        r_idx = get_mon_num(player_ptr, 0, level, 0);
        r_ptr = &monraces_info[r_idx];

        chameleon_change_m_idx = 0;
        if (!MonsterRace(r_idx).is_valid()) {
            return;
        }
    }

    m_ptr->r_idx = r_idx;
    m_ptr->ap_r_idx = r_idx;
    update_monster(player_ptr, m_idx, false);
    lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);

    auto old_r_idx = m_ptr->r_idx;
    if (monraces_info[old_r_idx].brightness_flags.has_any_of(ld_mask) || r_ptr->brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRedrawingFlag::MONSTER_LITE);
    }

    if (born) {
        if (r_ptr->kind_flags.has_any_of(alignment_mask)) {
            m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
            if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
                m_ptr->sub_align |= SUB_ALIGN_EVIL;
            }
            if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
                m_ptr->sub_align |= SUB_ALIGN_GOOD;
            }
        }

        return;
    }

    if (m_idx == player_ptr->riding) {
        msg_format(_("突然%sが変身した。", "Suddenly, %s transforms!"), old_m_name.data());
        if (!(r_ptr->flags7 & RF7_RIDING)) {
            if (process_fall_off_horse(player_ptr, 0, true)) {
                const auto m_name = monster_desc(player_ptr, m_ptr, 0);
                msg_print(_("地面に落とされた。", format("You have fallen from %s.", m_name.data())));
            }
        }
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
    if (m_ptr->maxhp < 1) {
        m_ptr->maxhp = 1;
    }
    m_ptr->hp = (long)(m_ptr->hp * m_ptr->max_maxhp) / oldmaxhp;
    m_ptr->dealt_damage = 0;
}

/*!
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param r_ptr モンスター種族の参照ポインタ
 * @return 加速値
 */
byte get_mspeed(FloorType *floor_ptr, MonsterRaceInfo *r_ptr)
{
    auto mspeed = r_ptr->speed;
    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE) && !floor_ptr->inside_arena) {
        /* Allow some small variation per monster */
        int i = speed_to_energy(r_ptr->speed) / (one_in_(4) ? 3 : 10);
        if (i) {
            mspeed += rand_spread(0, i);
        }
    }

    if (mspeed > 199) {
        mspeed = 199;
    }

    return mspeed;
}

/*!
 * @brief 指定したモンスターに隣接しているモンスターの数を返す。
 * / Count number of adjacent monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 隣接数を調べたいモンスターのID
 * @return 隣接しているモンスターの数
 */
int get_monster_crowd_number(FloorType *floor_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    POSITION my = m_ptr->fy;
    POSITION mx = m_ptr->fx;
    int count = 0;
    for (int i = 0; i < 7; i++) {
        int ay = my + ddy_ddd[i];
        int ax = mx + ddx_ddd[i];

        if (!in_bounds(floor_ptr, ay, ax)) {
            continue;
        }
        if (floor_ptr->grid_array[ay][ax].m_idx > 0) {
            count++;
        }
    }

    return count;
}
