#pragma once

class ItemEntity;
struct obj_desc_list;
class PlayerType;
void object_analyze(PlayerType *player_ptr, ItemEntity *o_ptr, obj_desc_list *desc_ptr);
void random_artifact_analyze(PlayerType *player_ptr, ItemEntity *o_ptr, obj_desc_list *desc_ptr);
