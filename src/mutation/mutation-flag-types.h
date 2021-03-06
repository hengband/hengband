#pragma once

enum racial_flag_type {
	MUT1_SPIT_ACID = 0x00000001L, /*!< 突然変異: 酸の唾 */
	MUT1_BR_FIRE = 0x00000002L, /*!< 突然変異: 炎のブレス */
	MUT1_HYPN_GAZE = 0x00000004L, /*!< 突然変異: 催眠睨み */
	MUT1_TELEKINES = 0x00000008L, /*!< 突然変異: 念動力 */
	MUT1_VTELEPORT = 0x00000010L, /*!< 突然変異: テレポート / Voluntary teleport */
	MUT1_MIND_BLST = 0x00000020L, /*!< 突然変異: 精神攻撃 */
	MUT1_RADIATION = 0x00000040L, /*!< 突然変異: 放射能 */
	MUT1_VAMPIRISM = 0x00000080L, /*!< 突然変異: 吸血 */
	MUT1_SMELL_MET = 0x00000100L, /*!< 突然変異: 金属嗅覚 */
	MUT1_SMELL_MON = 0x00000200L, /*!< 突然変異: 敵臭嗅覚 */
	MUT1_BLINK = 0x00000400L, /*!< 突然変異: ショート・テレポート */
	MUT1_EAT_ROCK = 0x00000800L, /*!< 突然変異: 岩喰い */
	MUT1_SWAP_POS = 0x00001000L, /*!< 突然変異: 位置交換 */
	MUT1_SHRIEK = 0x00002000L, /*!< 突然変異: 叫び */
	MUT1_ILLUMINE = 0x00004000L, /*!< 突然変異: 照明 */
	MUT1_DET_CURSE = 0x00008000L, /*!< 突然変異: 呪い感知 */
	MUT1_BERSERK = 0x00010000L, /*!< 突然変異: 狂戦士化 */
	MUT1_POLYMORPH = 0x00020000L, /*!< 突然変異: 変身 */
	MUT1_MIDAS_TCH = 0x00040000L, /*!< 突然変異: ミダスの手 */
	MUT1_GROW_MOLD = 0x00080000L, /*!< 突然変異: カビ発生 */
	MUT1_RESIST = 0x00100000L, /*!< 突然変異: エレメント耐性 */
	MUT1_EARTHQUAKE = 0x00200000L, /*!< 突然変異: 地震 */
	MUT1_EAT_MAGIC = 0x00400000L, /*!< 突然変異: 魔力喰い */
	MUT1_WEIGH_MAG = 0x00800000L, /*!< 突然変異: 魔力感知 */
	MUT1_STERILITY = 0x01000000L, /*!< 突然変異: 増殖阻止 */
	MUT1_HIT_AND_AWAY = 0x02000000L, /*!< 突然変異: ヒットアンドアウェイ */
	MUT1_DAZZLE = 0x04000000L, /*!< 突然変異: 眩惑 */
	MUT1_LASER_EYE = 0x08000000L, /*!< 突然変異: レーザー・アイ */
	MUT1_RECALL = 0x10000000L, /*!< 突然変異: 帰還 */
	MUT1_BANISH = 0x20000000L, /*!< 突然変異: 邪悪消滅 */
	MUT1_COLD_TOUCH = 0x40000000L, /*!< 突然変異: 凍結の手 */
	MUT1_LAUNCHER = 0x80000000L, /*!< 突然変異: アイテム投げ */
};

enum mutation_flag_type_1 {
	MUT2_BERS_RAGE = 0x00000001L, /*!< 突然変異: 狂戦士化の発作 */
	MUT2_COWARDICE = 0x00000002L, /*!< 突然変異: 臆病 */
	MUT2_RTELEPORT = 0x00000004L, /*!< 突然変異: ランダムテレポート / Random teleport, instability */
	MUT2_ALCOHOL = 0x00000008L, /*!< 突然変異: アルコール分泌 */
	MUT2_HALLU = 0x00000010L, /*!< 突然変異: 幻覚を引き起こす精神錯乱 */
	MUT2_FLATULENT = 0x00000020L, /*!< 突然変異: 猛烈な屁 */
	MUT2_SCOR_TAIL = 0x00000040L, /*!< 突然変異: サソリの尻尾 */
	MUT2_HORNS = 0x00000080L, /*!< 突然変異: ツノ */
	MUT2_BEAK = 0x00000100L, /*!< 突然変異: クチバシ */
	MUT2_ATT_DEMON = 0x00000200L, /*!< 突然変異: デーモンを引き付ける */
	MUT2_PROD_MANA = 0x00000400L, /*!< 突然変異: 制御できない魔力のエネルギー */
	MUT2_SPEED_FLUX = 0x00000800L, /*!< 突然変異: ランダムな加減速 */
	MUT2_BANISH_ALL = 0x00001000L, /*!< 突然変異: ランダムなモンスター消滅 */
	MUT2_EAT_LIGHT = 0x00002000L, /*!< 突然変異: 光源喰い */
	MUT2_TRUNK = 0x00004000L, /*!< 突然変異: 象の鼻 */
	MUT2_ATT_ANIMAL = 0x00008000L, /*!< 突然変異: 動物を引き寄せる */
	MUT2_TENTACLES = 0x00010000L, /*!< 突然変異: 邪悪な触手 */
	MUT2_RAW_CHAOS = 0x00020000L, /*!< 突然変異: 純カオス */
	MUT2_NORMALITY = 0x00040000L, /*!< 突然変異: ランダムな変異の消滅 */
	MUT2_WRAITH = 0x00080000L, /*!< 突然変異: ランダムな幽体化 */
	MUT2_POLY_WOUND = 0x00100000L, /*!< 突然変異: ランダムな傷の変化 */
	MUT2_WASTING = 0x00200000L, /*!< 突然変異: 衰弱 */
	MUT2_ATT_DRAGON = 0x00400000L, /*!< 突然変異: ドラゴンを引き寄せる */
	MUT2_WEIRD_MIND = 0x00800000L, /*!< 突然変異: ランダムなテレパシー */
	MUT2_NAUSEA = 0x01000000L, /*!< 突然変異: 落ち着きの無い胃 */
	MUT2_CHAOS_GIFT = 0x02000000L, /*!< 突然変異: カオスパトロン */
	MUT2_WALK_SHAD = 0x04000000L, /*!< 突然変異: ランダムな現実変容 */
	MUT2_WARNING = 0x08000000L, /*!< 突然変異: 警告 */
	MUT2_INVULN = 0x10000000L, /*!< 突然変異: ランダムな無敵化 */
	MUT2_SP_TO_HP = 0x20000000L, /*!< 突然変異: ランダムなMPからHPへの変換 */
	MUT2_HP_TO_SP = 0x40000000L, /*!< 突然変異: ランダムなHPからMPへの変換 */
	MUT2_DISARM = 0x80000000L, /*!< 突然変異: ランダムな武器落とし */
};

enum mutation_flag_type_2 {
	MUT3_HYPER_STR = 0x00000001L, /*!< 突然変異: 超人的な力 */
	MUT3_PUNY = 0x00000002L, /*!< 突然変異: 虚弱 */
	MUT3_HYPER_INT = 0x00000004L, /*!< 突然変異: 生体コンピュータ */
	MUT3_MORONIC = 0x00000008L, /*!< 突然変異: 精神薄弱 */
	MUT3_RESILIENT = 0x00000010L, /*!< 突然変異: 弾力のある体 */
	MUT3_XTRA_FAT = 0x00000020L, /*!< 突然変異: 異常な肥満 */
	MUT3_ALBINO = 0x00000040L, /*!< 突然変異: アルビノ */
	MUT3_FLESH_ROT = 0x00000080L, /*!< 突然変異: 腐敗した肉体 */
	MUT3_SILLY_VOI = 0x00000100L, /*!< 突然変異: 間抜けなキーキー声 */
	MUT3_BLANK_FAC = 0x00000200L, /*!< 突然変異: のっぺらぼう */
	MUT3_ILL_NORM = 0x00000400L, /*!< 突然変異: 幻影に覆われた体 */
	MUT3_XTRA_EYES = 0x00000800L, /*!< 突然変異: 第三の目 */
	MUT3_MAGIC_RES = 0x00001000L, /*!< 突然変異: 魔法防御 */
	MUT3_XTRA_NOIS = 0x00002000L, /*!< 突然変異: 騒音 */
	MUT3_INFRAVIS = 0x00004000L, /*!< 突然変異: 赤外線視力 */
	MUT3_XTRA_LEGS = 0x00008000L, /*!< 突然変異: 追加の脚 */
	MUT3_SHORT_LEG = 0x00010000L, /*!< 突然変異: 短い脚 */
	MUT3_ELEC_TOUC = 0x00020000L, /*!< 突然変異: 電撃オーラ */
	MUT3_FIRE_BODY = 0x00040000L, /*!< 突然変異: 火炎オーラ */
	MUT3_WART_SKIN = 0x00080000L, /*!< 突然変異: イボ肌 */
	MUT3_SCALES = 0x00100000L, /*!< 突然変異: 鱗肌 */
	MUT3_IRON_SKIN = 0x00200000L, /*!< 突然変異: 鉄の肌 */
	MUT3_WINGS = 0x00400000L, /*!< 突然変異: 翼 */
	MUT3_FEARLESS = 0x00800000L, /*!< 突然変異: 恐れ知らず */
	MUT3_REGEN = 0x01000000L, /*!< 突然変異: 急回復 */
	MUT3_ESP = 0x02000000L, /*!< 突然変異: テレパシー */
	MUT3_LIMBER = 0x04000000L, /*!< 突然変異: しなやかな肉体 */
	MUT3_ARTHRITIS = 0x08000000L, /*!< 突然変異: 関節の痛み */
	MUT3_BAD_LUCK = 0x10000000L, /*!< 突然変異: 黒いオーラ(不運) */
	MUT3_VULN_ELEM = 0x20000000L, /*!< 突然変異: 元素攻撃弱点 */
	MUT3_MOTION = 0x40000000L, /*!< 突然変異: 正確で力強い動作 */
	MUT3_GOOD_LUCK = 0x80000000L, /*!< 突然変異: 白いオーラ(幸運) */
};
