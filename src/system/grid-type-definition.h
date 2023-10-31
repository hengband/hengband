#pragma once

#include "object/object-index-list.h"
#include "system/angband.h"

/*
 * 特殊なマス状態フラグ / Special grid flags
 */
#define CAVE_MARK 0x0001 /*!< 現在プレイヤーの記憶に収まっている / memorized feature */
#define CAVE_GLOW 0x0002 /*!< マス自体が光源を持っている / self-illuminating */
#define CAVE_ICKY 0x0004 /*!< 生成されたVaultの一部である / part of a vault */
#define CAVE_ROOM 0x0008 /*!< 生成された部屋の一部である / part of a room */
#define CAVE_LITE 0x0010 /*!< 現在光に照らされている / lite flag  */
#define CAVE_VIEW 0x0020 /*!< 現在プレイヤーの視界に収まっている / view flag */
#define CAVE_TEMP 0x0040 /*!< 光源に関する処理のアルゴリズム用記録フラグ / temp flag */
#define CAVE_XTRA 0x0080 /*!< 視界に関する処理のアルゴリズム用記録フラグ(update_view()等参照) / misc flag */
#define CAVE_MNLT 0x0100 /*!< モンスターの光源によって照らされている / Illuminated by monster */
#define CAVE_MNDK 0x8000 /*!< モンスターの暗源によって暗闇になっている / Darken by monster */

/* Used only while floor generation */
#define CAVE_FLOOR 0x0200 /*!< フロア属性のあるマス */
#define CAVE_EXTRA 0x0400
#define CAVE_INNER 0x0800
#define CAVE_OUTER 0x1000
#define CAVE_SOLID 0x2000
#define CAVE_VAULT 0x4000
#define CAVE_MASK (CAVE_FLOOR | CAVE_EXTRA | CAVE_INNER | CAVE_OUTER | CAVE_SOLID | CAVE_VAULT)

// clang-format off

/* Used only after floor generation */
#define CAVE_KNOWN     0x0200 /* Directly viewed or map detected flag */
#define CAVE_NOTE      0x0400 /* Flag for delayed visual update (needs note_spot()) */
#define CAVE_REDRAW    0x0800 /* Flag for delayed visual update (needs lite_spot()) */
#define CAVE_OBJECT    0x1000 /* Mirror, rune, etc. */
#define CAVE_UNSAFE    0x2000 /* Might have trap */
#define CAVE_IN_DETECT 0x4000 /* trap detected area (inner circle only) */

// clang-format on

enum flow_type {
    FLOW_NORMAL = 0,
    FLOW_CAN_FLY = 1,
    FLOW_MAX = 2,
};

class MonsterRaceInfo;
enum class TerrainCharacteristics;
struct grid_type {
public:
    BIT_FLAGS info{}; /* Hack -- grid flags */

    FEAT_IDX feat{}; /* Hack -- feature type */
    ObjectIndexList o_idx_list; /* Object list in this grid */
    MONSTER_IDX m_idx{}; /* Monster in this grid */

    /*
     * 地形の特別な情報を保存する / Special grid info
     * 具体的な使用一覧はクエスト行き階段の移行先クエストID、
     * 各ダンジョン入口の移行先ダンジョンID、
     */
    int16_t special{};

    FEAT_IDX mimic{}; /* Feature to mimic */

    byte costs[FLOW_MAX]{}; /* Hack -- cost of flowing */
    byte dists[FLOW_MAX]{}; /* Hack -- distance from player */
    byte when{}; /* Hack -- when cost was computed */

    bool is_floor() const;
    bool is_room() const;
    bool is_extra() const;
    bool is_inner() const;
    bool is_outer() const;
    bool is_solid() const;
    bool is_icky() const;
    bool is_lite() const;
    bool is_redraw() const;
    bool is_view() const;
    bool is_object() const;
    bool is_mark() const;
    bool is_mirror() const;
    bool is_rune_protection() const;
    bool is_rune_explosion() const;
    byte get_cost(const MonsterRaceInfo *r_ptr) const;
    byte get_distance(const MonsterRaceInfo *r_ptr) const;
    FEAT_IDX get_feat_mimic() const;
    bool cave_has_flag(TerrainCharacteristics feature_flags) const;
    bool is_symbol(const int ch) const;
    void reset_costs();
    void reset_dists();
    bool has_los() const;

private:
    flow_type get_grid_flow_type(const MonsterRaceInfo *r_ptr) const;
};
