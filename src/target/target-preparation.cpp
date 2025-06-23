#include "target/target-preparation.h"
#include "game-option/input-options.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-sorter.h"
#include "target/target-types.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "window/main-window-util.h"
#include <algorithm>
#include <utility>
#include <vector>

/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targeting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    if (!monster.is_valid()) {
        return false;
    }

    if (player_ptr->effects()->hallucination().is_hallucinated()) {
        return false;
    }

    if (!monster.ml) {
        return false;
    }

    if (monster.is_riding()) {
        return true;
    }

    const auto p_pos = player_ptr->get_position();
    if (!projectable(floor, p_pos, monster.get_position())) {
        return false;
    }

    return true;
}

/*
 * Determine if a given location is "interesting"
 */
static bool target_set_accept(PlayerType *player_ptr, const Pos2D &pos)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (!(floor.contains(pos))) {
        return false;
    }

    if (player_ptr->is_located_at(pos)) {
        return true;
    }

    if (player_ptr->effects()->hallucination().is_hallucinated()) {
        return false;
    }

    const auto &grid = floor.get_grid(pos);
    if (grid.has_monster()) {
        auto &monster = floor.m_list[grid.m_idx];
        if (monster.ml) {
            return true;
        }
    }

    for (const auto this_o_idx : grid.o_idx_list) {
        const auto &item = *floor.o_list[this_o_idx];
        if (item.marked.has(OmType::FOUND)) {
            return true;
        }
    }

    if (!grid.is_mark()) {
        return false;
    }

    if (grid.is_object()) {
        return true;
    }

    return grid.get_terrain(TerrainKind::MIMIC).flags.has(TerrainCharacteristics::NOTICE);
}

/*!
 * @brief "interesting" な座標の一覧を返す。
 * @param mode ターゲット選択モード
 * @return "interesting" な座標の一覧
 */
std::vector<Pos2D> target_set_prepare(PlayerType *player_ptr, target_type mode)
{
    POSITION min_hgt, max_hgt, min_wid, max_wid;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto is_killable = any_bits(mode, TARGET_KILL);
    if (is_killable) {
        const auto max_range = AngbandSystem::get_instance().get_max_range();
        min_hgt = std::max((player_ptr->y - max_range), 0);
        max_hgt = std::min((player_ptr->y + max_range), floor.height - 1);
        min_wid = std::max((player_ptr->x - max_range), 0);
        max_wid = std::min((player_ptr->x + max_range), floor.width - 1);
    } else {
        min_hgt = panel_row_min;
        max_hgt = panel_row_max;
        min_wid = panel_col_min;
        max_wid = panel_col_max;
    }

    std::vector<Pos2D> pos_list;
    for (auto y = min_hgt; y <= max_hgt; y++) {
        for (auto x = min_wid; x <= max_wid; x++) {
            const Pos2D pos(y, x);
            if (!target_set_accept(player_ptr, pos)) {
                continue;
            }

            const auto &grid = floor.get_grid(pos);
            if (is_killable && !target_able(player_ptr, grid.m_idx)) {
                continue;
            }

            const auto &monster = floor.m_list[grid.m_idx];
            if (is_killable && !target_pet && monster.is_pet()) {
                continue;
            }

            pos_list.push_back(pos);
        }
    }

    TargetSorter sorter(player_ptr->get_position());
    if (is_killable) {
        std::stable_sort(pos_list.begin(), pos_list.end(), [&sorter](const auto &a, const auto &b) {
            return sorter.compare_distance(a, b);
        });
    } else {
        std::stable_sort(pos_list.begin(), pos_list.end(), [&sorter, &floor](const auto &a, const auto &b) {
            return sorter.compare_importance(floor, a, b);
        });
    }

    if (player_ptr->riding == 0 || !target_pet || (std::ssize(pos_list) <= 1) || !is_killable) {
        return pos_list;
    }

    // 乗っているモンスターがターゲットリストの先頭にならないようにする調整
    std::swap(pos_list[0], pos_list[1]);

    return pos_list;
}

void target_sensing_monsters_prepare(PlayerType *player_ptr, std::vector<MONSTER_IDX> &monster_list)
{
    monster_list.clear();

    // 幻覚時は正常に感知できない
    if (player_ptr->effects()->hallucination().is_hallucinated()) {
        return;
    }

    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[i];
        if (!monster.is_valid() || !monster.ml || monster.is_pet()) {
            continue;
        }

        // 感知魔法/スキルやESPで感知していない擬態モンスターはモンスター一覧に表示しない
        if (monster.is_mimicry() && monster.mflag2.has_none_of({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW }) && monster.mflag.has_not(MonsterTemporaryFlagType::ESP)) {
            continue;
        }

        monster_list.push_back(i);
    }

    auto comp_importance = [&floor = *player_ptr->current_floor_ptr](MONSTER_IDX idx1, MONSTER_IDX idx2) {
        const auto &monster1 = floor.m_list[idx1];
        const auto &monster2 = floor.m_list[idx2];
        const auto &monrace1 = monster1.get_appearance_monrace();
        const auto &monrace2 = monster2.get_appearance_monrace();

        /* Unique monsters first */
        if (monrace1.kind_flags.has(MonsterKindType::UNIQUE) != monrace2.kind_flags.has(MonsterKindType::UNIQUE)) {
            return monrace1.kind_flags.has(MonsterKindType::UNIQUE);
        }

        /* Shadowers first (あやしい影) */
        if (monster1.mflag2.has(MonsterConstantFlagType::KAGE) != monster2.mflag2.has(MonsterConstantFlagType::KAGE)) {
            return monster1.mflag2.has(MonsterConstantFlagType::KAGE);
        }

        /* Unknown monsters first */
        if ((monrace1.r_tkills == 0) != (monrace2.r_tkills == 0)) {
            return monrace1.r_tkills == 0;
        }

        /* Higher level monsters first (if known) */
        if (monrace1.r_tkills && monrace2.r_tkills && monrace1.level != monrace2.level) {
            return monrace1.order_level_strictly(monrace2);
        }

        /* Sort by index if all conditions are same */
        return monster1.ap_r_idx > monster2.ap_r_idx;
    };

    std::sort(monster_list.begin(), monster_list.end(), comp_importance);
}

/*!
 * @brief プレイヤーのペットの一覧を得る
 *
 * プレイヤーのペットのモンスターIDのリストを取得する。
 * リストは以下の通り、重要なペットと考えられる順にソートされる。
 *
 * - 乗馬している
 * - 名前をつけている
 * - ユニークモンスター
 * - LV順（降順）
 * - モンスターID順（昇順）
 *
 * @return ペットのモンスターIDのリスト
 */
std::vector<MONSTER_IDX> target_pets_prepare(PlayerType *player_ptr)
{
    std::vector<MONSTER_IDX> pets;
    const auto &floor = *player_ptr->current_floor_ptr;

    for (short i = 1; i < floor.m_max; ++i) {
        const auto &monster = floor.m_list[i];

        if (monster.is_valid() && monster.is_pet()) {
            pets.push_back(i);
        }
    }

    auto comp_importance = [&floor](MONSTER_IDX idx1, MONSTER_IDX idx2) {
        const auto &monster1 = floor.m_list[idx1];
        const auto &monster2 = floor.m_list[idx2];
        const auto &ap_monrace1 = monster1.get_appearance_monrace();
        const auto &ap_monrace2 = monster2.get_appearance_monrace();

        if (monster1.is_riding() != monster2.is_riding()) {
            return monster1.is_riding();
        }

        if (monster1.is_named_pet() != monster2.is_named_pet()) {
            return monster1.is_named_pet();
        }

        if (ap_monrace1.kind_flags.has(MonsterKindType::UNIQUE) != ap_monrace2.kind_flags.has(MonsterKindType::UNIQUE)) {
            return ap_monrace1.kind_flags.has(MonsterKindType::UNIQUE);
        }

        if (ap_monrace1.r_tkills && ap_monrace2.r_tkills && (ap_monrace1.level != ap_monrace2.level)) {
            return ap_monrace1.order_level_strictly(ap_monrace2);
        }

        return idx1 < idx2;
    };

    std::sort(pets.begin(), pets.end(), comp_importance);

    return pets;
}
