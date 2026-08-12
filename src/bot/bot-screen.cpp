/*!
 * @file bot-screen.cpp
 * @brief 端末の画面バッファをJSONへ変換する処理の実装
 * @details
 * 画面内容はz-termが保持する要求バッファ(term_type::scr)から直接読み出す。
 * このバッファはz-termが全フロントエンド共通で維持しており、term_fresh()の前後によらず
 * 常に最新の描画要求を保持しているため、フロントエンドの種類を問わず画面内容を再現できる。
 */

#include "bot/bot-screen.h"
#include "locale/character-encoding.h"
#include "term/z-term.h"
#include "util/string-processor.h"

namespace {

/*!
 * @brief 画面バッファの1行分の文字を取り出してUTF-8文字列に変換する
 * @param win 対象の画面バッファ
 * @param y 行番号
 * @param width 桁数
 * @return UTF-8に変換した1行分の文字列
 */
std::string make_line_text(const term_win &win, int y, int width)
{
    std::string line;
    line.reserve(static_cast<size_t>(width));
    for (auto x = 0; x < width; x++) {
        const auto ch = win.c[y][x];
        // 未描画セルにNULが残っている場合があるため空白に読み替える
        line.push_back((ch == '\0') ? ' ' : ch);
    }

    return to_json_utf8(line);
}

/*!
 * @brief 画面バッファの1行分の属性を16進文字列に変換する
 * @param win 対象の画面バッファ
 * @param y 行番号
 * @param width 桁数
 * @return 1セルあたり16進2桁を並べた文字列
 * @details
 * 日本語版では全角文字のセルの属性に色以外のビット(z-term.cppのAF_KANJI1: 0x10 / AF_KANJI2: 0x20)が
 * 乗るが、全角文字の1バイト目と2バイト目をクライアント側で判別できるよう、意図的にマスクせず生値を返す。
 * 色だけが必要な場合はクライアント側で0x0fとの論理積を取る。
 */
std::string make_line_attrs(const term_win &win, int y, int width)
{
    std::string attrs;
    attrs.reserve(static_cast<size_t>(width) * 2);
    for (auto x = 0; x < width; x++) {
        const auto attr = static_cast<uint8_t>(win.a[y][x]);
        attrs.push_back(hexify_upper(attr));
        attrs.push_back(hexify_lower(attr));
    }

    return attrs;
}

}

/*!
 * @brief ゲーム内部の文字コードの文字列をJSONに載せられるUTF-8文字列へ変換する
 * @param str 変換する文字列
 * @return UTF-8に変換した文字列。変換に失敗した場合は代替文字列
 * @details 変換の失敗でリクエスト全体を落とさないよう、bot-json-output.cppと同じ代替文字列を返す。
 */
std::string to_json_utf8(std::string_view str)
{
    return sys_to_utf8(str).value_or("<encoding-error>");
}

/*!
 * @brief 端末の画面バッファをJSONオブジェクトに変換する
 * @param t 対象の端末
 * @param with_attrs 色属性を含めるか否か
 * @return 画面内容を表すJSONオブジェクト
 * @details
 * lines[y] は1行分のセルを連結してUTF-8へ変換した文字列、
 * attrs[y] は1セルあたり16進2桁で属性を並べた文字列である。
 * 日本語版では全角1文字が2セルを占めるため、lines[y]の文字数と
 * attrs[y]の長さ(セル数×2)は一致しない。attrs側の添字がセル座標(x)に対応する。
 * 全角文字のセルの属性には色以外のビットも乗る (make_line_attrs()の説明を参照)。
 */
nlohmann::json make_bot_screen_json(const term_type &t, bool with_attrs)
{
    const auto width = static_cast<int>(t.wid);
    const auto height = static_cast<int>(t.hgt);
    const auto &win = *t.scr;

    auto lines = nlohmann::json::array();
    auto attrs = nlohmann::json::array();
    for (auto y = 0; y < height; y++) {
        lines.push_back(make_line_text(win, y, width));
        if (with_attrs) {
            attrs.push_back(make_line_attrs(win, y, width));
        }
    }

    nlohmann::json screen{
        { "width", width },
        { "height", height },
        { "cursor",
            {
                { "x", static_cast<int>(win.cx) },
                { "y", static_cast<int>(win.cy) },
                // cuは「カーソルが画面外にあり描画すべきでない」ことを示す
                { "visible", win.cv && !win.cu },
            } },
        { "lines", std::move(lines) },
    };

    if (with_attrs) {
        screen["attrs"] = std::move(attrs);
    }

    return screen;
}
