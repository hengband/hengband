/*!
 * @file main-win-cfg-reader.cpp
 * @brief Windows版固有実装(.cfgファイル処理)
 */

#include "main-win/main-win-cfg-reader.h"
#include "locale/japanese.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-tokenizer.h"
#include "main/sound-definitions-table.h"
#include "term/z-term.h"
#include "util/angband-files.h"
#include <span>
#include <windows.h>

// 1つの項目に設定可能な最大ファイル数
#define SAMPLE_MAX 16

/*!
 * @brief マップのキーを生成する
 * @details
 * typeを上位16ビット, valを下位16ビットに設定した値をマップのキーとする。
 * @param type the "actions" value of "term_xtra()". see:z-term.h TERM_XTRA_xxxxx
 * @param val the 2nd parameter of "term_xtra()"
 * @return 生成したキーを返す
 */
static cfg_key make_cfg_key(int type, int val)
{
    return (type << 16) | (val & 0xffff);
}

/*!
 * @brief 登録されている中からランダムに選択する
 * @param type the "actions" value of "term_xtra()". see:z-term.h TERM_XTRA_xxxxx
 * @param val the 2nd parameter of "term_xtra()"
 * @return キーに対応する値、複数のファイル名の中からからランダムに返す。登録されていない場合はstd::nulloptを返す。
 */
std::optional<std::string> CfgData::get_rand(int key1_type, int key2_val) const
{
    const auto it = this->map.find(make_cfg_key(key1_type, key2_val));
    if (it == this->map.end()) {
        return std::nullopt;
    }

    const auto &filenames = it->second;
    return filenames.at(Rand_external(filenames.size()));
}

bool CfgData::has_key(int key1_type, int key2_val) const
{
    return this->map.contains(make_cfg_key(key1_type, key2_val));
}

void CfgData::insert(int key1_type, int key2_val, cfg_values &&value)
{
    this->map.emplace(make_cfg_key(key1_type, key2_val), std::move(value));
}

/*!
 * @param dir .cfgファイルのディレクトリ
 * @param files .cfgファイル名。複数指定可能で、最初に見つかったファイルから読み取る。
 */
CfgReader::CfgReader(const std::filesystem::path &dir, std::initializer_list<concptr> files)
{
    this->dir = dir;
    auto exists_in_dir = [dir](const char *filename) { return is_regular_file(path_build(dir, filename)); };
    if (auto it = std::find_if(files.begin(), files.end(), exists_in_dir); it != files.end()) {
        this->cfg_path = path_build(dir, *it);
    }
}

/*!
 * @brief データの読み込みを行う。
 * @param sections 読み取るデータの指定
 * @return 読み込んだデータを登録したCfgDataを返す。
 */
CfgData CfgReader::read_sections(std::initializer_list<cfg_section> sections) const
{
    CfgData result;

    if (!is_regular_file(this->cfg_path)) {
        return result;
    }

    const auto cfg_path_str = this->cfg_path.string();

    for (auto &section : sections) {
        concptr read_key;
        char key_buf[80]{};
        for (auto index = 0; (read_key = section.key_at(index, key_buf)) != nullptr; ++index) {
            char buf[MAIN_WIN_MAX_PATH]{};
            if (GetPrivateProfileStringA(section.section_name, read_key, "", buf, MAIN_WIN_MAX_PATH, cfg_path_str.data()) == 0) {
                continue;
            }

#ifdef JP
            // .cfg (UTF-8) to Shift-JIS
            guess_convert_to_system_encoding(buf, MAIN_WIN_MAX_PATH);
#endif
            char *tokens[SAMPLE_MAX]{};
            const int num = tokenize_whitespace(buf, SAMPLE_MAX, tokens);

            cfg_values filenames;
            for (const std::string_view token : std::span(tokens, num)) {
                if (is_regular_file(path_build(this->dir, token))) {
                    filenames.emplace_back(token);
                }
            }
            if (filenames.empty()) {
                continue;
            }

            result.insert(section.action_type, index, std::move(filenames));
            if (section.has_data) {
                *(section.has_data) = true;
            }
        }
    }

    return result;
}
