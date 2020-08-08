#pragma once

/*!
 * @file h-type.h
 * @brief ゲーム中に用いる変数型定義 / Basic "types".
 * @date 2017/12/03
 * @author
 * 不明(変愚蛮怒スタッフ？)
 * @details
 * <pre>
 * Note the attempt to make all basic types have 4 letters.
 * This improves readibility and standardizes the code.
 * Likewise, all complex types are at least 4 letters.
 * Thus, almost every three letter word is a legal variable.
 * But beware of certain reserved words ('for' and 'if' and 'do').
 * Note that the type used in structures for bit flags should be uint.
 * As long as these bit flags are sequential, they will be space smart.
 * Note that on some machines, apparently "signed char" is illegal.
 * It must be true that char/byte takes exactly 1 byte
 * It must be true that sind/uind takes exactly 2 bytes
 * It must be true that sbig/ubig takes exactly 4 bytes
 * On Sparc's, a uint takes 4 bytes (2 is legal)
 * On Sparc's, a long takes 4 bytes (8 is legal)
 * On Sparc's, a huge takes 4 bytes (8 is legal)
 * On Sparc's, a vptr takes 4 bytes (8 is legal)
 * On Sparc's, a real takes 8 bytes (4 is legal)
 * Note that some files have already been included by "h-include.h"
 * These include <stdio.h> and <sys/types>, which define some types
 * In particular, uint is defined so we do not have to define it
 * Also, see <limits.h> for min/max values for sind, uind, long, huge
 * (SHRT_MIN, SHRT_MAX, USHRT_MAX, LONG_MIN, LONG_MAX, ULONG_MAX)
 * These limits should be verified and coded into "h-constant.h".
 * </pre>
 */

#ifndef INCLUDED_H_TYPE_H
#define INCLUDED_H_TYPE_H

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/*** Special 4 letter names for some standard types ***/
typedef void *vptr;       /*!< void型ポインタ定義 / A standard pointer (to "void" because ANSI C says so) */
typedef const char *concptr; /*!< 文字列定数用ポインタ定義 / A simple pointer (to unmodifiable strings) */

/*!
 * @brief エラーコードの定義 / Error codes for function return values
 * @details
 * 一般に成功時0、失敗時負数、何らかの問題時正数とする。
 * Success = 0, Failure = -N, Problem = +N 
 */
typedef int errr;

#define MAX_UCHAR       255 /*!< Maximum value storable in a "byte" (hard-coded) */
#define MAX_SHORT       32767    /*!< Maximum value storable in a "s16b" (hard-coded) */

#define MAX_NLEN        160 /*!< Maximum length of object's name */

/* Note that "signed char" is not always "defined" */
/* So always use "s16b" to hold small signed values */
/* A signed byte of memory */
/* typedef signed char syte; */
typedef unsigned char byte; /*!< byte型をunsighned charとして定義 / Note that unsigned values can cause math problems / An unsigned byte of memory */
typedef char bool; /*!< bool型をcharとして定義 / Note that a bool is smaller than a full "int" / Simple True/False type */
typedef unsigned int uint; /* uint型をintとして定義 /  An unsigned, "standard" integer (often pre-defined) */

/* The largest possible unsigned integer */
typedef unsigned long huge;

/* Signed/Unsigned 16 bit value */
#ifdef HAVE_STDINT_H
typedef int16_t s16b;
typedef uint16_t u16b;
#else
typedef signed short s16b;
typedef unsigned short u16b;
#endif

/* Signed/Unsigned 32 bit value */
#ifdef HAVE_STDINT_H
typedef int32_t s32b;
typedef uint32_t u32b;
#else
typedef signed long s32b;
typedef unsigned long u32b;
#endif

typedef s16b IDX;				/*!< ゲーム中のID型を定義 */

typedef s16b TOWN_IDX;			/*!< ゲーム中の町ID型を定義 */
typedef s16b FEAT_IDX;			/*!< ゲーム中の地形ID型を定義 */
typedef s16b FLOOR_IDX;			/*!< ゲーム中のフロアID型を定義 */

typedef byte SEX_IDX;			/*!< ゲーム中のプレイヤー性別ID型を定義 */
typedef s16b MIMIC_RACE_IDX;	/*!< ゲーム中のプレイヤー変身種族ID型を定義 */
typedef s16b PATRON_IDX;		/*!< ゲーム中のプレイヤーパトロンID型を定義 */
typedef s32b ACTION_IDX;		/*!< プレイヤーが現在取っている常時行動のID定義 */
typedef s16b BACT_IDX;			/*!< 町の施設処理のID定義 */
typedef s16b BACT_RESTRICT_IDX;	/*!< 町の施設処理の規制処理ID定義 */

typedef s16b MONRACE_IDX;		/*!< todo monster_race_typeに差し替えて消滅させる ゲーム中のモンスター種族ID型を定義 */
typedef s16b MONSTER_IDX;		/*!< todo monster_race_typeに差し替えて消滅させる ゲーム中のモンスター個体ID型を定義 */
typedef s16b DUNGEON_IDX;		/*!< ゲーム中のダンジョンID型を定義 */
typedef s16b REALM_IDX;			/*!< ゲーム中の魔法領域ID型を定義 */
typedef s16b ARTIFACT_IDX;		/*!< ゲーム中のアーティファクトID型を定義 */
typedef s16b EGO_IDX;			/*!< アイテムエゴのID型を定義 */
typedef s16b ACTIVATION_IDX;	/*!< アイテムの発動効果ID型を定義 */
typedef s16b VIRTUES_IDX;		/*!< ゲーム中の徳ID型を定義 */
typedef s16b QUEST_IDX;			/*!< ゲーム中のクエストID型を定義 */
typedef byte ROOM_IDX;			/*!< 部屋のID型を定義 */

typedef s16b INVENTORY_IDX;		/*!< ゲーム中の所持品ID型を定義 */
typedef s16b OBJECT_IDX;		/*!< ゲーム中のアイテムID型を定義 */
typedef s32b ESSENCE_IDX;		/*!< 鍛冶エッセンスのID型 */
typedef s16b KIND_OBJECT_IDX;	/*!< ゲーム中のベースアイテムID型を定義 */
typedef s16b VAULT_IDX;			/*!< 固定部屋のID型を定義 */
typedef int MUTATION_IDX;		/*!< 突然変異のID型を定義 */

typedef s32b POSITION;			/*!< ゲーム中の座標型を定義 */
typedef s16b POSITION_IDX;		/*!< ゲーム中の座標リストID型 */

typedef s32b ALIGNMENT;		/*!< 善悪属性の型定義 */
typedef byte FEAT_SUBTYPE;	/*!< 地形情報の副値 (トラップ種別/パターン種別/店舗種別)*/

typedef char GAME_TEXT;		/*!< ゲーム中のテキスト型定義 */

/*!
 * @var typedef s32b HIT_POINT
 * @brief HPとその増減量の型定義
 * @details 
 * HIT_POINTはプレイヤー及びモンスターのHPの各値とその増減量の型である。
 */
typedef s32b HIT_POINT;

/*!
 * @var typedef s32b MANA_POINT
 * @brief MPとその増減量の型定義
 * @details
 * MANA_POINTはプレイヤーのMPの各地とその増減量の型である。
 */
typedef s32b MANA_POINT;		/*!< ゲーム中のMP型を定義 */

typedef int ARTIFACT_BIAS_IDX;  /*!< ランダムアーティファクトのバイアス型 */

typedef s16b HIT_PROB;			/*!< ゲーム中の装備命中修正値を定義 */
typedef int HIT_RELIABILITY;	/*!< 最終算出命中力を定義 */
typedef s16b BASE_STATUS;		/*!< ゲーム中の基礎能力値型を定義 */

typedef s32b MONSTER_NUMBER;	/*!< ゲーム中のモンスター数型を定義 */
typedef s32b ITEM_NUMBER;		/*!< ゲーム中のアイテム数型を定義 */

typedef s16b ACTION_ENERGY;		/*!< ゲーム中の行動エネルギー型を定義 */
typedef s16b ARMOUR_CLASS;		/*!< ゲーム中の行動アーマークラス型を定義 */
typedef s16b TIME_EFFECT;		/*!< ゲーム中の時限期間の型を定義 */
typedef byte DISCOUNT_RATE;		/*!< ゲーム中の値引き率の型を定義 */
typedef s16b SPEED;				/*!< ゲーム中の加速値の型定義 */

/*!
 * @var typedef s16b ENEGRY
 * @brief 行動エネルギーの型定義
 * @details
 * ENERGYはプレイヤーとモンスターの行動順を定める行動エネルギーを示す型定義である。
 */
typedef s16b ENERGY;			/*!< ゲーム中の行動エネルギーの型定義 */


typedef s16b SLEEP_DEGREE;		/*!< モンスターの睡眠度の型定義 */

typedef s16b PLAYER_LEVEL;		/*!< ゲーム中のプレイヤーレベルの型を定義 */
typedef int DIRECTION;			/*!< ゲーム中の方角の型定義 */
typedef s32b EXP;				/*!< ゲーム中の主経験値の型定義 */
typedef s16b SUB_EXP;			/*!< ゲーム中の副経験値の型定義 */

typedef s32b OBJECT_SUBTYPE_VALUE; /*!< ゲーム中のアイテム副分類の型定義 */
typedef s16b PARAMETER_VALUE;      /*!< ゲーム中のアイテム能力値の型定義 */
typedef s32b WEIGHT;               /*!< ゲーム中の重量の型定義(ポンド) */

typedef int DICE_NUMBER; /*!< ゲーム中のダイス数の型定義 */
typedef int DICE_SID;    /*!< ゲーム中のダイス面の型定義 */
typedef s32b PRICE;      /*!< ゲーム中の金額価値の型定義 */
typedef s16b FEED;       /*!< ゲーム中の滋養度の型定義 */

typedef u32b STR_OFFSET;      /*!< テキストオフセットの型定義 */

typedef int POWER;		/*!< 魔法の効力定義*/

typedef s32b DEPTH;     /*!< ゲーム中の階層レベルの型定義 */
typedef byte RARITY;    /*!< ゲーム中の希少度の型定義 */

typedef s32b GAME_TURN;     /*!< ゲーム中のターンの型定義 */
typedef u32b REAL_TIME;     /*!< 実時刻の型定義 */

typedef s32b PERCENTAGE;    /*!< ゲーム中のパーセント表記の型定義(/100倍) */
typedef s16b MULTIPLY;      /*!< ゲーム中の倍率の型定義(/10倍) */

typedef u32b BIT_FLAGS;     /*!< 32ビットのフラグ配列の型定義 */
typedef u16b BIT_FLAGS16;   /*!< 16ビットのフラグ配列の型定義 */
typedef byte BIT_FLAGS8;    /*!< 8ビットのフラグ配列の型定義 */

typedef s16b XTRA16;     /*!< 汎用変数16ビットの型定義 */
typedef byte XTRA8;      /*!< 汎用変数8ビットの型定義 */

typedef s16b COMMAND_CODE;   /*!< コマンド内容の型定義 */
typedef s16b COMMAND_ARG;	 /*!< コマンド引数の型定義 */
typedef s16b COMMAND_NUM;	 /*!< コマンド数の型定義 */

typedef int TERM_LEN;     /*!< コンソール表示座標の型定義 */
typedef byte TERM_COLOR;  /*!< テキスト表示色の型定義 */
typedef char SYMBOL_CODE; /*!< キャラの文字の型定義 */

typedef s32b MAGIC_NUM1;   /*!< プレイヤーの汎用魔法情報配列1の型定義*/
typedef byte MAGIC_NUM2;   /*!< プレイヤーの汎用魔法情報配列2の型定義*/

typedef s32b SPELL_IDX;    /*!< 各魔法領域/職業能力ごとの呪文ID型定義 */
typedef s16b PROB;         /*!< 確率の重みの型定義 */
typedef byte FEAT_POWER;   /*!< 地形強度の型定義 */

typedef int QUANTITY; /*!< インターフェース上の指定個数 */

typedef int EFFECT_ID; /*!< 効果属性ID */

typedef s16b QUEST_TYPE;   /*!< クエストの種別ID */
typedef s16b QUEST_STATUS; /*!< クエストの状態ID */

typedef s16b ACTION_SKILL_POWER; /*!< 行動技能値 */

typedef int PET_COMMAND_IDX; /*!< ペットへの指示ID */
typedef byte FF_FLAGS_IDX;   /*!< 地形特性ID */

typedef s16b FEAT_PRIORITY; /*!< 地形の縮小表示優先順位 */

/*** Pointers to all the basic types defined above ***/
typedef char *char_ptr;
typedef byte *byte_ptr;

/*** Pointers to Functions of special types (for various purposes) ***/
/* A generic function takes a user data and a special data */
typedef errr	(*func_gen)(vptr, vptr);

/* An equality testing function takes two things to compare (bool) */
typedef bool	(*func_eql)(vptr, vptr);

/* A hasher takes a thing (and a max hash size) to hash (0 to siz - 1) */
typedef uint	(*func_hsh)(vptr, uint);

/* A key extractor takes a thing and returns (a pointer to) some key */
typedef vptr	(*func_key)(vptr);

typedef enum process_result {
    PROCESS_FALSE = 0,
    PROCESS_TRUE = 1,
    PROCESS_CONTINUE = 2,
} process_result;

#endif
