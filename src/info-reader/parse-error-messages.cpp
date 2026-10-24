/*!
 * @file parse-error-messages.cpp
 * @brief 変愚蛮怒のゲームデータ解析エラー名定義
 */

#include "info-reader/parse-error-messages.h"
#include "locale/character-encoding.h"
#include "system/angband.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

namespace {
/*!
 * エラーメッセージの名称定義 / Standard error message text
 * @details 外からは describe_parse_error() を通して引く
 */
concptr err_str[enum2i(ParseErrorType::MAX)] = {
    nullptr,
    _("文法エラー", "parse error"),
    _("古いファイル", "obsolete file"),
    _("記録ヘッダがない", "missing record header"),
    _("不連続レコード", "non-sequential records"),
    _("おかしなフラグ存在", "invalid flag specification"),
    _("未定義命令", "undefined directive"),
    _("メモリ不足", "out of memory"),
    _("座標範囲外", "coordinates out of bounds"),
    _("引数不足", "too few arguments"),
    _("未定義地形タグ", "undefined terrain tag"),
    _("不正なpval値", "invalid pval"),
    _("不正な値", "invalid value"),
    _("不正な型", "invalid type"),
};
}

/*!
 * @brief 読み込み中の診断を出力順に並べ、最後に終了理由を付ける
 * @param diagnostics 読み込み中に集めた診断 (空の行は飛ばす)
 * @param error 終了理由 (記録の位置とエラーの種類など)
 * @return 診断を1行ずつ並べ、最後に終了理由を付けた文字列
 * @details 診断は既にシステムの文字コードなので変換しない。
 */
std::string format_definition_load_error(std::span<const std::string> diagnostics, std::string_view error)
{
    std::string message;
    for (const auto &diagnostic : diagnostics) {
        if (!diagnostic.empty()) {
            message.append(diagnostic);
            message.push_back('\n');
        }
    }
    message.append(error);
    return message;
}

/*!
 * @brief 解析エラーの名称を返す
 * @param err 解析エラーの種類
 * @return エラーの名称 (エラーでない値や範囲外の値には「不明なエラー」)
 */
std::string_view describe_parse_error(ParseErrorType err)
{
    const auto code = enum2i(err);
    if ((code <= enum2i(ParseErrorType::NONE)) || (code >= enum2i(ParseErrorType::MAX)) || (err_str[code] == nullptr)) {
        return _("不明なエラー", "unknown error");
    }
    return err_str[code];
}

/*!
 * @brief 定義配列の要素の位置と解析エラーを含む診断を組み立てる
 * @param filename 定義ファイル名
 * @param keyname 定義配列のキー名
 * @param index 失敗した要素の配列の中の位置 (0始まり)
 * @param err 解析エラーの種類
 * @return 「ファイル名: $.キー[位置]: エラーの名称 (エラー番号)」の形の診断
 */
std::string format_record_parse_error(std::string_view filename, std::string_view keyname, size_t index, ParseErrorType err)
{
    return fmt::format(_("{}: $.{}[{}]: {} (エラー{})", "{}: $.{}[{}]: {} (error {})"), filename, keyname, index, describe_parse_error(err), enum2i(err));
}

/*!
 * @brief JSONの構文エラーの診断を組み立てる
 * @param filename 定義ファイル名
 * @param what JSONライブラリの例外の説明
 * @return 「ファイル名: JSONの構文エラー: 説明」の形の診断
 * @details 例外の説明には定義ファイルの一部 (UTF-8) が含まれ、途中で切れた文字のこともある。
 * システムの文字コードに変換し、変換できなければ ASCII 以外のバイトを '?' に置き換えて、日本語の文と混ざっても読めるようにする。
 */
std::string format_json_syntax_error(std::string_view filename, std::string_view what)
{
    auto detail = is_utf8_str(what) ? utf8_to_sys(what) : tl::optional<std::string>();
    if (!detail) {
        std::string ascii(what);
        for (auto &ch : ascii) {
            if (static_cast<unsigned char>(ch) >= 0x80) {
                ch = '?';
            }
        }
        detail = std::move(ascii);
    }

    return fmt::format(_("{}: JSONの構文エラー: {}", "{}: JSON syntax error: {}"), filename, *detail);
}
