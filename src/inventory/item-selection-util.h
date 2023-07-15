#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

/* Floor Item Selection*/
struct fis_type {
    COMMAND_CODE *cp;
    BIT_FLAGS mode;
    char n1;
    char n2;
    char which;
    COMMAND_CODE i1;
    COMMAND_CODE i2;
    COMMAND_CODE e1;
    COMMAND_CODE e2;
    COMMAND_CODE k;
    bool done;
    bool item;
    bool oops;
    bool equip;
    bool inven;
    bool floor;
    bool force;
    bool allow_equip;
    bool allow_inven;
    bool allow_floor;
    bool toggle;
    char tmp_val[320];
    char out_val[160];
    ITEM_NUMBER floor_num;
    OBJECT_IDX floor_list[23];
    int floor_top;
    TERM_LEN min_width;
    int menu_line;
    int max_inven;
    int max_equip;
    char cur_tag;
};

struct item_selection_type {
    COMMAND_CODE *cp;
    BIT_FLAGS mode;
    char which;
    OBJECT_IDX k;
    OBJECT_IDX i1;
    OBJECT_IDX i2;
    OBJECT_IDX e1;
    OBJECT_IDX e2;
    bool done;
    bool item;
    bool oops;
    bool equip;
    bool inven;
    bool floor;
    bool allow_floor;
    bool toggle;
    char tmp_val[320];
    char out_val[160];
    int menu_line;
    int max_inven;
    int max_equip;
    char cur_tag;
};

fis_type *initialize_fis_type(fis_type *fis_ptr, COMMAND_CODE *cp, BIT_FLAGS mode);
item_selection_type *initialize_item_selection_type(item_selection_type *item_selection_ptr, COMMAND_CODE *cp, BIT_FLAGS mode);
