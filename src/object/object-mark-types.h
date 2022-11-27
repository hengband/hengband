/*
 * @brief How an entity of an item is marked (flags in ItemEntity::marked)
 * @date 2020/05/28
 * @author Hourier
 * @details
 * OM_FOUND --- original boolean flag
 * OM_NOMSG --- temporary flag to suppress messages which were
 *              already printed in autopick_pickup_items().
 */

#pragma once

enum class OmType {
    FOUND = 0, /*!< アイテムを一度でも視界に収めたことがあるか */
    SUPRESS_MESSAGE = 1, /* temporary flag to suppress messages */
    NO_QUERY = 2, /* Query for auto-pick was already answered as 'No' */
    AUTODESTROY = 3, /* Destroy later to avoid illegal inventry shift */
    TOUCHED = 4, /* Object was touched by player */
    MAX,
};
