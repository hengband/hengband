#pragma once

#include "dungeon/quest.h"
#include "floor/floor-base-definitions.h"
#include "monster/monster-timed-effect-types.h"
#include "system/angband.h"
#include <array>
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

struct dungeon_type;
struct grid_type;
class MonsterEntity;
class ItemEntity;
class FloorType {
public:
    FloorType() = default;
    short dungeon_idx = 0;
    std::vector<std::vector<grid_type>> grid_array;
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

    std::vector<int16_t> mproc_list[MAX_MTIMED]{}; /*!< The array to process dungeon monsters[max_m_idx] */
    int16_t mproc_max[MAX_MTIMED]{}; /*!< Number of monsters to be processed */

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
    QuestId quest_number = QuestId::NONE; /* Inside quest level */
    bool inside_arena = false; /* Is character inside on_defeat_arena_monster? */

    bool is_in_dungeon() const;
    void set_dungeon_index(short dungeon_idx_); /*!< @todo 後でenum class にする */
    void reset_dungeon_index();
    dungeon_type &get_dungeon_definition() const;
};
