#pragma once

/* The "sval" codes for TV_SOFT_ARMOR */
enum sv_soft_armor_type {
    SV_T_SHIRT = 0,
    SV_FILTHY_RAG = 1,
    SV_ROBE = 2,
    SV_PAPER_ARMOR = 3, /* 4 */
    SV_SOFT_LEATHER_ARMOR = 4,
    SV_SOFT_STUDDED_LEATHER = 5,
    SV_HARD_LEATHER_ARMOR = 6,
    SV_HARD_STUDDED_LEATHER = 7,
    SV_RHINO_HIDE_ARMOR = 8,
    SV_CORD_ARMOR = 9, /*  6 */
    SV_PADDED_ARMOR = 10, /*  4 */
    SV_LEATHER_SCALE_MAIL = 11,
    SV_LEATHER_JACK = 12,
    SV_KUROSHOUZOKU = 13,
    SV_STONE_AND_HIDE_ARMOR = 15, /* 15 */
    SV_ABUNAI_MIZUGI = 50,
    SV_TWILIGHT_ROBE = 60,
    SV_NAMAKE_ARMOR = 63,
};

/* The "sval" codes for TV_HARD_ARMOR */
enum sv_hard_armor_type {
    SV_RUSTY_CHAIN_MAIL = 1, /* 14- */
    SV_RING_MAIL = 2, /* 12  */
    SV_METAL_SCALE_MAIL = 3, /* 13  */
    SV_CHAIN_MAIL = 4, /* 14  */
    SV_DOUBLE_RING_MAIL = 5, /* 15  */
    SV_AUGMENTED_CHAIN_MAIL = 6, /* 16  */
    SV_DOUBLE_CHAIN_MAIL = 7, /* 16  */
    SV_BAR_CHAIN_MAIL = 8, /* 18  */
    SV_METAL_BRIGANDINE_ARMOUR = 9, /* 19  */
    SV_SPLINT_MAIL = 10, /* 19  */
    SV_DO_MARU = 11, /* 20  */
    SV_PARTIAL_PLATE_ARMOUR = 12, /* 22  */
    SV_METAL_LAMELLAR_ARMOUR = 13, /* 23  */
    SV_HARAMAKIDO = 14, /* 17  */
    SV_FULL_PLATE_ARMOUR = 15, /* 25  */
    SV_O_YOROI = 16, /* 24  */
    SV_RIBBED_PLATE_ARMOUR = 18, /* 28  */
    SV_MITHRIL_CHAIN_MAIL = 20, /* 28+ */
    SV_MITHRIL_PLATE_MAIL = 25, /* 35+ */
    SV_ADAMANTITE_PLATE_MAIL = 30, /* 40+ */
};

/* The "sval" codes for TV_DRAG_ARMOR */
enum sv_dragon_armor_type {
    SV_DRAGON_BLACK = 1,
    SV_DRAGON_BLUE = 2,
    SV_DRAGON_WHITE = 3,
    SV_DRAGON_RED = 4,
    SV_DRAGON_GREEN = 5,
    SV_DRAGON_MULTIHUED = 6,
    SV_DRAGON_SHINING = 10,
    SV_DRAGON_LAW = 12,
    SV_DRAGON_BRONZE = 14,
    SV_DRAGON_GOLD = 16,
    SV_DRAGON_CHAOS = 18,
    SV_DRAGON_BALANCE = 20,
    SV_DRAGON_POWER = 30,
};
