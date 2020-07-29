#pragma once

#include "system/angband.h"

#define MIND_MINDCRAFTER    0 /*!< 特殊能力: 超能力 */
#define MIND_KI             1 /*!< 特殊能力: 練気 */
#define MIND_BERSERKER      2 /*!< 特殊能力: 怒り */
#define MIND_MIRROR_MASTER  3 /*!< 特殊能力: 鏡魔法 */
#define MIND_NINJUTSU       4 /*!< 特殊能力: 忍術 */

extern void mindcraft_info(player_type *caster_ptr, char *p, int use_mind, int power);
extern void do_cmd_mind(player_type *caster_ptr);
extern void do_cmd_mind_browse(player_type *caster_ptr);
extern void do_cmd_mind_browse(player_type *caster_ptr);

