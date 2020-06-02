#pragma once

/*
 * Bit flags for apply_magic()
 */
typedef enum item_am_type {
    AM_NO_FIXED_ART = 0x00000001, /*!< Don't allow roll for fixed artifacts */
    AM_GOOD = 0x00000002, /*!< Generate good items */
    AM_GREAT = 0x00000004, /*!< Generate great items */
    AM_SPECIAL = 0x00000008, /*!< Generate artifacts (for debug mode only) */
    AM_CURSED = 0x00000010, /*!< Generate cursed/worthless items */
    AM_FORBID_CHEST = 0x00000020, /*!< 箱からさらに箱が出現することを抑止する */
} item_am_type;
