#pragma once

typedef struct object_type object_type;
typedef struct obj_desc_list obj_desc_list;
typedef struct player_type player_type;
void object_analyze(player_type *player_ptr, object_type *o_ptr, obj_desc_list *desc_ptr);
void random_artifact_analyze(player_type *player_ptr, object_type *o_ptr, obj_desc_list *desc_ptr);
