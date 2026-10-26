#include "window/overview-map-cache.h"
#include "game-option/map-screen-options.h"
#include "system/deferred-update-requests.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "util/finalizer.h"
#include "view/display-map.h"
#include "window/main-window-util.h"

namespace {
/*!
 * @brief グリッドの表示を map_info() で求める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos グリッドの座標
 * @return グリッドの表示
 */
OverviewMapGrid make_grid(PlayerType *player_ptr, const Pos2D &pos)
{
    match_autopick = -1;
    autopick_obj = nullptr;
    feat_priority = -1;
    const auto symbol_pair = map_info(player_ptr, pos);
    tl::optional<OverviewMapAutopick> autopick;
    if (match_autopick != -1) {
        autopick = OverviewMapAutopick{ match_autopick, autopick_obj };
    }

    return { symbol_pair.symbol_foreground, feat_priority, autopick };
}
}

OverviewMapCache OverviewMapCache::instance{};

OverviewMapCache &OverviewMapCache::get_instance()
{
    return instance;
}

/*!
 * @brief グリッドの表示が変わったかもしれないので、次に全体図を描くときに求め直す
 * @param pos グリッドの座標
 */
void OverviewMapCache::mark_dirty(const Pos2D &pos)
{
    if (this->is_all_dirty || (pos.y < 0) || (pos.y >= this->height) || (pos.x < 0) || (pos.x >= this->width)) {
        return;
    }

    const auto index = this->index_of(pos);
    if (this->is_dirty[index]) {
        return;
    }

    this->is_dirty[index] = true;
    this->dirty_positions.push_back(pos);
}

/*!
 * @brief フロア全体の表示が変わったかもしれないので、次に全体図を描くときに全グリッドを求め直す
 */
void OverviewMapCache::mark_all_dirty()
{
    this->is_all_dirty = true;
}

/*!
 * @brief 印を付けたグリッドの表示を求め直す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * 全体図は照明による記号の変化を使わないので、表示のオプションを切り替えて map_info() で求める。
 * 求め直した表示が前と違えば、サブウィンドウの全体図を描き直すよう覚えておく (has_changed())。
 * フロアの切り替えは、マップ全体の描き直しの要求で知らされる。
 * 次の場合は全グリッドを求め直す。
 * - マップ全体の描き直しの要求が残っている (画面の保存中は print_map() が呼ばれず、印が付かないため)
 * - 自動拾いの対象を表示している (display_autopick、M コマンドの間だけ有効)。
 *   キャッシュにアイテムへのポインタを残さないよう、この結果は次に使わない
 * - フロアの大きさが前回と違う (配列を確保し直す)
 */
void OverviewMapCache::update(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if ((this->height != floor.height) || (this->width != floor.width)) {
        this->height = floor.height;
        this->width = floor.width;
        this->grids.resize(static_cast<size_t>(this->height * this->width));
        this->is_dirty.assign(this->grids.size(), false);
        this->dirty_positions.clear();
        this->is_all_dirty = true;
    }

    const auto is_autopick_displayed = display_autopick.any();
    if (DeferredUpdateRequests::get_instance().has(MainWindowRedrawingFlag::MAP) || is_autopick_displayed) {
        this->is_all_dirty = true;
    }

    if (!this->has_dirty_grids()) {
        return;
    }

    const auto restore_options = util::make_finalizer([old_special = view_special_lite, old_granite = view_granite_lite] {
        view_special_lite = old_special;
        view_granite_lite = old_granite;
    });
    view_special_lite = false;
    view_granite_lite = false;

    if (this->is_all_dirty) {
        for (auto y = 0; y < this->height; y++) {
            for (auto x = 0; x < this->width; x++) {
                this->grids[this->index_of({ y, x })] = make_grid(player_ptr, { y, x });
            }
        }

        this->is_dirty.assign(this->grids.size(), false);
        this->dirty_positions.clear();
        this->is_all_dirty = is_autopick_displayed;
        this->is_changed = true;
        this->start_new_generation();
        return;
    }

    for (const auto &pos : this->dirty_positions) {
        const auto index = this->index_of(pos);
        // 縮小するときは優先度で記号を選ぶので、記号が同じでも優先度が変われば全体図の表示が変わることがある
        const auto grid = make_grid(player_ptr, pos);
        auto &cached_grid = this->grids[index];
        if (cached_grid != grid) {
            this->is_changed = true;
            this->changed_positions.push_back(pos);
        }

        cached_grid = grid;
        this->is_dirty[index] = false;
    }

    this->dirty_positions.clear();

    // 変わったグリッドの一覧が長くなりすぎたら、縮小した全体図も全部作り直してもらう
    if (this->changed_positions.size() > this->grids.size()) {
        this->start_new_generation();
    }
}

/*!
 * @brief 印を付けたグリッドがあるか
 * @return 次の update() で求め直すグリッドがあるか
 */
bool OverviewMapCache::has_dirty_grids() const
{
    return this->is_all_dirty || !this->dirty_positions.empty();
}

/*!
 * @brief サブウィンドウの全体図を描いてから、求め直した表示が変わったか
 * @return 変わったか
 * @details M コマンドの全体図で求め直したときも覚えておき、サブウィンドウの全体図を描くまで残す
 */
bool OverviewMapCache::has_changed() const
{
    return this->is_changed;
}

/*!
 * @brief サブウィンドウの全体図を描いたので、表示が変わったことを忘れる
 */
void OverviewMapCache::reset_changed()
{
    this->is_changed = false;
}

/*!
 * @brief グリッドの表示を返す
 * @param pos グリッドの座標
 * @return 最後に update() で求めたグリッドの表示
 */
const OverviewMapGrid &OverviewMapCache::get_grid(const Pos2D &pos) const
{
    return this->grids[this->index_of(pos)];
}

/*!
 * @brief 表示が変わったグリッドの記録の世代を返す
 * @return 世代の番号
 * @details 全グリッドを求め直したときなど、get_changed_positions() に無いグリッドも変わったかもしれないときに変わる。
 * 世代が変わっていたら、縮小した全体図は全部作り直す
 */
int OverviewMapCache::get_generation() const
{
    return this->generation;
}

/*!
 * @brief 今の世代で表示が変わったグリッドの一覧を返す
 * @return 変わった順のグリッドの座標 (同じグリッドが何度も入ることがある)
 * @details 一覧は世代が変わるまで伸び続けるので、使う側は前回どこまで読んだかを覚えておき、その続きだけを読む
 */
const std::vector<Pos2D> &OverviewMapCache::get_changed_positions() const
{
    return this->changed_positions;
}

/*!
 * @brief 表示が変わったグリッドの記録を捨てて、世代を変える
 * @details 縮小した全体図は世代が変わったことを見て全部作り直すので、記録に無い変化も反映される
 */
void OverviewMapCache::start_new_generation()
{
    this->generation++;
    this->changed_positions.clear();
}

int OverviewMapCache::index_of(const Pos2D &pos) const
{
    return pos.y * this->width + pos.x;
}
