#pragma once

#include <cstdint>

class FloorItemSelection {
public:
    FloorItemSelection(uint32_t mode);

    uint32_t mode;
    bool equip;
    bool inven;
    bool floor;
    bool force;
    int menu_line;
    short cp = 0;
    char n1 = ' ';
    char n2 = ' ';
    char which = ' ';
    short i1 = 0;
    short i2 = 0;
    short e1 = 0;
    short e2 = 0;
    short k = 0;
    bool done = false;
    bool item = false;
    bool oops = false;
    bool allow_equip = false;
    bool allow_inven = false;
    bool allow_floor = false;
    bool toggle = false;
    int floor_num = 0;
    short floor_list[23]{};
    int floor_top = 0;
    int min_width = 0;
    int max_inven = 0;
    int max_equip = 0;
    char cur_tag = '\0';
};

class ItemSelection {
public:
    ItemSelection(uint32_t mode);
    uint32_t mode;
    int menu_line;
    short cp = 0;
    char which = '\0';
    short k = 0;
    short i1 = 0;
    short i2 = 0;
    short e1 = 0;
    short e2 = 0;
    bool done = false;
    bool item = false;
    bool oops = false;
    bool equip = false;
    bool inven = false;
    bool floor = false;
    bool allow_floor = false;
    bool toggle = false;
    int max_inven = 0;
    int max_equip = 0;
    char cur_tag = '\0';
};
