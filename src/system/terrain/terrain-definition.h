#pragma once

#include "system/angband.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "util/flag-group.h"
#include "view/display-symbol.h"
#include <map>
#include <optional>
#include <string_view>

/* Number of feats we change to (Excluding default). Used in TerrainDefinitions.txt. */
constexpr auto MAX_FEAT_STATES = 8;

/* Lighting levels of features' attr and char */
constexpr auto F_LIT_STANDARD = 0; /* Standard */
constexpr auto F_LIT_LITE = 1; /* Brightly lit */
constexpr auto F_LIT_DARK = 2; /* Darkened */
constexpr auto F_LIT_MAX = 3;
constexpr auto F_LIT_NS_BEGIN = 1; /* Nonstandard */

const std::map<int, DisplaySymbol> DEFAULT_SYMBOLS = { { F_LIT_STANDARD, {} }, { F_LIT_LITE, {} }, { F_LIT_DARK, {} } };

enum class TerrainAction {
    DESTROY = 1,
    NO_DROP = 2,
    CRASH_GLASS = 3,
    MAX,
};

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
    TerrainType();
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
    std::map<int, DisplaySymbol> symbol_definitions; //!< デフォルトの地形シンボル (色/文字).
    std::map<int, DisplaySymbol> symbol_configs; //!< 設定変更後の地形シンボル (色/文字).

    static bool has(TerrainCharacteristics tc, TerrainAction ta);

    bool is_permanent_wall() const;
    bool is_open() const;
    bool is_closed_door() const;
    bool is_trap() const;

    void reset_lighting(bool is_config = true);

private:
    void reset_lighting_ascii(std::map<int, DisplaySymbol> &symbols);
    void reset_lighting_graphics(std::map<int, DisplaySymbol> &symbols);
};

class TerrainList {
public:
    TerrainList(const TerrainList &) = delete;
    TerrainList(TerrainList &&) = delete;
    TerrainList operator=(const TerrainList &) = delete;
    TerrainList operator=(TerrainList &&) = delete;

    static TerrainList &get_instance();
    TerrainType &get_terrain(short terrain_id);
    const TerrainType &get_terrain(short terrain_id) const;
    short get_terrain_id_by_tag(std::string_view tag) const;
    std::vector<TerrainType>::iterator begin();
    std::vector<TerrainType>::const_iterator begin() const;
    std::vector<TerrainType>::reverse_iterator rbegin();
    std::vector<TerrainType>::const_reverse_iterator rbegin() const;
    std::vector<TerrainType>::iterator end();
    std::vector<TerrainType>::const_iterator end() const;
    std::vector<TerrainType>::reverse_iterator rend();
    std::vector<TerrainType>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void resize(size_t new_size);
    void shrink_to_fit();

    void retouch();

private:
    TerrainList() = default;

    static TerrainList instance;
    std::vector<TerrainType> terrains{};

    std::optional<short> search_real_terrain(std::string_view tag) const;
};
