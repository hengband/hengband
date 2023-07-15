#pragma once

class PlayerType;
void do_cmd_knowledge_artifacts(PlayerType *player_ptr);
void do_cmd_knowledge_objects(PlayerType *player_ptr, bool *need_redraw, bool visual_only, short direct_k_idx);
