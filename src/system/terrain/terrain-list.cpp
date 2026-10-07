/*!
 * @brief 地形特性の集合論的処理実装
 * @author Hourier
 * @date 2024/12/08
 */

#include "system/terrain/terrain-list.h"
#include "info-reader/feature-info-tokens-table.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/terrain/terrain-definition.h"
#include "util/enum-converter.h"
#include <algorithm>

TerrainList TerrainList::instance{};

// 暗黙のデストラクタだと、TerrainType が不完全な型のまま std::vector<TerrainType> のデストラクタを
// 実体化しようとするコンパイラがあるので、TerrainType が完全な型になるここで定義する
TerrainList::~TerrainList() = default;

TerrainList::TerrainList()
{
    this->normal_traps = {
        TerrainTag::TRAP_TRAPDOOR,
        TerrainTag::TRAP_PIT,
        TerrainTag::TRAP_SPIKED_PIT,
        TerrainTag::TRAP_POISON_PIT,
        TerrainTag::TRAP_TY_CURSE,
        TerrainTag::TRAP_TELEPORT,
        TerrainTag::TRAP_FIRE,
        TerrainTag::TRAP_ACID,
        TerrainTag::TRAP_SLOW,
        TerrainTag::TRAP_LOSE_STR,
        TerrainTag::TRAP_LOSE_DEX,
        TerrainTag::TRAP_LOSE_CON,
        TerrainTag::TRAP_BLIND,
        TerrainTag::TRAP_CONFUSE,
        TerrainTag::TRAP_POISON,
        TerrainTag::TRAP_SLEEP,
        TerrainTag::TRAP_TRAPS,
        TerrainTag::TRAP_ALARM,
    };
}

TerrainList &TerrainList::get_instance()
{
    return instance;
}

TerrainType &TerrainList::get_terrain(short terrain_id)
{
    return this->terrains.at(terrain_id);
}

const TerrainType &TerrainList::get_terrain(short terrain_id) const
{
    return this->terrains.at(terrain_id);
}

TerrainType &TerrainList::get_terrain(TerrainTag tag)
{
    return this->terrains.at(this->get_terrain_id(tag));
}

const TerrainType &TerrainList::get_terrain(TerrainTag tag) const
{
    return this->terrains.at(this->get_terrain_id(tag));
}

/*!
 * @brief 地形タグから地形IDを得る
 * @param tag 地形タグ
 * @throw std::out_of_range 地形が対応付けられていないタグが指定された
 * @return 地形タグに対応するID
 */
short TerrainList::get_terrain_id(TerrainTag tag) const
{
    const auto tag_value = enum2i(tag);
    const auto index = static_cast<size_t>(tag_value);
    if ((index >= this->tags.size()) || (this->tags[index] == UNASSIGNED_TERRAIN_ID)) {
        THROW_EXCEPTION(std::out_of_range, format("Terrain tag %d is not assigned to any terrain.", tag_value));
    }

    return this->tags[index];
}

/*!
 * @brief 地形タグからIDを得る
 * @param tag タグ文字列
 * @throw std::runtime_error 未定義のタグが指定された
 * @return 地形タグに対応するID
 */
short TerrainList::get_terrain_id(std::string_view tag) const
{
    const auto it = std::find_if(this->terrains.begin(), this->terrains.end(),
        [tag](const auto &terrain) {
            return terrain.tag == tag;
        });
    if (it == this->terrains.end()) {
        THROW_EXCEPTION(std::runtime_error, fmt::format(_("未定義のタグ '{}'。", "{} is undefined."), tag));
    }

    return static_cast<short>(std::distance(this->terrains.begin(), it));
}

TerrainTag TerrainList::select_normal_trap() const
{
    return rand_choice(this->normal_traps);
}

/*!
 * @brief 地形情報の各種タグからIDへ変換して結果を収める
 */
void TerrainList::retouch()
{
    for (auto &terrain : this->terrains) {
        terrain.mimic = this->search_real_terrain(terrain.mimic_tag).value_or(terrain.mimic);
        terrain.destroyed = this->search_real_terrain(terrain.destroyed_tag).value_or(terrain.destroyed);
        for (auto &ts : terrain.state) {
            ts.result = this->search_real_terrain(ts.result_tag).value_or(ts.result);
        }
        for (auto &change : terrain.generation_changes) {
            change.result = this->search_real_terrain(change.result_tag).value_or(change.result);
        }
    }
}

void TerrainList::emplace_tags()
{
    for (const auto &[tag_str, tag] : terrain_tags) {
        this->set_terrain_id(tag, this->get_terrain_id(tag_str));
    }
}

/*!
 * @brief 地形タグに地形IDを対応付ける
 * @param tag 地形タグ
 * @param terrain_id 対応付ける地形ID
 */
void TerrainList::set_terrain_id(TerrainTag tag, short terrain_id)
{
    const auto index = static_cast<size_t>(enum2i(tag));
    if (index >= this->tags.size()) {
        this->tags.resize(index + 1, UNASSIGNED_TERRAIN_ID);
    }

    this->tags[index] = terrain_id;
}

/*!
 * @brief 地形タグからIDを得る
 * @param tag タグ文字列のオフセット
 * @return 地形ID。該当がないならtl::nullopt
 */
tl::optional<short> TerrainList::search_real_terrain(std::string_view tag) const
{
    if (tag.empty()) {
        return tl::nullopt;
    }

    return this->get_terrain_id(tag);
}
