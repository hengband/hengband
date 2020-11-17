#pragma once

/* 部屋型ID / Room types for room_build() */
typedef enum room_type {
	ROOM_T_NORMAL = 0, /*!<部屋型ID:基本長方形 / Simple (33x11) */
    ROOM_T_OVERLAP = 1, /*!<部屋型ID:長方形二つ重ね / Overlapping (33x11) */
    ROOM_T_CROSS = 2, /*!<部屋型ID:十字 / Crossed (33x11) */
    ROOM_T_INNER_FEAT = 3, /*!<部屋型ID:二重壁 / Large (33x11) */
    ROOM_T_NEST = 4, /*!<部屋型ID:モンスターNEST / Monster nest (33x11) */
    ROOM_T_PIT = 5, /*!<部屋型ID:モンスターPIT / Monster pit (33x11) */
    ROOM_T_LESSER_VAULT = 6, /*!<部屋型ID:小型VAULT / Lesser vault (33x22) */
    ROOM_T_GREATER_VAULT = 7, /*!<部屋型ID:大型VAULT / Greater vault (66x44) */
    ROOM_T_FRACAVE = 8, /*!<部屋型ID:フラクタル地形 / Fractal room (42x24) */
    ROOM_T_RANDOM_VAULT = 9, /*!<部屋型ID:ランダムVAULT / Random vault (44x22) */
    ROOM_T_OVAL = 10, /*!<部屋型ID:円形部屋 / Circular rooms (22x22) */
    ROOM_T_CRYPT = 11, /*!<部屋型ID:聖堂 / Crypts (22x22) */
    ROOM_T_TRAP_PIT = 12, /*!<部屋型ID:トラップつきモンスターPIT / Trapped monster pit */
    ROOM_T_TRAP = 13, /*!<部屋型ID:トラップ部屋 / Piranha/Armageddon trap room */
    ROOM_T_GLASS = 14, /*!<部屋型ID:ガラス部屋 / Glass room */
    ROOM_T_ARCADE = 15, /*!<部屋型ID:商店 / Arcade */
    ROOM_T_FIXED = 16, /*!<部屋型ID:固定部屋 / Fixed room */
    ROOM_T_MAX = 17, /*!<部屋型ID最大数 */
} room_type;
