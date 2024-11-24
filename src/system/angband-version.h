#pragma once

#include <cstdint>
#include <string>

/*!
 * @brief 現在のバリアント名
 */
constexpr std::string_view VARIANT_NAME("Hengband");

/*!
 * @brief 基底バリアント名
 * @details
 * 変愚蛮怒から更にバリアントを派生させても、ここは変更しないこと.
 * 変更ではなく削除はOKだが、h_older_than() 及びそれを呼んでいる箇所全てを削除すること.
 */
constexpr std::string_view ROOT_VARIANT_NAME("Hengband");

/*!
 * @brief セーブファイル上のバージョン定義
 * @details v1.1.1以上にのみ適用.
 * angband.rc に影響があるため、constexpr ではなくdefine 定数のままにしておくこと.
 */
#define H_VER_MAJOR 3 //!< ゲームのバージョン定義(メジャー番号)
#define H_VER_MINOR 0 //!< ゲームのバージョン定義(マイナー番号)
#define H_VER_PATCH 1 //!< ゲームのバージョン定義(パッチ番号)
#define H_VER_EXTRA 21 //!< ゲームのバージョン定義(エクストラ番号)

/*!
 * @brief セーブファイルのバージョン(3.0.0から導入)
 */
constexpr uint32_t SAVEFILE_VERSION = 23;

/*!
 * @brief バージョンが開発版が安定版かを返す(廃止予定)
 */
constexpr bool IS_STABLE_VERSION = (H_VER_MINOR % 2 == 0 && H_VER_EXTRA == 0);

enum class VersionStatusType {
    ALPHA,
    BETA,
    RELEASE_CANDIDATE,
    RELEASE,
};

/*!
 * @brief バージョンの立ち位置
 */
constexpr VersionStatusType VERSION_STATUS = VersionStatusType::BETA;

enum class VersionExpression {
    WITHOUT_EXTRA,
    WITH_EXTRA,
    FULL,
};

class AngbandVersion {
public:
    AngbandVersion() = default;
    AngbandVersion(uint8_t major, uint8_t minor, uint8_t patch, uint8_t extra)
        : major(major)
        , minor(minor)
        , patch(patch)
        , extra(extra)
    {
    }

    uint8_t major = 0; //!< 変愚蛮怒バージョン(メジャー番号)
    uint8_t minor = 0; //!< 変愚蛮怒バージョン(マイナー番号)
    uint8_t patch = 0; //!< 変愚蛮怒バージョン(パッチ番号)
    uint8_t extra = 0; //!< 変愚蛮怒バージョン(エクストラ番号)

    std::string build_expression(VersionExpression expression) const;
};
