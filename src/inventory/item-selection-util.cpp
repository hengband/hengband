#include "inventory/item-selection-util.h"
#include "io/input-key-requester.h"
#include "object/item-use-flags.h"

fis_type *initialize_fis_type(fis_type *fis_ptr, COMMAND_CODE *cp, BIT_FLAGS mode)
{
    fis_ptr->cp = cp;
    fis_ptr->mode = mode;
    fis_ptr->n1 = ' ';
    fis_ptr->n2 = ' ';
    fis_ptr->which = ' ';
    fis_ptr->oops = false;
    fis_ptr->equip = (fis_ptr->mode & USE_EQUIP) != 0;
    fis_ptr->inven = (fis_ptr->mode & USE_INVEN) != 0;
    fis_ptr->floor = (fis_ptr->mode & USE_FLOOR) != 0;
    fis_ptr->force = (fis_ptr->mode & USE_FORCE) != 0;
    fis_ptr->allow_equip = false;
    fis_ptr->allow_inven = false;
    fis_ptr->allow_floor = false;
    fis_ptr->toggle = false;
    fis_ptr->floor_top = 0;
    fis_ptr->min_width = 0;
    fis_ptr->menu_line = use_menu ? 1 : 0;
    fis_ptr->max_inven = 0;
    fis_ptr->max_equip = 0;
    fis_ptr->cur_tag = '\0';
    return fis_ptr;
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
