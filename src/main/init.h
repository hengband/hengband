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

#endif /* INCLUDED_INIT_H */

void init_angband(player_type *player_ptr, void(*process_autopick_file_command)(char*));
concptr get_check_sum(void);
void init_file_paths(char *path);
errr init_v_info(void);
errr init_buildings(void);
