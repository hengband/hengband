#pragma once

#include "system/angband.h"
#include "monster-floor/monster-death-util.h"

struct player_type;
void monster_death(player_type *player_ptr, MONSTER_IDX m_idx, bool drop_item, EFFECT_ID effect_type);
bool drop_single_artifact(player_type *player_ptr, monster_death_type *md_ptr, ARTIFACT_IDX a_idx);
concptr extract_note_dies(MONRACE_IDX r_idx);
