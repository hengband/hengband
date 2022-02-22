#pragma once

#include "system/angband.h"
#include <vector>

struct menu_content {
    concptr name;
    byte cmd;
    bool fin;
};

enum class SpecialMenuType {
    NONE = 0,
    CLASS = 1,
    WILD = 2,
};

enum class PlayerClassType : short;
class SpecialMenuContent {
public:
    SpecialMenuContent(concptr name, byte window, byte number, SpecialMenuType menu_condition, PlayerClassType jouken_naiyou);
    concptr name;
    byte window;
    byte number;
    SpecialMenuType menu_condition;
    PlayerClassType jouken_naiyou;
};

#define MAX_COMMAND_PER_SCREEN 10
#define MAX_COMMAND_MENU_NUM 10

extern menu_content menu_info[MAX_COMMAND_MENU_NUM][MAX_COMMAND_PER_SCREEN];
extern const std::vector<SpecialMenuContent> special_menu_info;
