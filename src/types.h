#pragma once

/*!
 * @file types.h
 * @brief グローバルな構造体の定義 / global type declarations
 * @date 2014/08/10
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 * @details
 * <pre>
 * このファイルはangband.hでのみインクルードすること。
 * This file should ONLY be included by "angband.h"
 *
 * Note that "char" may or may not be signed, and that "signed char"
 * may or may not work on all machines.  So always use "s16b" or "s32b"
 * for signed values.  Also, note that unsigned values cause math problems
 * in many cases, so try to only use "u16b" and "u32b" for "bit flags",
 * unless you really need the extra bit of information, or you really
 * need to restrict yourself to a single byte for storage reasons.
 *
 * Also, if possible, attempt to restrict yourself to sub-fields of
 * known size (use "s16b" or "s32b" instead of "int", and "byte" instead
 * of "bool"), and attempt to align all fields along four-byte words, to
 * optimize storage issues on 32-bit machines.  Also, avoid "bit flags"
 * since these increase the code size and slow down execution.  When
 * you need to store bit flags, use one byte per flag, or, where space
 * is an issue, use a "byte" or "u16b" or "u32b", and add special code
 * to access the various bit flags.
 *
 * Many of these structures were developed to reduce the number of global
 * variables, facilitate structured program design, allow the use of ascii
 * template files, simplify access to indexed data, or facilitate efficient
 * clearing of many variables at once.
 *
 * Certain data is saved in multiple places for efficient access, currently,
 * this includes the tval/sval/weight fields in "object_type", various fields
 * in "header_type", and the "m_idx" and "o_idx" fields in "grid_type".  All
 * of these could be removed, but this would, in general, slow down the game
 * and increase the complexity of the code.
 * </pre>
 */

#include "h-type.h"
#include "defines.h"
//#include "player-skill.h"


/*
 * Information about "ego-items".
 */

typedef struct ego_item_type ego_item_type;

struct ego_item_type
{
	STR_OFFSET name;			/* Name (offset) */
	STR_OFFSET text;			/* Text (offset) */

	INVENTORY_IDX slot;		/*!< 装備部位 / Standard slot value */
	PRICE rating;		/*!< ベースアイテムからの価値加速 / Rating boost */

	DEPTH level;			/* Minimum level */
	RARITY rarity;		/* Object rarity */

	HIT_PROB max_to_h;		/* Maximum to-hit bonus */
	HIT_POINT max_to_d;		/* Maximum to-dam bonus */
	ARMOUR_CLASS max_to_a;		/* Maximum to-ac bonus */

	PARAMETER_VALUE max_pval;		/* Maximum pval */

	PRICE cost;			/* Ego-item "cost" */

	BIT_FLAGS flags[TR_FLAG_SIZE];	/* Ego-Item Flags */
	BIT_FLAGS gen_flags;		/* flags for generate */

	IDX act_idx;		/* Activative ability index */
};




/*
 * Monster blow structure
 *
 *	- Method (RBM_*)
 *	- Effect (RBE_*)
 *	- Damage Dice
 *	- Damage Sides
 */

typedef struct monster_blow monster_blow;

struct monster_blow
{
	BLOW_METHOD method;
	BLOW_EFFECT effect;
	DICE_NUMBER d_dice;
	DICE_SID d_side;
};


typedef struct mbe_info_type mbe_info_type;

struct mbe_info_type
{
	int power;        /* The attack "power" */
	int explode_type; /* Explosion effect */
};


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
 * A single "grid" in a Cave
 *
 * Note that several aspects of the code restrict the actual current_floor_ptr->grid_array
 * to a max size of 256 by 256.  In partcular, locations are often
 * saved as bytes, limiting each coordinate to the 0-255 range.
 *
 * The "o_idx" and "m_idx" fields are very interesting.  There are
 * many places in the code where we need quick access to the actual
 * monster or object(s) in a given grid.  The easiest way to
 * do this is to simply keep the index of the monster and object
 * (if any) with the grid, but this takes 198*66*4 bytes of memory.
 * Several other methods come to mind, which require only half this
 * amound of memory, but they all seem rather complicated, and would
 * probably add enough code that the savings would be lost.  So for
 * these reasons, we simply store an index into the "o_list" and
 * "current_floor_ptr->m_list" arrays, using "zero" when no monster/object is present.
 *
 * Note that "o_idx" is the index of the top object in a stack of
 * objects, using the "next_o_idx" field of objects (see below) to
 * create the singly linked list of objects.  If "o_idx" is zero
 * then there are no objects in the grid.
 *
 * Note the special fields for the "MONSTER_FLOW" code.
 */

typedef struct grid_type grid_type;

struct grid_type
{
	BIT_FLAGS info;		/* Hack -- current_floor_ptr->grid_array flags */

	FEAT_IDX feat;		/* Hack -- feature type */
	OBJECT_IDX o_idx;		/* Object in this grid */
	MONSTER_IDX m_idx;		/* Monster in this grid */

	/*! 地形の特別な情報を保存する / Special current_floor_ptr->grid_array info
	 * 具体的な使用一覧はクエスト行き階段の移行先クエストID、
	 * 各ダンジョン入口の移行先ダンジョンID、
	 * 
	 */
	s16b special;

	FEAT_IDX mimic;		/* Feature to mimic */

	byte cost;		/* Hack -- cost of flowing */
	byte dist;		/* Hack -- distance from player */
	byte when;		/* Hack -- when cost was computed */
};



/*
 * Simple structure to hold a map location
 */
typedef struct coord coord;

struct coord
{
	POSITION y;
	POSITION x;
};



/*
 * Object information, for a specific object.
 *
 * Note that a "discount" on an item is permanent and never goes away.
 *
 * Note that inscriptions are now handled via the "quark_str()" function
 * applied to the "note" field, which will return NULL if "note" is zero.
 *
 * Note that "object" records are "copied" on a fairly regular basis,
 * and care must be taken when handling such objects.
 *
 * Note that "object flags" must now be derived from the object kind,
 * the artifact and ego-item indexes, and the two "xtra" fields.
 *
 * Each grid points to one (or zero) objects via the "o_idx"
 * field (above).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a "stack" of objects in the same grid.
 *
 * Each monster points to one (or zero) objects via the "hold_o_idx"
 * field (below).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a pile of objects held by the monster.
 *
 * The "held_m_idx" field is used to indicate which monster, if any,
 * is holding the object.  Objects being held have "ix=0" and "iy=0".
 */

typedef struct object_type object_type;

struct object_type
{
	KIND_OBJECT_IDX k_idx;			/* Kind index (zero if "dead") */

	POSITION iy;			/* Y-position on map, or zero */
	POSITION ix;			/* X-position on map, or zero */

	OBJECT_TYPE_VALUE tval;			/* Item type (from kind) */
	OBJECT_SUBTYPE_VALUE sval;			/* Item sub-type (from kind) */

	PARAMETER_VALUE pval;			/* Item extra-parameter */

	DISCOUNT_RATE discount;		/* Discount (if any) */

	ITEM_NUMBER number;	/* Number of items */

	WEIGHT weight;		/* Item weight */

	ARTIFACT_IDX name1;		/* Artifact type, if any */
	EGO_IDX name2;			/* Ego-Item type, if any */

	XTRA8 xtra1;			/* Extra info type (now unused) */
	XTRA8 xtra2;			/* Extra info activation index */
	XTRA8 xtra3;			/* Extra info for weaponsmith */
	XTRA16 xtra4;			/*!< 光源の残り寿命、あるいは捕らえたモンスターの現HP / Extra info fuel or captured monster's current HP */
	XTRA16 xtra5;			/*!< 捕らえたモンスターの最大HP / Extra info captured monster's max HP */

	HIT_PROB to_h;			/* Plusses to hit */
	HIT_POINT to_d;			/* Plusses to damage */
	ARMOUR_CLASS to_a;			/* Plusses to AC */

	ARMOUR_CLASS ac;			/* Normal AC */

	DICE_NUMBER dd;
	DICE_SID ds;		/* Damage dice/sides */

	TIME_EFFECT timeout;	/* Timeout Counter */

	byte ident;			/* Special flags  */
	byte marked;		/* Object is marked */

	u16b inscription;	/* Inscription index */
	u16b art_name;      /* Artifact name (random artifacts) */

	byte feeling;          /* Game generated inscription number (eg, pseudo-id) */

	BIT_FLAGS art_flags[TR_FLAG_SIZE];        /* Extra Flags for ego and artifacts */
	BIT_FLAGS curse_flags;        /* Flags for curse */

	OBJECT_IDX next_o_idx;	/* Next object in stack (if any) */
	MONSTER_IDX held_m_idx;	/* Monster holding us (if any) */

	ARTIFACT_BIAS_IDX artifact_bias; /*!< ランダムアーティファクト生成時のバイアスID */
};



/*
 * Monster information, for a specific monster.
 * Note: fy, fx constrain dungeon size to 256x256
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */

typedef struct monster_type monster_type;

struct monster_type
{
	MONRACE_IDX r_idx;		/* Monster race index 0 = dead. */
	MONRACE_IDX ap_r_idx;	/* Monster race appearance index */
	byte sub_align;		/* Sub-alignment for a neutral monster */

	POSITION fy;		/* Y location on map */
	POSITION fx;		/* X location on map */

	HIT_POINT hp;		/* Current Hit points */
	HIT_POINT maxhp;		/* Max Hit points */
	HIT_POINT max_maxhp;		/* Max Max Hit points */
	HIT_POINT dealt_damage;		/* Sum of damages dealt by player */

	TIME_EFFECT mtimed[MAX_MTIMED];	/* Timed status counter */

	SPEED mspeed;	        /* Monster "speed" */
	ACTION_ENERGY energy_need;	/* Monster "energy" */

	POSITION cdis;		/* Current dis from player */

	BIT_FLAGS8 mflag;	/* Extra monster flags */
	#define MFLAG_VIEW      0x01    /* Monster is in line of sight */
	#define MFLAG_LOS       0x02    /* Monster is marked for project_all_los() */
	#define MFLAG_XXX2      0x04    /* (unused) */
	#define MFLAG_ETF       0x08    /* Monster is entering the field. */
	#define MFLAG_BORN      0x10    /* Monster is still being born */
	#define MFLAG_NICE      0x20    /* Monster is still being nice */

	BIT_FLAGS8 mflag2;	/* Extra monster flags */
	#define MFLAG2_KAGE      0x01    /* Monster is kage */
	#define MFLAG2_NOPET     0x02    /* Cannot make monster pet */
	#define MFLAG2_NOGENO    0x04    /* Cannot genocide */
	#define MFLAG2_CHAMELEON 0x08    /* Monster is chameleon */
	#define MFLAG2_NOFLOW    0x10    /* Monster is in no_flow_by_smell mode */
	#define MFLAG2_SHOW      0x20    /* Monster is recently memorized */
	#define MFLAG2_MARK      0x40    /* Monster is currently memorized */

	bool ml;		/* Monster is "visible" */

	OBJECT_IDX hold_o_idx;	/* Object being held (if any) */

	POSITION target_y;		/* Can attack !los player */
	POSITION target_x;		/* Can attack !los player */

	STR_OFFSET nickname;		/* Monster's Nickname */

	EXP exp;

	/* TODO: クローン、ペット、有効化は意義が異なるので別変数に切り離すこと。save/loadのバージョン更新が面倒そうだけど */
	BIT_FLAGS smart; /*!< Field for "smart_learn" - Some bit-flags for the "smart" field */
	#define SM_RES_ACID             0x00000001 /*!< モンスターの学習フラグ: プレイヤーに酸耐性あり */
	#define SM_RES_ELEC             0x00000002 /*!< モンスターの学習フラグ: プレイヤーに電撃耐性あり */
	#define SM_RES_FIRE             0x00000004 /*!< モンスターの学習フラグ: プレイヤーに火炎耐性あり */
	#define SM_RES_COLD             0x00000008 /*!< モンスターの学習フラグ: プレイヤーに冷気耐性あり */
	#define SM_RES_POIS             0x00000010 /*!< モンスターの学習フラグ: プレイヤーに毒耐性あり */
	#define SM_RES_NETH             0x00000020 /*!< モンスターの学習フラグ: プレイヤーに地獄耐性あり */
	#define SM_RES_LITE             0x00000040 /*!< モンスターの学習フラグ: プレイヤーに閃光耐性あり */
	#define SM_RES_DARK             0x00000080 /*!< モンスターの学習フラグ: プレイヤーに暗黒耐性あり */
	#define SM_RES_FEAR             0x00000100 /*!< モンスターの学習フラグ: プレイヤーに恐怖耐性あり */
	#define SM_RES_CONF             0x00000200 /*!< モンスターの学習フラグ: プレイヤーに混乱耐性あり */
	#define SM_RES_CHAOS            0x00000400 /*!< モンスターの学習フラグ: プレイヤーにカオス耐性あり */
	#define SM_RES_DISEN            0x00000800 /*!< モンスターの学習フラグ: プレイヤーに劣化耐性あり */
	#define SM_RES_BLIND            0x00001000 /*!< モンスターの学習フラグ: プレイヤーに盲目耐性あり */
	#define SM_RES_NEXUS            0x00002000 /*!< モンスターの学習フラグ: プレイヤーに因果混乱耐性あり */
	#define SM_RES_SOUND            0x00004000 /*!< モンスターの学習フラグ: プレイヤーに轟音耐性あり */
	#define SM_RES_SHARD            0x00008000 /*!< モンスターの学習フラグ: プレイヤーに破片耐性あり */
	#define SM_OPP_ACID             0x00010000 /*!< モンスターの学習フラグ: プレイヤーに二重酸耐性あり */
	#define SM_OPP_ELEC             0x00020000 /*!< モンスターの学習フラグ: プレイヤーに二重電撃耐性あり */
	#define SM_OPP_FIRE             0x00040000 /*!< モンスターの学習フラグ: プレイヤーに二重火炎耐性あり */
	#define SM_OPP_COLD             0x00080000 /*!< モンスターの学習フラグ: プレイヤーに二重冷気耐性あり */
	#define SM_OPP_POIS             0x00100000 /*!< モンスターの学習フラグ: プレイヤーに二重毒耐性あり */
	#define SM_OPP_XXX1             0x00200000 /*!< 未使用 / (unused) */
	#define SM_CLONED               0x00400000 /*!< クローンである / Cloned */
	#define SM_PET                  0x00800000 /*!< ペットである / Pet */
	#define SM_IMM_ACID             0x01000000 /*!< モンスターの学習フラグ: プレイヤーに酸免疫あり */
	#define SM_IMM_ELEC             0x02000000 /*!< モンスターの学習フラグ: プレイヤーに電撃免疫あり */
	#define SM_IMM_FIRE             0x04000000 /*!< モンスターの学習フラグ: プレイヤーに火炎免疫あり */
	#define SM_IMM_COLD             0x08000000 /*!< モンスターの学習フラグ: プレイヤーに冷気免疫あり */
	#define SM_FRIENDLY             0x10000000 /*!< 友好的である / Friendly */
	#define SM_IMM_REFLECT          0x20000000 /*!< モンスターの学習フラグ: プレイヤーに反射あり */
	#define SM_IMM_FREE             0x40000000 /*!< モンスターの学習フラグ: プレイヤーに麻痺耐性あり */
	#define SM_IMM_MANA             0x80000000 /*!< モンスターの学習フラグ: プレイヤーにMPがない */

	MONSTER_IDX parent_m_idx;
};




/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */

typedef struct alloc_entry alloc_entry;

struct alloc_entry
{
	KIND_OBJECT_IDX index;		/* The actual index */

	DEPTH level;		/* Base dungeon level */
	PROB prob1;		/* Probability, pass 1 */
	PROB prob2;		/* Probability, pass 2 */
	PROB prob3;		/* Probability, pass 3 */

	u16b total;		/* Unused for now */
};


/*
 * A store owner
 */
typedef struct owner_type owner_type;

struct owner_type
{
	concptr owner_name;	/* Name */
	PRICE max_cost;		/* Purse limit */
	byte max_inflate;	/* Inflation (max) */
	byte min_inflate;	/* Inflation (min) */
	byte haggle_per;	/* Haggle unit */
	byte insult_max;	/* Insult limit */
	byte owner_race;	/* Owner race */
};




/*
 * A store, with an owner, various state flags, a current stock
 * of items, and a table of items that are often purchased.
 */
typedef struct store_type store_type;

struct store_type
{
	byte type;				/* Store type */

	byte owner;				/* Owner index */
	byte extra;				/* Unused for now */

	s16b insult_cur;		/* Insult counter */

	s16b good_buy;			/* Number of "good" buys */
	s16b bad_buy;			/* Number of "bad" buys */

	s32b store_open;		/* Closed until this current_world_ptr->game_turn */

	s32b last_visit;		/* Last visited on this current_world_ptr->game_turn */

	s16b table_num;			/* Table -- Number of entries */
	s16b table_size;		/* Table -- Total Size of Array */
	s16b *table;			/* Table -- Legal item kinds */

	s16b stock_num;			/* Stock -- Number of entries */
	s16b stock_size;		/* Stock -- Total Size of Array */
	object_type *stock;		/* Stock -- Actual stock items */
};


/*
 * The "name" of spell 'N' is stored as spell_names[X][N],
 * where X is 0 for mage-spells and 1 for priest-spells.
 */
typedef struct magic_type magic_type;

struct magic_type
{
	PLAYER_LEVEL slevel;	/* Required level (to learn) */
	MANA_POINT smana;		/* Required mana (to cast) */
	PERCENTAGE sfail;		/* Minimum chance of failure */
	EXP sexp;				/* Encoded experience bonus */
};


/*
 * Player sex info
 */

typedef struct player_sex player_sex;

struct player_sex
{
	concptr title;			/* Type of sex */
	concptr winner;		/* Name of winner */
#ifdef JP
	concptr E_title;		/* 英語性別 */
	concptr E_winner;		/* 英語性別 */
#endif
};


/*
 * Player racial info
 */

typedef struct player_race player_race;

struct player_race
{
	concptr title;			/* Type of race */

#ifdef JP
	concptr E_title;		/* 英語種族 */
#endif
	s16b r_adj[6];		/* Racial stat bonuses */

	s16b r_dis;			/* disarming */
	s16b r_dev;			/* magic devices */
	s16b r_sav;			/* saving throw */
	s16b r_stl;			/* stealth */
	s16b r_srh;			/* search ability */
	s16b r_fos;			/* search frequency */
	s16b r_thn;			/* combat (normal) */
	s16b r_thb;			/* combat (shooting) */

	byte r_mhp;			/* Race hit-dice modifier */
	byte r_exp;			/* Race experience factor */

	byte b_age;			/* base age */
	byte m_age;			/* mod age */

	byte m_b_ht;		/* base height (males) */
	byte m_m_ht;		/* mod height (males) */
	byte m_b_wt;		/* base weight (males) */
	byte m_m_wt;		/* mod weight (males) */

	byte f_b_ht;		/* base height (females) */
	byte f_m_ht;		/* mod height (females)	  */
	byte f_b_wt;		/* base weight (females) */
	byte f_m_wt;		/* mod weight (females) */

	byte infra;			/* Infra-vision	range */

	u32b choice;        /* Legal class choices */
/*    byte choice_xtra;   */
};


typedef struct player_seikaku player_seikaku;
struct player_seikaku
{
	concptr title;			/* Type of seikaku */

#ifdef JP
	concptr E_title;		/* 英語性格 */
#endif

	s16b a_adj[6];		/* seikaku stat bonuses */

	s16b a_dis;			/* seikaku disarming */
	s16b a_dev;			/* seikaku magic devices */
	s16b a_sav;			/* seikaku saving throw */
	s16b a_stl;			/* seikaku stealth */
	s16b a_srh;			/* seikaku search ability */
	s16b a_fos;			/* seikaku search frequency */
	s16b a_thn;			/* seikaku combat (normal) */
	s16b a_thb;			/* seikaku combat (shooting) */

	s16b a_mhp;			/* Race hit-dice modifier */

	byte no;			/* の */
	byte sex;			/* seibetu seigen */
};


/*
 * Most of the "player" information goes here.
 *
 * This stucture gives us a large collection of player variables.
 *
 * This structure contains several "blocks" of information.
 *   (1) the "permanent" info
 *   (2) the "variable" info
 *   (3) the "transient" info
 *
 * All of the "permanent" info, and most of the "variable" info,
 * is saved in the savefile.  The "transient" info is recomputed
 * whenever anything important changes.
 */

typedef struct player_type player_type;

struct player_type
{
	POSITION oldpy;		/* Previous player location -KMW- */
	POSITION oldpx;		/* Previous player location -KMW- */

	SEX_IDX psex;		/* Sex index */
	RACE_IDX prace;		/* Race index */
	CLASS_IDX pclass;	/* Class index */
	CHARACTER_IDX pseikaku;	/* Seikaku index */
	REALM_IDX realm1;		/* First magic realm */
	REALM_IDX realm2;		/* Second magic realm */
	CHARACTER_IDX oops;		/* Unused */

	DICE_SID hitdie;	/* Hit dice (sides) */
	u16b expfact;	/* Experience factor
					* Note: was byte, causing overflow for Amberite
					* characters (such as Amberite Paladins)
					*/

	s16b age;			/* Characters age */
	s16b ht;			/* Height */
	s16b wt;			/* Weight */
	s16b sc;			/* Social Class */

	PRICE au;			/* Current Gold */

	EXP max_max_exp;	/* Max max experience (only to calculate score) */
	EXP max_exp;		/* Max experience */
	EXP exp;			/* Cur experience */
	u32b exp_frac;		/* Cur exp frac (times 2^16) */

	PLAYER_LEVEL lev;			/* Level */

	TOWN_IDX town_num;			/* Current town number */
	s16b arena_number;		/* monster number in arena -KMW- */
	bool inside_arena;		/* Is character inside arena? */
	QUEST_IDX inside_quest;		/* Inside quest level */
	bool inside_battle;		/* Is character inside tougijou? */

	DUNGEON_IDX dungeon_idx; /* current dungeon index */
	POSITION wilderness_x;	/* Coordinates in the wilderness */
	POSITION wilderness_y;
	bool wild_mode;

	HIT_POINT mhp;			/* Max hit pts */
	HIT_POINT chp;			/* Cur hit pts */
	u32b chp_frac;		/* Cur hit frac (times 2^16) */
	PERCENTAGE mutant_regenerate_mod;

	MANA_POINT msp;			/* Max mana pts */
	MANA_POINT csp;			/* Cur mana pts */
	u32b csp_frac;		/* Cur mana frac (times 2^16) */

	s16b max_plv;		/* Max Player Level */

	BASE_STATUS stat_max[6];	/* Current "maximal" stat values */
	BASE_STATUS stat_max_max[6];	/* Maximal "maximal" stat values */
	BASE_STATUS stat_cur[6];	/* Current "natural" stat values */

	s16b learned_spells;
	s16b add_spells;

	u32b count;

	TIME_EFFECT fast;		/* Timed -- Fast */
	TIME_EFFECT slow;		/* Timed -- Slow */
	TIME_EFFECT blind;		/* Timed -- Blindness */
	TIME_EFFECT paralyzed;		/* Timed -- Paralysis */
	TIME_EFFECT confused;		/* Timed -- Confusion */
	TIME_EFFECT afraid;		/* Timed -- Fear */
	TIME_EFFECT image;		/* Timed -- Hallucination */
	TIME_EFFECT poisoned;		/* Timed -- Poisoned */
	TIME_EFFECT cut;		/* Timed -- Cut */
	TIME_EFFECT stun;		/* Timed -- Stun */

	TIME_EFFECT protevil;		/* Timed -- Protection */
	TIME_EFFECT invuln;		/* Timed -- Invulnerable */
	TIME_EFFECT ult_res;		/* Timed -- Ultimate Resistance */
	TIME_EFFECT hero;		/* Timed -- Heroism */
	TIME_EFFECT shero;		/* Timed -- Super Heroism */
	TIME_EFFECT shield;		/* Timed -- Shield Spell */
	TIME_EFFECT blessed;		/* Timed -- Blessed */
	TIME_EFFECT tim_invis;		/* Timed -- See Invisible */
	TIME_EFFECT tim_infra;		/* Timed -- Infra Vision */
	TIME_EFFECT tsuyoshi;		/* Timed -- Tsuyoshi Special */
	TIME_EFFECT ele_attack;	/* Timed -- Elemental Attack */
	TIME_EFFECT ele_immune;	/* Timed -- Elemental Immune */

	TIME_EFFECT oppose_acid;	/* Timed -- oppose acid */
	TIME_EFFECT oppose_elec;	/* Timed -- oppose lightning */
	TIME_EFFECT oppose_fire;	/* Timed -- oppose heat */
	TIME_EFFECT oppose_cold;	/* Timed -- oppose cold */
	TIME_EFFECT oppose_pois;	/* Timed -- oppose poison */

	TIME_EFFECT tim_esp;       /* Timed ESP */
	TIME_EFFECT wraith_form;   /* Timed wraithform */

	TIME_EFFECT resist_magic;  /* Timed Resist Magic (later) */
	TIME_EFFECT tim_regen;
	TIME_EFFECT kabenuke;
	TIME_EFFECT tim_stealth;
	TIME_EFFECT tim_levitation;
	TIME_EFFECT tim_sh_touki;
	TIME_EFFECT lightspeed;
	TIME_EFFECT tsubureru;
	TIME_EFFECT magicdef;
	TIME_EFFECT tim_res_nether;	/* Timed -- Nether resistance */
	TIME_EFFECT tim_res_time;	/* Timed -- Time resistance */
	MIMIC_RACE_IDX mimic_form;
	TIME_EFFECT tim_mimic;
	TIME_EFFECT tim_sh_fire;
	TIME_EFFECT tim_sh_holy;
	TIME_EFFECT tim_eyeeye;

	/* for mirror master */
	TIME_EFFECT tim_reflect;       /* Timed -- Reflect */
	TIME_EFFECT multishadow;       /* Timed -- Multi-shadow */
	TIME_EFFECT dustrobe;          /* Timed -- Robe of dust */

	bool timewalk;
	GAME_TURN resting;	/* Current counter for resting, if any */

	PATRON_IDX chaos_patron;

	BIT_FLAGS muta1; /*!< レイシャル型の変異 / "Activatable" mutations must be in MUT1_* */	
	#define MUT1_SPIT_ACID                  0x00000001L /*!< 突然変異: 酸の唾 */
	#define MUT1_BR_FIRE                    0x00000002L /*!< 突然変異: 炎のブレス */
	#define MUT1_HYPN_GAZE                  0x00000004L /*!< 突然変異: 催眠睨み */
	#define MUT1_TELEKINES                  0x00000008L /*!< 突然変異: 念動力 */
	#define MUT1_VTELEPORT                  0x00000010L /*!< 突然変異: テレポート / Voluntary teleport */
	#define MUT1_MIND_BLST                  0x00000020L /*!< 突然変異: 精神攻撃 */
	#define MUT1_RADIATION                  0x00000040L /*!< 突然変異: 放射能 */
	#define MUT1_VAMPIRISM                  0x00000080L /*!< 突然変異: 吸血 */
	#define MUT1_SMELL_MET                  0x00000100L /*!< 突然変異: 金属嗅覚 */
	#define MUT1_SMELL_MON                  0x00000200L /*!< 突然変異: 敵臭嗅覚 */
	#define MUT1_BLINK                      0x00000400L /*!< 突然変異: ショート・テレポート */
	#define MUT1_EAT_ROCK                   0x00000800L /*!< 突然変異: 岩喰い */
	#define MUT1_SWAP_POS                   0x00001000L /*!< 突然変異: 位置交換 */
	#define MUT1_SHRIEK                     0x00002000L /*!< 突然変異: 叫び */
	#define MUT1_ILLUMINE                   0x00004000L /*!< 突然変異: 照明 */
	#define MUT1_DET_CURSE                  0x00008000L /*!< 突然変異: 呪い感知 */
	#define MUT1_BERSERK                    0x00010000L /*!< 突然変異: 狂戦士化 */
	#define MUT1_POLYMORPH                  0x00020000L /*!< 突然変異: 変身 */
	#define MUT1_MIDAS_TCH                  0x00040000L /*!< 突然変異: ミダスの手 */
	#define MUT1_GROW_MOLD                  0x00080000L /*!< 突然変異: カビ発生 */
	#define MUT1_RESIST                     0x00100000L /*!< 突然変異: エレメント耐性 */
	#define MUT1_EARTHQUAKE                 0x00200000L /*!< 突然変異: 地震 */
	#define MUT1_EAT_MAGIC                  0x00400000L /*!< 突然変異: 魔力喰い */
	#define MUT1_WEIGH_MAG                  0x00800000L /*!< 突然変異: 魔力感知 */
	#define MUT1_STERILITY                  0x01000000L /*!< 突然変異: 増殖阻止 */
	#define MUT1_PANIC_HIT                  0x02000000L /*!< 突然変異: ヒットアンドアウェイ */
	#define MUT1_DAZZLE                     0x04000000L /*!< 突然変異: 眩惑 */
	#define MUT1_LASER_EYE                  0x08000000L /*!< 突然変異: レーザー・アイ */
	#define MUT1_RECALL                     0x10000000L /*!< 突然変異: 帰還 */
	#define MUT1_BANISH                     0x20000000L /*!< 突然変異: 邪悪消滅 */
	#define MUT1_COLD_TOUCH                 0x40000000L /*!< 突然変異: 凍結の手 */
	#define MUT1_LAUNCHER                   0x80000000L /*!< 突然変異: アイテム投げ */

	BIT_FLAGS muta2; /*!< 常時効果つきの変異1 / Randomly activating mutations must be MUT2_* */
	#define MUT2_BERS_RAGE                  0x00000001L /*!< 突然変異: 狂戦士化の発作 */
	#define MUT2_COWARDICE                  0x00000002L /*!< 突然変異: 臆病 */
	#define MUT2_RTELEPORT                  0x00000004L /*!< 突然変異: ランダムテレポート / Random teleport, instability */
	#define MUT2_ALCOHOL                    0x00000008L /*!< 突然変異: アルコール分泌 */
	#define MUT2_HALLU                      0x00000010L /*!< 突然変異: 幻覚を引き起こす精神錯乱 */
	#define MUT2_FLATULENT                  0x00000020L /*!< 突然変異: 猛烈な屁 */
	#define MUT2_SCOR_TAIL                  0x00000040L /*!< 突然変異: サソリの尻尾 */
	#define MUT2_HORNS                      0x00000080L /*!< 突然変異: ツノ */
	#define MUT2_BEAK                       0x00000100L /*!< 突然変異: クチバシ */
	#define MUT2_ATT_DEMON                  0x00000200L /*!< 突然変異: デーモンを引き付ける */
	#define MUT2_PROD_MANA                  0x00000400L /*!< 突然変異: 制御できない魔力のエネルギー */
	#define MUT2_SPEED_FLUX                 0x00000800L /*!< 突然変異: ランダムな加減速 */
	#define MUT2_BANISH_ALL                 0x00001000L /*!< 突然変異: ランダムなモンスター消滅 */
	#define MUT2_EAT_LIGHT                  0x00002000L /*!< 突然変異: 光源喰い */
	#define MUT2_TRUNK                      0x00004000L /*!< 突然変異: 象の鼻 */
	#define MUT2_ATT_ANIMAL                 0x00008000L /*!< 突然変異: 動物を引き寄せる */
	#define MUT2_TENTACLES                  0x00010000L /*!< 突然変異: 邪悪な触手 */
	#define MUT2_RAW_CHAOS                  0x00020000L /*!< 突然変異: 純カオス */
	#define MUT2_NORMALITY                  0x00040000L /*!< 突然変異: ランダムな変異の消滅 */
	#define MUT2_WRAITH                     0x00080000L /*!< 突然変異: ランダムな幽体化 */
	#define MUT2_POLY_WOUND                 0x00100000L /*!< 突然変異: ランダムな傷の変化 */
	#define MUT2_WASTING                    0x00200000L /*!< 突然変異: 衰弱 */
	#define MUT2_ATT_DRAGON                 0x00400000L /*!< 突然変異: ドラゴンを引き寄せる */
	#define MUT2_WEIRD_MIND                 0x00800000L /*!< 突然変異: ランダムなテレパシー */
	#define MUT2_NAUSEA                     0x01000000L /*!< 突然変異: 落ち着きの無い胃 */
	#define MUT2_CHAOS_GIFT                 0x02000000L /*!< 突然変異: カオスパトロン */
	#define MUT2_WALK_SHAD                  0x04000000L /*!< 突然変異: ランダムな現実変容 */
	#define MUT2_WARNING                    0x08000000L /*!< 突然変異: 警告 */
	#define MUT2_INVULN                     0x10000000L /*!< 突然変異: ランダムな無敵化 */
	#define MUT2_SP_TO_HP                   0x20000000L /*!< 突然変異: ランダムなMPからHPへの変換 */
	#define MUT2_HP_TO_SP                   0x40000000L /*!< 突然変異: ランダムなHPからMPへの変換 */
	#define MUT2_DISARM                     0x80000000L /*!< 突然変異: ランダムな武器落とし */

	BIT_FLAGS muta3; /*!< 常時効果つきの変異2 / Other mutations will be mainly in MUT3_* */
	#define MUT3_HYPER_STR                  0x00000001L /*!< 突然変異: 超人的な力 */
	#define MUT3_PUNY                       0x00000002L /*!< 突然変異: 虚弱 */
	#define MUT3_HYPER_INT                  0x00000004L /*!< 突然変異: 生体コンピュータ */
	#define MUT3_MORONIC                    0x00000008L /*!< 突然変異: 精神薄弱 */
	#define MUT3_RESILIENT                  0x00000010L /*!< 突然変異: 弾力のある体 */
	#define MUT3_XTRA_FAT                   0x00000020L /*!< 突然変異: 異常な肥満 */
	#define MUT3_ALBINO                     0x00000040L /*!< 突然変異: アルビノ */
	#define MUT3_FLESH_ROT                  0x00000080L /*!< 突然変異: 腐敗した肉体 */
	#define MUT3_SILLY_VOI                  0x00000100L /*!< 突然変異: 間抜けなキーキー声 */
	#define MUT3_BLANK_FAC                  0x00000200L /*!< 突然変異: のっぺらぼう */
	#define MUT3_ILL_NORM                   0x00000400L /*!< 突然変異: 幻影に覆われた体 */
	#define MUT3_XTRA_EYES                  0x00000800L /*!< 突然変異: 第三の目 */
	#define MUT3_MAGIC_RES                  0x00001000L /*!< 突然変異: 魔法防御 */
	#define MUT3_XTRA_NOIS                  0x00002000L /*!< 突然変異: 騒音 */
	#define MUT3_INFRAVIS                   0x00004000L /*!< 突然変異: 赤外線視力 */
	#define MUT3_XTRA_LEGS                  0x00008000L /*!< 突然変異: 追加の脚 */
	#define MUT3_SHORT_LEG                  0x00010000L /*!< 突然変異: 短い脚 */
	#define MUT3_ELEC_TOUC                  0x00020000L /*!< 突然変異: 電撃オーラ */
	#define MUT3_FIRE_BODY                  0x00040000L /*!< 突然変異: 火炎オーラ */
	#define MUT3_WART_SKIN                  0x00080000L /*!< 突然変異: イボ肌 */
	#define MUT3_SCALES                     0x00100000L /*!< 突然変異: 鱗肌 */
	#define MUT3_IRON_SKIN                  0x00200000L /*!< 突然変異: 鉄の肌 */
	#define MUT3_WINGS                      0x00400000L /*!< 突然変異: 翼 */
	#define MUT3_FEARLESS                   0x00800000L /*!< 突然変異: 恐れ知らず */
	#define MUT3_REGEN                      0x01000000L /*!< 突然変異: 急回復 */
	#define MUT3_ESP                        0x02000000L /*!< 突然変異: テレパシー */
	#define MUT3_LIMBER                     0x04000000L /*!< 突然変異: しなやかな肉体 */
	#define MUT3_ARTHRITIS                  0x08000000L /*!< 突然変異: 関節の痛み */
	#define MUT3_BAD_LUCK                   0x10000000L /*!< 突然変異: 黒いオーラ(不運) */
	#define MUT3_VULN_ELEM                  0x20000000L /*!< 突然変異: 元素攻撃弱点 */
	#define MUT3_MOTION                     0x40000000L /*!< 突然変異: 正確で力強い動作 */
	#define MUT3_GOOD_LUCK                  0x80000000L /*!< 突然変異: 白いオーラ(幸運) */

	s16b virtues[8];
	s16b vir_types[8];

	TIME_EFFECT word_recall;	  /* Word of recall counter */
	TIME_EFFECT alter_reality;	  /* Alter reality counter */
	DUNGEON_IDX recall_dungeon;      /* Dungeon set to be recalled */

	ENERGY energy_need;	  /* Energy needed for next move */
	ENERGY enchant_energy_need;	  /* Energy needed for next upkeep effect	 */

	FEED food;		  /* Current nutrition */

	/*
	 * p_ptr->special_attackによるプレイヤーの攻撃状態の定義 / Bit flags for the "p_ptr->special_attack" variable. -LM-
	 *
	 * Note:  The elemental and poison attacks should be managed using the
	 * function "set_ele_attack", in spell2.c.  This provides for timeouts and
	 * prevents the player from getting more than one at a time.
	 */
	BIT_FLAGS special_attack;
	#define ATTACK_CONFUSE	0x00000001 /*!< プレイヤーのステータス:混乱打撃 */
	#define ATTACK_XXX1		0x00000002 /*!< プレイヤーのステータス:未使用1 */
	#define ATTACK_XXX2		0x00000004 /*!< プレイヤーのステータス:未使用2 */
	#define ATTACK_XXX3	    0x00000008 /*!< プレイヤーのステータス:未使用3 */
	#define ATTACK_ACID		0x00000010 /*!< プレイヤーのステータス:魔法剣/溶解 */
	#define ATTACK_ELEC		0x00000020 /*!< プレイヤーのステータス:魔法剣/電撃 */
	#define ATTACK_FIRE		0x00000040 /*!< プレイヤーのステータス:魔法剣/火炎 */
	#define ATTACK_COLD		0x00000080 /*!< プレイヤーのステータス:魔法剣/冷凍 */
	#define ATTACK_POIS		0x00000100 /*!< プレイヤーのステータス:魔法剣/毒殺 */
	#define ATTACK_HOLY		0x00000200 /*!< プレイヤーのステータス:対邪?(未使用) */
	#define ATTACK_SUIKEN	0x00000400 /*!< プレイヤーのステータス:酔拳 */

	/*
	 * p_ptr->special_defenseによるプレイヤーの防御状態の定義 / Bit flags for the "p_ptr->special_defense" variable. -LM-
	 */
	BIT_FLAGS special_defense;
	#define DEFENSE_ACID	0x00000001 /*!< プレイヤーのステータス:酸免疫 */
	#define DEFENSE_ELEC	0x00000002 /*!< プレイヤーのステータス:電撃免疫 */
	#define DEFENSE_FIRE	0x00000004 /*!< プレイヤーのステータス:火炎免疫 */
	#define DEFENSE_COLD	0x00000008 /*!< プレイヤーのステータス:冷気免疫 */
	#define DEFENSE_POIS	0x00000010 /*!< プレイヤーのステータス:毒免疫 */
	#define KAMAE_GENBU     0x00000020 /*!< プレイヤーのステータス:玄武の構え */
	#define KAMAE_BYAKKO    0x00000040 /*!< プレイヤーのステータス:白虎の構え */
	#define KAMAE_SEIRYU    0x00000080 /*!< プレイヤーのステータス:青竜の構え */
	#define KAMAE_SUZAKU    0x00000100 /*!< プレイヤーのステータス:朱雀の構え */
	#define KATA_IAI        0x00000200 /*!< プレイヤーのステータス:居合 */
	#define KATA_FUUJIN     0x00000400 /*!< プレイヤーのステータス:風塵 */
	#define KATA_KOUKIJIN   0x00000800 /*!< プレイヤーのステータス:降鬼陣 */
	#define KATA_MUSOU      0x00001000 /*!< プレイヤーのステータス:無想 */
	#define NINJA_KAWARIMI  0x00002000 /*!< プレイヤーのステータス:変わり身 */
	#define NINJA_S_STEALTH 0x00004000 /*!< プレイヤーのステータス:超隠密 */
	#define MAX_KAMAE 4 /*!< 修行僧の構え最大数 */
	#define KAMAE_MASK (KAMAE_GENBU | KAMAE_BYAKKO | KAMAE_SEIRYU | KAMAE_SUZAKU) /*!< 修行僧の構えビット配列 */
	#define MAX_KATA 4 /*!< 修行僧の型最大数 */
	#define KATA_MASK (KATA_IAI | KATA_FUUJIN | KATA_KOUKIJIN | KATA_MUSOU) /*!< 修行僧の型ビット配列 */

	ACTION_IDX action;		  /* Currently action */
	#define ACTION_NONE     0 /*!< 持続行動: なし */
	#define ACTION_SEARCH   1 /*!< 持続行動: 探索 */
	#define ACTION_REST     2 /*!< 持続行動: 休憩 */
	#define ACTION_LEARN    3 /*!< 持続行動: 青魔法ラーニング */
	#define ACTION_FISH     4 /*!< 持続行動: 釣り */
	#define ACTION_KAMAE    5 /*!< 持続行動: 修行僧の構え */
	#define ACTION_KATA     6 /*!< 持続行動: 剣術家の型 */
	#define ACTION_SING     7 /*!< 持続行動: 歌 */
	#define ACTION_HAYAGAKE 8 /*!< 持続行動: 早駆け */
	#define ACTION_SPELL    9 /*!< 持続行動: 呪術 */

	BIT_FLAGS spell_learned1;	  /* bit mask of spells learned */
	BIT_FLAGS spell_learned2;	  /* bit mask of spells learned */
	BIT_FLAGS spell_worked1;	  /* bit mask of spells tried and worked */
	BIT_FLAGS spell_worked2;	  /* bit mask of spells tried and worked */
	BIT_FLAGS spell_forgotten1;	  /* bit mask of spells learned but forgotten */
	BIT_FLAGS spell_forgotten2;	  /* bit mask of spells learned but forgotten */
	SPELL_IDX spell_order[64];  /* order spells learned/remembered/forgotten */

	SUB_EXP spell_exp[64];        /* Proficiency of spells */
	SUB_EXP weapon_exp[5][64];    /* Proficiency of weapons */
	SUB_EXP skill_exp[GINOU_MAX]; /* Proficiency of misc. skill */

	MAGIC_NUM1 magic_num1[108];     /*!< Array for non-spellbook type magic */
	MAGIC_NUM2 magic_num2[108];     /*!< 魔道具術師の取り込み済魔道具使用回数 / Flags for non-spellbook type magics */

	SPELL_IDX mane_spell[MAX_MANE];
	HIT_POINT mane_dam[MAX_MANE];
	s16b mane_num;

	s16b concent;      /* Sniper's concentration level */

	HIT_POINT player_hp[PY_MAX_LEVEL];
	char died_from[80];   	  /* What killed the player */
	concptr last_message;        /* Last message on death or retirement */
	char history[4][60];  	  /* Textual "history" for the Player */

	u16b total_winner;	  /* Total winner */
	u16b panic_save;	  /* Panic save */

	u16b noscore;		  /* Cheating flags */

	bool wait_report_score;   /* Waiting to report score */
	bool is_dead;		  /* Player is dead */
	bool now_damaged;
	bool ambush_flag;

	bool wizard;		  /* Player is in wizard mode */

	MONSTER_IDX riding;              /* Riding on a monster of this index */
	byte knowledge;           /* Knowledge about yourself */
	BIT_FLAGS visit;               /* Visited towns */

	RACE_IDX start_race;          /* Race at birth */
	BIT_FLAGS old_race1;           /* Record of race changes */
	BIT_FLAGS old_race2;           /* Record of race changes */
	s16b old_realm;           /* Record of realm changes */

	s16b pet_follow_distance; /* Length of the imaginary "leash" for pets */
	s16b pet_extra_flags;     /* Various flags for controling pets */

	s16b today_mon;           /* Wanted monster */

	bool dtrap;               /* Whether you are on trap-safe grids */
	FLOOR_IDX floor_id;            /* Current floor location */ 

	bool autopick_autoregister; /* auto register is in-use or not */

	byte feeling;		/* Most recent dungeon feeling */
	s32b feeling_turn;	/* The current_world_ptr->game_turn of the last dungeon feeling */

	object_type *inventory_list; /* The player's p_ptr->inventory_list [INVEN_TOTAL] */
	s16b inven_cnt; /* Number of items in inventory */
	s16b equip_cnt; /* Number of items in equipment */

							/*** Temporary fields ***/

	bool playing;			/* True if player is playing */
	bool leaving;			/* True if player is leaving */

	bool monk_armour_aux;
	bool monk_notify_aux;

	byte leave_bldg;
	byte exit_bldg;			/* Goal obtained in arena? -KMW- */

	bool leaving_dungeon;	/* True if player is leaving the dungeon */
	bool teleport_town;
	bool enter_dungeon;     /* Just enter the dungeon */

	IDX health_who;	/* Health bar trackee */

	MONRACE_IDX monster_race_idx;	/* Monster race trackee */

	KIND_OBJECT_IDX object_kind_idx;	/* Object kind trackee */

	s16b new_spells;	/* Number of spells available */
	s16b old_spells;

	s16b old_food_aux;	/* Old value of food */

	bool old_cumber_armor;
	bool old_cumber_glove;
	bool old_heavy_wield[2];
	bool old_heavy_shoot;
	bool old_icky_wield[2];
	bool old_riding_wield[2];
	bool old_riding_ryoute;
	bool old_monlite;

	POSITION old_lite;		/* Old radius of lite (if any) */

	bool cumber_armor;	/* Mana draining armor */
	bool cumber_glove;	/* Mana draining gloves */
	bool heavy_wield[2];	/* Heavy weapon */
	bool heavy_shoot;	/* Heavy shooter */
	bool icky_wield[2];	/* Icky weapon */
	bool riding_wield[2];	/* Riding weapon */
	bool riding_ryoute;	/* Riding weapon */
	bool monlite;

	POSITION cur_lite;		/* Radius of lite (if any) */

	BIT_FLAGS update;	/* Pending Updates */
		#define PU_BONUS        0x00000001L     /*!< ステータス更新フラグ: 能力値修正 / Calculate bonuses */
		#define PU_TORCH        0x00000002L     /*!< ステータス更新フラグ: 光源半径 / Calculate torch radius */
		#define PU_HP           0x00000010L     /*!< ステータス更新フラグ: HP / Calculate chp and mhp */
		#define PU_MANA         0x00000020L     /*!< ステータス更新フラグ: MP / Calculate csp and msp */
		#define PU_SPELLS       0x00000040L     /*!< ステータス更新フラグ: 魔法学習数 / Calculate spells */
		#define PU_COMBINE      0x00000100L     /*!< アイテム処理フラグ: アイテムの結合を要する / Combine the pack */
		#define PU_REORDER      0x00000200L     /*!< アイテム処理フラグ: アイテムの並び替えを要する / Reorder the pack */
		#define PU_AUTODESTROY  0x00000400L     /*!< アイテム処理フラグ: アイテムの自動破壊を要する / Auto-destroy marked item */
		#define PU_UN_VIEW      0x00010000L     /*!< ステータス更新フラグ: 地形の視界外化 / Forget view */
		#define PU_UN_LITE      0x00020000L     /*!< ステータス更新フラグ: 明暗範囲の視界外化 / Forget lite */
		#define PU_VIEW         0x00100000L     /*!< ステータス更新フラグ: 視界 / Update view */
		#define PU_LITE         0x00200000L     /*!< ステータス更新フラグ: 明暗範囲 / Update lite */
		#define PU_MON_LITE     0x00400000L     /*!< ステータス更新フラグ: モンスターの光源範囲 / Monster illumination */
		#define PU_DELAY_VIS    0x00800000L     /*!< ステータス更新フラグ: 視界の追加更新 / Mega-Hack -- Delayed visual update */
		#define PU_MONSTERS     0x01000000L     /*!< ステータス更新フラグ: モンスターのステータス / Update monsters */
		#define PU_DISTANCE     0x02000000L     /*!< ステータス更新フラグ: プレイヤーとモンスターの距離 / Update distances */
		#define PU_FLOW         0x10000000L     /*!< ステータス更新フラグ: プレイヤーから各マスへの到達距離 / Update flow */

	BIT_FLAGS redraw;	/* Normal Redraws */
	BIT_FLAGS window;	/* Window Redraws */

	s16b stat_use[A_MAX];	/* Current modified stats */
	s16b stat_top[A_MAX];	/* Maximal modified stats */

	bool sutemi;
	bool counter;

	ALIGNMENT align; /* Good/evil/neutral */
	POSITION run_py;
	POSITION run_px;
	DIRECTION fishing_dir;


	/*** Extracted fields ***/

	WEIGHT total_weight;	/*!< 所持品と装備品の計算総重量 / Total weight being carried */

	s16b stat_add[A_MAX];	/* Modifiers to stat values */
	s16b stat_ind[A_MAX];	/* Indexes into stat tables */

	bool hack_mutation;

	bool immune_acid;	/* Immunity to acid */
	bool immune_elec;	/* Immunity to lightning */
	bool immune_fire;	/* Immunity to fire */
	bool immune_cold;	/* Immunity to cold */

	bool resist_acid;	/* Resist acid */
	bool resist_elec;	/* Resist lightning */
	bool resist_fire;	/* Resist fire */
	bool resist_cold;	/* Resist cold */
	bool resist_pois;	/* Resist poison */

	bool resist_conf;	/* Resist confusion */
	bool resist_sound;	/* Resist sound */
	bool resist_lite;	/* Resist light */
	bool resist_dark;	/* Resist darkness */
	bool resist_chaos;	/* Resist chaos */
	bool resist_disen;	/* Resist disenchant */
	bool resist_shard;	/* Resist shards */
	bool resist_nexus;	/* Resist nexus */
	bool resist_blind;	/* Resist blindness */
	bool resist_neth;	/* Resist nether */
	bool resist_fear;	/* Resist fear */
	bool resist_time;	/* Resist time */
	bool resist_water;	/* Resist water */

	bool reflect;       /* Reflect 'bolt' attacks */
	bool sh_fire;       /* Fiery 'immolation' effect */
	bool sh_elec;       /* Electric 'immolation' effect */
	bool sh_cold;       /* Cold 'immolation' effect */

	bool anti_magic;    /* Anti-magic */
	bool anti_tele;     /* Prevent teleportation */

	bool sustain_str;	/* Keep strength */
	bool sustain_int;	/* Keep intelligence */
	bool sustain_wis;	/* Keep wisdom */
	bool sustain_dex;	/* Keep dexterity */
	bool sustain_con;	/* Keep constitution */
	bool sustain_chr;	/* Keep charisma */

	BIT_FLAGS cursed;	/* Player is cursed */

	bool can_swim;		/* No damage falling */
	bool levitation;		/* No damage falling */
	bool lite;		/* Permanent light */
	bool free_act;		/* Never paralyzed */
	bool see_inv;		/* Can see invisible */
	bool regenerate;	/* Regenerate hit pts */
	bool hold_exp;		/* Resist exp draining */

	bool telepathy;		/* Telepathy */
	bool esp_animal;
	bool esp_undead;
	bool esp_demon;
	bool esp_orc;
	bool esp_troll;
	bool esp_giant;
	bool esp_dragon;
	bool esp_human;
	bool esp_evil;
	bool esp_good;
	bool esp_nonliving;
	bool esp_unique;

	bool slow_digest;	/* Slower digestion */
	bool bless_blade;	/* Blessed blade */
	bool xtra_might;	/* Extra might bow */
	bool impact[2];		/* Earthquake blows */
	bool pass_wall;     /* Permanent wraithform */
	bool kill_wall;
	bool dec_mana;
	bool easy_spell;
	bool heavy_spell;
	bool warning;
	bool mighty_throw;
	bool see_nocto;		/* Noctovision */

	DICE_NUMBER to_dd[2]; /* Extra dice/sides */
	DICE_SID to_ds[2];

	HIT_PROB dis_to_h[2];	/*!< 判明している現在の表記上の近接武器命中修正値 /  Known bonus to hit (wield) */
	HIT_PROB dis_to_h_b;	/*!< 判明している現在の表記上の射撃武器命中修正値 / Known bonus to hit (bow) */
	HIT_POINT dis_to_d[2];	/*!< 判明している現在の表記上の近接武器ダメージ修正値 / Known bonus to dam (wield) */
	ARMOUR_CLASS dis_to_a;	/*!< 判明している現在の表記上の装備AC修正値 / Known bonus to ac */
	ARMOUR_CLASS dis_ac;	/*!< 判明している現在の表記上の装備AC基礎値 / Known base ac */

	s16b to_h[2];		/* Bonus to hit (wield) */
	s16b to_h_b;		/* Bonus to hit (bow) */
	s16b to_h_m;		/* Bonus to hit (misc) */
	s16b to_d[2];		/* Bonus to dam (wield) */
	s16b to_d_m;		/* Bonus to dam (misc) */
	s16b to_a;			/* Bonus to ac */

	s16b to_m_chance;		/* Minusses to cast chance */

	bool ryoute;
	bool migite;
	bool hidarite;
	bool no_flowed;

	ARMOUR_CLASS ac;	/*!< 装備無しの基本AC / Base ac */

	ACTION_SKILL_POWER see_infra;	/*!< 赤外線視能力の強さ /Infravision range */
	ACTION_SKILL_POWER skill_dis;	/*!< 行動技能値:解除能力 / Skill: Disarming */
	ACTION_SKILL_POWER skill_dev;	/*!< 行動技能値:魔道具使用 / Skill: Magic Devices */
	ACTION_SKILL_POWER skill_sav;	/*!< 行動技能値:魔法防御 / Skill: Saving throw */
	ACTION_SKILL_POWER skill_stl;	/*!< 行動技能値:隠密 / Skill: Stealth factor */

	/*! 
	 * 行動技能値:知覚 / Skill: Searching ability
	 * この値はsearch()による地形の隠し要素発見処理などで混乱、盲目、幻覚、無光源などの
	 * 状態異常がない限り、難易度修正などがないままそのままパーセンテージ値として使われる。
	 * 100以上ならば必ず全てのトラップなどを見つけることが出来る。
	 */
	ACTION_SKILL_POWER skill_srh;

	ACTION_SKILL_POWER skill_fos;	/*!< 行動技能値:探索 / Skill: Searching frequency */
	ACTION_SKILL_POWER skill_thn;	/*!< 行動技能値:打撃命中能力 / Skill: To hit (normal) */
	ACTION_SKILL_POWER skill_thb;	/*!< 行動技能値:射撃命中能力 / Skill: To hit (shooting) */
	ACTION_SKILL_POWER skill_tht;	/*!< 行動技能値:投射命中能力 / Skill: To hit (throwing) */
	ACTION_SKILL_POWER skill_dig;	/*!< 行動技能値:掘削 / Skill: Digging */

	s16b num_blow[2];	/* Number of blows */
	s16b num_fire;		/* Number of shots */

	byte tval_xtra;		/* Correct xtra tval */
	byte tval_ammo;		/* Correct ammo tval */

	byte pspeed;		/* Current speed */

	ENERGY energy_use;	/* Energy use this current_world_ptr->game_turn */

	POSITION y;	/* Player location in dungeon */
	POSITION x;	/* Player location in dungeon */
	GAME_TEXT name[32]; /*!< 現在のプレイヤー名 / Current player's character name */
};


/*
 * A structure to hold "rolled" information
 */
typedef struct birther birther;

struct birther
{
	SEX_IDX psex;		/* Sex index */
	RACE_IDX prace;		/* Race index */
	CLASS_IDX pclass;	/* Class index */
	CHARACTER_IDX pseikaku;	/* Seikaku index */
	REALM_IDX realm1;	/* First magic realm */
	REALM_IDX realm2;	/* Second magic realm */

	s16b age;
	s16b ht;
	s16b wt;
	s16b sc;

	PRICE au; /*!< 初期の所持金 */

	BASE_STATUS stat_max[6];	/* Current "maximal" stat values */
	BASE_STATUS stat_max_max[6];	/* Maximal "maximal" stat values */
	HIT_POINT player_hp[PY_MAX_LEVEL];

	PATRON_IDX chaos_patron;

	s16b vir_types[8];

	char history[4][60];

	bool quick_ok;
};


/* For Monk martial arts */

typedef struct martial_arts martial_arts;

struct martial_arts
{
	concptr desc;       /* A verbose attack description */
	PLAYER_LEVEL min_level;  /* Minimum level to use */
	int chance;     /* Chance of 'success' */
	int dd;         /* Damage dice */
	int ds;         /* Damage sides */
	int effect;     /* Special effects */
};

typedef struct kamae kamae;

struct kamae
{
	concptr desc;       /* A verbose kamae description */
	PLAYER_LEVEL min_level;  /* Minimum level to use */
	concptr info;
};


/* Imitator */

typedef struct monster_power monster_power;
struct monster_power
{
	PLAYER_LEVEL level;
	MANA_POINT smana;
	PERCENTAGE fail;
	int     manedam;
	int     manefail;
	int     use_stat;
	concptr    name;
};


/*
 * A structure describing a town with
 * stores and buildings
 */
typedef struct town_type town_type;
struct town_type
{
	GAME_TEXT name[32];
	u32b seed;      /* Seed for RNG */
	store_type *store;    /* The stores [MAX_STORES] */
	byte numstores;
};

/*
 * Sort-array element
 */
typedef struct tag_type tag_type;

struct tag_type
{
	int tag;
	int index;
};

typedef bool (*monsterrace_hook_type)(MONRACE_IDX r_idx);


/*
 * This seems like a pretty standard "typedef"
 */
typedef int (*inven_func)(object_type *);


typedef struct
{
	FEAT_IDX feat;    /* Feature tile */
	PERCENTAGE percent; /* Chance of type */
}
feat_prob;


/*!
 * @struct autopick_type
 * @brief 自動拾い/破壊設定データの構造体 / A structure type for entry of auto-picker/destroyer
 */
typedef struct {
	concptr name;          /*!< 自動拾い/破壊定義の名称一致基準 / Items which have 'name' as part of its name match */
	concptr insc;          /*!< 対象となったアイテムに自動で刻む内容 / Items will be auto-inscribed as 'insc' */
	BIT_FLAGS flag[2];       /*!< キーワードに関する汎用的な条件フラグ / Misc. keyword to be matched */
	byte action;        /*!< 対象のアイテムを拾う/破壊/放置するかの指定フラグ / Auto-pickup or Destroy or Leave items */
	byte dice;          /*!< 武器のダイス値基準値 / Weapons which have more than 'dice' dice match */
	byte bonus;         /*!< アイテムのボーナス基準値 / Items which have more than 'bonus' magical bonus match */
} autopick_type;


/*
 *  A structure type for the saved floor
 */
typedef struct 
{
	FLOOR_IDX floor_id;        /* No recycle until 65536 IDs are all used */
	s16b savefile_id;     /* ID for savefile (from 0 to MAX_SAVED_FLOOR) */
	DEPTH dun_level;
	s32b last_visit;      /* Time count of last visit. 0 for new floor. */
	u32b visit_mark;      /* Older has always smaller mark. */
	FLOOR_IDX upper_floor_id;  /* a floor connected with level teleportation */
	FLOOR_IDX lower_floor_id;  /* a floor connected with level tel. and trap door */
} saved_floor_type;


/*
 *  A structure type for terrain template of saving dungeon floor
 */
typedef struct
{
	BIT_FLAGS info;
	FEAT_IDX feat;
	FEAT_IDX mimic;
	s16b special;
	u16b occurrence;
} cave_template_type;


#ifdef TRAVEL
/*
 *  A structure type for travel command
 */
typedef struct {
	int run; /* Remaining grid number */
	int cost[MAX_HGT][MAX_WID];
	POSITION x; /* Target X */
	POSITION y; /* Target Y */
	DIRECTION dir; /* Running direction */
} travel_type;
#endif

typedef struct {
	concptr flag;
	byte index;
	byte level;
	s32b value;
	struct {
		int constant;
		DICE_NUMBER dice;
	} timeout;
	concptr desc;
} activation_type;

typedef struct {
	int flag;
	int type;
	concptr name;
} dragonbreath_type;
