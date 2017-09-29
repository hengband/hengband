/*!
 * @file h-type.h
 * @brief ゲーム中に用いる変数型定義 /
 * Basic "types".
 * @date 2014/08/17
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
 * On Sparc's, a sint takes 4 bytes (2 is legal)
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
typedef const char *cptr; /*!< 文字列定数用ポインタ定義 / A simple pointer (to unmodifiable strings) */
typedef double real;      /*!< doubleをreal型として定義 / Since float's are silly, hard code real numbers as doubles */


/*!
 * @brief エラーコードの定義 / Error codes for function return values
 * @details
 * 一般に成功時0、失敗時負数、何らかの問題時整数とする。
 * Success = 0, Failure = -N, Problem = +N 
 */
typedef int errr;

#undef uint
#define uint uint_hack /*!< 非マッキントッシュ環境で重複を避けるためのuint_hack型定義 / Hack -- prevent problems with non-MACINTOSH */

#undef huge
#define huge huge_hack /*!< WINDOWS環境で重複を避けるためのhuge_hack定義 / Hack -- prevent problems with WINDOWS */

#undef byte
#define byte byte_hack /*!< AMIGA環境で重複を避けるためのbyte_hack定義 / Hack -- prevent problems with AMIGA */

#undef bool
#define bool bool_hack /*!< C++環境で重複を避けるためのbool_hack定義 Hack -- prevent problems with C++ */


/* Note that "signed char" is not always "defined" */
/* So always use "s16b" to hold small signed values */
/* A signed byte of memory */
/* typedef signed char syte; */

typedef unsigned char byte; /*!< byte型をunsighned charとして定義 / Note that unsigned values can cause math problems / An unsigned byte of memory */
typedef char bool; /*!< bool型をcharとして定義 / Note that a bool is smaller than a full "int" / Simple True/False type */
typedef int sint; /*!< sint型をintとして定義 / A signed, standard integer (at least 2 bytes) */
typedef unsigned int uint; /* uint型をintとして定義 /  An unsigned, "standard" integer (often pre-defined) */


/* The largest possible signed integer (pre-defined) */
/* typedef long long; */

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


typedef s16b IDX;			/*!< ゲーム中のID型を定義 */
typedef s32b POSITION;		/*!< ゲーム中の座標型を定義 */
typedef s32b HIT_POINT;		/*!< ゲーム中のHP/ダメージ型を定義 */
typedef s32b MANA_POINT;	/*!< ゲーム中のMP型を定義 */
typedef s16b HIT_PROB;		/*!< ゲーム中の命中修正値を定義 */
typedef s16b BASE_STATUS;	/*!< ゲーム中の基礎能力値型を定義 */
typedef s32b ITEM_NUMBER;	/*!< ゲーム中のアイテム数型を定義 */
typedef s16b ACTION_ENERGY;	/*!< ゲーム中の行動エネルギー型を定義 */
typedef s16b ARMOUR_CLASS;	/*!< ゲーム中の行動アーマークラス型を定義 */
typedef s16b TIME_EFFECT;   /*!< ゲーム中の時限期間の型を定義 */
typedef byte CHARACTER_IDX; /*!< ゲーム中のキャラクター特性各種IDの型を定義 */
typedef byte DISCOUNT_RATE; /*!< ゲーム中の値引き率の型を定義 */

typedef s16b PLAYER_LEVEL;  /*!< ゲーム中のプレイヤーレベルの型を定義 */
typedef int DIRECTION;		/*!< ゲーム中の方角の型定義 */
typedef s32b EXP;			/*!< ゲーム中の主経験値の型定義 */
typedef s16b SUB_EXP;		/*!< ゲーム中の副経験値の型定義 */

typedef byte OBJECT_TYPE_VALUE;    /*!< ゲーム中のアイテム主分類の型定義 */
typedef byte OBJECT_SUBTYPE_VALUE; /*!< ゲーム中のアイテム副分類の型定義 */
typedef s16b PARAMETER_VALUE;      /*!< ゲーム中のアイテム能力値の型定義 */
typedef s16b WEIGHT;               /*!< ゲーム中の重量の型定義 */

typedef int DICE_NUMBER; /*!< ゲーム中のダイス数の型定義 */
typedef int DICE_SID;    /*!< ゲーム中のダイス面の型定義 */
typedef s32b PRICE;      /*!< ゲーム中の金額価値の型定義 */

typedef u32b STR_OFFSET;      /*!< テキストオフセットの型定義 */

typedef s32b DEPTH;     /*!< ゲーム中の階層レベルの型定義 */
typedef byte RARITY;    /*!< ゲーム中の希少度の型定義 */

typedef s32b GAME_TURN;     /*!< ゲーム中のターンの型定義 */

typedef s16b PERCENTAGE;     /*!< ゲーム中のパーセント表記の型定義 */

typedef u32b BIT_FLAGS;     /*!< 32ビットのフラグ配列の型定義 */
typedef byte BIT_FLAGS8;    /*!< 8ビットのフラグ配列の型定義 */

/*** Pointers to all the basic types defined above ***/

typedef real *real_ptr;
typedef errr *errr_ptr;
typedef char *char_ptr;
typedef byte *byte_ptr;
typedef bool *bool_ptr;
typedef sint *sint_ptr;
typedef uint *uint_ptr;
typedef long *long_ptr;
typedef huge *huge_ptr;
typedef s16b *s16b_ptr;
typedef u16b *u16b_ptr;
typedef s32b *s32b_ptr;
typedef u32b *u32b_ptr;
typedef vptr *vptr_ptr;
typedef cptr *cptr_ptr;



/*** Pointers to Functions of special types (for various purposes) ***/

/* A generic function takes a user data and a special data */
typedef errr	(*func_gen)(vptr, vptr);

/* An equality testing function takes two things to compare (bool) */
typedef bool	(*func_eql)(vptr, vptr);

/* A comparison function takes two things and to compare (-1,0,+1) */
typedef sint	(*func_cmp)(vptr, vptr);

/* A hasher takes a thing (and a max hash size) to hash (0 to siz - 1) */
typedef uint	(*func_hsh)(vptr, uint);

/* A key extractor takes a thing and returns (a pointer to) some key */
typedef vptr	(*func_key)(vptr);



#endif

