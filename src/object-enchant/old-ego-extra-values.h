/*
 * @brief 変愚ver1.5.0以前に使われていたアイテムの追加特性フラグ / Hack -- special "xtra" object powers
 * @date 2020/05/28
 * @author Hourier
 * @details いずれ消したい
 */

#pragma once

enum class OldEgoType {
    XTRA_SUSTAIN = 1, /*!< 旧版アイテムフラグ(非推奨): 追加維持能力 / Sustain one stat */
    XTRA_POWER = 2, /*!< 旧版アイテムフラグ(非推奨): 追加上級耐性 / High resist */
    XTRA_ABILITY = 3 /*!< 旧版アイテムフラグ(非推奨): 追加能力 / Special ability */
};
