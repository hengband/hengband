/*!
 * @brief 地形特性種別定義
 * @author Hourier
 * @date 2025/01/24
 */

#pragma once

enum class TerrainKind {
    NORMAL, //!< 普通の特性.
    MIMIC, //!< 未確認地形・隠しドア・罠といった見た目と中身が違う特性の内、壁や床の方.
    MIMIC_RAW, //!< 見た目と中身が違う特性の内、隠しドアや罠の方.
};
