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

item_selection_type *initialize_item_selection_type(item_selection_type *item_selection_ptr, COMMAND_CODE *cp, BIT_FLAGS mode)
{
    item_selection_ptr->cp = cp;
    item_selection_ptr->mode = mode;
    item_selection_ptr->which = ' ';
    item_selection_ptr->oops = false;
    item_selection_ptr->equip = false;
    item_selection_ptr->inven = false;
    item_selection_ptr->floor = false;
    item_selection_ptr->allow_floor = false;
    item_selection_ptr->toggle = false;
    item_selection_ptr->menu_line = (use_menu ? 1 : 0);
    item_selection_ptr->max_inven = 0;
    item_selection_ptr->max_equip = 0;
    item_selection_ptr->cur_tag = '\0';
    return item_selection_ptr;
}
