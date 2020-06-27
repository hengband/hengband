#pragma once

#include "system/angband.h"

/*
 * Number of feats we change to (Excluding default). Used in f_info.txt.
 */
#define MAX_FEAT_STATES	 8

 /*
  * Feature flags - should be used instead of feature indexes unless generating.
  * Originally from UnAngband, and modified into TR-like style in Hengband
  */

#define FF_LOS           0              /*!< 視界が通る地形である */
#define FF_PROJECT       1              /*!< 飛び道具が通過できる地形である */
#define FF_MOVE          2              /*!< 移動可能な地形である */
#define FF_PLACE         3              /*!< モンスター配置をしても良い地形である(cave_empty_bold/cave_empty_gridで利用) */
#define FF_DROP          4              /*!< アイテムを落としてよい地形である */
#define FF_SECRET        5              /*!< 隠し扉やトラップが潜んでいる地形である */
#define FF_NOTICE        6              /*!< 何か興味を引くものがある地形である(シフトキー＋方向で走行中の時に止まる基準) */
#define FF_REMEMBER      7              /*!< 常に記憶対象となる地形である(記憶喪失時に忘れたりしなくなる) */
#define FF_OPEN          8              /*!< 開けるコマンドの対象となる地形である */
#define FF_CLOSE         9              /*!< 閉じるコマンドの対象となる地形である */
#define FF_BASH          10             /*!< 体当たりコマンドの対象となる地形である */
#define FF_SPIKE         11             /*!< くさびを打つコマンドの対象となる地形である */
#define FF_DISARM        12             /*!< 解除コマンドの対象となる地形である */
#define FF_STORE         13             /*!< 店舗の入口となる地形である */
#define FF_TUNNEL        14             /*!< 魔王変化などで掘り進められる地形である */
#define FF_MAY_HAVE_GOLD 15             /*!< 何か財宝を隠した可能性のある地形である？(f_infoに使用している地形なし) */
#define FF_HAS_GOLD      16             /*!< 財宝を含んだ地形である */
#define FF_HAS_ITEM      17             /*!< アイテムを含んだ地形である */
#define FF_DOOR          18             /*!< ドアのある地形である */
#define FF_TRAP          19             /*!< トラップのある地形である */
#define FF_STAIRS        20             /*!< 階段のある地形である */
#define FF_GLYPH         21             /*!< 守りのルーンが張られた地形である */
#define FF_LESS          22             /*!< 階上に通じる地形である */
#define FF_MORE          23             /*!< 階下に通じる地形である */
#define FF_AVOID_RUN     24             /*!< 自動移動機能時に障害として迂回すべき地形である */
#define FF_FLOOR         25             /*!< 床のある地形である */
#define FF_WALL          26             /*!< 壁のある地形である */
#define FF_PERMANENT     27             /*!< 絶対に破壊できない永久地形である */
  /* #define FF_XXX00         28  未定義 */
  /* #define FF_XXX01         29  未定義 */
  /* #define FF_XXX02         30  未定義 */
#define FF_HIT_TRAP      31             /*!< トラップのある地形である(TRAPと常に重複している？) */
/* #define FF_BRIDGE        32  未使用 */
/* #define FF_RIVER         33  未使用 */
/* #define FF_LAKE          34  未使用 */
/* #define FF_BRIDGED       35  未使用 */
/* #define FF_COVERED       36  未使用 */
#define FF_GLOW          37             /*!< 常に光っている地形である */
#define FF_ENSECRET      38             /*!< 不明(f_info.txt上で利用している地形がない) */
#define FF_WATER         39             /*!< 水のある地形である */
#define FF_LAVA          40             /*!< 溶岩のある地形である */
#define FF_SHALLOW       41             /*!< 浅い地形である */
#define FF_DEEP          42             /*!< 深い地形である */
#define FF_POISON_PUDDLE 43             /*!< 毒溜まりがある */
#define FF_HURT_ROCK     44             /*!< 岩石溶解の対象となる地形である */
/* #define FF_HURT_FIRE     45 */       /*!< 未使用 */
/* #define FF_HURT_COLD     46 */       /*!< 未使用 */
/* #define FF_HURT_ACID     47 */       /*!< 未使用 */
#define FF_COLD_PUDDLE   48             /*!< 冷気溜まりがある */
#define FF_ACID_PUDDLE   49             /*!< 酸溜まりがある */
/* #define FF_OIL           50 */       /*!< 未使用 */
#define FF_ELEC_PUDDLE   51             /*!< 接地部が帯電している */
/* #define FF_CAN_CLIMB     52 */       /*!< 未使用 */
#define FF_CAN_FLY       53             /*!< 飛行可能な地形である */
#define FF_CAN_SWIM      54             /*!< 泳ぐことが可能な地形である */
#define FF_CAN_PASS      55             /*!< 通過可能な地形である */
/* #define FF_CAN_OOZE      56 */       /*!< 未使用 */
#define FF_CAN_DIG       57             /*!< 掘削コマンドの対象となる地形である */
/* #define FF_HIDE_ITEM     58  未使用 */
/* #define FF_HIDE_SNEAK    59  未使用 */
/* #define FF_HIDE_SWIM     60  未使用 */
/* #define FF_HIDE_DIG      61  未使用 */
/* #define FF_KILL_HUGE     62  未使用 */
/* #define FF_KILL_MOVE     63  未使用 */
/* #define FF_PICK_TRAP     64  未使用 */
/* #define FF_PICK_DOOR     65  未使用 */
/* #define FF_ALLOC         66  未使用 */
/* #define FF_CHEST         67  未使用 */
/* #define FF_DROP_1D2      68  未使用 */
/* #define FF_DROP_2D2      69  未使用 */
/* #define FF_DROP_GOOD     70  未使用 */
/* #define FF_DROP_GREAT    71  未使用 */
/* #define FF_HURT_POIS     72  未使用 */
/* #define FF_HURT_ELEC     73  未使用 */
/* #define FF_HURT_WATER    74  未使用 */
/* #define FF_HURT_BWATER   75  未使用 */
/* #define FF_USE_FEAT      76  未使用 */
/* #define FF_GET_FEAT      77  未使用 */
/* #define FF_GROUND        78  未使用 */
/* #define FF_OUTSIDE       79  未使用 */
/* #define FF_EASY_HIDE     80  未使用 */
/* #define FF_EASY_CLIMB    81  未使用 */
/* #define FF_MUST_CLIMB    82  未使用 */
#define FF_TREE          83             /*!< 木の生えた地形である */
/* #define FF_NEED_TREE     84  未使用 */
/* #define FF_BLOOD         85  未使用 */
/* #define FF_DUST          86  未使用 */
/* #define FF_SLIME         87  未使用 */
#define FF_PLANT         88             /*!< 植物の生えた地形である */
/* #define FF_XXX2          89  未定義 */
/* #define FF_INSTANT       90  未使用 */
/* #define FF_EXPLODE       91  未使用 */
/* #define FF_TIMED         92  未使用 */
/* #define FF_ERUPT         93  未使用 */
/* #define FF_STRIKE        94  未使用 */
/* #define FF_SPREAD        95  未使用 */
#define FF_SPECIAL       96             /*!< クエストやダンジョンに関わる特別な地形である */
#define FF_HURT_DISI     97             /*!< 分解属性の対象となる地形である */
#define FF_QUEST_ENTER   98             /*!< クエストの入り口である */
#define FF_QUEST_EXIT    99             /*!< クエストの出口である */
#define FF_QUEST         100            /*!< クエストに関する地形である */
#define FF_SHAFT         101            /*!< 坑道である。(2階層移動する階段である) */
#define FF_MOUNTAIN      102            /*!< ダンジョンの山地形である */
#define FF_BLDG          103            /*!< 施設の入り口である */
#define FF_MINOR_GLYPH   104            /*!< 爆発のルーンのある地形である */
#define FF_PATTERN       105            /*!< パターンのある地形である */
#define FF_TOWN          106            /*!< 広域マップ用の街がある地形である */
#define FF_ENTRANCE      107            /*!< 広域マップ用のダンジョンがある地形である */
#define FF_MIRROR        108            /*!< 鏡使いの鏡が張られた地形である */
#define FF_UNPERM        109            /*!< 破壊不能な地形である(K:フラグ向け？) */
#define FF_TELEPORTABLE  110            /*!< テレポート先の対象となる地形である */
#define FF_CONVERT       111            /*!< 地形生成処理中の疑似フラグ */
#define FF_GLASS         112            /*!< ガラス製の地形である */

#define FF_FLAG_MAX      113
#define FF_FLAG_SIZE     (1 + ((FF_FLAG_MAX - 1) / 32))


/* Lighting levels of features' attr and char */

#define F_LIT_STANDARD 0 /* Standard */
#define F_LIT_LITE     1 /* Brightly lit */
#define F_LIT_DARK     2 /* Darkened */

#define F_LIT_NS_BEGIN 1 /* Nonstandard */
#define F_LIT_MAX      3


/*!
 * @struct feature_state
 * @brief 地形状態変化指定構造体 / Feature state structure
 */
typedef struct feature_state {
	FF_FLAGS_IDX action; /*!< 変化条件をFF_*のIDで指定 / Action (FF_*) */
	STR_OFFSET result_tag; /*!< 変化先ID / Result (f_info ID) */
	FEAT_IDX result; /*!< 変化先ID / Result (f_info ID) */
} feature_state;

typedef struct feat_prob {
	FEAT_IDX feat;    /* Feature tile */
	PERCENTAGE percent; /* Chance of type */
} feat_prob;

/*!
 * @struct feature_type
 * @brief 地形情報の構造体 / Information about terrain "features"
 */
typedef struct feature_type {
	STR_OFFSET name;                /*!< 地形名参照のためのネームバッファオフセット値 / Name (offset) */
	STR_OFFSET text;                /*!< 地形説明参照のためのネームバッファオフセット値 /  Text (offset) */
	STR_OFFSET tag;                 /*!< 地形特性タグ参照のためのネームバッファオフセット値 /  Tag (offset) */

	STR_OFFSET mimic_tag;
	STR_OFFSET destroyed_tag;

	FEAT_IDX mimic;               /*!< 未確定時の外形地形ID / Feature to mimic */
	FEAT_IDX destroyed;           /*!< *破壊*に巻き込まれた時の地形移行先(未実装？) / Default destroyed state */

	BIT_FLAGS flags[FF_FLAG_SIZE]; /*!< 地形の基本特性ビット配列 / Flags */

	FEAT_PRIORITY priority;            /*!< 縮小表示で省略する際の表示優先度 / Map priority */

	feature_state state[MAX_FEAT_STATES]; /*!< feature_state テーブル */

	FEAT_SUBTYPE subtype;  /*!< 副特性値 */
	FEAT_POWER power;    /*!< 地形強度 */

	TERM_COLOR d_attr[F_LIT_MAX];   /*!< デフォルトの地形シンボルカラー / Default feature attribute */
	SYMBOL_CODE d_char[F_LIT_MAX];   /*!< デフォルトの地形シンボルアルファベット / Default feature character */

	TERM_COLOR x_attr[F_LIT_MAX];   /*!< 設定変更後の地形シンボルカラー / Desired feature attribute */
	SYMBOL_CODE x_char[F_LIT_MAX];   /*!< 設定変更後の地形シンボルアルファベット / Desired feature character */
} feature_type;

extern FEAT_IDX max_f_idx;
extern feature_type *f_info;
extern char *f_name;
extern char *f_tag;

extern bool is_closed_door(player_type *player_ptr, FEAT_IDX feat);
extern bool is_trap(player_type *player_ptr, FEAT_IDX feat);
void apply_default_feat_lighting(TERM_COLOR *f_attr, SYMBOL_CODE *f_char);
bool is_ascii_graphics(char x);

/*** Terrain feature variables ***/
extern FEAT_IDX feat_none;
extern FEAT_IDX feat_floor;
extern FEAT_IDX feat_glyph;
extern FEAT_IDX feat_explosive_rune;
extern FEAT_IDX feat_mirror;
extern FEAT_IDX feat_up_stair;
extern FEAT_IDX feat_down_stair;
extern FEAT_IDX feat_entrance;
extern FEAT_IDX feat_trap_open;
extern FEAT_IDX feat_trap_armageddon;
extern FEAT_IDX feat_trap_piranha;
extern FEAT_IDX feat_rubble;
extern FEAT_IDX feat_magma_vein;
extern FEAT_IDX feat_quartz_vein;
extern FEAT_IDX feat_granite;
extern FEAT_IDX feat_permanent;
extern FEAT_IDX feat_glass_floor;
extern FEAT_IDX feat_glass_wall;
extern FEAT_IDX feat_permanent_glass_wall;
extern FEAT_IDX feat_pattern_start;
extern FEAT_IDX feat_pattern_1;
extern FEAT_IDX feat_pattern_2;
extern FEAT_IDX feat_pattern_3;
extern FEAT_IDX feat_pattern_4;
extern FEAT_IDX feat_pattern_end;
extern FEAT_IDX feat_pattern_old;
extern FEAT_IDX feat_pattern_exit;
extern FEAT_IDX feat_pattern_corrupted;
extern FEAT_IDX feat_black_market;
extern FEAT_IDX feat_town;
extern FEAT_IDX feat_deep_water;
extern FEAT_IDX feat_shallow_water;
extern FEAT_IDX feat_deep_lava;
extern FEAT_IDX feat_shallow_lava;
extern FEAT_IDX feat_heavy_cold_zone;
extern FEAT_IDX feat_cold_zone;
extern FEAT_IDX feat_heavy_electrical_zone;
extern FEAT_IDX feat_electrical_zone;
extern FEAT_IDX feat_deep_acid_puddle;
extern FEAT_IDX feat_shallow_acid_puddle;
extern FEAT_IDX feat_deep_poisonous_puddle;
extern FEAT_IDX feat_shallow_poisonous_puddle;
extern FEAT_IDX feat_dirt;
extern FEAT_IDX feat_grass;
extern FEAT_IDX feat_flower;
extern FEAT_IDX feat_brake;
extern FEAT_IDX feat_tree;
extern FEAT_IDX feat_mountain;
extern FEAT_IDX feat_swamp;
extern FEAT_IDX feat_undetected;

extern FEAT_IDX feat_wall_outer;
extern FEAT_IDX feat_wall_inner;
extern FEAT_IDX feat_wall_solid;
extern FEAT_IDX feat_ground_type[100], feat_wall_type[100];
