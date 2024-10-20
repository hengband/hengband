#include "monster/monster-timed-effects.h"
#include "locale/language-switcher.h"

const std::map<MonsterTimedEffect, std::string> effect_type_to_label = {
    { MonsterTimedEffect::CSLEEP, _("睡眠", "sleep") },
    { MonsterTimedEffect::FAST, _("加速", "haste") },
    { MonsterTimedEffect::SLOW, _("減速", "slow") },
    { MonsterTimedEffect::STUNNED, _("朦朧", "stun") },
    { MonsterTimedEffect::CONFUSED, _("混乱", "confuse") },
    { MonsterTimedEffect::MONFEAR, _("恐怖", "scare") },
    { MonsterTimedEffect::INVULNER, _("無敵", "invulnerable") }
};
