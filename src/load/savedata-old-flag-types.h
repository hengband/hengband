/*!
 * @file savedata-old-flag-types.h
 * @brief 過去に存在したセーブデータ有無フラグを定義する。古いセーブデータからのマイグレーション用。
 */

#pragma once

//! セーブデータバージョン7でart_flagsをFlagGroupクラスに移行する前のセーブデータ有無フラグ
enum class SavedataItemOlderThan7FlagType {
    ART_FLAGS0 = 0x00004000,
    ART_FLAGS1 = 0x00008000,
    ART_FLAGS2 = 0x00010000,
    ART_FLAGS3 = 0x00020000,
    ART_FLAGS4 = 0x10000000,
};

enum class SavedataItemOlderThan12FlagType {
    XTRA1 = 0x00100000,
    XTRA3 = 0x00400000,
};

enum class SavedataItemOlderThan13FlagType {
    XTRA4 = 0x00800000,
};

enum class SavedataLoreOlderThan10FlagType {
    AURA_FIRE_OLD = 0x00004000,
    AURA_COLD_OLD = 0x00000400,
    AURA_ELEC_OLD = 0x00008000,
};

enum class SavedataLoreOlderThan14FlagType {
    RFR_IM_ACID = 0x00000001, /* Immunity acid */
    RFR_IM_ELEC = 0x00000002, /* Immunity elec */
    RFR_IM_FIRE = 0x00000004, /* Immunity fire */
    RFR_IM_COLD = 0x00000008, /* Immunity cold */
    RFR_IM_POIS = 0x00000010, /* Immunity poison */
    RFR_RES_LITE = 0x00000020, /* Resist lite */
    RFR_RES_DARK = 0x00000040, /* Resist dark */
    RFR_RES_NETH = 0x00000080, /* Resist nether */
    RFR_RES_WATE = 0x00000100, /* Resist water */
    RFR_RES_PLAS = 0x00000200, /* Resist plasma */
    RFR_RES_SHAR = 0x00000400, /* Resist shards */
    RFR_RES_SOUN = 0x00000800, /* Resist sound */
    RFR_RES_CHAO = 0x00001000, /* Resist chaos */
    RFR_RES_NEXU = 0x00002000, /* Resist nexus */
    RFR_RES_DISE = 0x00004000, /* Resist disenchantment */
    RFR_RES_WALL = 0x00008000, /* Resist force */
    RFR_RES_INER = 0x00010000, /* Resist inertia */
    RFR_RES_TIME = 0x00020000, /* Resist time */
    RFR_RES_GRAV = 0x00040000, /* Resist gravity */
    RFR_RES_ALL = 0x00080000, /* Resist all */
    RFR_RES_TELE = 0x00100000, /* Resist teleportation */
};

enum class SavedataLoreOlderThan18FlagType {
    RF1_ONLY_GOLD = 0x00100000, /*!< モンスター特性: 財宝しか落とさない / Drop only gold */
    RF1_ONLY_ITEM = 0x00200000, /*!< モンスター特性: アイテムしか落とさない / Drop only items */
    RF1_DROP_60 = 0x00400000, /*!< モンスター特性: 落とすアイテム数60%で+1/ Drop an item/gold (60%) */
    RF1_DROP_90 = 0x00800000, /*!< モンスター特性: 落とすアイテム数90%で+1 / Drop an item/gold (90%) */
    RF1_DROP_1D2 = 0x01000000, /*!< モンスター特性: 落とすアイテム数+1d2 / Drop 1d2 items/gold */
    RF1_DROP_2D2 = 0x02000000, /*!< モンスター特性: 落とすアイテム数+2d2 / Drop 2d2 items/gold */
    RF1_DROP_3D2 = 0x04000000, /*!< モンスター特性: 落とすアイテム数+3d2 / Drop 3d2 items/gold */
    RF1_DROP_4D2 = 0x08000000, /*!< モンスター特性: 落とすアイテム数+4d2 / Drop 4d2 items/gold */
    RF1_DROP_GOOD = 0x10000000, /*!< モンスター特性: 必ず上質品をドロップする / Drop good items */
    RF1_DROP_GREAT = 0x20000000, /*!< モンスター特性: 必ず高級品をドロップする / Drop great items */
};

enum class SavedataLoreOlderThan19FlagType {
    RF2_PASS_WALL = 0x00040000, /*!< モンスター特性: 壁を抜けることができる / Monster can pass walls */
    RF2_KILL_WALL = 0x00080000, /*!< モンスター特性: 壁を破壊して進む / Monster can destroy walls */
};

enum class OldQuestId15 {
    CITY_SEA = 17,
};
