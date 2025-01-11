#pragma once

#include "floor/floor-base-definitions.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "util/point-2d.h"
#include <array>
#include <map>
#include <optional>
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

enum class DungeonId;
enum class GridCountKind;
enum class MonsterTimedEffect : int;
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

    std::vector<ItemEntity> o_list; /*!< The array of dungeon items [max_o_idx] */
    OBJECT_IDX o_max = 0; /* Number of allocated objects */
    OBJECT_IDX o_cnt = 0; /* Number of live objects */

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

    Grid &get_grid(const Pos2D pos);
    const Grid &get_grid(const Pos2D pos) const;
    bool is_underground() const;
    bool is_in_quest() const;
    void set_dungeon_index(DungeonId id);
    void reset_dungeon_index();
    DungeonDefinition &get_dungeon_definition() const;
    QuestId get_random_quest_id(std::optional<int> level_opt = std::nullopt) const;
    QuestId get_quest_id(const int bonus = 0) const;
    bool has_los(const Pos2D &pos) const;
    bool has_terrain_characteristics(const Pos2D &pos, TerrainCharacteristics tc) const;
    bool is_special() const;
    bool can_teleport_level(bool to_player = false) const;
    bool is_mark(const Pos2D &pos) const;
    bool is_closed_door(const Pos2D &pos, bool is_mimic = false) const;
    bool is_trap(const Pos2D &pos) const;
    std::pair<int, Pos2D> count_doors_traps(const Pos2D &p_pos, GridCountKind gck, bool under) const;
    bool check_terrain_state(const Pos2D &pos, GridCountKind gck) const;
    bool order_pet_whistle(short index1, short index2) const;
    bool order_pet_dismission(short index1, short index2, short riding_index) const;

    ItemEntity make_gold(std::optional<BaseitemKey> bi_key = std::nullopt) const;
    std::optional<ItemEntity> try_make_instant_artifact() const;
    short select_baseitem_id(int level_initial, uint32_t mode) const;

    void reset_mproc();
    void reset_mproc_max();
    std::optional<int> get_mproc_index(short m_idx, MonsterTimedEffect mte);
    void add_mproc(short m_idx, MonsterTimedEffect mte);
    void remove_mproc(short m_idx, MonsterTimedEffect mte);

    short pop_empty_index_monster();
    short pop_empty_index_item();
    bool is_grid_changeable(const Pos2D &pos) const;
    void place_random_stairs(const Pos2D &pos);
    void set_terrain_id(const Pos2D &pos, TerrainTag tag);
    void set_terrain_id(const Pos2D &pos, short terrain_id);

private:
    static int decide_selection_count();
};
