#pragma once

class ItemEntity;
class ArtifactsDumpInfo;
class PlayerType;
void object_analyze(PlayerType *player_ptr, const ItemEntity *o_ptr, ArtifactsDumpInfo *desc_ptr);
void random_artifact_analyze(PlayerType *player_ptr, const ItemEntity *o_ptr, ArtifactsDumpInfo *desc_ptr);
