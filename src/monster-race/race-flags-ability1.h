#pragma once

enum race_flags_ability {
	RF5_BA_ACID = 0x00000001, /*!< モンスター能力: アシッド・ボール / Acid Ball */
    RF5_BA_ELEC = 0x00000002, /*!< モンスター能力: サンダー・ボール / Elec Ball */
    RF5_BA_FIRE = 0x00000004, /*!< モンスター能力: ファイア・ボール / Fire Ball */
    RF5_BA_COLD = 0x00000008, /*!< モンスター能力: アイス・ボール / Cold Ball */
    RF5_BA_POIS = 0x00000010, /*!< モンスター能力: 悪臭雲 / Poison Ball */
    RF5_BA_NETH = 0x00000020, /*!< モンスター能力: 地獄球 / Nether Ball */
    RF5_BA_WATE = 0x00000040, /*!< モンスター能力: ウォーター・ボール / Water Ball */
    RF5_BA_MANA = 0x00000080, /*!< モンスター能力: 魔力の嵐 / Mana Storm */
    RF5_BA_DARK = 0x00000100, /*!< モンスター能力: 暗黒の嵐 / Darkness Storm */
    RF5_DRAIN_MANA = 0x00000200, /*!< モンスター能力: 魔力吸収 / Drain Mana */
    RF5_MIND_BLAST = 0x00000400, /*!< モンスター能力: 精神攻撃 / Blast Mind */
    RF5_BRAIN_SMASH = 0x00000800, /*!< モンスター能力: 脳攻撃 / Smash Brain */
    RF5_CAUSE_1 = 0x00001000, /*!< モンスター能力: 軽傷の呪い / Cause Light Wound */
    RF5_CAUSE_2 = 0x00002000, /*!< モンスター能力: 重症の頃い / Cause Serious Wound */
    RF5_CAUSE_3 = 0x00004000, /*!< モンスター能力: 致命傷の呪い / Cause Critical Wound */
    RF5_CAUSE_4 = 0x00008000, /*!< モンスター能力: 秘孔を突く / Cause Mortal Wound */
    RF5_BO_ACID = 0x00010000, /*!< モンスター能力: アシッド・ボルト / Acid Bolt */
    RF5_BO_ELEC = 0x00020000, /*!< モンスター能力: サンダー・ボルト / Elec Bolt */
    RF5_BO_FIRE = 0x00040000, /*!< モンスター能力: ファイア・ボルト / Fire Bolt */
    RF5_BO_COLD = 0x00080000, /*!< モンスター能力: アイス・ボルト / Cold Bolt */
    RF5_BA_LITE = 0x00100000, /*!< モンスター能力: スター・バースト / StarBurst */
    RF5_BO_NETH = 0x00200000, /*!< モンスター能力: 地獄の矢 / Nether Bolt */
    RF5_BO_WATE = 0x00400000, /*!< モンスター能力: ウォーター・ボルト / Water Bolt */
    RF5_BO_MANA = 0x00800000, /*!< モンスター能力: 魔力の矢 / Mana Bolt */
    RF5_BO_PLAS = 0x01000000, /*!< モンスター能力: プラズマ・ボルト / Plasma Bolt */
    RF5_BO_ICEE = 0x02000000, /*!< モンスター能力: 極寒の矢 / Ice Bolt */
    RF5_MISSILE = 0x04000000, /*!< モンスター能力: マジック・ミサイルt / Magic Missile */
    RF5_SCARE = 0x08000000, /*!< モンスター能力: 恐慌 / Frighten Player */
    RF5_BLIND = 0x10000000, /*!< モンスター能力: 盲目 / Blind Player */
    RF5_CONF = 0x20000000, /*!< モンスター能力: 混乱 / Confuse Player */
    RF5_SLOW = 0x40000000, /*!< モンスター能力: 減速 / Slow Player */
    RF5_HOLD = 0x80000000, /*!< モンスター能力: 麻痺 / Paralyze Player */
};
