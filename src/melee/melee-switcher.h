﻿#pragma once

#include "system/angband.h"
#include "melee/melee-util.h"

enum be_type {
	BLOW_EFFECT_TYPE_NONE = 0,
    BLOW_EFFECT_TYPE_FEAR = 1,
    BLOW_EFFECT_TYPE_SLEEP = 2,
    BLOW_EFFECT_TYPE_HEAL = 3,
};

void describe_melee_method(player_type *subject_ptr, mam_type *mam_ptr);
void decide_monster_attack_effect(player_type *subject_ptr, mam_type *mam_ptr);
void describe_monster_missed_monster(player_type *subject_ptr, mam_type *mam_ptr);
