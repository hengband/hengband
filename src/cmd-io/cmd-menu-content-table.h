#pragma once

#include "system/angband.h"
#include <vector>

struct menu_content {
    concptr name;
    byte cmd;
    bool fin;
};

enum class PlayerClassType : short;
class SpecialMenuContent {
public:
    SpecialMenuContent(concptr name, byte window, byte number, byte jouken, PlayerClassType jouken_naiyou);
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

extern menu_content menu_info[MAX_COMMAND_MENU_NUM][MAX_COMMAND_PER_SCREEN];
extern const std::vector<SpecialMenuContent> special_menu_info;
