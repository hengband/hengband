/*!
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

typedef void (*process_autopick_file_command_pf)(char *);
void init_angband(player_type *player_ptr, process_autopick_file_command_pf process_autopick_file_command, bool no_term);
void init_file_paths(char *path, char *varpath);

#endif /* INCLUDED_INIT_H */
