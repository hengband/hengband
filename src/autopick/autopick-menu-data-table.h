#pragma once

#include "system/angband.h"

#define MENU_DATA_NUM 92

struct command_menu_type {
    concptr name;
    int level;
    int key;
    int com_id;
};

extern command_menu_type menu_data[MENU_DATA_NUM];
