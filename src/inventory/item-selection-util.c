#include "inventory/item-selection-util.h"
#include "io/input-key-requester.h"
#include "object/item-use-flags.h"

fis_type *initialize_fis_type(fis_type *fis_ptr, BIT_FLAGS mode)
{
    fis_ptr->n1 = ' ';
    fis_ptr->n2 = ' ';
    fis_ptr->which = ' ';
    fis_ptr->oops = FALSE;
    fis_ptr->equip = (mode & USE_EQUIP) != 0;
    fis_ptr->inven = (mode & USE_INVEN) != 0;
    fis_ptr->floor = (mode & USE_FLOOR) != 0;
    fis_ptr->force = (mode & USE_FORCE) != 0;
    fis_ptr->allow_equip = FALSE;
    fis_ptr->allow_inven = FALSE;
    fis_ptr->allow_floor = FALSE;
    fis_ptr->toggle = FALSE;
    fis_ptr->floor_top = 0;
    fis_ptr->min_width = 0;
    fis_ptr->menu_line = use_menu ? 1 : 0;
    fis_ptr->max_inven = 0;
    fis_ptr->max_equip = 0;
    fis_ptr->cur_tag = '\0';
    return fis_ptr;
}
