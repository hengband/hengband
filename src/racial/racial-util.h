#pragma once

#include "system/angband.h"
#include <string>
#include <vector>

/*!
 * @brief レイシャル/クラスパワー呼び出し番号
 */
enum rc_index : int {
    RC_IDX_RACE_0 = -1, //!< 種族0
    RC_IDX_RACE_1 = -2, //!< 種族1
    RC_IDX_CLASS_0 = -3, //!< 職業0
    RC_IDX_CLASS_1 = -4, //!< 職業1
};

/*!
 * レイシャル/クラスパワー定義構造体
 */
struct rpi_type {
    std::string racial_name{}; //!< パワー名
    PLAYER_LEVEL min_level{}; //!< 使用可能最小レベル
    int cost{}; //!< コスト
    int stat{}; //!< 使用に必要な能力値
    PERCENTAGE fail{}; //!< 難易度(失敗率)
    int number{}; //!< 呼び出し番号
    int racial_cost{}; //!< @todo 種族コスト、おそらく不要
};

/*!
 * レイシャル/クラスパワー管理構造体
 */
struct rc_type {
    std::vector<rpi_type> power_desc{}; //!< パワー定義配列
    COMMAND_CODE command_code{}; //!< 使用しようとしているパワー番号
    int ask{}; //!< 選択後確認するかどうか
    PLAYER_LEVEL lvl{}; //!< プレイヤーレベル
    bool is_warrior{}; //!< 戦士/狂戦士かどうか
    bool flag{}; //!< パワーを選んだかどうか
    bool redraw{}; //!< 再描画するかどうか
    bool cast{}; //!< パワーが使用されたかどうか
    char choice{}; //!< コマンドキー
    char out_val[160]{}; //!< 出力文字列用バッファ
    int menu_line{}; //!< 現在選択中の行

    /*!
     * @brief コンストラクタ
     * @param creature_ptr プレイヤー情報への参照ポインタ
     * @return 管理構造体
     */
    rc_type(player_type *creature_prt);

    /*!
     * @brief 指定したパワー名のレイシャル/クラスパワー定義構造体を返す
     * @param name パワー名
     * @return パワー定義構造体
     */
    rpi_type make_power(std::string name);

    /*!
     * @brief レイシャル/クラスパワー定義を追加
     * @param rpi レイシャル/クラスパワー定義(参照渡し)
     * @param number 呼び出し番号
     */
    void add_power(rpi_type &rpi, int number);

    /*!
     * @brief レイシャル/クラスパワー数を返す
     * @return パワー数
     */
    COMMAND_CODE size();
};
