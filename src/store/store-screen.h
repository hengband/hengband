#pragma once

#include <string_view>

class Store;

/*!
 * @brief 店舗の画面の状態
 * @details 店に入っている間だけ意味を持つ、対象の店舗・入店した地形・表示中のページを持つ。
 * ページの先頭は常にページの大きさの倍数になる。
 */
class StoreScreen {
public:
    StoreScreen(Store &store, short terrain_id, int terminal_height);
    StoreScreen(const StoreScreen &) = delete;
    StoreScreen(StoreScreen &&) = delete;
    StoreScreen &operator=(const StoreScreen &) = delete;
    StoreScreen &operator=(StoreScreen &&) = delete;

    Store &get_store();
    const Store &get_store() const;
    std::string_view get_name() const;

    int get_page_top() const;
    int get_page_size() const;
    int get_page_item_count() const;
    int get_page_position(int pos) const;
    bool has_multiple_pages() const;
    void reset_page();
    void show_page_containing(int pos);
    void adjust_page_after_removal();
    void turn_page_forward();
    void turn_page_backward();

    int get_status_row() const;
    int get_command_row(int offset) const;

private:
    Store &store;
    short terrain_id; //!< 入店した地形のID
    int extra_rows; //!< 端末が高いときに増える行数
    int page_top = 0; //!< 表示中のページの先頭にある在庫の番号
};
