#pragma once

#include "grid/feature-flag-types.h"
#include "system/angband.h"
#include "util/flag-group.h"

/* Number of feats we change to (Excluding default). Used in TerrainDefinitions.txt. */
constexpr auto MAX_FEAT_STATES = 8;

constexpr auto F_LIT_MAX = 3;

/*!
 * @brief 地形状態変化指定構造体
 */
class TerrainState {
public:
    TerrainState() = default;
    TerrainCharacteristics action{}; /*!< 変化条件をFF_*のIDで指定 */
    std::string result_tag{}; /*!< 変化先ID */
    FEAT_IDX result{}; /*!< 変化先ID */
};

/*!
 * @brief 地形情報の構造体
 */
class TerrainType {
public:
    TerrainType() = default;
    FEAT_IDX idx{};
    std::string name; /*!< 地形名 */
    std::string text; /*!< 地形説明 */
    std::string tag; /*!< 地形特性タグ */
    std::string mimic_tag;
    std::string destroyed_tag;
    FEAT_IDX mimic{}; /*!< 未確定時の外形地形ID / Feature to mimic */
    FEAT_IDX destroyed{}; /*!< *破壊*に巻き込まれた時の地形移行先(未実装？) / Default destroyed state */
    EnumClassFlagGroup<TerrainCharacteristics> flags{}; /*!< 地形の基本特性ビット配列 / Flags */
    int16_t priority{}; /*!< 縮小表示で省略する際の表示優先度 / Map priority */
    TerrainState state[MAX_FEAT_STATES]{}; /*!< TerrainState テーブル */
    FEAT_SUBTYPE subtype{}; /*!< 副特性値 */
    FEAT_POWER power{}; /*!< 地形強度 */
    TERM_COLOR d_attr[F_LIT_MAX]{}; /*!< デフォルトの地形シンボルカラー / Default feature attribute */
    char d_char[F_LIT_MAX]{}; /*!< デフォルトの地形シンボルアルファベット / Default feature character */
    TERM_COLOR x_attr[F_LIT_MAX]{}; /*!< 設定変更後の地形シンボルカラー / Desired feature attribute */
    char x_char[F_LIT_MAX]{}; /*!< 設定変更後の地形シンボルアルファベット / Desired feature character */

    bool is_permanent_wall() const;
};

class TerrainList {
public:
    TerrainList(const TerrainList &) = delete;
    TerrainList(TerrainList &&) = delete;
    TerrainList operator=(const TerrainList &) = delete;
    TerrainList operator=(TerrainList &&) = delete;
    TerrainType &operator[](short terrain_id);
    const TerrainType &operator[](short terrain_id) const;

    static TerrainList &get_instance();
    std::vector<TerrainType> &get_raw_vector();
    std::vector<TerrainType>::iterator begin();
    const std::vector<TerrainType>::const_iterator begin() const;
    std::vector<TerrainType>::iterator end();
    const std::vector<TerrainType>::const_iterator end() const;
    size_t size() const;
    bool empty() const;
    void resize(size_t new_size);

private:
    TerrainList() = default;

    static TerrainList instance;
    std::vector<TerrainType> terrains{};
};
