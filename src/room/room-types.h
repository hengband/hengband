#pragma once

#include "util/enum-converter.h"
#include "util/enum-range.h"

/* 部屋型ID / Room types for room_build() */
enum class RoomType {
    NORMAL = 0, /*!<部屋型ID:基本長方形 / Simple (33x11) */
    OVERLAP = 1, /*!<部屋型ID:長方形二つ重ね / Overlapping (33x11) */
    CROSS = 2, /*!<部屋型ID:十字 / Crossed (33x11) */
    INNER_FEAT = 3, /*!<部屋型ID:二重壁 / Large (33x11) */
    NEST = 4, /*!<部屋型ID:モンスターNEST / Monster nest (33x11) */
    PIT = 5, /*!<部屋型ID:モンスターPIT / Monster pit (33x11) */
    LESSER_VAULT = 6, /*!<部屋型ID:小型VAULT / Lesser vault (33x22) */
    GREATER_VAULT = 7, /*!<部屋型ID:大型VAULT / Greater vault (66x44) */
    FRACAVE = 8, /*!<部屋型ID:フラクタル地形 / Fractal room (42x24) */
    RANDOM_VAULT = 9, /*!<部屋型ID:ランダムVAULT / Random vault (44x22) */
    OVAL = 10, /*!<部屋型ID:円形部屋 / Circular rooms (22x22) */
    CRYPT = 11, /*!<部屋型ID:聖堂 / Crypts (22x22) */
    TRAP_PIT = 12, /*!<部屋型ID:トラップつきモンスターPIT / Trapped monster pit */
    TRAP = 13, /*!<部屋型ID:トラップ部屋 / Piranha/Armageddon trap room */
    GLASS = 14, /*!<部屋型ID:ガラス部屋 / Glass room */
    ARCADE = 15, /*!<部屋型ID:商店 / Arcade */
    FIXED = 16, /*!<部屋型ID:固定部屋 / Fixed room */
    MAX = 17, /*!<部屋型ID最大数 */
};

constexpr int ROOM_TYPE_MAX = enum2i(RoomType::MAX);

constexpr auto ROOM_TYPE_LIST = EnumRange(RoomType::NORMAL, RoomType::FIXED);
