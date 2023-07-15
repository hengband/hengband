#pragma once

/*
 * アイテムの簡易鑑定定義 / Game generated inscription indices. These are stored in the object,
 * and are used to index the string array from tables.c.
 */
enum item_feel_type {
    FEEL_NONE = 0, /*!< 簡易鑑定: 未鑑定 */
    FEEL_BROKEN = 1, /*!< 簡易鑑定: 壊れている */
    FEEL_TERRIBLE = 2, /*!< 簡易鑑定: 恐ろしい */
    FEEL_WORTHLESS = 3, /*!< 簡易鑑定: 無価値 */
    FEEL_CURSED = 4, /*!< 簡易鑑定: 呪われている */
    FEEL_UNCURSED = 5, /*!< 簡易鑑定: 呪われていない */
    FEEL_AVERAGE = 6, /*!< 簡易鑑定: 並 */
    FEEL_GOOD = 7, /*!< 簡易鑑定: 上質 */
    FEEL_EXCELLENT = 8, /*!< 簡易鑑定: 高級 */
    FEEL_SPECIAL = 9, /*!< 簡易鑑定: 特別 */
    FEEL_MAX = 9, /*!< 簡易鑑定の種別数 */
};
