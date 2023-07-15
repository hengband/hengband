#include "window/main-window-row-column.h"
#include "locale/language-switcher.h"

const std::map<monster_timed_effect_type, std::string> effect_type_to_label = {
    { MTIMED_CSLEEP, _("睡眠", "sleep") },
    { MTIMED_FAST, _("加速", "haste") },
    { MTIMED_SLOW, _("減速", "slow") },
    { MTIMED_STUNNED, _("朦朧", "stun") },
    { MTIMED_CONFUSED, _("混乱", "confuse") },
    { MTIMED_MONFEAR, _("恐怖", "scare") },
    { MTIMED_INVULNER, _("無敵", "invulnerable") }
};
