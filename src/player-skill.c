#include "angband.h"
#include "player-skill.h"

/*!
 * @brief 技能値到達表記テーブル
 */
const concptr exp_level_str[5] =
#ifdef JP
{ "[初心者]", "[入門者]", "[熟練者]", "[エキスパート]", "[達人]" };
#else
{"[Unskilled]", "[Beginner]", "[Skilled]", "[Expert]", "[Master]"};
#endif
