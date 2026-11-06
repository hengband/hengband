/*!
 * @file parse-error-messages.cpp
 * @brief 変愚蛮怒のゲームデータ解析エラー名定義
 */

#include "info-reader/parse-error-messages.h"
#include "locale/character-encoding.h"
#include "system/angband.h"
#include "util/enum-converter.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace {
/*!
 * 診断に出す記録の識別子の最大の文字数 (これより長いものは先頭だけ出す)
 */
constexpr size_t RECORD_IDENTIFIER_MAX_CHARS = 40;

/*!
 * @brief UTF-8の文字列をシステムの文字コードに変換する
 * @param utf8 変換する文字列
 * @return 変換した文字列 (変換できなければ ASCII 以外のバイトを '?' に置き換えた文字列)
 */
std::string to_system_encoding_or_ascii(std::string_view utf8)
{
    if (is_utf8_str(utf8)) {
        if (auto converted = utf8_to_sys(utf8)) {
            return std::move(*converted);
        }
    }

    std::string ascii(utf8);
    for (auto &ch : ascii) {
        if (static_cast<unsigned char>(ch) >= 0x80) {
            ch = '?';
        }
    }
    return ascii;
}

/*!
 * @brief UTF-8の文字列を文字の途中で切らずに先頭の数文字に縮める
 * @param utf8 縮める文字列
 * @param max_chars 残す最大の文字数
 * @return 縮めた文字列 (縮めたときは末尾に "..." を付ける)
 * @details UTF-8として不正なバイトに当たったら、そこで切る。
 */
std::string truncate_utf8(std::string_view utf8, size_t max_chars)
{
    size_t pos = 0;
    for (size_t chars = 0; (pos < utf8.size()) && (chars < max_chars); ++chars) {
        const auto length = utf8_next_char_byte_length(utf8.substr(pos));
        if (length <= 0) {
            break;
        }
        pos += static_cast<size_t>(length);
    }

    if (pos >= utf8.size()) {
        return std::string(utf8);
    }
    return fmt::format("{}...", utf8.substr(0, pos));
}
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
 * @return エラーの名称 (エラーでない値や欠番・範囲外の値には「不明なエラー」)
 * @details 種類の番号には欠番があるため、番号で引く配列ではなく種類ごとに名称を返す。
 * 種類を追加したときにコンパイラの警告で気づけるよう、default は置かずにすべての種類を列挙する。
 */
std::string_view describe_parse_error(ParseErrorType err)
{
    switch (err) {
    case ParseErrorType::GENERIC:
        return _("文法エラー", "parse error");
    case ParseErrorType::NON_SEQUENTIAL_RECORDS:
        return _("不連続レコード", "non-sequential records");
    case ParseErrorType::INVALID_FLAG:
        return _("おかしなフラグ存在", "invalid flag specification");
    case ParseErrorType::UNDEFINED_DIRECTIVE:
        return _("未定義命令", "undefined directive");
    case ParseErrorType::OUT_OF_BOUNDS:
        return _("座標範囲外", "coordinates out of bounds");
    case ParseErrorType::TOO_FEW_ARGUMENTS:
        return _("引数不足", "too few arguments");
    case ParseErrorType::UNDEFINED_TERRAIN_TAG:
        return _("未定義地形タグ", "undefined terrain tag");
    case ParseErrorType::INVALID_PVAL:
        return _("不正なpval値", "invalid pval");
    case ParseErrorType::INVALID_VALUE:
        return _("不正な値", "invalid value");
    case ParseErrorType::INVALID_TYPE:
        return _("不正な型", "invalid type");
    case ParseErrorType::NONE:
    case ParseErrorType::MAX:
        break;
    }

    return _("不明なエラー", "unknown error");
}

/*!
 * @brief 定義配列の要素の位置・識別子と解析エラーを含む診断を組み立てる
 * @param filename 定義ファイル名
 * @param keyname 定義配列のキー名
 * @param index 失敗した要素の配列の中の位置 (0始まり)
 * @param identifier 位置の後ろに括弧で添える要素の識別子 (describe_record_identifier() の結果。空なら添えない)
 * @param err 解析エラーの種類
 * @return 「ファイル名: $.キー[位置] (id: 値): エラーの名称 (エラー番号)」の形の診断 (識別子が空なら「ファイル名: $.キー[位置]: ...」)
 */
std::string format_record_parse_error(std::string_view filename, std::string_view keyname, size_t index, std::string_view identifier, ParseErrorType err)
{
    const auto position = identifier.empty() ? fmt::format("$.{}[{}]", keyname, index) : fmt::format("$.{}[{}] ({})", keyname, index, identifier);
    return fmt::format(_("{}: {}: {} (エラー{})", "{}: {}: {} (error {})"), filename, position, describe_parse_error(err), enum2i(err));
}

/*!
 * @brief 定義の要素を見分けるための項目を探す
 * @param record 定義の要素
 * @return "id" (型は問わない)、無ければ文字列の "name" の項目。どちらも無いか、要素がオブジェクトでなければ value が nullptr
 * @details 値の表記は作らず、要素の中の値を指すだけにする (表記は失敗したときだけ describe_record_identifier() で作る)。
 * nlohmann::json のオブジェクトは std::map なので、解析器が operator[] で項目を足しても指す先は変わらない。
 */
RecordIdentifier find_record_identifier(const nlohmann::json &record)
{
    if (!record.is_object()) {
        return {};
    }

    if (const auto it = record.find("id"); it != record.end()) {
        return { "id", &*it };
    }
    if (const auto it = record.find("name"); (it != record.end()) && it->is_string()) {
        return { "name", &*it };
    }
    return {};
}

/*!
 * @brief 定義の要素を見分けるための識別子を、書いてあるとおりの値で返す
 * @param identifier find_record_identifier() で探した項目
 * @return 「id: 値」または「name: "値"」。項目が無ければ空文字列
 * @details 識別子が整数でない・範囲外のときにも書いてあるとおりの値が分かるよう、型を問わず JSON の表記 (dump) で出す。
 * 長い値は先頭の RECORD_IDENTIFIER_MAX_CHARS 文字だけにして "..." を付け、システムの文字コードに変換する。
 */
std::string describe_record_identifier(const RecordIdentifier &identifier)
{
    if (identifier.value == nullptr) {
        return "";
    }

    const auto raw = identifier.value->dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    return fmt::format("{}: {}", identifier.key, to_system_encoding_or_ascii(truncate_utf8(raw, RECORD_IDENTIFIER_MAX_CHARS)));
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
    return fmt::format(_("{}: JSONの構文エラー: {}", "{}: JSON syntax error: {}"), filename, to_system_encoding_or_ascii(what));
}
