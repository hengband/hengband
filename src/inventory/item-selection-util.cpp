#include "inventory/item-selection-util.h"
#include "io/input-key-requester.h"
#include "object/item-use-flags.h"
#include "util/bit-flags-calculator.h"

FloorItemSelection::FloorItemSelection(uint32_t mode)
    : mode(mode)
    , equip(any_bits(mode, USE_EQUIP))
    , inven(any_bits(mode, USE_INVEN))
    , floor(any_bits(mode, USE_FLOOR))
    , force(any_bits(mode, USE_FORCE))
    , menu_line(use_menu ? 1 : 0)
{
}

ItemSelection::ItemSelection(uint32_t mode)
    : mode(mode)
    , menu_line(use_menu ? 1 : 0)
    , equip(any_bits(mode, USE_EQUIP))
    , inven(any_bits(mode, USE_INVEN))
    , floor(any_bits(mode, USE_FLOOR))
{
}
