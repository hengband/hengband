#pragma once

#include "system/angband.h"
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <initializer_list>

typedef uint cfg_key;
using cfg_values = std::vector<concptr>;
using cfg_map = std::unordered_map<cfg_key, cfg_values *>;
using key_name_func = concptr (*)(int, char *);

/*!
 * @brief .cfgファイルの読み取り対象とCfgData格納先のキー指定
 * @details
 * "term_xtra()"の第1引数(action-type)、第2引数(action-val)をデータの格納キーとする。
 * action-typeはaction_typeメンバで指定する。
 * key_name_funcにより、action-valと.cfg内の読取対象キーの対応を取る。
 * key_name_funcの引数にaction-valが渡され、対応する読取対象キーを返す。
 * key_name_func引数のaction-valは0から1,2,3...と順に呼ばれ、key_name_funcがnullptrを返すまで続ける。
 */
struct cfg_section {

    //! The name of the section in cfg file
    concptr section_name;
    //! the "actions" value of "term_xtra()". see:z-term.h TERM_XTRA_xxxxx
    int action_type;
    /*!
     * Returns a reference to the name of the key at a specified action-val* in the section.
     * *action-val : the 2nd parameter of "term_xtra()"
     */
    key_name_func key_at;
    //! 1つでもデータを読み込めた場合にtrueを設定する。（nullptrの場合を除く）
    bool *has_data = nullptr;
};

class CfgData {
public:
    CfgData()
    {
        map = new cfg_map();
    }
    virtual ~CfgData()
    {
        delete map;
    }
    concptr get_rand(int key1_type, int key2_val);
    bool has_key(int key1_type, int key2_val);
    void insert(int key1_type, int key2_val, cfg_values *value);

protected:
    cfg_map *map;
};

class CfgReader {
public:
    CfgReader(concptr dir, std::initializer_list<concptr> files);
    CfgData *read_sections(std::initializer_list<cfg_section> sections);
    concptr get_cfg_path()
    {
        return cfg_path.c_str();
    }

protected:
    concptr dir;
    std::string cfg_path;
};
