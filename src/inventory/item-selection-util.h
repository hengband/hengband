#pragma once

#include "system/angband.h"

/* Floor Item Selection*/
typedef struct fis_type {
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
    char tmp_val[160];
    char out_val[160];
    ITEM_NUMBER floor_num;
    OBJECT_IDX floor_list[23];
    int floor_top;
    TERM_LEN min_width;
    int menu_line;
    int max_inven;
    int max_equip;
    char cur_tag;
} fis_type;

fis_type *initialize_fis_type(fis_type *fis_ptr, BIT_FLAGS mode);
