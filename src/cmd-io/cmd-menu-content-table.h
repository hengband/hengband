#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>
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
    SpecialMenuContent(concptr name, byte window, byte number, SpecialMenuType menu_condition, tl::optional<PlayerClassType> class_condition, tl::optional<bool> in_wilderness);
    concptr name;
    byte window;
    byte number;
    SpecialMenuType menu_condition;
    tl::optional<PlayerClassType> class_condition;
    bool matches_current_wild_mode() const;

private:
    tl::optional<bool> wild_mode;
};

constexpr int MAX_COMMAND_PER_SCREEN = 10;
constexpr int MAX_COMMAND_MENU_NUM = 10;

extern menu_content menu_info[MAX_COMMAND_MENU_NUM][MAX_COMMAND_PER_SCREEN];
extern const std::vector<SpecialMenuContent> special_menu_info;
