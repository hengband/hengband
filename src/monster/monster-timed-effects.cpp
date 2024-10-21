#include "monster/monster-timed-effects.h"
#include "locale/language-switcher.h"

const std::map<MonsterTimedEffect, std::string> effect_type_to_label = {
    { MonsterTimedEffect::SLEEP, _("睡眠", "sleep") },
    { MonsterTimedEffect::FAST, _("加速", "haste") },
    { MonsterTimedEffect::SLOW, _("減速", "slow") },
    { MonsterTimedEffect::STUN, _("朦朧", "stun") },
    { MonsterTimedEffect::CONFUSION, _("混乱", "confuse") },
    { MonsterTimedEffect::FEAR, _("恐怖", "scare") },
    { MonsterTimedEffect::INVULNERABILITY, _("無敵", "invulnerable") }
};
