#pragma once

typedef enum inventory_slot_type {
	INVEN_PACK = 23, /*!< アイテムスロット…所持品(0～) */
	INVEN_RARM = 24, /*!< アイテムスロット…右手 */
    INVEN_LARM = 25, /*!< アイテムスロット…左手 */
    INVEN_BOW = 26, /*!< アイテムスロット…射撃 */
    INVEN_RIGHT = 27, /*!< アイテムスロット…右手指 */
    INVEN_LEFT = 28, /*!< アイテムスロット…左手指 */
    INVEN_NECK = 29, /*!< アイテムスロット…首 */
    INVEN_LITE = 30, /*!< アイテムスロット…光源 */
    INVEN_BODY = 31, /*!< アイテムスロット…体 */
    INVEN_OUTER = 32, /*!< アイテムスロット…体の上 */
    INVEN_HEAD = 33, /*!< アイテムスロット…頭部 */
    INVEN_HANDS = 34, /*!< アイテムスロット…腕部 */
    INVEN_FEET = 35, /*!< アイテムスロット…脚部 */
    INVEN_AMMO = 23, /*!< used for get_random_ego()  */
    INVEN_TOTAL = 36, /*!< Total number of inventory_list slots (hard-coded). */
    INVEN_FORCE = 1111, /*!< inventory_list slot for selecting force (hard-coded). */
} inventory_slot_type;
