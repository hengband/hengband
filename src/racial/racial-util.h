#pragma once

#include <string>
#include <vector>

#include "mutation/mutation-flag-types.h"
#include "system/angband.h"

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
    std::string info{}; //!< パワー情報
    std::string text{}; //パワー説明文
    PLAYER_LEVEL min_level{}; //!< 使用可能最小レベル
    int cost{}; //!< コスト
    int stat{}; //!< 使用に必要な能力値
    PERCENTAGE fail{}; //!< 難易度(失敗率)
    int number{}; //!< 呼び出し番号
    int racial_cost{}; //!< @todo 種族コスト、おそらく不要

    /*!
     * @brief コンストラクタ
     * @param name パワー名
     */
    rpi_type(std::string name = {})
        : racial_name(name) {}
};

/*!
 * レイシャル/クラスパワー管理構造体
 */
struct player_type;
struct rc_type {
    std::vector<rpi_type> power_desc{}; //!< パワー定義配列
    COMMAND_CODE command_code{}; //!< 使用しようとしているパワー番号
    bool browse_mode{}; //!< 閲覧のみかどうか
    int page{}; //!< 表示中のページ
    int max_page{}; //!< ページ数
    int ask{}; //!< 選択後確認するかどうか
    PLAYER_LEVEL lvl{}; //!< プレイヤーレベル
    bool is_warrior{}; //!< 戦士/狂戦士かどうか
    bool is_chosen{}; //!< 選択したかどうか
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
     * @brief レイシャル/クラスパワー定義を追加
     * @param rpi レイシャル/クラスパワー定義(参照渡し)
     * @param number 呼び出し番号
     */
    void add_power(rpi_type &rpi, int number);

    /*!
     * @brief レイシャル/クラスパワー定義を追加
     * @param rpi レイシャル/クラスパワー定義(参照渡し)
     * @param number 突然変異ID
     */
    void add_power(rpi_type &rpi, MUTA flag);

    /*!
     * @brief レイシャル/クラスパワー数を返す
     * @return パワー数
     */
    COMMAND_CODE power_count();
};
