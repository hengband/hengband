#pragma once

/*
 * Bit flags for apply_magic()
 */
enum item_am_type : uint32_t {
    AM_NONE = 0x00000000,
    AM_NO_FIXED_ART = 0x00000001, /*!< Don't allow roll for fixed artifacts */
    AM_GOOD = 0x00000002, /*!< Generate good items */
    AM_GREAT = 0x00000004, /*!< Generate great items */
    AM_SPECIAL = 0x00000008, /*!< Generate artifacts (for debug mode only) */
    AM_CURSED = 0x00000010, /*!< Generate cursed/worthless items */
    AM_FORBID_CHEST = 0x00000020, /*!< 箱からさらに箱が出現することを抑止する */
};

// @todo v3.0 正式リリース以降、上記enum をこちらに差し替える.
enum class ItemMagicAppliance {
    CURSED,
    NO_FIXED_ART,
    GOOD,
    GREAT,
    EGO,
    SPECIAL,
    FORBID_CHEST,
    MAX,
};
