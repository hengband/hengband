#pragma once

enum inventory_slot_type {
    INVEN_PACK = 23, /*!< アイテムスロット…所持品(0～) */
    INVEN_MAIN_HAND = 24, /*!< アイテムスロット…利手 */
    INVEN_SUB_HAND = 25, /*!< アイテムスロット…逆手 */
    INVEN_BOW = 26, /*!< アイテムスロット…射撃 */
    INVEN_MAIN_RING = 27, /*!< アイテムスロット…利手指 */
    INVEN_SUB_RING = 28, /*!< アイテムスロット…逆手指 */
    INVEN_NECK = 29, /*!< アイテムスロット…首 */
    INVEN_LITE = 30, /*!< アイテムスロット…光源 */
    INVEN_BODY = 31, /*!< アイテムスロット…体 */
    INVEN_OUTER = 32, /*!< アイテムスロット…体の上 */
    INVEN_HEAD = 33, /*!< アイテムスロット…頭部 */
    INVEN_ARMS = 34, /*!< アイテムスロット…腕部 */
    INVEN_FEET = 35, /*!< アイテムスロット…脚部 */
    INVEN_AMMO = 23, /*!< used for get_random_ego()  */
    INVEN_TOTAL = 36, /*!< Total number of inventory_list slots (hard-coded). */
    INVEN_NONE = 1000, /*!< アイテムスロット非選択状態 */
    INVEN_FORCE = 1111, /*!< inventory_list slot for selecting force (hard-coded). */
};
