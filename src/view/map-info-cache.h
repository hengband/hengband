#pragma once

/*!
 * @file map-info-cache.h
 * @brief グリッドごとの map_info() の結果のキャッシュ
 * @details
 * 自分の周囲のサブウィンドウ (fix_dungeon()) は、描き直すたびに範囲内の全グリッドの表示を map_info() で求めるので重い。
 * 求めた結果を覚えておき、表示が変わったかもしれないグリッドだけ、使うときに求め直す。
 * 表示が変わるグリッドは lite_spot() で、全体に関わる変化は print_map() で知らされる。
 * どちらもメイン画面のために map_info() で表示を求めているので、その結果を覚えておく。
 * 自動拾いの対象を表示している間 (M コマンドの間) は map_info() の結果が変わるので、覚えずに毎回求める。
 */

#include "util/point-2d.h"
#include "view/display-symbol.h"
#include <cstdint>
#include <vector>

class PlayerType;
class MapInfoCache {
public:
    MapInfoCache(const MapInfoCache &) = delete;
    MapInfoCache(MapInfoCache &&) = delete;
    MapInfoCache &operator=(const MapInfoCache &) = delete;
    MapInfoCache &operator=(MapInfoCache &&) = delete;

    static MapInfoCache &get_instance();

    void invalidate(const Pos2D &pos);
    void invalidate_all();
    bool store(const Pos2D &pos, const DisplaySymbolPair &symbol_pair);
    void prepare(const PlayerType &player);
    DisplaySymbolPair get(PlayerType *player_ptr, const Pos2D &pos);

private:
    MapInfoCache() = default;

    static MapInfoCache instance;

    /*!
     * @brief 1 グリッド分の map_info() の結果
     */
    struct Entry {
        DisplaySymbolPair symbol_pair{ {}, {} };
        uint64_t generation = 0; //!< 求めたときの世代。0 は求め直す印
    };

    int height = 0;
    int width = 0;
    uint64_t generation = 1; //!< 現在の世代。世代が違う結果は使わない (64 ビットなので一周しない)
    std::vector<Entry> entries; //!< フロアの全グリッドの表示 (行優先)

    bool contains(const Pos2D &pos) const;
    int index_of(const Pos2D &pos) const;
};
