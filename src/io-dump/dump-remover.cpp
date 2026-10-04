#include "io-dump/dump-remover.h"
#include "io-dump/dump-util.h"
#include "io/read-pref-file.h"
#include "io/temp-file.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include <cstdio>
#include <fmt/format.h>
#include <fstream>
#include <vector>

/*!
 * @brief prefファイルを選択して処理する /
 * Ask for a "user pref line" and process it
 * @brief prf出力内容を消去する /
 * Remove old lines automatically generated before.
 * @param orig_file 消去を行うファイル名
 * @param auto_dump_mark 出力するヘッダマーク
 * @return エラー文字列。成功時はtl::nullopt
 */
tl::optional<std::string> remove_auto_dump(const std::filesystem::path &orig_file, std::string_view auto_dump_mark)
{
    const auto header_mark_str = format(auto_dump_header, auto_dump_mark.data());
    const auto footer_mark_str = format(auto_dump_footer, auto_dump_mark.data());
    const auto mark_len = footer_mark_str.length();
    std::ifstream ifs(orig_file);
    if (!ifs) {
        // 初回オープンだとファイルがあるとは限らないので、エラーにはしない.
        return tl::nullopt;
    }

    std::vector<std::string> lines;
    std::string line_ifs;
    while (std::getline(ifs, line_ifs)) {
        lines.push_back(line_ifs);
    }

    if (ifs.bad() || (ifs.fail() && !ifs.eof())) {
        constexpr auto fmt = _("ファイルの読み込みに失敗しました: {}", "Failed to read file: {}");
        return fmt::format(fmt, orig_file.string());
    }

    ifs.close();
    std::vector<std::string> output_lines;
    output_lines.reserve(lines.size());

    auto between_mark = false;
    auto changed = false;
    auto line_num = 0;
    size_t header_location = 0;
    size_t i = 0;
    while (i < lines.size()) {
        const auto &line = lines[i];
        if (!between_mark) {
            if (line == header_mark_str) {
                header_location = i + 1;
                line_num = 0;
                between_mark = true;
                changed = true;
            } else {
                output_lines.push_back(line);
            }

            ++i;
            continue;
        }

        if (line.compare(0, mark_len, footer_mark_str) == 0) {
            int parsed = 0;
            const auto parsed_ok = std::sscanf(line.data() + mark_len, " (%d)", &parsed) == 1;
            if (!parsed_ok || parsed != line_num) {
                i = header_location;
            } else {
                ++i;
            }

            between_mark = false;
            continue;
        }

        ++line_num;
        ++i;
    }

    if (between_mark) {
        output_lines.insert(output_lines.end(), lines.begin() + static_cast<std::ptrdiff_t>(header_location), lines.end());
    }

    if (!changed) {
        return tl::nullopt;
    }

    TempFile tf;
    if (const auto &error_message = tf.get_error_message(); error_message) {
        return *error_message;
    }

    tf.write_lines(output_lines);
    if (const auto &error_message = tf.get_error_message(); error_message) {
        return *error_message;
    }

    const auto tmp_lines = tf.read_all();
    if (const auto &error_message = tf.get_error_message(); error_message) {
        return *error_message;
    }

    std::ofstream ofs(orig_file, std::ios::trunc);
    if (!ofs) {
        constexpr auto fmt = _("ファイルの書き込みに失敗しました: {}", "Failed to write file: {}");
        return fmt::format(fmt, orig_file.string());
    }

    for (const auto &out_line : tmp_lines) {
        ofs << out_line << '\n';
    }

    ofs.flush();
    if (!ofs) {
        constexpr auto fmt = _("ファイルの書き込みに失敗しました: {}", "Failed to write file: {}");
        return fmt::format(fmt, orig_file.string());
    }

    return tl::nullopt;
}
