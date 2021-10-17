#pragma once

#include "system/angband.h"

struct menu_content {
    concptr name;
    byte cmd;
    bool fin;
};

enum class PlayerClassType : short;
struct special_menu_content {
    concptr name;
    byte window;
    byte number;
    byte jouken;
    PlayerClassType jouken_naiyou;
};

#define MAX_COMMAND_PER_SCREEN 10
#define MAX_COMMAND_MENU_NUM 10

#define MENU_CLASS 1
#define MENU_WILD 2
#define MAX_SPECIAL_MENU_NUM 12

extern menu_content menu_info[MAX_COMMAND_MENU_NUM][MAX_COMMAND_PER_SCREEN];
extern special_menu_content special_menu_info[MAX_SPECIAL_MENU_NUM];
