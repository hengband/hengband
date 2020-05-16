#pragma once

typedef struct menu_content {
    concptr name;
    byte cmd;
    bool fin;
} menu_content;

#define MAX_COMMAND_PER_SCREEN 10
#define MAX_COMMAND_MENU_NUM 10

extern menu_content menu_info[MAX_COMMAND_MENU_NUM][MAX_COMMAND_PER_SCREEN];
