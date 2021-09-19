#pragma once

#include "system/angband.h"

/*!
 * @enum フロア切り替えモードのbit設定 / Change Floor Mode.
 */
enum cfm_type {
	CFM_UP = 0x0001, /*!< 上の階層に上る / Move up */
	CFM_DOWN = 0x0002, /*!< 下の階層に下る /  Move down */
	CFM_XXX1 = 0x0004, /*!< 未実装 / unused */
	CFM_XXX2 = 0x0008, /*!< 未実装 / unused */
	CFM_SHAFT = 0x0010, /*!< 坑道である(2階層分移動する) / Shaft */
	CFM_RAND_PLACE = 0x0020, /*!< 移動先フロアにランダム配置される / Arrive at random grid */
	CFM_RAND_CONNECT = 0x0040, /*!< 移動先フロアの階段にランダムに接続する / Connect with random stairs */
	CFM_SAVE_FLOORS = 0x0080, /*!< 保存済フロアに移動する / Save floors */
	CFM_NO_RETURN = 0x0100, /*!< 移動元に戻す手段が失われる / Flee from random quest etc... */
	CFM_FIRST_FLOOR = 0x0200, /*!< ダンジョンに入った最初のフロアである / Create exit from the dungeon */
};

struct player_type;
void prepare_change_floor_mode(player_type *player_ptr, BIT_FLAGS mode);
