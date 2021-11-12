#pragma once

struct object_type;
typedef struct obj_desc_list obj_desc_list;
class PlayerType;
void object_analyze(PlayerType *player_ptr, object_type *o_ptr, obj_desc_list *desc_ptr);
void random_artifact_analyze(PlayerType *player_ptr, object_type *o_ptr, obj_desc_list *desc_ptr);
