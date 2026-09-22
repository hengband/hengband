#include "store/store-screen.h"
#include "game-option/birth-options.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include <algorithm>

namespace {
constexpr auto MIN_PAGE_SIZE = 12; //!< 1ページに表示する在庫の最小数
constexpr auto MAX_EXTRA_ROWS = 14 + 26; //!< 端末が高いときに増やせる行数の上限
}

/*!
 * @brief 店舗の画面を作る
 * @param store 対象の店舗
 * @param town_index 店舗がある町のID
 * @param terrain_id 入店した地形のID (店名の表示に使う)
 * @param terminal_height メインの端末の高さ
 */
StoreScreen::StoreScreen(Store &store, size_t town_index, short terrain_id, int terminal_height)
    : store(store)
    , town_index(town_index)
    , terrain_id(terrain_id)
    , extra_rows(std::clamp(terminal_height - MAIN_TERM_MIN_ROWS, 0, MAX_EXTRA_ROWS))
{
}

Store &StoreScreen::get_store()
{
    return this->store;
}

const Store &StoreScreen::get_store() const
{
    return this->store;
}

/*!
 * @brief 店舗がある町のIDを返す
 */
size_t StoreScreen::get_town_index() const
{
    return this->town_index;
}

/*!
 * @brief 店名 (入店した地形の名前) を返す
 */
std::string_view StoreScreen::get_name() const
{
    return TerrainList::get_instance().get_terrain(this->terrain_id).name;
}

int StoreScreen::get_page_top() const
{
    return this->page_top;
}

int StoreScreen::get_page_size() const
{
    return MIN_PAGE_SIZE + this->extra_rows;
}

/*!
 * @brief 表示中のページにある在庫の数を返す
 */
int StoreScreen::get_page_item_count() const
{
    return std::min(this->store.stock_num - this->page_top, this->get_page_size());
}

/*!
 * @brief 在庫の番号から、表示中のページ内での位置を求める
 * @param pos 表示中のページにある在庫の番号
 */
int StoreScreen::get_page_position(int pos) const
{
    return pos - this->page_top;
}

/*!
 * @brief 在庫が1ページに収まらないかを返す
 */
bool StoreScreen::has_multiple_pages() const
{
    return this->store.stock_num > this->get_page_size();
}

/*!
 * @brief 最初のページを表示する
 */
void StoreScreen::reset_page()
{
    this->page_top = 0;
}

/*!
 * @brief 指定した在庫を含むページを表示する
 * @param pos 在庫の番号
 */
void StoreScreen::show_page_containing(int pos)
{
    const auto page_size = this->get_page_size();
    this->page_top = (pos / page_size) * page_size;
}

/*!
 * @brief 在庫が減って表示中のページが空になったら、前のページに戻る
 */
void StoreScreen::adjust_page_after_removal()
{
    if (this->store.stock_num == 0) {
        this->page_top = 0;
    } else if (this->page_top >= this->store.stock_num) {
        this->page_top -= this->get_page_size();
    }
}

/*!
 * @brief 次のページに進む。最後のページからは最初のページに戻る
 * @details 隠しオプション (powerup_home) が無効なときは、我が家では2ページまでしか表示しない
 */
void StoreScreen::turn_page_forward()
{
    this->page_top += this->get_page_size();
    const auto stock_max = store_get_stock_max(this->store.get_sale_type(), powerup_home);
    if ((this->page_top >= this->store.stock_num) || (this->page_top >= stock_max)) {
        this->page_top = 0;
    }
}

/*!
 * @brief 前のページに戻る。最初のページからは最後のページに進む
 * @details 隠しオプション (powerup_home) が無効なときは、我が家では2ページまでしか表示しない
 */
void StoreScreen::turn_page_backward()
{
    const auto page_size = this->get_page_size();
    this->page_top -= page_size;
    if (this->page_top < 0) {
        this->show_page_containing(this->store.stock_num - 1);
    }

    const auto is_narrow_home = (this->store.get_sale_type() == StoreSaleType::HOME) && !powerup_home;
    if (is_narrow_home && (this->page_top >= page_size)) {
        this->page_top = page_size;
    }
}

/*!
 * @brief 所持金や在庫数を表示する行を返す
 */
int StoreScreen::get_status_row() const
{
    return 19 + this->extra_rows;
}

/*!
 * @brief コマンドの一覧を表示する行を返す
 * @param offset 一覧の先頭の行からのずれ (0～3)
 */
int StoreScreen::get_command_row(int offset) const
{
    return 20 + this->extra_rows + offset;
}
