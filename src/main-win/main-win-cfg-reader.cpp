/*!
 * @file main-win-cfg-reader.cpp
 * @brief Windows版固有実装(.cfgファイル処理)
 */

#include "main-win/main-win-cfg-reader.h"
#include "locale/japanese.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-tokenizer.h"
#include "main/sound-definitions-table.h"
#include "term/z-term.h"
#include "util/angband-files.h"
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
 * @brief マップのキーに対応する値を取得する
 * @param key1_type the "actions" value of "term_xtra()". see:z-term.h TERM_XTRA_xxxxx
 * @param key2_val the 2nd parameter of "term_xtra()"
 * @return キーに対応する値を返す。登録されていない場合はnullptrを返す。
 */
static cfg_values *get_map_value(cfg_map *map, int key1_type, int key2_val)
{
    cfg_values *value = nullptr;
    auto ite = map->find(make_cfg_key(key1_type, key2_val));
    if (ite != map->end()) {
        value = ite->second;
    }
    return value;
}

/*!
 * @brief 登録されている中からランダムに選択する
 * @param type the "actions" value of "term_xtra()". see:z-term.h TERM_XTRA_xxxxx
 * @param val the 2nd parameter of "term_xtra()"
 * @return キーに対応する値、複数のファイル名の中からからランダムに返す。登録されていない場合はnullptrを返す。
 */
concptr CfgData::get_rand(int key1_type, int key2_val)
{
    cfg_values *filenames = get_map_value(this->map, key1_type, key2_val);
    if (!filenames) {
        return nullptr;
    }

    return filenames->at(Rand_external(filenames->size()));
}

bool CfgData::has_key(int key1_type, int key2_val)
{
    auto ite = map->find(make_cfg_key(key1_type, key2_val));
    return ite != map->end();
}

void CfgData::insert(int key1_type, int key2_val, cfg_values *value)
{
    this->map->insert(std::make_pair(make_cfg_key(key1_type, key2_val), value));
}

/*!
 * @param dir .cfgファイルのディレクトリ
 * @param files .cfgファイル名。複数指定可能で、最初に見つかったファイルから読み取る。
 */
CfgReader::CfgReader(std::filesystem::path dir, std::initializer_list<concptr> files)
{
    this->dir = dir;
    this->cfg_path = find_any_file(dir, files);
}

/*!
 * @brief データの読み込みを行う。
 * @param sections 読み取るデータの指定
 * @return 読み込んだデータを登録したCfgDataを返す。
 */
CfgData *CfgReader::read_sections(std::initializer_list<cfg_section> sections)
{
    CfgData *result = new CfgData();

    if (!check_file(this->cfg_path.data())) {
        return result;
    }

    char key_buf[80];
    char buf[MAIN_WIN_MAX_PATH];
    char *tokens[SAMPLE_MAX];

    for (auto &section : sections) {

        bool has_data = false;
        int index = 0;
        concptr read_key;
        while ((read_key = section.key_at(index, key_buf)) != nullptr) {
            GetPrivateProfileStringA(section.section_name, read_key, "", buf, MAIN_WIN_MAX_PATH, this->cfg_path.data());
            if (*buf != '\0') {
                cfg_values *filenames = new cfg_values();
#ifdef JP
                // .cfg (UTF-8) to Shift-JIS
                guess_convert_to_system_encoding(buf, MAIN_WIN_MAX_PATH);
#endif
                const int num = tokenize_whitespace(buf, SAMPLE_MAX, tokens);
                for (auto j = 0; j < num; j++) {
                    const auto &path = path_build(dir, tokens[j]);
                    if (check_file(path)) {
                        filenames->push_back(string_make(tokens[j]));
                    }
                }
                if (filenames->empty()) {
                    delete filenames;
                } else {
                    result->insert(section.action_type, index, filenames);
                    has_data = true;
                }
            }

            index++;
        }

        if (section.has_data) {
            *(section.has_data) = has_data;
        }
    }

    return result;
}
