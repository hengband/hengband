#include "angband.h"
#include "Python.h"

extern void eat_callback(int sval);
extern void kill_monster_callback(int m_idx);
extern bool player_move_callback(int y, int x);
extern bool cmd_open_callback(int y, int x);
extern bool cmd_search_callback(int y, int x);
extern bool cmd_feeling_callback(int feeling);
extern bool cmd_go_up_callback(void);
extern bool building_command_callback(int number, int action);
extern void callbacks_load_callback(char *data);
extern cptr callbacks_save_callback(void);

/* Dungeon grids */
extern bool player_enter_grid_callback(int y, int x);
extern bool player_search_grid_callback(int y, int x);

/* Dungeon levels */
extern bool generate_level_callback(int level);
extern void leave_level_callback(int level);
extern void enter_level_callback(int level);

/* Wilderness */
extern bool wilderness_init_callback(void);
extern bool generate_wilderness_callback(int y, int x);
extern bool enter_wilderness_callback(int y, int x);
extern bool leave_wilderness_callback(int y, int x);

extern void store_examine_callback(object_type *o_ptr);
extern bool monster_move_callback(int *mm, int m_idx);
extern void create_monster_callback(int m_idx);
extern void delete_monster_callback(int m_idx);
extern void copy_monster_callback(int i1, int i2);
extern char inkey_borg_callback(bool inkey_base, bool inkey_xtra,
                                bool inkey_flag, bool inkey_scan);
extern char inkey_callback(char key);

/* Birth */
extern long get_world_callback(void);
extern long get_player_class_callback(void);
extern long get_player_realms_callback(void);
extern long get_player_race_callback(void);
extern long get_player_seikaku_callback(void);

extern bool get_player_flags_callback(void);
extern bool player_outfit_callback(void);

extern long sense_inventory_callback(void);
extern bool destroy_object_callback(object_type *o_ptr, int number);

/* Object callbacks - global */
extern PyObject* object_create_callback(object_type *o_ptr);
extern PyObject* object_load_callback(char *code);

/* Object callbacks - object specific */
extern bool object_eat_callback(object_type *o_ptr);
extern bool object_browse_callback(object_type *o_ptr);
extern bool object_cast_callback(object_type *o_ptr);
extern cptr object_save_callback(object_type *o_ptr);
extern void object_delete_callback(object_type *o_ptr);
extern PyObject* object_copy_callback(object_type *o_ptr, object_type *j_ptr);
extern long get_object_level_callback(object_type *o_ptr);
extern long get_object_cost_callback(object_type *o_ptr);
extern cptr get_object_name_callback(object_type *o_ptr);
extern char get_object_d_char_callback(object_type *o_ptr);
extern char get_object_x_char_callback(object_type *o_ptr);
extern byte get_object_d_attr_callback(object_type *o_ptr);
extern byte get_object_x_attr_callback(object_type *o_ptr);
extern bool get_object_aware_callback(object_type *o_ptr);
extern bool get_object_tried_callback(object_type *o_ptr);

/* Object_kind callbacks */
extern bool free_object_kind_list_callback(void);
extern bool init_object_kind_list_callback(void);

extern bool use_skill_callback(void);


#define CMD_EAT_EVENT                1
#define PLAYER_MOVE_EVENT            2
#define CMD_OPEN_EVENT               3
#define CMD_SEARCH_EVENT             4
#define PLAYER_SEARCH_GRID_EVENT     5
#define CMD_FEELING_EVENT            6
#define CMD_GO_UP_EVENT              7
#define CALLBACKS_LOAD_EVENT         8
#define CALLBACKS_SAVE_EVENT         9
#define KILL_MONSTER_EVENT          10
#define BUILDING_COMMAND_EVENT      11
#define LEAVE_LEVEL_EVENT           12
#define PLAYER_ENTER_GRID_EVENT     13
#define ENTER_LEVEL_EVENT           14
#define GENERATE_LEVEL_EVENT        15
#define GENERATE_WILDERNESS_EVENT   16
#define ENTER_WILDERNESS_EVENT      17
#define LEAVE_WILDERNESS_EVENT      18
#define STORE_EXAMINE_EVENT         19
#define MONSTER_MOVE_EVENT          20
#define CREATE_MONSTER_EVENT        21
#define DELETE_MONSTER_EVENT        22
#define INKEY_BORG_EVENT            23
#define INKEY_EVENT                 24
#define GET_PLAYER_CLASS_EVENT      25
#define GET_PLAYER_FLAGS_EVENT      26
#define SENSE_INVENTORY_EVENT       27
#define DESTROY_OBJECT_EVENT        28
#define GET_PLAYER_RACE_EVENT       29
#define OBJECT_CREATE_EVENT         30
#define OBJECT_LOAD_EVENT           31
#define PLAYER_OUTFIT_EVENT         32
#define WILDERNESS_INIT_EVENT       33
#define FREE_OBJECT_KIND_LIST_EVENT 34
#define INIT_OBJECT_KIND_LIST_EVENT 35
#define GET_PLAYER_REALMS_EVENT     36
#define GET_WORLD_EVENT             37
#define COPY_MONSTER_EVENT          38
#define USE_SKILL_EVENT             39

#define MAX_EVENT                   40
