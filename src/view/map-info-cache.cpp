#include "view/map-info-cache.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-map.h"

MapInfoCache MapInfoCache::instance{};

MapInfoCache &MapInfoCache::get_instance()
{
    return instance;
}

/*!
 * @brief グリッドの表示が変わったかもしれないので、次に使うときに求め直す
 * @param pos グリッドの座標
 */
void MapInfoCache::invalidate(const Pos2D &pos)
{
    if (!this->contains(pos)) {
        return;
    }

    this->entries[this->index_of(pos)].generation = 0;
}

/*!
 * @brief フロア全体の表示が変わったかもしれないので、次に使うときに全グリッドを求め直す
 * @details 世代を進めて、それまでの結果を全て使わないようにする
 */
void MapInfoCache::invalidate_all()
{
    this->generation++;
}

/*!
 * @brief 求めたグリッドの表示を覚える
 * @param pos グリッドの座標
 * @param symbol_pair 通常の表示のオプションで map_info() が返した表示 (単色の色を置き換える前のもの)
 * @return 覚えていた結果と違うか、覚えていた結果が無効だったか (サブウィンドウには違う表示が出ているかもしれない)
 * @details 自動拾いの対象を表示している間は、通常の表示と違うので覚えず、覚えていた結果を無効にする
 */
bool MapInfoCache::store(const Pos2D &pos, const DisplaySymbolPair &symbol_pair)
{
    if (!this->contains(pos)) {
        return false;
    }

    auto &entry = this->entries[this->index_of(pos)];
    if (display_autopick.any()) {
        entry.generation = 0;
        return false;
    }

    const auto is_changed = (entry.generation != this->generation) || (entry.symbol_pair != symbol_pair);
    entry = { symbol_pair, this->generation };
    return is_changed;
}

/*!
 * @brief 使う前に、覚えている結果が使えるか確かめる
 * @param player プレイヤーへの参照
 * @details
 * フロアの切り替えは、マップ全体の描き直しの要求で知らされる。
 * 次の場合は全グリッドを求め直す。
 * - マップ全体の描き直しの要求が残っている (画面の保存中は print_map() が呼ばれないため)
 * - フロアの大きさが前回と違う (配列を確保し直す)
 */
void MapInfoCache::prepare(const PlayerType &player)
{
    const auto &floor = *player.current_floor_ptr;
    if ((this->height != floor.height) || (this->width != floor.width)) {
        this->height = floor.height;
        this->width = floor.width;
        this->entries.assign(static_cast<size_t>(this->height * this->width), {});
    }

    if (RedrawingFlagsUpdater::get_instance().has(MainWindowRedrawingFlag::MAP)) {
        this->invalidate_all();
    }
}

/*!
 * @brief グリッドの表示を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos グリッドの座標
 * @return グリッドの表示。覚えていなければ map_info() で求めて覚える
 * @details 先に prepare() を呼んで、覚えている結果が使えるか確かめておくこと。
 * 自動拾いの対象を表示している間は、覚えずに毎回求める。
 */
DisplaySymbolPair MapInfoCache::get(PlayerType *player_ptr, const Pos2D &pos)
{
    if (display_autopick.any() || !this->contains(pos)) {
        return map_info(player_ptr, pos);
    }

    auto &entry = this->entries[this->index_of(pos)];
    if (entry.generation != this->generation) {
        entry = { map_info(player_ptr, pos), this->generation };
    }

    return entry.symbol_pair;
}

bool MapInfoCache::contains(const Pos2D &pos) const
{
    return (pos.y >= 0) && (pos.y < this->height) && (pos.x >= 0) && (pos.x < this->width);
}

int MapInfoCache::index_of(const Pos2D &pos) const
{
    return pos.y * this->width + pos.x;
}
