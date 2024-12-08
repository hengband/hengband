/*
 * @brief 地形特性定義
 * @author Hourier
 * @date 2022/10/15
 */

#include "system/terrain-type-definition.h"
#include "grid/lighting-colors-table.h"
#include <algorithm>

TerrainType::TerrainType()
    : symbol_definitions(DEFAULT_SYMBOLS)
    , symbol_configs(DEFAULT_SYMBOLS)
{
}

bool TerrainType::is_permanent_wall() const
{
    return this->flags.has_all_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::PERMANENT });
}

/*!
 * @brief ドアやカーテンが開いているかを調べる
 * @return 開いているか否か
 * @details 上流でダンジョン側の判定と併せて判定すること
 */
bool TerrainType::is_open() const
{
    return this->flags.has(TerrainCharacteristics::CLOSE);
}

/*!
 * @brief 閉じたドアであるかを調べる
 * @return 閉じたドアか否か
 */
bool TerrainType::is_closed_door() const
{
    return (this->flags.has(TerrainCharacteristics::OPEN) || this->flags.has(TerrainCharacteristics::BASH)) &&
           this->flags.has_not(TerrainCharacteristics::MOVE);
}

/*!
 * @brief 罠持ちの地形であるかを調べる
 * @return 罠持ちか否か
 * @todo 発見済の罠はtrueになるが、未発見の罠でどうなるかは要確認
 */
bool TerrainType::is_trap() const
{
    return this->flags.has(TerrainCharacteristics::TRAP);
}
/*!
 * @brief 地形のライティング状況をリセットする
 * @param is_config 設定値ならばtrue、定義値ならばfalse (定義値が入るのは初期化時のみ)
 */
void TerrainType::reset_lighting(bool is_config)
{
    auto &symbols = is_config ? this->symbol_configs : this->symbol_definitions;
    if (symbols[F_LIT_STANDARD].is_ascii_graphics()) {
        this->reset_lighting_ascii(symbols);
        return;
    }

    this->reset_lighting_graphics(symbols);
}

void TerrainType::reset_lighting_ascii(std::map<int, DisplaySymbol> &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    symbols[F_LIT_LITE].color = lighting_colours[color_standard & 0x0f][0];
    symbols[F_LIT_DARK].color = lighting_colours[color_standard & 0x0f][1];
    for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].character = character_standard;
    }
}

void TerrainType::reset_lighting_graphics(std::map<int, DisplaySymbol> &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    for (auto i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].color = color_standard;
    }

    symbols[F_LIT_LITE].character = character_standard + 2;
    symbols[F_LIT_DARK].character = character_standard + 1;
}

TerrainList TerrainList::instance{};

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

/*!
 * @brief 地形タグからIDを得る
 * @param tag タグ文字列
 * @throw std::runtime_error 未定義のタグが指定された
 * @return 地形タグに対応するID
 */
short TerrainList::get_terrain_id_by_tag(std::string_view tag) const
{
    const auto it = std::find_if(this->terrains.begin(), this->terrains.end(),
        [tag](const auto &terrain) {
            return terrain.tag == tag;
        });
    if (it == this->terrains.end()) {
        THROW_EXCEPTION(std::runtime_error, format(_("未定義のタグ '%s'。", "%s is undefined."), tag.data()));
    }

    return static_cast<short>(std::distance(this->terrains.begin(), it));
}

std::vector<TerrainType>::iterator TerrainList::begin()
{
    return this->terrains.begin();
}

std::vector<TerrainType>::const_iterator TerrainList::begin() const
{
    return this->terrains.cbegin();
}

std::vector<TerrainType>::reverse_iterator TerrainList::rbegin()
{
    return this->terrains.rbegin();
}

std::vector<TerrainType>::const_reverse_iterator TerrainList::rbegin() const
{
    return this->terrains.crbegin();
}

std::vector<TerrainType>::iterator TerrainList::end()
{
    return this->terrains.end();
}

std::vector<TerrainType>::const_iterator TerrainList::end() const
{
    return this->terrains.cend();
}

std::vector<TerrainType>::reverse_iterator TerrainList::rend()
{
    return this->terrains.rend();
}

std::vector<TerrainType>::const_reverse_iterator TerrainList::rend() const
{
    return this->terrains.crend();
}

size_t TerrainList::size() const
{
    return this->terrains.size();
}

bool TerrainList::empty() const
{
    return this->terrains.empty();
}

void TerrainList::resize(size_t new_size)
{
    this->terrains.resize(new_size);
}

void TerrainList::shrink_to_fit()
{
    this->terrains.shrink_to_fit();
}

/*!
 * @brief 地形タグからIDを得る
 * @param tag タグ文字列のオフセット
 * @return 地形ID。該当がないならstd::nullopt
 */
std::optional<short> TerrainList::search_real_terrain(std::string_view tag) const
{
    if (tag.empty()) {
        return std::nullopt;
    }

    return this->get_terrain_id_by_tag(tag);
}
