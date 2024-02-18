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

enum class SavedataLoreOlderThan11FlagType : uint32_t {
    RF1_NEVER_BLOW = 0x00010000, /*!< モンスター特性: 打撃を一切行わない / Never make physical blow */
    RF1_NEVER_MOVE = 0x00020000, /*!< モンスター特性: 移動を一切行わない / Never make physical move */
    RF1_RAND_25 = 0x00040000, /*!< モンスター特性: ランダムに移動する確率+25%/ Moves randomly (25%) */
    RF1_RAND_50 = 0x00080000, /*!< モンスター特性: ランダムに移動する確率+50%/ Moves randomly (50%) */
    RF2_STUPID = 0x00000001, /*!< モンスター特性: 愚かな行動を取る / Monster is stupid */
    RF2_SMART = 0x00000002, /*!< モンスター特性: 賢い行動を取る / Monster is smart */
    RF2_OPEN_DOOR = 0x00010000, /*!< モンスター特性: ドアを開けることができる / Monster can open doors */
    RF2_BASH_DOOR = 0x00020000, /*!< モンスター特性: ドアを破壊することができる / Monster can bash doors */
    RF2_PASS_WALL = 0x00040000, /*!< モンスター特性: 壁を抜けることができる / Monster can pass walls */
    RF2_KILL_WALL = 0x00080000, /*!< モンスター特性: 壁を破壊して進む / Monster can destroy walls */
    RF2_MOVE_BODY = 0x00100000, /*!< モンスター特性: 道中の弱いモンスターを押しのけることができる / Monster can move monsters */
    RF2_KILL_BODY = 0x00200000, /*!< モンスター特性: 道中の弱いモンスターを殺して進む / Monster can kill monsters */
    RF2_TAKE_ITEM = 0x00400000, /*!< モンスター特性: 道中のアイテムを拾う / Monster can pick up items */
    RF2_KILL_ITEM = 0x00800000, /*!< モンスター特性: 道中のアイテムを破壊する / Monster can crush items */
};

enum class SavedataLoreOlderThan12FlagType : uint32_t {
    RF1_UNIQUE = 0x00000001, /*!< モンスター特性: ユニーク / Unique Monster */
    RF1_QUESTOR = 0x00000002, /*!< モンスター特性: クエストモンスター / Quest Monster */
    RF2_HUMAN = 0x40000000, /*!< モンスター特性: 人間 / Human */
    RF2_QUANTUM = 0x80000000, /*!< モンスター特性: 量子的な振る舞いをする / Monster has quantum behavior */
    RF3_ORC = 0x00000001, /*!< モンスター特性: オーク / Orc */
    RF3_TROLL = 0x00000002, /*!< モンスター特性: トロル / Troll */
    RF3_GIANT = 0x00000004, /*!< モンスター特性: 巨人 / Giant */
    RF3_DRAGON = 0x00000008, /*!< モンスター特性: ドラゴン / Dragon */
    RF3_DEMON = 0x00000010, /*!< モンスター特性: 悪魔 / Demon */
    RF3_UNDEAD = 0x00000020, /*!< モンスター特性: アンデッド / Undead */
    RF3_EVIL = 0x00000040, /*!< モンスター特性: 邪悪 / Evil */
    RF3_ANIMAL = 0x00000080, /*!< モンスター特性: 動物 / Animal */
    RF3_AMBERITE = 0x00000100, /*!< モンスター特性: アンバーの血族 / TY: Amberite */
    RF3_GOOD = 0x00000200, /*!< モンスター特性: 善良 / Good */
    RF3_ANGEL = 0x00010000, /*!< モンスター特性: 天使 / ANGEL */
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

enum class SavedataLoreOlderThan19FlagType_No_Debuff : uint32_t {
    RF3_NO_FEAR = 0x10000000, /*!< モンスター特性: 恐怖しない / Cannot be scared */
    RF3_NO_STUN = 0x20000000, /*!< モンスター特性: 朦朧としない / Cannot be stunned */
    RF3_NO_CONF = 0x40000000, /*!< モンスター特性: 混乱しない / Cannot be confused and resist confusion */
    RF3_NO_SLEEP = 0x80000000, /*!< モンスター特性: 眠らない / Cannot be slept */
};

enum class SavedataLoreOlderThan20FlagType {
    RF1_QUESTOR = 0x00000002, /*!< モンスター特性: クエストモンスター / Quest Monster */
    RF1_FORCE_DEPTH = 0x00000100, /*!< モンスター特性: 指定階未満では生成されない / Start at "correct" depth */
    RF1_FORCE_MAXHP = 0x00000200, /*!< モンスター特性: 通常生成時必ずHPがダイス最大値になる / Start with max hitpoints */
    RF1_FRIENDS = 0x00002000, /*!< モンスター特性: 同種の友軍を用意している / Arrive with some friends */
    RF1_ESCORT = 0x00004000, /*!< モンスター特性: 護衛を用意している/ Arrive with an escort */
    RF1_ESCORTS = 0x00008000, /*!< モンスター特性: さらに大量の護衛を用意している / Arrive with some escorts */
    RF2_REFLECTING = 0x00000008, /*!< モンスター特性: 矢やボルト魔法を反射する / Reflects bolts */
    RF2_INVISIBLE = 0x00000010, /*!< モンスター特性: 透明視力がないと見えない / Monster avoids vision */
    RF2_COLD_BLOOD = 0x00000020, /*!< モンスター特性: 冷血動物である / Monster avoids infra */
    RF2_EMPTY_MIND = 0x00000040, /*!< モンスター特性: 知性を持たない(テレパシー回避) / Monster avoids telepathy */
    RF2_WEIRD_MIND = 0x00000080, /*!< モンスター特性: 異質な知性(テレパシーで感知づらい) / Monster avoids telepathy? */
    RF2_MULTIPLY = 0x00000100, /*!< モンスター特性: 増殖する / Monster reproduces */
    RF2_REGENERATE = 0x00000200, /*!< モンスター特性: 急激に回復する / Monster regenerates */
    RF2_POWERFUL = 0x00001000, /*!< モンスター特性: 強力に魔法をあやつる / Monster has strong breath */
    RF2_ELDRITCH_HORROR = 0x00002000, /*!< モンスター特性: 狂気を呼び起こす / Sanity-blasting horror    */
};

enum class OldQuestId15 {
    CITY_SEA = 17,
};
