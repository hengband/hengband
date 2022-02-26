#pragma once

enum race_flags3 {
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
    RF3_XX10 = 0x00000400, /*!< 予約領域。元冷気オーラ */
    RF3_NONLIVING = 0x00000800, /*!< モンスター特性: 無生物 / TY: Non-Living (?) */
    RF3_HURT_LITE = 0x00001000, /*!< モンスター特性: 通常の光(GF_WEAK_LITE)でダメージを受ける / Hurt by lite */
    RF3_HURT_ROCK = 0x00002000, /*!< モンスター特性: 岩石溶解(KILL_WALL)でダメージを受ける / Hurt by rock remover */
    RF3_HURT_FIRE = 0x00004000, /*!< モンスター特性: 火炎が弱点 / Hurt badly by fire */
    RF3_HURT_COLD = 0x00008000, /*!< モンスター特性: 冷気が弱点 / Hurt badly by cold */
    RF3_ANGEL = 0x00010000, /*!< モンスター特性: 天使 / ANGEL */
    RF3_XXX17 = 0x00020000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX18 = 0x00040000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX19 = 0x00080000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX20 = 0x00100000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX21 = 0x00200000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX22 = 0x00400000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX23 = 0x00800000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX24 = 0x01000000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX25 = 0x02000000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX26 = 0x04000000, /*!< モンスター特性: 未使用 / XXX */
    RF3_XXX27 = 0x08000000, /*!< モンスター特性: 未使用 / XXX */
    RF3_NO_FEAR = 0x10000000, /*!< モンスター特性: 恐怖しない / Cannot be scared */
    RF3_NO_STUN = 0x20000000, /*!< モンスター特性: 朦朧としない / Cannot be stunned */
    RF3_NO_CONF = 0x40000000, /*!< モンスター特性: 混乱しない / Cannot be confused and resist confusion */
    RF3_NO_SLEEP = 0x80000000, /*!< モンスター特性: 眠らない / Cannot be slept */
};
