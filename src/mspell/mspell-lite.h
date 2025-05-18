#pragma once

enum class TerrainCharacteristics;
struct msa_type;
class MonsterEntity;
class PlayerType;
void decide_lite_range(PlayerType *player_ptr, msa_type *msa_ptr);
bool decide_lite_projection(PlayerType *player_ptr, msa_type *msa_ptr);
void decide_lite_area(PlayerType *player_ptr, msa_type *msa_ptr);
