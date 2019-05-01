#pragma once

/*
 * Monster "race" information, including racial memories
 *
 * Note that "d_attr" and "d_char" are used for MORE than "visual" stuff.
 *
 * Note that "x_attr" and "x_char" are used ONLY for "visual" stuff.
 *
 * Note that "cur_num" (and "max_num") represent the number of monsters
 * of the given race currently on (and allowed on) the current level.
 * This information yields the "dead" flag for Unique monsters.
 *
 * Note that "max_num" is reset when a new player is created.
 * Note that "cur_num" is reset when a new level is created.
 *
 * Note that several of these fields, related to "recall", can be
 * scrapped if space becomes an issue, resulting in less "complete"
 * monster recall (no knowledge of spells, etc).  All of the "recall"
 * fields have a special prefix to aid in searching for them.
 */


typedef struct monster_race monster_race;

struct monster_race
{
	STR_OFFSET name;	/*!< 名前データのオフセット(日本語) /  Name offset(Japanese) */
#ifdef JP
	STR_OFFSET E_name;		/*!< 名前データのオフセット(英語) /  Name offset(English) */
#endif
	STR_OFFSET text;		/*!< 思い出テキストのオフセット / Lore text offset */

	DICE_NUMBER hdice;		/*!< HPのダイス数 / Creatures hit dice count */
	DICE_SID hside;			/*!< HPのダイス面数 / Creatures hit dice sides */

	ARMOUR_CLASS ac;		/*!< アーマークラス / Armour Class */

	SLEEP_DEGREE sleep;				/*!< 睡眠値 / Inactive counter (base) */
	POSITION aaf;				/*!< 感知範囲(1-100スクエア) / Area affect radius (1-100) */
	SPEED speed;				/*!< 加速(110で+0) / Speed (normally 110) */

	EXP mexp;				/*!< 殺害時基本経験値 / Exp value for kill */

	BIT_FLAGS16 extra;				/*!< 未使用 /  Unused (for now) */

	RARITY freq_spell;		/*!< 魔法＆特殊能力仕様頻度(1/n) /  Spell frequency */

	BIT_FLAGS flags1;			/* Flags 1 (general) */
#define RF1_UNIQUE              0x00000001  /*!< モンスター特性: ユニーク / Unique Monster */
#define RF1_QUESTOR             0x00000002  /*!< モンスター特性: クエストモンスター / Quest Monster */
#define RF1_MALE                0x00000004  /*!< モンスター特性: 男性 / Male gender */
#define RF1_FEMALE              0x00000008  /*!< モンスター特性: 女性 / Female gender */
#define RF1_CHAR_CLEAR          0x00000010  /*!< モンスター特性: シンボルが完全に透明 / Absorbs symbol */
#define RF1_SHAPECHANGER        0x00000020  /*!< モンスター特性: シンボルアルファベットがランダムになる / TY: shapechanger */
#define RF1_ATTR_CLEAR          0x00000040  /*!< モンスター特性: シンボルカラーが透明色になる(地形と同じ色になる) / Absorbs color */
#define RF1_ATTR_MULTI          0x00000080  /*!< モンスター特性: シンボルカラーがランダムに変化する(基本7色) / Changes color */
#define RF1_FORCE_DEPTH         0x00000100  /*!< モンスター特性: 指定階未満では生成されない / Start at "correct" depth */
#define RF1_FORCE_MAXHP         0x00000200  /*!< モンスター特性: 通常生成時必ずHPがダイス最大値になる / Start with max hitpoints */
#define RF1_FORCE_SLEEP         0x00000400  /*!< モンスター特性: 通常生成時必ず寝ている / Start out sleeping */
#define RF1_FORCE_EXTRA         0x00000800  /*!< モンスター特性: (未使用) / Start out something */
#define RF1_ATTR_SEMIRAND       0x00001000  /*!< モンスター特性: シンボルカラーがランダムに変化する(15色) / Color is determined semi-randomly */
#define RF1_FRIENDS             0x00002000  /*!< モンスター特性: 同種の友軍を用意している / Arrive with some friends */
#define RF1_ESCORT              0x00004000  /*!< モンスター特性: 護衛を用意している/ Arrive with an escort */
#define RF1_ESCORTS             0x00008000  /*!< モンスター特性: さらに大量の護衛を用意している / Arrive with some escorts */
#define RF1_NEVER_BLOW          0x00010000  /*!< モンスター特性: 打撃を一切行わない / Never make physical blow */
#define RF1_NEVER_MOVE          0x00020000  /*!< モンスター特性: 移動を一切行わない / Never make physical move */
#define RF1_RAND_25             0x00040000  /*!< モンスター特性: ランダムに移動する確率+25%/ Moves randomly (25%) */
#define RF1_RAND_50             0x00080000  /*!< モンスター特性: ランダムに移動する確率+50%/ Moves randomly (50%) */
#define RF1_ONLY_GOLD           0x00100000  /*!< モンスター特性: 財宝しか落とさない / Drop only gold */
#define RF1_ONLY_ITEM           0x00200000  /*!< モンスター特性: アイテムしか落とさない / Drop only items */
#define RF1_DROP_60             0x00400000  /*!< モンスター特性: 落とすアイテム数60%で+1/ Drop an item/gold (60%) */
#define RF1_DROP_90             0x00800000  /*!< モンスター特性: 落とすアイテム数90%で+1 / Drop an item/gold (90%) */
#define RF1_DROP_1D2            0x01000000  /*!< モンスター特性: 落とすアイテム数+1d2 / Drop 1d2 items/gold */
#define RF1_DROP_2D2            0x02000000  /*!< モンスター特性: 落とすアイテム数+2d2 / Drop 2d2 items/gold */
#define RF1_DROP_3D2            0x04000000  /*!< モンスター特性: 落とすアイテム数+3d2 / Drop 3d2 items/gold */
#define RF1_DROP_4D2            0x08000000  /*!< モンスター特性: 落とすアイテム数+4d2 / Drop 4d2 items/gold */
#define RF1_DROP_GOOD           0x10000000  /*!< モンスター特性: 必ず上質品をドロップする / Drop good items */
#define RF1_DROP_GREAT          0x20000000  /*!< モンスター特性: 必ず高級品をドロップする / Drop great items */
#define RF1_XXX2                0x40000000  /*!< モンスター特性: 未使用 / XXX */
#define RF1_XXX3                0x80000000  /*!< モンスター特性: 未使用 / XXX */

	BIT_FLAGS flags2;			/* Flags 2 (abilities) */
#define RF2_STUPID          0x00000001  /*!< モンスター特性: 愚かな行動を取る / Monster is stupid */
#define RF2_SMART           0x00000002  /*!< モンスター特性: 賢い行動を取る / Monster is smart */
#define RF2_CAN_SPEAK       0x00000004  /*!< モンスター特性: 台詞をしゃべる / TY: can speak */
#define RF2_REFLECTING      0x00000008  /*!< モンスター特性: 矢やボルト魔法を反射する / Reflects bolts */
#define RF2_INVISIBLE       0x00000010  /*!< モンスター特性: 透明視力がないと見えない / Monster avoids vision */
#define RF2_COLD_BLOOD      0x00000020  /*!< モンスター特性: 冷血動物である / Monster avoids infra */
#define RF2_EMPTY_MIND      0x00000040  /*!< モンスター特性: 知性を持たない(テレパシー回避) / Monster avoids telepathy */
#define RF2_WEIRD_MIND      0x00000080  /*!< モンスター特性: 異質な知性(テレパシーで感知づらい) / Monster avoids telepathy? */
#define RF2_MULTIPLY        0x00000100  /*!< モンスター特性: 増殖する / Monster reproduces */
#define RF2_REGENERATE      0x00000200  /*!< モンスター特性: 急激に回復する / Monster regenerates */
#define RF2_CHAR_MULTI      0x00000400  /*!< モンスター特性: 未使用 / (Not implemented) */
#define RF2_ATTR_ANY        0x00000800  /*!< モンスター特性: ATTR_MULTIの色数が増える / TY: Attr_any */
#define RF2_POWERFUL        0x00001000  /*!< モンスター特性: 強力に魔法をあやつる / Monster has strong breath */
#define RF2_ELDRITCH_HORROR 0x00002000  /*!< モンスター特性: 狂気を呼び起こす / Sanity-blasting horror    */
#define RF2_AURA_FIRE       0x00004000  /*!< モンスター特性: 火炎のオーラを持つ / Burns in melee */
#define RF2_AURA_ELEC       0x00008000  /*!< モンスター特性: 電撃のオーラを持つ / Shocks in melee */
#define RF2_OPEN_DOOR       0x00010000  /*!< モンスター特性: ドアを開けることができる / Monster can open doors */
#define RF2_BASH_DOOR       0x00020000  /*!< モンスター特性: ドアを破壊することができる / Monster can bash doors */
#define RF2_PASS_WALL       0x00040000  /*!< モンスター特性: 壁を抜けることができる / Monster can pass walls */
#define RF2_KILL_WALL       0x00080000  /*!< モンスター特性: 壁を破壊して進む / Monster can destroy walls */
#define RF2_MOVE_BODY       0x00100000  /*!< モンスター特性: 道中の弱いモンスターを押しのけることができる / Monster can move monsters */
#define RF2_KILL_BODY       0x00200000  /*!< モンスター特性: 道中の弱いモンスターを殺して進む / Monster can kill monsters */
#define RF2_TAKE_ITEM       0x00400000  /*!< モンスター特性: 道中のアイテムを拾う / Monster can pick up items */
#define RF2_KILL_ITEM       0x00800000  /*!< モンスター特性: 道中のアイテムを破壊する / Monster can crush items */
#define RF2_XXX1            0x01000000  /*!< モンスター特性: 未使用 / XXX */
#define RF2_XXX2            0x02000000  /*!< モンスター特性: 未使用 / XXX */
#define RF2_XXX3            0x04000000  /*!< モンスター特性: 未使用 / XXX */
#define RF2_XXX4            0x08000000  /*!< モンスター特性: 未使用 / XXX */
#define RF2_XXX5            0x10000000  /*!< モンスター特性: 未使用 / XXX */
#define RF2_XXX6            0x20000000  /*!< モンスター特性: 未使用 / XXX */
#define RF2_HUMAN           0x40000000  /*!< モンスター特性: 人間 / Human */
#define RF2_QUANTUM         0x80000000  /*!< モンスター特性: 量子的な振る舞いをする / Monster has quantum behavior */

	BIT_FLAGS flags3;			/* Flags 3 (race/resist) */
#define RF3_ORC             0x00000001  /*!< モンスター特性: オーク / Orc */
#define RF3_TROLL           0x00000002  /*!< モンスター特性: トロル / Troll */
#define RF3_GIANT           0x00000004  /*!< モンスター特性: 巨人 / Giant */
#define RF3_DRAGON          0x00000008  /*!< モンスター特性: ドラゴン / Dragon */
#define RF3_DEMON           0x00000010  /*!< モンスター特性: 悪魔 / Demon */
#define RF3_UNDEAD          0x00000020  /*!< モンスター特性: アンデッド / Undead */
#define RF3_EVIL            0x00000040  /*!< モンスター特性: 邪悪 / Evil */
#define RF3_ANIMAL          0x00000080  /*!< モンスター特性: 動物 / Animal */
#define RF3_AMBERITE        0x00000100  /*!< モンスター特性: アンバーの血族 / TY: Amberite */
#define RF3_GOOD            0x00000200  /*!< モンスター特性: 善良 / Good */
#define RF3_AURA_COLD       0x00000400  /*!< モンスター特性: 冷気オーラ / Freezes in melee */
#define RF3_NONLIVING       0x00000800  /*!< モンスター特性: 無生物 / TY: Non-Living (?) */
#define RF3_HURT_LITE       0x00001000  /*!< モンスター特性: 通常の光(GF_WEAK_LITE)でダメージを受ける / Hurt by lite */
#define RF3_HURT_ROCK       0x00002000  /*!< モンスター特性: 岩石溶解(GF_KILL_WALL)でダメージを受ける / Hurt by rock remover */
#define RF3_HURT_FIRE       0x00004000  /*!< モンスター特性: 火炎が弱点 / Hurt badly by fire */
#define RF3_HURT_COLD       0x00008000  /*!< モンスター特性: 冷気が弱点 / Hurt badly by cold */
#define RF3_ANGEL           0x00010000  /*!< モンスター特性: 天使 / ANGEL */
#define RF3_XXX17           0x00020000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX18           0x00040000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX19           0x00080000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX20           0x00100000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX21           0x00200000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX22           0x00400000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX23           0x00800000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX24           0x01000000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX25           0x02000000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX26           0x04000000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_XXX27           0x08000000  /*!< モンスター特性: 未使用 / XXX */
#define RF3_NO_FEAR         0x10000000  /*!< モンスター特性: 恐怖しない / Cannot be scared */
#define RF3_NO_STUN         0x20000000  /*!< モンスター特性: 朦朧としない / Cannot be stunned */
#define RF3_NO_CONF         0x40000000  /*!< モンスター特性: 混乱しない / Cannot be confused and resist confusion */
#define RF3_NO_SLEEP        0x80000000  /*!< モンスター特性: 眠らない / Cannot be slept */

	BIT_FLAGS flags4;			/* Flags 4 (inate/breath) */
#define RF4_SHRIEK          0x00000001  /*!< モンスター能力: 叫ぶ / Shriek for help */
#define RF4_XXX1            0x00000002  /*!< モンスター能力: 未使用 / XXX */
#define RF4_DISPEL          0x00000004  /*!< モンスター能力: 魔力消去 / Dispel magic */
#define RF4_ROCKET          0x00000008  /*!< モンスター能力: ロケット / TY: Rocket */
#define RF4_SHOOT           0x00000010  /*!< モンスター能力: 射撃/ Fire missiles */
#define RF4_XXX2            0x00000020  /*!< モンスター能力: 未使用 / XXX */
#define RF4_XXX3            0x00000040  /*!< モンスター能力: 未使用 / XXX */
#define RF4_XXX4            0x00000080  /*!< モンスター能力: 未使用 / XXX */
#define RF4_BR_ACID         0x00000100  /*!< モンスター能力: 酸のブレス / Breathe Acid */
#define RF4_BR_ELEC         0x00000200  /*!< モンスター能力: 電撃のブレス / Breathe Elec */
#define RF4_BR_FIRE         0x00000400  /*!< モンスター能力: 火炎のブレス / Breathe Fire */
#define RF4_BR_COLD         0x00000800  /*!< モンスター能力: 冷気のブレス / Breathe Cold */
#define RF4_BR_POIS         0x00001000  /*!< モンスター能力: 毒のブレス / Breathe Poison */
#define RF4_BR_NETH         0x00002000  /*!< モンスター能力: 地獄のブレス / Breathe Nether */
#define RF4_BR_LITE         0x00004000  /*!< モンスター能力: 閃光のブレス / Breathe Lite */
#define RF4_BR_DARK         0x00008000  /*!< モンスター能力: 暗黒のブレス / Breathe Dark */
#define RF4_BR_CONF         0x00010000  /*!< モンスター能力: 混乱のブレス / Breathe Confusion */
#define RF4_BR_SOUN         0x00020000  /*!< モンスター能力: 轟音のブレス / Breathe Sound */
#define RF4_BR_CHAO         0x00040000  /*!< モンスター能力: カオスのブレス / Breathe Chaos */
#define RF4_BR_DISE         0x00080000  /*!< モンスター能力: 劣化のブレス / Breathe Disenchant */
#define RF4_BR_NEXU         0x00100000  /*!< モンスター能力: 因果混乱のブレス / Breathe Nexus */
#define RF4_BR_TIME         0x00200000  /*!< モンスター能力: 時間逆転のブレス / Breathe Time */
#define RF4_BR_INER         0x00400000  /*!< モンスター能力: 遅鈍のブレス / Breathe Inertia */
#define RF4_BR_GRAV         0x00800000  /*!< モンスター能力: 重力のブレス / Breathe Gravity */
#define RF4_BR_SHAR         0x01000000  /*!< モンスター能力: 破片のブレス / Breathe Shards */
#define RF4_BR_PLAS         0x02000000  /*!< モンスター能力: プラズマのブレス / Breathe Plasma */
#define RF4_BR_WALL         0x04000000  /*!< モンスター能力: フォースのブレス / Breathe Force */
#define RF4_BR_MANA         0x08000000  /*!< モンスター能力: 魔力のブレス / Breathe Mana */
#define RF4_BA_NUKE         0x10000000  /*!< モンスター能力: 放射能球 / TY: Nuke Ball */
#define RF4_BR_NUKE         0x20000000  /*!< モンスター能力: 放射性廃棄物のブレス / TY: Toxic Breath */
#define RF4_BA_CHAO         0x40000000  /*!< モンスター能力: ログルス球 / TY: Logrus Ball */
#define RF4_BR_DISI         0x80000000  /*!< モンスター能力: 分解のブレス / Breathe Disintegration */

	BIT_FLAGS flags7;			/* Flags 7 (movement related abilities) */
#define RF7_AQUATIC             0x00000001  /* Aquatic monster */
#define RF7_CAN_SWIM            0x00000002  /* Monster can swim */
#define RF7_CAN_FLY             0x00000004  /* Monster can fly */
#define RF7_FRIENDLY            0x00000008  /* Monster is friendly */
#define RF7_NAZGUL              0x00000010  /* Is a "Nazgul" unique */
#define RF7_UNIQUE2             0x00000020  /* Fake unique */
#define RF7_RIDING              0x00000040  /* Good for riding */
#define RF7_KAGE                0x00000080  /* Is kage */
#define RF7_HAS_LITE_1          0x00000100  /* Monster carries light */
#define RF7_SELF_LITE_1         0x00000200  /* Monster lights itself */
#define RF7_HAS_LITE_2          0x00000400  /* Monster carries light */
#define RF7_SELF_LITE_2         0x00000800  /* Monster lights itself */
#define RF7_GUARDIAN            0x00001000  /* Guardian of a dungeon */
#define RF7_CHAMELEON           0x00002000  /* Chameleon can change */
#define RF7_XXXX4XXX            0x00004000  /* Now Empty */
#define RF7_TANUKI              0x00008000  /* Tanuki disguise */
#define RF7_HAS_DARK_1          0x00010000  /* Monster carries darkness */
#define RF7_SELF_DARK_1         0x00020000  /* Monster darkens itself */
#define RF7_HAS_DARK_2          0x00040000  /* Monster carries darkness */
#define RF7_SELF_DARK_2         0x00080000  /* Monster darkens itself */

	BIT_FLAGS flags8;			/* Flags 8 (wilderness info) */
#define RF8_WILD_ONLY           0x00000001
#define RF8_WILD_TOWN           0x00000002
#define RF8_XXX8X02             0x00000004
#define RF8_WILD_SHORE          0x00000008
#define RF8_WILD_OCEAN          0x00000010
#define RF8_WILD_WASTE          0x00000020
#define RF8_WILD_WOOD           0x00000040
#define RF8_WILD_VOLCANO        0x00000080
#define RF8_XXX8X08             0x00000100
#define RF8_WILD_MOUNTAIN       0x00000200
#define RF8_WILD_GRASS          0x00000400
#define RF8_WILD_ALL            0x80000000

	BIT_FLAGS flags9;			/* Flags 9 (drops info) */
#define RF9_DROP_CORPSE         0x00000001
#define RF9_DROP_SKELETON       0x00000002
#define RF9_EAT_BLIND           0x00000004
#define RF9_EAT_CONF            0x00000008
#define RF9_EAT_MANA            0x00000010
#define RF9_EAT_NEXUS           0x00000020
#define RF9_EAT_SLEEP           0x00000040
#define RF9_EAT_BERSERKER       0x00000080
#define RF9_EAT_ACIDIC          0x00000100
#define RF9_EAT_SPEED           0x00000200
#define RF9_EAT_CURE            0x00000400
#define RF9_EAT_FIRE_RES        0x00000800
#define RF9_EAT_COLD_RES        0x00001000
#define RF9_EAT_ACID_RES        0x00002000
#define RF9_EAT_ELEC_RES        0x00004000
#define RF9_EAT_POIS_RES        0x00008000
#define RF9_EAT_INSANITY        0x00010000
#define RF9_EAT_DRAIN_EXP       0x00020000
#define RF9_EAT_POISONOUS       0x00040000
#define RF9_EAT_GIVE_STR        0x00080000
#define RF9_EAT_GIVE_INT        0x00100000
#define RF9_EAT_GIVE_WIS        0x00200000
#define RF9_EAT_GIVE_DEX        0x00400000
#define RF9_EAT_GIVE_CON        0x00800000
#define RF9_EAT_GIVE_CHR        0x01000000
#define RF9_EAT_LOSE_STR        0x02000000
#define RF9_EAT_LOSE_INT        0x04000000
#define RF9_EAT_LOSE_WIS        0x08000000
#define RF9_EAT_LOSE_DEX        0x10000000
#define RF9_EAT_LOSE_CON        0x20000000
#define RF9_EAT_LOSE_CHR        0x40000000
#define RF9_EAT_DRAIN_MANA      0x80000000

	BIT_FLAGS flagsr;			/* Flags R (resistances info) */
#define RFR_IM_ACID         0x00000001  /* Immunity acid */
#define RFR_IM_ELEC         0x00000002  /* Immunity elec */
#define RFR_IM_FIRE         0x00000004  /* Immunity fire */
#define RFR_IM_COLD         0x00000008  /* Immunity cold */
#define RFR_IM_POIS         0x00000010  /* Immunity poison */
#define RFR_RES_LITE        0x00000020  /* Resist lite */
#define RFR_RES_DARK        0x00000040  /* Resist dark */
#define RFR_RES_NETH        0x00000080  /* Resist nether */
#define RFR_RES_WATE        0x00000100  /* Resist water */
#define RFR_RES_PLAS        0x00000200  /* Resist plasma */
#define RFR_RES_SHAR        0x00000400  /* Resist shards */
#define RFR_RES_SOUN        0x00000800  /* Resist sound */
#define RFR_RES_CHAO        0x00001000  /* Resist chaos */
#define RFR_RES_NEXU        0x00002000  /* Resist nexus */
#define RFR_RES_DISE        0x00004000  /* Resist disenchantment */
#define RFR_RES_WALL        0x00008000  /* Resist force */
#define RFR_RES_INER        0x00010000  /* Resist inertia */
#define RFR_RES_TIME        0x00020000  /* Resist time */
#define RFR_RES_GRAV        0x00040000  /* Resist gravity */
#define RFR_RES_ALL         0x00080000  /* Resist all */
#define RFR_RES_TELE        0x00100000  /* Resist teleportation */
#define RFR_XXX21           0x00200000
#define RFR_XXX22           0x00400000
#define RFR_XXX23           0x00800000
#define RFR_XXX24           0x01000000
#define RFR_XXX25           0x02000000
#define RFR_XXX26           0x04000000
#define RFR_XXX27           0x08000000
#define RFR_XXX28           0x10000000
#define RFR_XXX29           0x20000000
#define RFR_XXX30           0x40000000
#define RFR_XXX31           0x80000000

	BIT_FLAGS a_ability_flags1;	/* Activate Ability Flags 5 (normal spells) */
#define RF5_BA_ACID         0x00000001  /*!< モンスター能力: アシッド・ボール / Acid Ball */
#define RF5_BA_ELEC         0x00000002  /*!< モンスター能力: サンダー・ボール / Elec Ball */
#define RF5_BA_FIRE         0x00000004  /*!< モンスター能力: ファイア・ボール / Fire Ball */
#define RF5_BA_COLD         0x00000008  /*!< モンスター能力: アイス・ボール / Cold Ball */
#define RF5_BA_POIS         0x00000010  /*!< モンスター能力: 悪臭雲 / Poison Ball */
#define RF5_BA_NETH         0x00000020  /*!< モンスター能力: 地獄球 / Nether Ball */
#define RF5_BA_WATE         0x00000040  /*!< モンスター能力: ウォーター・ボール / Water Ball */
#define RF5_BA_MANA         0x00000080  /*!< モンスター能力: 魔力の嵐 / Mana Storm */
#define RF5_BA_DARK         0x00000100  /*!< モンスター能力: 暗黒の嵐 / Darkness Storm */
#define RF5_DRAIN_MANA      0x00000200  /*!< モンスター能力: 魔力吸収 / Drain Mana */
#define RF5_MIND_BLAST      0x00000400  /*!< モンスター能力: 精神攻撃 / Blast Mind */
#define RF5_BRAIN_SMASH     0x00000800  /*!< モンスター能力: 脳攻撃 / Smash Brain */
#define RF5_CAUSE_1         0x00001000  /*!< モンスター能力: 軽傷の呪い / Cause Light Wound */
#define RF5_CAUSE_2         0x00002000  /*!< モンスター能力: 重症の頃い / Cause Serious Wound */
#define RF5_CAUSE_3         0x00004000  /*!< モンスター能力: 致命傷の呪い / Cause Critical Wound */
#define RF5_CAUSE_4         0x00008000  /*!< モンスター能力: 秘孔を突く / Cause Mortal Wound */
#define RF5_BO_ACID         0x00010000  /*!< モンスター能力: アシッド・ボルト / Acid Bolt */
#define RF5_BO_ELEC         0x00020000  /*!< モンスター能力: サンダー・ボルト / Elec Bolt */
#define RF5_BO_FIRE         0x00040000  /*!< モンスター能力: ファイア・ボルト / Fire Bolt */
#define RF5_BO_COLD         0x00080000  /*!< モンスター能力: アイス・ボルト / Cold Bolt */
#define RF5_BA_LITE         0x00100000  /*!< モンスター能力: スター・バースト / StarBurst */
#define RF5_BO_NETH         0x00200000  /*!< モンスター能力: 地獄の矢 / Nether Bolt */
#define RF5_BO_WATE         0x00400000  /*!< モンスター能力: ウォーター・ボルト / Water Bolt */
#define RF5_BO_MANA         0x00800000  /*!< モンスター能力: 魔力の矢 / Mana Bolt */
#define RF5_BO_PLAS         0x01000000  /*!< モンスター能力: プラズマ・ボルト / Plasma Bolt */
#define RF5_BO_ICEE         0x02000000  /*!< モンスター能力: 極寒の矢 / Ice Bolt */
#define RF5_MISSILE         0x04000000  /*!< モンスター能力: マジック・ミサイルt / Magic Missile */
#define RF5_SCARE           0x08000000  /*!< モンスター能力: 恐慌 / Frighten Player */
#define RF5_BLIND           0x10000000  /*!< モンスター能力: 盲目 / Blind Player */
#define RF5_CONF            0x20000000  /*!< モンスター能力: 混乱 / Confuse Player */
#define RF5_SLOW            0x40000000  /*!< モンスター能力: 減速 / Slow Player */
#define RF5_HOLD            0x80000000  /*!< モンスター能力: 麻痺 / Paralyze Player */

	BIT_FLAGS a_ability_flags2;	/* Activate Ability Flags 6 (special spells) */
#define RF6_HASTE           0x00000001  /* Speed self */
#define RF6_HAND_DOOM       0x00000002  /* Hand of Doom */
#define RF6_HEAL            0x00000004  /* Heal self */
#define RF6_INVULNER        0x00000008  /* INVULNERABILITY! */
#define RF6_BLINK           0x00000010  /* Teleport Short */
#define RF6_TPORT           0x00000020  /* Teleport Long */
#define RF6_WORLD           0x00000040  /* world */
#define RF6_SPECIAL         0x00000080  /* Special Attack */
#define RF6_TELE_TO         0x00000100  /* Move player to monster */
#define RF6_TELE_AWAY       0x00000200  /* Move player far away */
#define RF6_TELE_LEVEL      0x00000400  /* Move player vertically */
#define RF6_PSY_SPEAR       0x00000800  /* Psyco-spear */
#define RF6_DARKNESS        0x00001000  /* Create Darkness */
#define RF6_TRAPS           0x00002000  /* Create Traps */
#define RF6_FORGET          0x00004000  /* Cause amnesia */
#define RF6_RAISE_DEAD      0x00008000  /* Raise Dead */
#define RF6_S_KIN           0x00010000  /* Summon "kin" */
#define RF6_S_CYBER         0x00020000  /* Summon Cyberdemons! */
#define RF6_S_MONSTER       0x00040000  /* Summon Monster */
#define RF6_S_MONSTERS      0x00080000  /* Summon Monsters */
#define RF6_S_ANT           0x00100000  /* Summon Ants */
#define RF6_S_SPIDER        0x00200000  /* Summon Spiders */
#define RF6_S_HOUND         0x00400000  /* Summon Hounds */
#define RF6_S_HYDRA         0x00800000  /* Summon Hydras */
#define RF6_S_ANGEL         0x01000000  /* Summon Angel */
#define RF6_S_DEMON         0x02000000  /* Summon Demon */
#define RF6_S_UNDEAD        0x04000000  /* Summon Undead */
#define RF6_S_DRAGON        0x08000000  /* Summon Dragon */
#define RF6_S_HI_UNDEAD     0x10000000  /* Summon Greater Undead */
#define RF6_S_HI_DRAGON     0x20000000  /* Summon Ancient Dragon */
#define RF6_S_AMBERITES     0x40000000  /* Summon Amberites */
#define RF6_S_UNIQUE        0x80000000  /* Summon Unique Monster */

	BIT_FLAGS a_ability_flags3;	/* Activate Ability Flags 7 (implementing) */
	BIT_FLAGS a_ability_flags4;	/* Activate Ability Flags 8 (implementing) */

	monster_blow blow[4];	/* Up to four blows per round */
	MONRACE_IDX reinforce_id[6];
	DICE_NUMBER reinforce_dd[6];
	DICE_SID reinforce_ds[6];

	ARTIFACT_IDX artifact_id[4];	/* 特定アーティファクトドロップID */
	RARITY artifact_rarity[4];	/* 特定アーティファクトレア度 */
	PERCENTAGE artifact_percent[4]; /* 特定アーティファクトドロップ率 */

	PERCENTAGE arena_ratio;		/* アリーナの評価修正値(%基準 / 0=100%) / Arena */

	MONRACE_IDX next_r_idx;
	EXP next_exp;

	DEPTH level;			/* Level of creature */
	RARITY rarity;			/* Rarity of creature */

	TERM_COLOR d_attr;		/* Default monster attribute */
	SYMBOL_CODE d_char;			/* Default monster character */

	TERM_COLOR x_attr;		/* Desired monster attribute */
	SYMBOL_CODE x_char;			/* Desired monster character */


	MONSTER_NUMBER max_num;	/* Maximum population allowed per level */
	MONSTER_NUMBER cur_num;	/* Monster population on current level */

	FLOOR_IDX floor_id;		/* Location of unique monster */


	MONSTER_NUMBER r_sights;	/* Count sightings of this monster */
	MONSTER_NUMBER r_deaths;	/* Count deaths from this monster */

	MONSTER_NUMBER r_pkills;	/* Count visible monsters killed in this life */
	MONSTER_NUMBER r_akills;	/* Count all monsters killed in this life */
	MONSTER_NUMBER r_tkills;	/* Count monsters killed in all lives */

	byte r_wake;			/* Number of times woken up (?) */
	byte r_ignore;			/* Number of times ignored (?) */

	byte r_xtra1;			/* Something (unused) */
	byte r_xtra2;			/* Something (unused) */

	ITEM_NUMBER r_drop_gold;	/*!< これまでに撃破時に落とした財宝の数 / Max number of gold dropped at once */
	ITEM_NUMBER r_drop_item;	/*!< これまでに撃破時に落としたアイテムの数 / Max number of item dropped at once */

	byte r_cast_spell;		/* Max number of other spells seen */

	byte r_blows[4];		/* Number of times each blow type was seen */

	u32b r_flags1;			/* Observed racial flags */
	u32b r_flags2;			/* Observed racial flags */
	u32b r_flags3;			/* Observed racial flags */
	u32b r_flags4;			/* Observed racial flags */
	u32b r_flags5;			/* Observed racial flags */
	u32b r_flags6;			/* Observed racial flags */
	/* u32b r_flags7; */	/* Observed racial flags */
	u32b r_flagsr;			/* Observed racial resistance flags */
};


/*
 * The monster race arrays
 */
monster_race *r_info;
char *r_name;
char *r_text;