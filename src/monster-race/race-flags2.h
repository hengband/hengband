#pragma once

enum race_flags2 {
	RF2_STUPID = 0x00000001, /*!< モンスター特性: 愚かな行動を取る / Monster is stupid */
    RF2_SMART = 0x00000002, /*!< モンスター特性: 賢い行動を取る / Monster is smart */
    RF2_CAN_SPEAK = 0x00000004, /*!< モンスター特性: 台詞をしゃべる / TY: can speak */
    RF2_REFLECTING = 0x00000008, /*!< モンスター特性: 矢やボルト魔法を反射する / Reflects bolts */
    RF2_INVISIBLE = 0x00000010, /*!< モンスター特性: 透明視力がないと見えない / Monster avoids vision */
    RF2_COLD_BLOOD = 0x00000020, /*!< モンスター特性: 冷血動物である / Monster avoids infra */
    RF2_EMPTY_MIND = 0x00000040, /*!< モンスター特性: 知性を持たない(テレパシー回避) / Monster avoids telepathy */
    RF2_WEIRD_MIND = 0x00000080, /*!< モンスター特性: 異質な知性(テレパシーで感知づらい) / Monster avoids telepathy? */
    RF2_MULTIPLY = 0x00000100, /*!< モンスター特性: 増殖する / Monster reproduces */
    RF2_REGENERATE = 0x00000200, /*!< モンスター特性: 急激に回復する / Monster regenerates */
    RF2_CHAR_MULTI = 0x00000400, /*!< モンスター特性: 未使用 / (Not implemented) */
    RF2_ATTR_ANY = 0x00000800, /*!< モンスター特性: ATTR_MULTIの色数が増える / TY: Attr_any */
    RF2_POWERFUL = 0x00001000, /*!< モンスター特性: 強力に魔法をあやつる / Monster has strong breath */
    RF2_ELDRITCH_HORROR = 0x00002000, /*!< モンスター特性: 狂気を呼び起こす / Sanity-blasting horror    */
    RF2_XX14 = 0x00004000, /*!< 予約領域。元火炎オーラ */
    RF2_XX15 = 0x00008000, /*!< 予約領域。元電撃オーラ */
    RF2_OPEN_DOOR = 0x00010000, /*!< モンスター特性: ドアを開けることができる / Monster can open doors */
    RF2_BASH_DOOR = 0x00020000, /*!< モンスター特性: ドアを破壊することができる / Monster can bash doors */
    RF2_PASS_WALL = 0x00040000, /*!< モンスター特性: 壁を抜けることができる / Monster can pass walls */
    RF2_KILL_WALL = 0x00080000, /*!< モンスター特性: 壁を破壊して進む / Monster can destroy walls */
    RF2_MOVE_BODY = 0x00100000, /*!< モンスター特性: 道中の弱いモンスターを押しのけることができる / Monster can move monsters */
    RF2_KILL_BODY = 0x00200000, /*!< モンスター特性: 道中の弱いモンスターを殺して進む / Monster can kill monsters */
    RF2_TAKE_ITEM = 0x00400000, /*!< モンスター特性: 道中のアイテムを拾う / Monster can pick up items */
    RF2_KILL_ITEM = 0x00800000, /*!< モンスター特性: 道中のアイテムを破壊する / Monster can crush items */
    RF2_XXX1 = 0x01000000, /*!< モンスター特性: 未使用 / XXX */
    RF2_XXX2 = 0x02000000, /*!< モンスター特性: 未使用 / XXX */
    RF2_XXX3 = 0x04000000, /*!< モンスター特性: 未使用 / XXX */
    RF2_XXX4 = 0x08000000, /*!< モンスター特性: 未使用 / XXX */
    RF2_XXX5 = 0x10000000, /*!< モンスター特性: 未使用 / XXX */
    RF2_XXX6 = 0x20000000, /*!< モンスター特性: 未使用 / XXX */
    RF2_HUMAN = 0x40000000, /*!< モンスター特性: 人間 / Human */
    RF2_QUANTUM = 0x80000000, /*!< モンスター特性: 量子的な振る舞いをする / Monster has quantum behavior */
};
