#include "floor/fixed-map-generator.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "floor/floor-object.h"
#include "grid/object-placer.h"
#include "info-reader/general-parser.h"
#include "info-reader/legend-ego-validator.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/angband-system.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "window/main-window-util.h"
#include <algorithm>
#include <cassert>

qtwg_type *initialize_quest_generator_type(qtwg_type *qtwg_ptr, int ymin, int xmin, int ymax, int xmax, int *y, int *x)
{
    qtwg_ptr->ymin = ymin;
    qtwg_ptr->xmin = xmin;
    qtwg_ptr->ymax = ymax;
    qtwg_ptr->xmax = xmax;
    qtwg_ptr->y = y;
    qtwg_ptr->x = x;
    return qtwg_ptr;
}

/*!
 * @brief フロアの所定のマスにオブジェクトを配置する
 * Place the object j_ptr to a grid
 * @param floor 現在フロアへの参照
 * @param item アイテムの参照
 * @param y 配置先Y座標
 * @param x 配置先X座標
 * @return エラーコード
 */
static void drop_here(FloorType &floor, ItemEntity &&item, POSITION y, POSITION x)
{
    const auto item_idx = floor.pop_empty_index_item();
    auto &dropped_item = *floor.o_list[item_idx];
    dropped_item = std::move(item);
    dropped_item.iy = y;
    dropped_item.ix = x;
    dropped_item.held_m_idx = 0;
    auto &grid = floor.grid_array[y][x];
    grid.o_idx_list.add(floor, item_idx);
}

static void generate_artifact(PlayerType *player_ptr, qtwg_type *qtwg_ptr, const FixedArtifactId fa_id)
{
    if (fa_id == FixedArtifactId::NONE) {
        return;
    }

    if (!ArtifactRecords::get_instance().get_generated(fa_id) && create_named_art(player_ptr, fa_id, *qtwg_ptr->y, *qtwg_ptr->x)) {
        return;
    }

    ItemEntity item({ ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT });
    drop_here(*player_ptr->current_floor_ptr, std::move(item), *qtwg_ptr->y, *qtwg_ptr->x);
}

static void parse_qtw_D(PlayerType *player_ptr, qtwg_type *qtwg_ptr, std::string_view row)
{
    *qtwg_ptr->x = qtwg_ptr->xmin;
    auto &floor = *player_ptr->current_floor_ptr;
    const auto len = row.size();
    auto &monraces = MonraceList::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    for (size_t i = 0; ((*qtwg_ptr->x < qtwg_ptr->xmax) && (i < len)); (*qtwg_ptr->x)++, i++) {
        auto &grid = floor.grid_array[*qtwg_ptr->y][*qtwg_ptr->x];
        const auto idx = static_cast<unsigned char>(row[i]);
        const auto &cell = fixed_map_letter_at(idx);
        const auto item_index = cell.object;
        auto monster_index = cell.monster;
        const auto random = cell.random;
        grid.feat = dungeon.convert_terrain_id(cell.feature);
        if (init_flags & INIT_ONLY_FEATURES) {
            continue;
        }

        grid.info = cell.cave_info;
        if (random & RANDOM_MONSTER) {
            floor.monster_level = floor.base_level + monster_index;
            place_random_monster(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_NO_QUEST));
            floor.monster_level = floor.base_level;
        } else if (monster_index) {
            auto clone = false;
            if (monster_index < 0) {
                monster_index = -monster_index;
                clone = true;
            }

            const auto monrace_id = i2enum<MonraceId>(monster_index);
            auto &monrace = monraces.get_monrace(monrace_id);
            const auto old_cur_num = monrace.cur_num;
            const auto old_max_num = monrace.max_num;
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
                monrace.reset_current_numbers();
                monrace.max_num = MAX_UNIQUE_NUM;
            } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
                if (monrace.cur_num == monrace.max_num) {
                    monrace.max_num++;
                }
            }

            const auto m_idx = place_specific_monster(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, monrace_id, (PM_ALLOW_SLEEP | PM_NO_KAGE));
            if (clone && m_idx) {
                floor.m_list[*m_idx].mflag2.set(MonsterConstantFlagType::CLONED);
                monrace.cur_num = old_cur_num;
                monrace.max_num = old_max_num;
            }
        }

        if ((random & RANDOM_OBJECT) && (random & RANDOM_TRAP)) {
            floor.object_level = floor.base_level + item_index;

            /*
             * Random trap and random treasure defined
             * 25% chance for trap and 75% chance for object
             */
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            if (evaluate_percent(75)) {
                place_object(player_ptr, pos, 0);
            } else {
                floor.place_trap_at(pos);
            }

            floor.object_level = floor.base_level;
        } else if (random & RANDOM_OBJECT) {
            floor.object_level = floor.base_level + item_index;
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            if (evaluate_percent(75)) {
                place_object(player_ptr, pos, 0);
            } else if (evaluate_percent(80)) {
                place_object(player_ptr, pos, AM_GOOD);
            } else {
                place_object(player_ptr, pos, AM_GOOD | AM_GREAT);
            }

            floor.object_level = floor.base_level;
        } else if (random & RANDOM_TRAP) {
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            floor.place_trap_at(pos);
        } else if (cell.trap) {
            grid.mimic = grid.feat;
            grid.feat = dungeon.convert_terrain_id(cell.trap);
        } else if (item_index) {
            ItemEntity item(item_index);
            if (item.bi_key.tval() == ItemKindType::GOLD) {
                item = floor.make_gold(item.bi_key);
            }

            assert(validate_legend_ego(cell, false, cell.trap != 0 || (random & RANDOM_TRAP)).has_value());
            if (cell.ego != EgoType::NONE) {
                ItemMagicApplier(player_ptr, &item, floor.base_level, AM_NO_FIXED_ART | AM_GOOD).execute_with_ego(cell.ego);
            } else if (random & RANDOM_EGO) {
                ItemMagicApplier(player_ptr, &item, floor.base_level, AM_NO_FIXED_ART | AM_GOOD).execute_with_random_ego();
            } else {
                ItemMagicApplier(player_ptr, &item, floor.base_level, AM_NO_FIXED_ART | AM_GOOD).execute();
            }
            drop_here(floor, std::move(item), *qtwg_ptr->y, *qtwg_ptr->x);
        }

        generate_artifact(player_ptr, qtwg_ptr, cell.artifact);
        grid.special = cell.special;
    }
}

/*!
 * @brief 印字可能なASCII行を適用し、行カーソルを進める。不正な記号やNULがある行は変更せずエラーを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param qtwg_ptr 固定マップの生成範囲と、次に書く行・列のカーソル
 * @param row 適用する1行分の記号の並び
 * @return 成功時は空の値、失敗時は解析エラーコード (固定マップで使えない記号があれば ParseErrorType::INVALID_VALUE)
 * @details 建物だけを読み込むとき (init_flags に INIT_ONLY_BUILDINGS が立っているとき) は、行の記号を検証せず、
 * 行カーソルも進めずに成功を返す。
 */
tl::expected<void, ParseErrorType> apply_fixed_map_row(PlayerType *player_ptr, qtwg_type *qtwg_ptr, std::string_view row)
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return {};
    }

    if (!std::all_of(row.begin(), row.end(), is_fixed_map_symbol)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    parse_qtw_D(player_ptr, qtwg_ptr, row);
    (*qtwg_ptr->y)++;
    return {};
}

static void apply_fixed_map_size(PlayerType *player_ptr, qtwg_type *qtwg_ptr)
{
    int panels_y = (*qtwg_ptr->y / SCREEN_HGT);
    if (*qtwg_ptr->y % SCREEN_HGT) {
        panels_y++;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    floor.height = panels_y * SCREEN_HGT;
    int panels_x = (*qtwg_ptr->x / SCREEN_WID);
    if (*qtwg_ptr->x % SCREEN_WID) {
        panels_x++;
    }

    floor.width = panels_x * SCREEN_WID;
    panel_row_min = floor.height;
    panel_col_min = floor.width;
}

/*!
 * @brief 生成済みの行・列からパネルサイズを設定し、開始位置を適用する.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param qtwg_ptr 固定マップの生成範囲と、生成し終えた行・列のカーソル
 * @param start_y 開始位置の y 座標 (フロアの座標。固定マップの1行目を 0 とする)
 * @param start_x 開始位置の x 座標 (フロアの座標。固定マップの1列目を 0 とする)
 * @details クエストの中ではプレイヤーをこの位置に置き、それ以外 (町など) では前の位置が無いときに戻り先として使う。
 */
void apply_fixed_map_start(PlayerType *player_ptr, qtwg_type *qtwg_ptr, int start_y, int start_x)
{
    if ((init_flags & INIT_CREATE_DUNGEON) == 0) {
        return;
    }

    apply_fixed_map_size(player_ptr, qtwg_ptr);
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_in_quest()) {
        player_ptr->set_position(Pos2D(start_y, start_x));
        delete_monster(player_ptr, player_ptr->get_position());
        return;
    }

    if (!player_ptr->oldpx && !player_ptr->oldpy) {
        player_ptr->oldpy = start_y;
        player_ptr->oldpx = start_x;
    }
}

/*!
 * @brief startVariants から現在の退出クエストに合う開始位置を選ぶ
 */
static const QuestStartPosition *select_quest_start(const QuestFixedMap &fixed_map)
{
    if (fixed_map.starts.empty()) {
        return nullptr;
    }

    const auto current_leaving = enum2i(leaving_quest);
    const QuestStartPosition *fallback = nullptr;
    for (const auto &start : fixed_map.starts) {
        if (!start.leaving_quest) {
            fallback = &start;
            continue;
        }

        if (*start.leaving_quest == current_leaving) {
            return &start;
        }
    }

    // 条件付きのみで一致が無い場合は配置しない (旧挙動: 該当 ?: が無ければ P: を実行しない)
    return fallback;
}

/*!
 * @brief JSONCから読み込んだ固定クエストのレイアウトでフロアを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param quest 対象クエスト (報酬解決のため参照)
 * @param fixed_map 生成に用いるレイアウト情報
 * @return 成功時は空の値、失敗時は解析エラーコード (凡例やマップに固定マップで使えない記号があれば ParseErrorType::INVALID_VALUE)
 */
tl::expected<void, ParseErrorType> generate_quest_floor_from_json(PlayerType *player_ptr, QuestType &quest, const QuestFixedMap &fixed_map)
{
    // 型付きレイアウトを直接渡す場合も、凡例やセルの書き込み前に記号を検証する。
    const auto &base_legend = QuestFixedMapList::get_instance().get_base_legend();
    const auto valid_legend = [](const auto &legend) {
        return std::all_of(legend.begin(), legend.end(), [](const auto &entry) {
            return is_fixed_map_symbol(static_cast<unsigned char>(entry.first));
        });
    };
    if (!valid_legend(base_legend) || !valid_legend(fixed_map.legend)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    const auto variant_count = fixed_map.maps.size();
    const auto variant_index = (variant_count <= 1) ? 0 : (AngbandSystem::get_instance().get_seed_town() % variant_count);
    if (fixed_map.has_map()) {
        for (const auto &row : fixed_map.maps[variant_index]) {
            if (!std::all_of(row.begin(), row.end(), is_fixed_map_symbol)) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
        }
    }

    // 1. 共通ベース凡例 (X/./%/D/< など) を letter[] へ適用 (旧 QuestPreferences.txt、起動時に読込済み)
    for (const auto &[symbol, cell] : base_legend) {
        fixed_map_letter_at(static_cast<unsigned char>(symbol)) = cell.grid;
    }

    // 2. このクエスト固有の凡例を letter[] に上書き (報酬は受託時 resolve_quest_reward で解決済み)
    for (const auto &[symbol, cell] : fixed_map.legend) {
        auto grid = cell.grid;
        if (cell.object_is_quest_reward && quest.has_reward() && !quest.is_reward_instant_artifact()) {
            grid.object = quest.get_reward_bi_id();
        }
        if (cell.artifact_is_quest_reward) {
            grid.artifact = quest.get_reward().value_or(FixedArtifactId::NONE);
        }

        fixed_map_letter_at(static_cast<unsigned char>(symbol)) = grid;
    }

    if (!fixed_map.has_map()) {
        return {};
    }

    // 3. マップバリアント選択: 旧 $RANDOMn は seed_town % n で決まる (乱数ではなくキャラ毎に固定)
    const auto &rows = fixed_map.maps[variant_index];

    // 4. 既存のセル配置ロジックでグリッドを敷く
    auto y = 0;
    auto x = 0;
    qtwg_type qg;
    initialize_quest_generator_type(&qg, 0, 0, MAX_HGT, MAX_WID, &y, &x);
    for (const auto &row : rows) {
        parse_qtw_D(player_ptr, &qg, row);
        (*qg.y)++;
    }

    // 5. 開始位置とフロアサイズを型付きの値から適用する
    if (const auto *start = select_quest_start(fixed_map); start != nullptr) {
        apply_fixed_map_start(player_ptr, &qg, start->y, start->x);
    }

    return {};
}
