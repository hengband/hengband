#pragma once

/*!
 * @brief ゲームデータの解析エラーの種類
 * @details 値はエラーの番号として診断に表示する。保存はしない。
 * 使われなくなった種類を削除した番号 (2, 3, 7) は、ほかの種類の番号を変えないよう欠番のままにする。
 */
enum class ParseErrorType : int {
    NONE = 0,
    GENERIC = 1,
    NON_SEQUENTIAL_RECORDS = 4,
    INVALID_FLAG = 5,
    UNDEFINED_DIRECTIVE = 6,
    OUT_OF_BOUNDS = 8,
    TOO_FEW_ARGUMENTS = 9,
    UNDEFINED_TERRAIN_TAG = 10,
    INVALID_PVAL = 11,
    INVALID_VALUE = 12,
    INVALID_TYPE = 13,
    MAX,
};
