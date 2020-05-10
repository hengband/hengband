/*!
 * todo 呼び出し関係を良く読んで消す方針で進めたい.
 * @brief グローバル変数の残骸
 * @date 2013/12/31
 */

#include "angband.h"
#include "core.h"

 /*!
  * todo どこからも呼ばれていない。main関数辺りに移設するか、そもそもコメントでいいと思われる
  * コピーライト情報 / Link a copyright message into the executable
  */
const concptr copyright[5] =
{
	"Copyright (c) 1989 James E. Wilson, Robert A. Keoneke",
	"",
	"This software may be copied and distributed for educational, research,",
	"and not for profit purposes provided that this copyright and statement",
	"are included in all such copies."
};

concptr ANGBAND_SYS = "xxx";

#ifdef JP
concptr ANGBAND_KEYBOARD = "JAPAN";
#else
concptr ANGBAND_KEYBOARD = "0";
#endif

concptr ANGBAND_GRAF = "ascii";

/*
 * Flags for initialization
 */
int init_flags;
