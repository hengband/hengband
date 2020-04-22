#pragma once

extern void do_cmd_pref(player_type *creature_ptr);
extern void do_cmd_reload_autopick(player_type *creature_ptr);
extern void do_cmd_colors(player_type *creature_ptr);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(player_type *creature_ptr);
extern void do_cmd_knowledge_quests_completed(player_type *creature_ptr, FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge_quests_failed(player_type *creature_ptr, FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge(player_type *creature_ptr);
extern void do_cmd_checkquest(player_type *creature_ptr);
extern void do_cmd_time(player_type *creature_ptr);

/*
 * Forward declare
 */
extern void do_cmd_spoilers(player_type *creature_ptr);

/* wizard1.c */
extern void spoil_random_artifact(player_type *creature_ptr, concptr fname);

/* 暫定。後で移す. */
void do_cmd_knowledge_monsters(player_type *creature_ptr, bool *need_redraw, bool visual_only, IDX direct_r_idx);
void do_cmd_knowledge_objects(player_type *creature_ptr, bool *need_redraw, bool visual_only, IDX direct_k_idx);
void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, IDX direct_f_idx, IDX *lighting_level);
