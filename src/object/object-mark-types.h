/*
 * @brief How object is marked (flags in ObjectType.mark)
 * @date 2020/05/28
 * @author Hourier
 * @details
 * OM_FOUND --- original boolean flag
 * OM_NOMSG --- temporary flag to suppress messages which were
 *              already printed in autopick_pickup_items().
 */

#pragma once

enum om_type {
    OM_FOUND = 0x01, /*!< アイテムを一度でも視界に収めたことがあるか */
    OM_NOMSG = 0x02, /* temporary flag to suppress messages */
    OM_NO_QUERY = 0x04, /* Query for auto-pick was already answered as 'No' */
    OM_AUTODESTROY = 0x08, /* Destroy later to avoid illegal inventry shift */
    OM_TOUCHED = 0x10, /* Object was touched by player */
};
