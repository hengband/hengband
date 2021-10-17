#pragma once

#include <stdint.h>

#define VERSION_NAME "Hengband" /*!< バリアント名称 / Name of the version/variant */

/*!
 * @brief セーブファイル上のバージョン定義(メジャー番号) / "Savefile Version Number" for Hengband 1.1.1 and later
 * @details
 * 当面FAKE_VER_*を参照しておく。
 * <pre>
 * Program Version of Hengband version is
 *   "(H_VER_MAJOR).(H_VER_MINOR).(H_VER_PATCH).(H_VER_EXTRA)".
 * Upper compatibility is always guaranteed when it is more than 1.0.0 .
 * </pre>
 */
#define H_VER_MAJOR  3 //!< ゲームのバージョン定義(メジャー番号)
#define H_VER_MINOR  0 //!< ゲームのバージョン定義(マイナー番号)
#define H_VER_PATCH  0 //!< ゲームのバージョン定義(パッチ番号)
#define H_VER_EXTRA 41 //!< ゲームのバージョン定義(エクストラ番号)

/*!
 * @brief セーブファイルのバージョン(3.0.0から導入)
 */
constexpr uint32_t SAVEFILE_VERSION = 10;

/*!
 * @brief バージョンが開発版が安定版かを返す(廃止予定)
 */
#define IS_STABLE_VERSION (H_VER_MINOR % 2 == 0 && H_VER_EXTRA == 0)

/*!
 * @brief 状態がアルファ版かどうかを返す
 * @note アルファ版はエクストラ番号一定値までをアルファとし、一定まで進めて安定次第ベータ版、さらにそれも解除して無印版とする。
 */
#define IS_ALPHA_VERSION 1

/*!
 * @brief ゲームのバージョン番号定義 / "Program Version Number" of the game
 * @details
 * 本FAKE_VERSIONそのものは未使用である。Zangと整合性を合わせるための疑似的処理のためFAKE_VER_MAJORは実値-10が該当のバージョン番号となる。
 * <pre>
 * FAKE_VER_MAJOR=1,2 were reserved for ZAngband version 1.x.x/2.x.x .
 * </pre>
 */
#define FAKE_VER_PLUS 10 //!< 偽バージョン番号としていくつ足すか
#define FAKE_VER_MAJOR (H_VER_MAJOR + FAKE_VER_PLUS) //!< 偽バージョン番号定義(メジャー番号) */
#define FAKE_VER_MINOR H_VER_MINOR //!< 偽バージョン番号定義(マイナー番号) */
#define FAKE_VER_PATCH H_VER_PATCH //!< 偽バージョン番号定義(パッチ番号) */
#define FAKE_VER_EXTRA H_VER_EXTRA //!< 偽バージョン番号定義(エクストラ番号) */

void put_version(char *buf);
