#pragma once

#include "system/angband.h"
#include "monster-floor/monster-death-util.h"
#include "effect/attribute-types.h"

class PlayerType;
void monster_death(PlayerType *player_ptr, MONSTER_IDX m_idx, bool drop_item, AttributeFlags attribute_flags);
void monster_death(PlayerType *player_ptr, MONSTER_IDX m_idx, bool drop_item, AttributeType type);
bool drop_single_artifact(PlayerType *player_ptr, monster_death_type *md_ptr, ARTIFACT_IDX a_idx);
concptr extract_note_dies(MONRACE_IDX r_idx);
