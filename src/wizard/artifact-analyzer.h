#pragma once

class ItemEntity;
class ArtifactsDumpInfo;
class PlayerType;
ArtifactsDumpInfo object_analyze(PlayerType *player_ptr, const ItemEntity *o_ptr);
ArtifactsDumpInfo random_artifact_analyze(PlayerType *player_ptr, const ItemEntity *o_ptr);
