﻿#pragma once

/*!
 *  @file defines.h
 *  @brief 主要なマクロ定義ヘッダ / Purpose: global constants and macro definitions
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 *  @details
 * Do not edit this file unless you know *exactly* what you are doing.\n
 *\n
 * Some of the values in this file were chosen to preserve game balance,\n
 * while others are hard-coded based on the format of old save-files, the\n
 * definition of arrays in various places, mathematical properties, fast\n
 * computation, storage limits, or the format of external text files.\n
 *\n
 * Changing some of these values will induce crashes or memory errors or\n
 * savefile mis-reads.  Most of the comments in this file are meant as\n
 * reminders, not complete descriptions, and even a complete knowledge\n
 * of the source may not be sufficient to fully understand the effects\n
 * of changing certain definitions.\n
 *\n
 * Lastly, note that the code does not always use the symbolic constants\n
 * below, and sometimes uses various hard-coded values that may not even\n
 * be defined in this file, but which may be related to definitions here.\n
 * This is of course bad programming practice, but nobody is perfect...\n
 *\n
 * For example, there are MANY things that depend on the screen being\n
 * 80x24, with the top line used for messages, the bottom line being\n
 * used for status, and exactly 22 lines used to show the dungeon.\n
 * Just because your screen can hold 46 lines does not mean that the\n
 * game will work if you try to use 44 lines to show the dungeon.\n
 *\n
 * You have been warned.\n
 */

/*!
 * @brief 再描画処理用配列サイズ / Maximum size of the "redraw" array (see "current_floor_ptr->grid_array.c")
 * @details We must be large for proper functioning of delayed redrawing.
 * We must also be as large as two times of the largest view area.
 * Note that maximum view grids are 1149 entries.
 */
#define REDRAW_MAX 2298


/*!
 * @brief マクロ登録の最大数 / Maximum number of macros (see "io.c")
 * @note Default: assume at most 256 macros are used
 */
#define MACRO_MAX       256

/*!
 * @brief 銘情報の最大数 / Maximum number of "quarks" (see "io.c")
 * @note 
 * Default: assume at most 512 different inscriptions are used<br>
 * Was 512... 256 quarks added for random artifacts<br>
 */
#define QUARK_MAX       768

/*
 * OPTION: Maximum number of messages to remember (see "io.c")
 * Default: assume maximal memorization of 2048 total messages
 */
#define MESSAGE_MAX  81920

/*
 * OPTION: Maximum space for the message text buffer (see "io.c")
 * Default: assume that each of the 2048 messages is repeated an
 * average of three times, and has an average length of 48
 */
#define MESSAGE_BUF 655360


/*** Screen Locations ***/

/*
 * Is the monster seen by the player?
 */
#define is_seen(A) \
	((bool)((A)->ml && (!ignore_unview || p_ptr->inside_battle || \
	 (player_can_see_bold((A)->fy, (A)->fx) && projectable(p_ptr->y, p_ptr->x, (A)->fy, (A)->fx)))))


/* Maximum "Nazguls" number */
#define MAX_NAZGUL_NUM 5

/*
  Language selection macro
*/
#ifdef JP
#define _(JAPANESE,ENGLISH) (JAPANESE)
#else
#define _(JAPANESE,ENGLISH) (ENGLISH)
#endif
