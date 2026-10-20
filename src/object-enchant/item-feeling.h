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

/*!
 * @brief 呪われていることを表す感触か
 * @param feel 感触
 * @return 恐ろしい・無価値・呪われているなら true
 * @details 恐ろしい (アーティファクト)・無価値 (エゴ) は、呪われているか壊れているときに付くので、呪われているかもしれないことを表す。
 * 擬似鑑定は呪いを壊れより先に判定するので、壊れているの感触は、付けた時点で呪われていない品にしか付かない
 * (後から呪いが付いたときは、呪われた品の感触に置き換える。to_cursed_feeling() を参照)。
 */
constexpr bool is_cursed_feeling(int feel)
{
    return (feel == FEEL_TERRIBLE) || (feel == FEEL_WORTHLESS) || (feel == FEEL_CURSED);
}

/*!
 * @brief 呪いが付いた品の感触を、前の感触から求める
 * @param feel 前の感触
 * @return 恐ろしい・特別製なら恐ろしい、無価値・高級品なら無価値、それ以外 (感触なしを含む) は呪われている
 * @details 重い擬似鑑定で呪われた品に付く感触にそろえる。アーティファクト・エゴであることは前の感触から
 * 分かっていた分だけを残す。上質・並 (重い擬似鑑定で、アーティファクト・エゴでないと分かっている品) と
 * 呪われていない (軽い擬似鑑定で、どちらか分からない品) は呪われているにする。
 */
constexpr item_feel_type to_cursed_feeling(int feel)
{
    switch (feel) {
    case FEEL_TERRIBLE:
    case FEEL_SPECIAL:
        return FEEL_TERRIBLE;
    case FEEL_WORTHLESS:
    case FEEL_EXCELLENT:
        return FEEL_WORTHLESS;
    default:
        return FEEL_CURSED;
    }
}
