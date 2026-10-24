#pragma once

#include "autopick/autopick-methods-table.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <memory>
#include <string>
#include <vector>

constexpr auto MAX_LINELEN = 1024;

enum class PickType {
    DEFAULT = 0, //!< picktype.prf
    WITH_PNAME = 1, //!< picktype-<playername>.prf
};

/*!
 * @struct autopick_type
 * @brief 自動拾い/破壊設定データの構造体 / A structure type for entry of auto-picker/destroyer
 */
struct autopick_type {
    std::string name = ""; /*!< 自動拾い/破壊定義の名称一致基準 / Items which have 'name' as part of its name match */
    std::string insc = ""; /*!< 対象となったアイテムに自動で刻む内容 / Items will be auto-inscribed as 'insc' */
    BIT_FLAGS flags[2]{}; /*!< キーワードに関する汎用的な条件フラグ / Misc. keyword to be matched */
    EnumClassFlagGroup<AutopickMethod> action{}; /*!< 対象のアイテムを拾う/破壊/放置するかの指定フラグ / Auto-pickup or Destroy or Leave items */
    byte dice = 0; /*!< 武器のダイス値基準値 / Weapons which have more than 'dice' dice match */
    byte bonus = 0; /*!< アイテムのボーナス基準値 / Items which have more than 'bonus' magical bonus match */
    bool has(int flag) const;
    void add(int flag);
    void remove(int flag);
};

enum class EditorCommandId;
class ItemEntity;
enum class AutopickMark {
    MARK = 0,
    BY_SHIFT = 1,
    MAX,
};

enum class AutopickEntryStatus {
    BYPASS = 0,
    EXPRESSION = 1,
    AUTOREGISTER = 2,
    EXPRESSION_ERROR = 3,
    INCLUDE = 4,
    MAX,
};

class AutopickEntryLine {
public:
    AutopickEntryLine() = default;
    AutopickEntryLine(std::string line, const EnumClassFlagGroup<AutopickEntryStatus> &state);

    AutopickEntryLine(const AutopickEntryLine &) = delete;
    AutopickEntryLine &operator=(const AutopickEntryLine &) = delete;
    AutopickEntryLine(AutopickEntryLine &&) = default;
    AutopickEntryLine &operator=(AutopickEntryLine &&) = default;

    std::unique_ptr<std::string> line = std::make_unique<std::string>();
    EnumClassFlagGroup<AutopickEntryStatus> state;
};

/*!
 * @brief 自動拾い設定ファイル用テキストエディタの状態を保持するクラス
 *
 * 編集中の行データ・各行の状態・カーソル位置・画面スクロール位置・選択範囲・検索条件・ヤンクバッファ・再描画用ダーティ情報・変更フラグなどをまとめて管理する。
 * ユーザがエディタを操作する度に、このクラスのインスタンスを通じて状態が更新される。
 */
class AutopickTextBody {
public:
    AutopickTextBody(int cx, int cy); // 画面表示はX→Yの順番であることが多いので一応揃える.

    void adjust_cursor_column();
    void update_cursor_column_record(EditorCommandId com_id);
    bool is_cursor_on_second_byte_of_kanji() const;
    bool has_line(int y) const;
    bool is_last_line(int y) const;
    void set_lines(std::vector<std::string> new_lines);

    int cx;
    int cy;
    int rec_cx = 0;
    int wid = 0;
    int hgt = 0;
    int upper = 0;
    int left = 0;
    int old_wid = -1;
    int old_hgt = -1;
    int old_cy = -1;
    int old_upper = -1;
    int old_left = -1;
    int mx = 0;
    int my = 0;
    EnumClassFlagGroup<AutopickMark> marks{};

    std::shared_ptr<ItemEntity> search_item = nullptr;
    std::string search_str = "";
    std::string last_destroyed = "";

    std::vector<std::string> yank{};
    bool yank_eol = false;

    std::vector<AutopickEntryLine> lines;

    uint16_t dirty_flags = 0;
    int dirty_line = -1;
    PickType filename_mode{};
    EditorCommandId old_com_id{};

    bool changed = false;

    std::vector<std::string> get_entry_lines() const;
};

/*
 *  List for auto-picker/destroyer entries
 */
extern std::vector<autopick_type> autopick_list;
extern ItemEntity autopick_last_destroyed_object;

bool auto_inscribe_item(ItemEntity &item, int idx);
