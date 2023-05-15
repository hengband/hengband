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

#include <cassert>
#include <stdint.h>

/*** Special 4 letter names for some standard types ***/
typedef void *vptr; /*!< void型ポインタ定義 / A standard pointer (to "void" because ANSI C says so) */

/*!
 * @brief 文字列定数用ポインタ定義 / Unmodifiable strings
 * @todo std::stringに置換したい.
 */
typedef const char *concptr;

/*!
 * @brief エラーコードの定義 / Error codes for function return values
 * @details
 * 一般に成功時0、失敗時負数、何らかの問題時正数とする。
 * Success = 0, Failure = -N, Problem = +N
 */
typedef int errr;

#define MAX_UCHAR 255 /*!< Maximum value storable in a "byte" (hard-coded) */
#define MAX_SHORT 32767 /*!< Maximum value storable in a "int16_t" (hard-coded) */

#define MAX_NLEN 160 /*!< Maximum length of object's name */
#define MAX_INSCRIPTION _(76, 69) /*!< Maximum length of object's inscription */
#define MAX_MONSTER_NAME 160 /*!< モンスター名称の最大バイト数 / Max characters of monster's name */

/*!
 * @brief 符号なし整数の簡潔な定義
 */
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

// 整数型のバイト数が2021年現在の通常環境と異なるならばコンパイルを通さない.
static_assert(sizeof(char) == 1);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
// static_assert(sizeof(long) == 8); // 将来のための予約.

typedef int16_t IDX; /*!< ゲーム中のID型を定義 */

typedef int16_t FEAT_IDX; /*!< ゲーム中の地形ID型を定義 */
typedef int16_t FLOOR_IDX; /*!< ゲーム中のフロアID型を定義 */

typedef int16_t MONSTER_IDX; /*!< ゲーム中のモンスター個体ID型を定義 */
typedef int16_t DUNGEON_IDX; /*!< ゲーム中のダンジョンID型を定義 */
typedef int16_t EGO_IDX; /*!< アイテムエゴのID型を定義 */
typedef int16_t QUEST_IDX; /*!< ゲーム中のクエストID型を定義 */

typedef int16_t INVENTORY_IDX; /*!< ゲーム中の所持品ID型を定義 */
typedef int16_t OBJECT_IDX; /*!< ゲーム中のアイテムID型を定義 */
typedef int MUTATION_IDX; /*!< 突然変異のID型を定義 */

typedef int32_t POSITION; /*!< ゲーム中の座標型を定義 */
typedef int16_t POSITION_IDX; /*!< ゲーム中の座標リストID型 */

typedef byte FEAT_SUBTYPE; /*!< 地形情報の副値 (トラップ種別/パターン種別/店舗種別)*/

typedef char GAME_TEXT; /*!< ゲーム中のテキスト型定義 */

/*!
 * @var typedef int32_t MANA_POINT
 * @brief MPとその増減量の型定義
 * @details
 * MANA_POINTはプレイヤーのMPの各値とその増減量の型である。
 */
typedef int32_t MANA_POINT; /*!< ゲーム中のMP型を定義 */

typedef int16_t HIT_PROB; /*!< ゲーム中の装備命中修正値を定義 */

typedef int32_t MONSTER_NUMBER; /*!< ゲーム中のモンスター数型を定義 */
typedef int32_t ITEM_NUMBER; /*!< ゲーム中のアイテム数型を定義 */

typedef int16_t ACTION_ENERGY; /*!< ゲーム中の行動エネルギー型を定義 */
typedef int16_t ARMOUR_CLASS; /*!< ゲーム中の行動アーマークラス型を定義 */
typedef int16_t TIME_EFFECT; /*!< ゲーム中の時限期間の型を定義 */

/*!
 * @var typedef int16_t ENEGRY
 * @brief 行動エネルギーの型定義
 * @details
 * ENERGYはプレイヤーとモンスターの行動順を定める行動エネルギーを示す型定義である。
 */
typedef int16_t ENERGY; /*!< ゲーム中の行動エネルギーの型定義 */

typedef int16_t SLEEP_DEGREE; /*!< モンスターの睡眠度の型定義 */

typedef int16_t PLAYER_LEVEL; /*!< ゲーム中のプレイヤーレベルの型を定義 */
typedef int DIRECTION; /*!< ゲーム中の方角の型定義 */
typedef int32_t EXP; /*!< ゲーム中の主経験値の型定義 */
typedef int16_t SUB_EXP; /*!< ゲーム中の副経験値の型定義 */

typedef int32_t OBJECT_SUBTYPE_VALUE; /*!< ゲーム中のアイテム副分類の型定義 */
typedef int16_t PARAMETER_VALUE; /*!< ゲーム中のアイテム能力値の型定義 */
typedef int32_t WEIGHT; /*!< ゲーム中の重量の型定義(ポンド) */

typedef int DICE_NUMBER; /*!< ゲーム中のダイス数の型定義 */
typedef int DICE_SID; /*!< ゲーム中のダイス面の型定義 */
typedef int32_t PRICE; /*!< ゲーム中の金額価値の型定義 */

typedef int POWER; /*!< 魔法の効力定義*/

typedef int32_t DEPTH; /*!< ゲーム中の階層レベルの型定義 */
typedef byte RARITY; /*!< ゲーム中の希少度の型定義 */

typedef int32_t GAME_TURN; /*!< ゲーム中のターンの型定義 */
typedef uint32_t REAL_TIME; /*!< 実時刻の型定義 */

typedef int32_t PERCENTAGE; /*!< ゲーム中のパーセント表記の型定義(/100倍) */
typedef int16_t MULTIPLY; /*!< ゲーム中の倍率の型定義(/10倍) */

typedef uint32_t BIT_FLAGS; /*!< 32ビットのフラグ配列の型定義 */
typedef uint16_t BIT_FLAGS16; /*!< 16ビットのフラグ配列の型定義 */
typedef byte BIT_FLAGS8; /*!< 8ビットのフラグ配列の型定義 */

typedef int16_t COMMAND_CODE; /*!< コマンド内容の型定義 */
typedef int16_t COMMAND_ARG; /*!< コマンド引数の型定義 */

typedef int TERM_LEN; /*!< コンソール表示座標の型定義 */
typedef byte TERM_COLOR; /*!< テキスト表示色の型定義 */
typedef int32_t SPELL_IDX; /*!< 各魔法領域/職業能力ごとの呪文ID型定義 */
typedef int16_t PROB; /*!< 確率の重みの型定義 */
typedef byte FEAT_POWER; /*!< 地形強度の型定義 */

typedef int QUANTITY; /*!< インターフェース上の指定個数 */

typedef int16_t ACTION_SKILL_POWER; /*!< 行動技能値 */

enum class ProcessResult {
    PROCESS_FALSE = 0,
    PROCESS_TRUE = 1,
    PROCESS_CONTINUE = 2,
    PROCESS_LOOP_CONTINUE = 3,
    PROCESS_LOOP_BREAK = 4,
};
