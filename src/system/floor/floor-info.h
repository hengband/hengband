#pragma once

#include "floor/floor-base-definitions.h"
#include "floor/geometry.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/enums/terrain/terrain-kind.h"
#include "util/point-2d.h"
#include <array>
#include <map>
#include <tl/optional.hpp>
#include <utility>
#include <vector>

/*!
 * @brief プレイヤー用光源処理配列サイズ / Max array size of player's lite
 * @details 光源の最大半径は14,実グリッド数では581である.
 */
constexpr auto LITE_MAX = 600;

/*!
 * @brief モンスター用光源処理配列サイズ / Max array size of monster's lite
 * @details 視界の最大半径は20、実際は8角形で1520グリッドである.
 * モンスターの可視範囲はCAVE_VIEWフラグに依存する.
 */
constexpr auto MON_LITE_MAX = 1536;

/*!
 * @brief 視界処理配列サイズ / Max array size of the "view"
 * @details 視界の最大半径は20、実際は8角形で1520グリッドである.
 */
constexpr auto VIEW_MAX = 1536;

/*!
 * @brief 再描画処理用配列サイズ / Max array size of the "redraw"
 * @details 遅延再描画を適切に機能させるため、最大ビュー領域の 2倍の大きさにする.
 * ビューグリッド数の最大値は1149なのでその2倍とする.
 */
constexpr auto REDRAW_MAX = 2298;

enum class FloorBoundary {
    OUTER_WALL_EXCLUSIVE,
    OUTER_WALL_INCLUSIVE,
};

enum class DungeonId;
enum class GridCountKind;
enum class MonsterTimedEffect : int;
enum class MonraceHook;
enum class MonraceHookTerrain;
enum class MonraceId : short;
enum class QuestId : short;
enum class TerrainCharacteristics;
enum class TerrainTag;
class DungeonDefinition;
class Grid;
class MonsterEntity;
class ItemEntity;
class FloorType {
public:
    FloorType();
    DungeonId dungeon_id{};
    std::vector<std::vector<Grid>> grid_array;
    DEPTH dun_level = 0; /*!< 現在の実ダンジョン階層 base_level の参照元となる / Current dungeon level */
    DEPTH base_level = 0; /*!< 基本生成レベル、後述のobject_level, monster_levelの参照元となる / Base dungeon level */
    DEPTH object_level = 0; /*!< アイテムの生成レベル、 base_level を起点に一時変更する時に参照 / Current object creation level */
    DEPTH monster_level = 0; /*!< モンスターの生成レベル、 base_level を起点に一時変更する時に参照 / Current monster creation level */
    POSITION width = 0; /*!< Current dungeon width */
    POSITION height = 0; /*!< Current dungeon height */
    MONSTER_NUMBER num_repro = 0; /*!< Current reproducer count */

    GAME_TURN generated_turn = 0; /* Turn when level began */

    std::vector<std::shared_ptr<ItemEntity>> o_list; /*!< The array of dungeon items [max_o_idx] */
    bool prevent_repeat_floor_item_idx = false;

    std::vector<MonsterEntity> m_list; /*!< The array of dungeon monsters [max_m_idx] */
    MONSTER_IDX m_max = 0; /* Number of allocated monsters */
    MONSTER_IDX m_cnt = 0; /* Number of live monsters */

    std::map<MonsterTimedEffect, std::vector<short>> mproc_list; /*!< The array to process dungeon monsters[max_m_idx] */
    std::map<MonsterTimedEffect, short> mproc_max; /*!< Number of monsters to be processed */

    POSITION_IDX lite_n = 0; //!< Array of grids lit by player lite
    std::array<POSITION, LITE_MAX> lite_y{};
    std::array<POSITION, LITE_MAX> lite_x{};

    POSITION_IDX mon_lite_n = 0; //!< Array of grids lit by player lite
    std::array<POSITION, MON_LITE_MAX> mon_lite_y{};
    std::array<POSITION, MON_LITE_MAX> mon_lite_x{};

    POSITION_IDX view_n = 0; //!< Array of grids viewable to the player
    std::array<POSITION, VIEW_MAX> view_y{};
    std::array<POSITION, VIEW_MAX> view_x{};

    POSITION_IDX redraw_n = 0; //!< Array of grids for delayed visual updating
    std::array<POSITION, REDRAW_MAX> redraw_y{};
    std::array<POSITION, REDRAW_MAX> redraw_x{};

    bool monster_noise = false;
    QuestId quest_number;
    bool inside_arena = false; /* Is character inside on_defeat_arena_monster? */

    int get_level() const;
    Grid &get_grid(const Pos2D pos);
    const Grid &get_grid(const Pos2D pos) const;
    Rect2D get_area(FloorBoundary fb = FloorBoundary::OUTER_WALL_INCLUSIVE) const;
    bool is_entering_dungeon() const;
    bool is_leaving_dungeon() const;
    bool is_underground() const;
    bool is_in_quest() const;
    void set_dungeon_index(DungeonId id);
    void reset_dungeon_index();
    const DungeonDefinition &get_dungeon_definition() const; //!< @details 定義データなので非const 版の使用は禁止.
    QuestId get_random_quest_id(tl::optional<int> level_opt = tl::nullopt) const;
    QuestId get_quest_id(const int bonus = 0) const;
    bool has_los_at(const Pos2D &pos) const;
    bool has_los_terrain_at(const Pos2D &pos) const;
    bool has_terrain_characteristics(const Pos2D &pos, TerrainCharacteristics tc) const;
    bool is_special() const;
    bool can_teleport_level(bool to_player = false) const;
    bool has_marked_grid_at(const Pos2D &pos) const;
    bool has_closed_door_at(const Pos2D &pos, bool is_mimic = false) const;
    bool has_trap_at(const Pos2D &pos) const;
    std::pair<int, Direction> count_doors_traps(const Pos2D &p_pos, GridCountKind gck, bool under) const;
    bool check_terrain_state(const Pos2D &pos, GridCountKind gck) const;
    bool order_pet_whistle(short index1, short index2) const;
    bool order_pet_dismission(short index1, short index2, short riding_index) const;
    bool contains(const Pos2D &pos, FloorBoundary fb = FloorBoundary::OUTER_WALL_EXCLUSIVE) const;
    bool is_empty_at(const Pos2D &pos) const;
    bool can_generate_monster_at(const Pos2D &pos) const;
    bool can_block_disintegration_at(const Pos2D &pos) const;
    bool can_drop_item_at(const Pos2D &pos) const;
    bool is_illuminated_at(const Pos2D &p_pos, const Pos2D &pos) const;

    ItemEntity make_gold(tl::optional<BaseitemKey> bi_key = tl::nullopt) const;
    tl::optional<ItemEntity> try_make_instant_artifact() const;
    short select_baseitem_id(int level_initial, uint32_t mode) const;
    bool filter_monrace_terrain(MonraceId monrace_id, MonraceHookTerrain hook) const;
    TerrainTag select_random_trap() const;
    MonraceHook get_monrace_hook() const;
    MonraceHookTerrain get_monrace_hook_terrain_at(const Pos2D &pos) const;

    void enter_dungeon(bool state);
    void leave_dungeon(bool state);
    void reset_mproc();
    void reset_mproc_max();
    tl::optional<int> get_mproc_index(short m_idx, MonsterTimedEffect mte);
    void add_mproc(short m_idx, MonsterTimedEffect mte);
    void remove_mproc(short m_idx, MonsterTimedEffect mte);

    short pop_empty_index_monster();
    short pop_empty_index_item();
    void forget_lite();
    void forget_view();
    void forget_mon_lite();
    void reset_lite_area();
    void set_lite_at(const Pos2D &pos);
    bool is_grid_changeable(const Pos2D &pos) const;
    void place_random_stairs(const Pos2D &pos);
    void set_terrain_id_at(const Pos2D &pos, TerrainTag tag, TerrainKind tk = TerrainKind::NORMAL);
    void set_terrain_id_at(const Pos2D &pos, short terrain_id, TerrainKind tk = TerrainKind::NORMAL);
    void place_trap_at(const Pos2D &pos);

private:
    static int decide_selection_count();

    bool entering_dungeon = false;
    bool leaving_dungeon = false;
};
