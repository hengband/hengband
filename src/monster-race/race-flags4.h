#pragma once

enum race_flags4 {
	RF4_SHRIEK = 0x00000001, /*!< モンスター能力: 叫ぶ / Shriek for help */
    RF4_XXX1 = 0x00000002, /*!< モンスター能力: 未使用 / XXX */
    RF4_DISPEL = 0x00000004, /*!< モンスター能力: 魔力消去 / Dispel magic */
    RF4_ROCKET = 0x00000008, /*!< モンスター能力: ロケット / TY: Rocket */
    RF4_SHOOT = 0x00000010, /*!< モンスター能力: 射撃/ Fire missiles */
    RF4_XXX2 = 0x00000020, /*!< モンスター能力: 未使用 / XXX */
    RF4_XXX3 = 0x00000040, /*!< モンスター能力: 未使用 / XXX */
    RF4_XXX4 = 0x00000080, /*!< モンスター能力: 未使用 / XXX */
    RF4_BR_ACID = 0x00000100, /*!< モンスター能力: 酸のブレス / Breathe Acid */
    RF4_BR_ELEC = 0x00000200, /*!< モンスター能力: 電撃のブレス / Breathe Elec */
    RF4_BR_FIRE = 0x00000400, /*!< モンスター能力: 火炎のブレス / Breathe Fire */
    RF4_BR_COLD = 0x00000800, /*!< モンスター能力: 冷気のブレス / Breathe Cold */
    RF4_BR_POIS = 0x00001000, /*!< モンスター能力: 毒のブレス / Breathe Poison */
    RF4_BR_NETH = 0x00002000, /*!< モンスター能力: 地獄のブレス / Breathe Nether */
    RF4_BR_LITE = 0x00004000, /*!< モンスター能力: 閃光のブレス / Breathe Lite */
    RF4_BR_DARK = 0x00008000, /*!< モンスター能力: 暗黒のブレス / Breathe Dark */
    RF4_BR_CONF = 0x00010000, /*!< モンスター能力: 混乱のブレス / Breathe Confusion */
    RF4_BR_SOUN = 0x00020000, /*!< モンスター能力: 轟音のブレス / Breathe Sound */
    RF4_BR_CHAO = 0x00040000, /*!< モンスター能力: カオスのブレス / Breathe Chaos */
    RF4_BR_DISE = 0x00080000, /*!< モンスター能力: 劣化のブレス / Breathe Disenchant */
    RF4_BR_NEXU = 0x00100000, /*!< モンスター能力: 因果混乱のブレス / Breathe Nexus */
    RF4_BR_TIME = 0x00200000, /*!< モンスター能力: 時間逆転のブレス / Breathe Time */
    RF4_BR_INER = 0x00400000, /*!< モンスター能力: 遅鈍のブレス / Breathe Inertia */
    RF4_BR_GRAV = 0x00800000, /*!< モンスター能力: 重力のブレス / Breathe Gravity */
    RF4_BR_SHAR = 0x01000000, /*!< モンスター能力: 破片のブレス / Breathe Shards */
    RF4_BR_PLAS = 0x02000000, /*!< モンスター能力: プラズマのブレス / Breathe Plasma */
    RF4_BR_WALL = 0x04000000, /*!< モンスター能力: フォースのブレス / Breathe Force */
    RF4_BR_MANA = 0x08000000, /*!< モンスター能力: 魔力のブレス / Breathe Mana */
    RF4_BA_NUKE = 0x10000000, /*!< モンスター能力: 放射能球 / TY: Nuke Ball */
    RF4_BR_NUKE = 0x20000000, /*!< モンスター能力: 放射性廃棄物のブレス / TY: Toxic Breath */
    RF4_BA_CHAO = 0x40000000, /*!< モンスター能力: ログルス球 / TY: Logrus Ball */
    RF4_BR_DISI = 0x80000000, /*!< モンスター能力: 分解のブレス / Breathe Disintegration */
};
