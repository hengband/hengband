#pragma once

enum keymap_mode {
    KEYMAP_MODE_ORIG = 0, /*!< オリジナルキー配置 / Mode for original keyset commands */
    KEYMAP_MODE_ROGUE = 1, /*!< ローグライクキー配置 / Mode for roguelike keyset commands */
    KEYMAP_MODES = 2, /*!< キー配置の数 / Number of keymap modes */
};

int get_keymap_dir(char ch);
