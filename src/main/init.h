/*!
 * @file init.h
 * @brief ゲームデータ初期化処理のヘッダファイル
 * @date 2015/01/02
 * @author
 * Copyright (c) 2000 Robert Ruehlmann
 * @details
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#ifndef INCLUDED_INIT_H
#define INCLUDED_INIT_H

#include "system/angband.h"
#include "info-reader/info-reader-util.h"

extern angband_header f_head;

#endif /* INCLUDED_INIT_H */

s16b f_tag_to_index_in_init(concptr str);
void init_angband(player_type *player_ptr, void(*process_autopick_file_command)(char*));
concptr get_check_sum(void);
void init_file_paths(char *path);
