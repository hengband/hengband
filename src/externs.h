/*!
 * @file externs.h
 * @brief Angband(変愚蛮怒)基本関数、グローバル変数ヘッダファイル / 
 * extern declarations (variables and functions)
 * @date 2014/08/08
 * @author
 * Copyright (c) 1997 Ben Harrison
 * @details
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */
#include "geometry.h"

/*
 * Automatically generated "variable" declarations
 */
extern int max_macrotrigger;
extern concptr macro_template;
extern concptr macro_modifier_chr;
extern concptr macro_modifier_name[MAX_MACRO_MOD];
extern concptr macro_trigger_name[MAX_MACRO_TRIG];
extern concptr macro_trigger_keycode[2][MAX_MACRO_TRIG];

extern int level_up;

/* 
 *  List for auto-picker/destroyer entries
 */
extern int max_autopick;
extern int max_max_autopick;
extern autopick_type *autopick_list;

/* tables.c */
extern const POSITION ddd[9];
extern const POSITION ddx[10];
extern const POSITION ddy[10];
extern const POSITION ddx_ddd[9];
extern const POSITION ddy_ddd[9];
extern const POSITION cdd[8];
extern const POSITION ddx_cdd[8];
extern const POSITION ddy_cdd[8];
extern const char hexsym[16];
extern const char listsym[];
extern const concptr color_char;
extern const byte adj_mag_study[];
extern const byte adj_mag_mana[];
extern const byte adj_mag_fail[];
extern const byte adj_mag_stat[];
extern const byte adj_chr_gold[];
extern const byte adj_int_dev[];
extern const byte adj_wis_sav[];
extern const byte adj_dex_dis[];
extern const byte adj_int_dis[];
extern const byte adj_dex_ta[];
extern const byte adj_str_td[];
extern const byte adj_dex_th[];
extern const byte adj_str_th[];
extern const byte adj_str_wgt[];
extern const byte adj_str_hold[];
extern const byte adj_str_dig[];
extern const byte adj_str_blow[];
extern const byte adj_dex_blow[];
extern const byte adj_dex_safe[];
extern const byte adj_con_fix[];
extern const byte adj_con_mhp[];
extern const byte adj_chr_chm[];
extern const byte blows_table[12][12];
extern const byte extract_energy[200];
extern const player_sex sex_info[MAX_SEXES];
extern const player_race race_info[MAX_RACES];
extern const player_class class_info[MAX_CLASS];
extern const magic_type technic_info[NUM_TECHNIC][32];
extern const player_seikaku seikaku_info[MAX_SEIKAKU];
extern const player_race mimic_info[];
extern const u32b fake_spell_flags[4];
extern const s32b realm_choices1[];
extern const s32b realm_choices2[];
extern const concptr realm_names[];
#ifdef JP
extern const concptr E_realm_names[];
#endif
extern const concptr spell_names[VALID_REALM][32];
extern const concptr player_title[MAX_CLASS][PY_MAX_LEVEL/5];
extern const concptr color_names[16];
extern const concptr stat_names[6];
extern const concptr stat_names_reduced[6];
extern const concptr window_flag_desc[32];
extern const martial_arts ma_blows[MAX_MA];
extern const int monk_ave_damage[PY_MAX_LEVEL+1][3];
extern const concptr game_inscriptions[];
extern const kamae kamae_shurui[MAX_KAMAE];
extern const kamae kata_shurui[MAX_KATA];
extern const concptr exp_level_str[5];
extern const concptr silly_attacks[MAX_SILLY_ATTACK];
#ifdef JP
extern const concptr silly_attacks2[MAX_SILLY_ATTACK];
#endif
extern const concptr ident_info[];
extern const mbe_info_type mbe_info[];
extern const byte feature_action_flags[FF_FLAG_MAX];
extern const dragonbreath_type dragonbreath_info[];

extern const option_type option_info[];
extern const option_type cheat_info[CHEAT_MAX];
extern const option_type autosave_info[2];

/* variable.c */
extern const concptr copyright[5];
extern byte h_ver_major;
extern byte h_ver_minor;
extern byte h_ver_patch;
extern byte h_ver_extra;
extern byte sf_extra;
extern u32b sf_system;
extern byte z_major;
extern byte z_minor;
extern byte z_patch;
extern u32b sf_when;
extern u16b sf_lives;
extern u16b sf_saves;
extern bool arg_fiddle;
extern bool arg_wizard;
extern bool arg_music;
extern bool arg_sound;
extern byte arg_graphics;
extern bool arg_monochrome;
extern bool arg_force_original;
extern bool arg_force_roguelike;
extern bool arg_bigtile;
extern bool character_generated;
extern bool character_dungeon;
extern bool character_loaded;
extern bool character_saved;
extern bool character_icky;
extern bool character_xtra;
extern bool creating_savefile;
extern u32b seed_flavor;
extern u32b seed_town;
extern COMMAND_CODE command_cmd;
extern COMMAND_ARG command_arg;
extern s16b command_rep;
extern DIRECTION command_dir;
extern s16b command_see;
extern TERM_LEN command_gap;
extern s16b command_wrk;
extern s16b command_new;
extern bool msg_flag;
extern s16b running;
extern bool invoking_midnight_curse;
extern GAME_TURN old_battle;
extern bool use_sound;
extern bool use_music;
extern bool use_graphics;
extern bool use_bigtile;
extern s16b signal_count;
extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;
extern bool get_com_no_macros;
extern OBJECT_SUBTYPE_VALUE coin_type;
extern bool opening_chest;
extern bool shimmer_monsters;
extern bool shimmer_objects;
extern bool repair_monsters;
extern bool repair_objects;
extern s16b inven_cnt;
extern s16b equip_cnt;
extern OBJECT_IDX o_max;
extern OBJECT_IDX o_cnt;
extern MONSTER_IDX m_max;
extern MONSTER_IDX m_cnt;
extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;
extern int total_friends;
extern int leaving_quest;
extern bool reinit_wilderness;
extern bool multi_rew;
extern char summon_kin_type;
extern bool is_loading_now;
extern bool hack_mutation;
extern bool reset_concent;
extern bool is_fired;



/*
 * Software options (set via the '=' command).  See "tables.c"
 */


extern char record_o_name[MAX_NLEN];
extern GAME_TURN record_turn;
extern bool closing_flag;
extern POSITION panel_row_min, panel_row_max;
extern POSITION panel_col_min, panel_col_max;
extern POSITION panel_col_prt, panel_row_prt;
extern MONSTER_IDX target_who;
extern POSITION target_col;
extern POSITION target_row;
extern int player_uid;
extern int player_euid;
extern int player_egid;
extern char player_base[32];
extern char savefile[1024];
extern char savefile_base[40];
extern pos_list tmp_pos;
extern s16b macro__num;
extern concptr *macro__pat;
extern concptr *macro__act;
extern bool *macro__cmd;
extern char *macro__buf;
extern STR_OFFSET quark__num;
extern concptr *quark__str;
extern u32b message__next;
extern u32b message__last;
extern u32b message__head;
extern u32b message__tail;
extern u32b *message__ptr;
extern char *message__buf;
extern BIT_FLAGS option_flag[8];
extern BIT_FLAGS option_mask[8];
extern BIT_FLAGS window_flag[8];
extern BIT_FLAGS window_mask[8];
extern term *angband_term[8];
extern const char angband_term_name[8][16];
extern byte angband_color_table[256][4];
extern const concptr angband_sound_name[SOUND_MAX];
extern const concptr angband_music_basic_name[MUSIC_BASIC_MAX];
extern floor_type *current_floor_ptr;
extern world_type *current_world_ptr;
extern saved_floor_type saved_floors[MAX_SAVED_FLOORS];
extern FLOOR_IDX max_floor_id;
extern u32b saved_floor_file_sign;
extern TOWN_IDX max_towns;
extern town_type *town_info;
extern object_type *inventory;
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern TERM_COLOR misc_to_attr[256];
extern SYMBOL_CODE misc_to_char[256];
extern TERM_COLOR tval_to_attr[128];
extern SYMBOL_CODE tval_to_char[128];
extern concptr keymap_act[KEYMAP_MODES][256];
extern player_type *p_ptr;
extern const player_sex *sp_ptr;
extern const player_race *rp_ptr;
extern const player_class *cp_ptr;
extern const player_seikaku *ap_ptr;
extern const player_magic *mp_ptr;
extern birther previous_char;
extern vault_type *v_info;
extern char *v_name;
extern char *v_text;
extern skill_table *s_info;
extern player_magic *m_info;
extern feature_type *f_info;
extern char *f_name;
extern char *f_tag;
extern object_kind *k_info;
extern char *k_name;
extern char *k_text;
extern artifact_type *a_info;
extern char *a_name;
extern char *a_text;
extern ego_item_type *e_info;
extern char *e_name;
extern char *e_text;
extern monster_race *r_info;
extern char *r_name;
extern char *r_text;
extern dungeon_type *d_info;
extern char *d_name;
extern char *d_text;
extern concptr ANGBAND_SYS;
extern concptr ANGBAND_KEYBOARD;
extern concptr ANGBAND_GRAF;
extern concptr ANGBAND_DIR;
extern concptr ANGBAND_DIR_APEX;
extern concptr ANGBAND_DIR_BONE;
extern concptr ANGBAND_DIR_DATA;
extern concptr ANGBAND_DIR_EDIT;
extern concptr ANGBAND_DIR_SCRIPT;
extern concptr ANGBAND_DIR_FILE;
extern concptr ANGBAND_DIR_HELP;
extern concptr ANGBAND_DIR_INFO;
extern concptr ANGBAND_DIR_PREF;
extern concptr ANGBAND_DIR_SAVE;
extern concptr ANGBAND_DIR_USER;
extern concptr ANGBAND_DIR_XTRA;
extern OBJECT_TYPE_VALUE item_tester_tval;
extern bool (*item_tester_hook)(object_type *o_ptr);
extern monsterrace_hook_type get_mon_num_hook;
extern monsterrace_hook_type get_mon_num2_hook;
extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);
extern building_type building[MAX_BLDG];
extern QUEST_IDX max_q_idx;
extern MONRACE_IDX max_r_idx;
extern KIND_OBJECT_IDX max_k_idx;
extern VAULT_IDX max_v_idx;
extern FEAT_IDX max_f_idx;
extern ARTIFACT_IDX max_a_idx;
extern EGO_IDX max_e_idx;
extern DUNGEON_IDX max_d_idx;
extern quest_type *quest;
extern char quest_text[10][80];
extern int quest_text_line;
extern int init_flags;
extern int highscore_fd;
extern bool can_save;
extern int cap_mon;
extern int cap_mspeed;
extern HIT_POINT cap_hp;
extern HIT_POINT cap_maxhp;
extern STR_OFFSET cap_nickname;
extern MONRACE_IDX battle_mon[4];
extern int sel_monster;
extern int battle_odds;
extern PRICE kakekin;
extern u32b mon_odds[4];
extern MONSTER_IDX pet_t_m_idx;
extern MONSTER_IDX riding_t_m_idx;
extern MONRACE_IDX today_mon;
extern bool write_level;
extern u32b start_time;
extern bool sukekaku;
extern bool new_mane;
extern bool mon_fight;
extern bool generate_encounter;
extern concptr screen_dump;


extern DEPTH *max_dlv;
extern FEAT_IDX feat_wall_outer;
extern FEAT_IDX feat_wall_inner;
extern FEAT_IDX feat_wall_solid;
extern FEAT_IDX feat_ground_type[100], feat_wall_type[100];
extern COMMAND_CODE now_message;
extern bool use_menu;

/* autopick.c */
extern void autopick_load_pref(bool disp_mes);
extern errr process_autopick_file_command(char *buf);
extern concptr autopick_line_from_entry(autopick_type *entry);
extern int is_autopick(object_type *o_ptr);
extern void autopick_alter_item(INVENTORY_IDX item, bool destroy);
extern void autopick_delayed_alter(void);
extern void autopick_pickup_items(grid_type *g_ptr);
extern bool autopick_autoregister(object_type *o_ptr);
extern void do_cmd_edit_autopick(void);


/* grids.c */
extern void update_local_illumination(POSITION y, POSITION x);
extern bool player_can_see_bold(POSITION y, POSITION x);
extern bool cave_valid_bold(POSITION y, POSITION x);
extern bool no_lite(void);
extern void print_rel(SYMBOL_CODE c, TERM_COLOR a, TERM_LEN y, TERM_LEN x);
extern void delayed_visual_update(void);
extern void forget_flow(void);
extern void update_flow(void);
extern void update_smell(void);
extern void cave_set_feat(POSITION y, POSITION x, FEAT_IDX feat);

extern bool cave_monster_teleportable_bold(MONSTER_IDX m_idx, POSITION y, POSITION x, BIT_FLAGS mode);
extern bool cave_player_teleportable_bold(POSITION y, POSITION x, BIT_FLAGS mode);

/* cmd4.c */
#ifndef JP
extern concptr get_ordinal_number_suffix(int num);
#endif
extern errr do_cmd_write_nikki(int type, int num, concptr note);
extern void do_cmd_nikki(void);
extern void do_cmd_redraw(void);
extern void do_cmd_change_name(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(int num_now);
extern void do_cmd_options_aux(int page, concptr info);
extern void do_cmd_options(void);
extern void do_cmd_pref(void);
extern void do_cmd_reload_autopick(void);
extern void do_cmd_macros(void);
extern void do_cmd_visuals(void);
extern void do_cmd_colors(void);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(void);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen_html_aux(char *filename, int message);
extern void do_cmd_save_screen(void);
extern void do_cmd_knowledge_quests_completed(FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge_quests_failed(FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge(void);
extern void plural_aux(char * Name);
extern void do_cmd_checkquest(void);
extern void do_cmd_time(void);
extern void do_cmd_suicide(void);

/* dungeon.c */
extern void extract_option_vars(void);
extern void determine_bounty_uniques(void);
extern void determine_today_mon(bool conv_old);
extern void play_game(bool new_game);
extern s32b turn_real(s32b hoge);
extern void prevent_turn_overflow(void);
extern void close_game(void);

/* load.c */
extern errr rd_savefile_new(void);
extern bool load_floor(saved_floor_type *sf_ptr, BIT_FLAGS mode);

/* monster2.c */
extern void set_target(monster_type *m_ptr, POSITION y, POSITION x);
extern void reset_target(monster_type *m_ptr);
extern monster_race *real_r_ptr(monster_type *m_ptr);
extern MONRACE_IDX real_r_idx(monster_type *m_ptr);
extern void delete_monster_idx(MONSTER_IDX i);
extern void delete_monster(POSITION y, POSITION x);
extern void compact_monsters(int size);
extern void wipe_m_list(void);
extern MONSTER_IDX m_pop(void);
extern errr get_mon_num_prep(monsterrace_hook_type monster_hook, monsterrace_hook_type monster_hook2);
extern MONRACE_IDX get_mon_num(DEPTH level);
extern int lore_do_probe(MONRACE_IDX r_idx);
extern void lore_treasure(MONSTER_IDX m_idx, ITEM_NUMBER num_item, ITEM_NUMBER num_gold);
extern void update_monster(MONSTER_IDX m_idx, bool full);
extern void update_monsters(bool full);
extern bool multiply_monster(MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);
extern void update_smart_learn(MONSTER_IDX m_idx, int what);
extern void choose_new_monster(MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx);
extern SPEED get_mspeed(monster_race *r_ptr);
extern void monster_drop_carried_objects(monster_type *m_ptr);

/* object1.c */
extern void reset_visuals(void);
extern void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern concptr item_activation(object_type *o_ptr);
extern bool screen_object(object_type *o_ptr, BIT_FLAGS mode);
extern char index_to_label(int i);
extern INVENTORY_IDX label_to_inven(int c);
extern INVENTORY_IDX label_to_equip(int c);
extern s16b wield_slot(object_type *o_ptr);
extern concptr mention_use(int i);
extern concptr describe_use(int i);
extern bool check_book_realm(const OBJECT_TYPE_VALUE book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
extern bool item_tester_okay(object_type *o_ptr);
extern void display_inven(void);
extern void display_equip(void);
extern COMMAND_CODE show_inven(int target_item, BIT_FLAGS mode);
extern COMMAND_CODE show_equip(int target_item, BIT_FLAGS mode);
extern void toggle_inven_equip(void);
extern bool can_get_item(void);
extern bool get_item(OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode);
extern object_type *choose_object(OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option);
PERCENTAGE breakage_chance(object_type *o_ptr, SPELL_IDX snipe_type);

/* object2.c */
extern void excise_object_idx(OBJECT_IDX o_idx);
extern void delete_object_idx(OBJECT_IDX o_idx);
extern void delete_object(POSITION y, POSITION x);
extern void compact_objects(int size);
extern void wipe_o_list(void);
extern OBJECT_IDX o_pop(void);
extern OBJECT_IDX get_obj_num(DEPTH level);
extern void object_known(object_type *o_ptr);
extern void object_aware(object_type *o_ptr);
extern void object_tried(object_type *o_ptr);
extern byte value_check_aux1(object_type *o_ptr);
extern byte value_check_aux2(object_type *o_ptr);
extern PRICE object_value(object_type *o_ptr);
extern PRICE object_value_real(object_type *o_ptr);
extern void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
extern void reduce_charges(object_type *o_ptr, int amt);
extern int object_similar_part(object_type *o_ptr, object_type *j_ptr);
extern bool object_similar(object_type *o_ptr, object_type *j_ptr);
extern void object_absorb(object_type *o_ptr, object_type *j_ptr);
extern IDX lookup_kind(OBJECT_TYPE_VALUE tval, OBJECT_SUBTYPE_VALUE sval);
extern void object_wipe(object_type *o_ptr);
extern void object_prep(object_type *o_ptr, KIND_OBJECT_IDX k_idx);
extern void object_copy(object_type *o_ptr, object_type *j_ptr);
extern void apply_magic(object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);
extern bool make_object(object_type *j_ptr, BIT_FLAGS mode);
extern void place_object(POSITION y, POSITION x, BIT_FLAGS mode);
extern bool make_gold(object_type *j_ptr);
extern void place_gold(POSITION y, POSITION x);
extern OBJECT_IDX drop_near(object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
extern void inven_item_charges(INVENTORY_IDX item);
extern void inven_item_describe(INVENTORY_IDX item);
extern void inven_item_increase(INVENTORY_IDX item, ITEM_NUMBER num);
extern void inven_item_optimize(INVENTORY_IDX item);
extern void floor_item_charges(INVENTORY_IDX item);
extern void floor_item_describe(INVENTORY_IDX item);
extern void floor_item_increase(INVENTORY_IDX item, ITEM_NUMBER num);
extern void floor_item_optimize(INVENTORY_IDX item);
extern bool inven_carry_okay(object_type *o_ptr);
extern bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
extern s16b inven_carry(object_type *o_ptr);
extern INVENTORY_IDX inven_takeoff(INVENTORY_IDX item, ITEM_NUMBER amt);
extern void inven_drop(INVENTORY_IDX item, ITEM_NUMBER amt);
extern void combine_pack(void);
extern void reorder_pack(void);
extern void display_koff(KIND_OBJECT_IDX k_idx);
extern void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
extern void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
extern void torch_lost_fuel(object_type *o_ptr);
extern concptr essence_name[];

/* racial.c */
extern void do_cmd_racial_power(void);

/* save.c */
extern bool save_player(void);
extern bool load_player(void);
extern void remove_loc(void);
extern bool save_floor(saved_floor_type *sf_ptr, BIT_FLAGS mode);

/* spells1.c */
extern PERCENTAGE beam_chance(void);
extern bool in_disintegration_range(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern void breath_shape(u16b *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ);
extern int take_hit(int damage_type, HIT_POINT damage, concptr kb_str, int monspell);
extern u16b bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, EFFECT_ID typ);
extern POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern bool project(MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flg, int monspell);
extern int project_length;
extern bool binding_field(HIT_POINT dam);
extern void seal_of_mirror(HIT_POINT dam);
extern concptr spell_category_name(OBJECT_TYPE_VALUE tval);

/* spells2.c */
extern bool detect_traps(POSITION range, bool known);
extern bool detect_doors(POSITION range);
extern bool detect_stairs(POSITION range);
extern bool detect_treasure(POSITION range);
extern bool detect_objects_gold(POSITION range);
extern bool detect_objects_normal(POSITION range);
extern bool detect_objects_magic(POSITION range);
extern bool detect_monsters_normal(POSITION range);
extern bool detect_monsters_invis(POSITION range);
extern bool detect_monsters_evil(POSITION range);
extern bool detect_monsters_xxx(POSITION range, u32b match_flag);
extern bool detect_monsters_string(POSITION range, concptr);
extern bool detect_monsters_nonliving(POSITION range);
extern bool detect_monsters_mind(POSITION range);
extern bool detect_all(POSITION range);
extern bool wall_stone(void);
extern bool speed_monsters(void);
extern bool slow_monsters(int power);
extern bool sleep_monsters(int power);
extern void aggravate_monsters(MONSTER_IDX who);
extern bool genocide_aux(MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, concptr spell_name);
extern bool symbol_genocide(int power, bool player_cast);
extern bool mass_genocide(int power, bool player_cast);
extern bool mass_genocide_undead(int power, bool player_cast);
extern bool probing(void);
extern bool banish_evil(int dist);
extern bool dispel_evil(HIT_POINT dam);
extern bool dispel_good(HIT_POINT dam);
extern bool dispel_undead(HIT_POINT dam);
extern bool dispel_monsters(HIT_POINT dam);
extern bool dispel_living(HIT_POINT dam);
extern bool dispel_demons(HIT_POINT dam);
extern bool cleansing_nova(player_type *creature_ptr, bool magic, bool powerful);
extern bool unleash_mana_storm(player_type *creature_ptr, bool powerful);
extern bool crusade(void);
extern bool turn_undead(void);
extern bool destroy_area(POSITION y1, POSITION x1, POSITION r, bool in_generate);
extern bool earthquake_aux(POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx);
extern bool earthquake(POSITION cy, POSITION cx, POSITION r);
extern void lite_room(POSITION y1, POSITION x1);
extern bool starlight(bool magic);
extern void unlite_room(POSITION y1, POSITION x1);
extern bool lite_area(HIT_POINT dam, POSITION rad);
extern bool unlite_area(HIT_POINT dam, POSITION rad);
extern bool fire_ball(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_breath(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_rocket(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_ball_hide(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_meteor(MONSTER_IDX who, EFFECT_ID typ, POSITION x, POSITION y, HIT_POINT dam, POSITION rad);
extern bool fire_bolt(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
extern bool fire_blast(EFFECT_ID typ, DIRECTION dir, DICE_NUMBER dd, DICE_SID ds, int num, int dev);
extern void call_chaos(void);
extern bool fire_beam(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
extern bool fire_bolt_or_beam(PERCENTAGE prob, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
extern bool lite_line(DIRECTION dir, HIT_POINT dam);
extern bool hypodynamic_bolt(DIRECTION dir, HIT_POINT dam);
extern bool death_ray(DIRECTION dir, PLAYER_LEVEL plev);
extern bool wall_to_mud(DIRECTION dir, HIT_POINT dam);
extern bool destroy_door(DIRECTION dir);
extern bool disarm_trap(DIRECTION dir);
extern bool wizard_lock(DIRECTION dir);
extern bool teleport_monster(DIRECTION dir, int distance);
extern bool door_creation(void);
extern bool trap_creation(POSITION y, POSITION x);
extern bool tree_creation(void);
extern bool glyph_creation(void);
extern bool destroy_doors_touch(void);
extern bool disarm_traps_touch(void);
extern bool animate_dead(MONSTER_IDX who, POSITION y, POSITION x);
extern bool sleep_monsters_touch(void);
extern bool activate_ty_curse(bool stop_ty, int *count);
extern int activate_hi_summon(POSITION y, POSITION x, bool can_pet);
extern void wall_breaker(void);
extern bool confuse_monsters(HIT_POINT dam);
extern bool charm_monsters(HIT_POINT dam);
extern bool charm_animals(HIT_POINT dam);
extern bool stun_monsters(HIT_POINT dam);
extern bool stasis_monsters(HIT_POINT dam);
extern bool banish_monsters(int dist);
extern bool turn_monsters(HIT_POINT dam);
extern bool turn_evil(HIT_POINT dam);
extern bool deathray_monsters(void);
extern bool charm_monster(DIRECTION dir, PLAYER_LEVEL plev);
extern bool control_one_undead(DIRECTION dir, PLAYER_LEVEL plev);
extern bool control_one_demon(DIRECTION dir, PLAYER_LEVEL plev);
extern bool charm_animal(DIRECTION dir, PLAYER_LEVEL plev);
extern bool mindblast_monsters(HIT_POINT dam);
extern s32b flag_cost(object_type *o_ptr, int plusses);
extern bool teleport_swap(DIRECTION dir);
extern bool project_hook(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, BIT_FLAGS flg);
extern bool project_all_los(EFFECT_ID typ, HIT_POINT dam);
extern bool eat_magic(int power);
extern void discharge_minion(void);
extern bool kawarimi(bool success);
extern bool rush_attack(bool *mdeath);
extern void remove_all_mirrors(bool explode);
extern void ring_of_power(DIRECTION dir);
extern void wild_magic(int spell);
extern void cast_meteor(HIT_POINT dam, POSITION rad);
extern bool cast_wrath_of_the_god(HIT_POINT dam, POSITION rad);
extern void cast_wonder(DIRECTION dir);
extern void cast_invoke_spirits(DIRECTION dir);
extern void cast_shuffle(void);
extern void stop_mouth(void);
extern bool_hack vampirism(void);
extern bool panic_hit(void);
extern bool psychometry(void);
extern bool draconian_breath(player_type *creature_ptr);
extern bool android_inside_weapon(player_type *creature_ptr);
extern bool create_ration(player_type *crature_ptr);
extern void hayagake(player_type *creature_ptr);
extern bool double_attack(player_type *creature_ptr);
extern bool comvert_hp_to_mp(player_type *creature_ptr);
extern bool comvert_mp_to_hp(player_type *creature_ptr);
extern bool demonic_breath(player_type *creature_ptr);
extern bool mirror_concentration(player_type *creature_ptr);
extern bool sword_dancing(player_type *creature_ptr);
extern bool confusing_light(player_type *creature_ptr);
extern bool rodeo(player_type *creature_ptr);
extern bool clear_mind(player_type *creature_ptr);
extern bool concentration(player_type *creature_ptr);

/* xtra1.c */
extern void handle_stuff(void);
extern void update_output(void);
extern void print_monster_list(TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
extern void update_playtime(void);

/* xtra2.c */
extern void panel_bounds_center(void);
extern void verify_panel(void);
extern bool target_able(MONSTER_IDX m_idx);
extern bool target_okay(void);
extern bool target_set(BIT_FLAGS mode);
extern void target_set_prepare_look(void);
extern bool get_aim_dir(DIRECTION *dp);
extern bool get_hack_dir(DIRECTION *dp);
extern bool get_direction(DIRECTION *dp, bool allow_under, bool with_steed);
extern bool get_rep_dir(DIRECTION *dp, bool under);
extern bool tgt_pt(POSITION *x, POSITION *y);

/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef SET_UID
/* util.c */
extern void user_name(char *buf, int id);
#endif

#if 0
#ifndef HAS_STRICMP
/* util.c */
extern int stricmp(concptr a, concptr b);
#endif
#endif

#ifndef HAVE_USLEEP
/* util.c */
extern int usleep(huge usecs);
#endif

#ifdef MACINTOSH
/* main-mac.c */
/* extern void main(void); */
#endif

#if defined(MAC_MPW) || defined(MACH_O_CARBON)
/* Globals needed */
extern  u32b _ftype;
extern  u32b _fcreator;
#endif

#if defined(MAC_MPW) && defined(CARBON)
extern void convert_pathname(char *path);
#endif

#if defined(MACH_O_CARBON)
extern void fsetfileinfo(concptr path, u32b fcreator, u32b ftype);
#endif

#ifdef WINDOWS
/* main-win.c */
/* extern int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, ...); */
#endif

/* util.c */
extern void repeat_push(COMMAND_CODE what);
extern bool repeat_pull(COMMAND_CODE *what);
extern void repeat_check(void);

extern bool easy_disarm;
extern bool easy_floor;
extern bool easy_open;


/* object1.c */
extern ITEM_NUMBER scan_floor(OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode);
extern COMMAND_CODE show_floor(int target_item, POSITION y, POSITION x, TERM_LEN *min_width);
extern bool get_item_floor(COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode);
extern void py_pickup_floor(bool pickup);

/* wizard1.c */
extern void spoil_random_artifact(concptr fname);

/* wizard2.c */
extern void strip_name(char *buf, KIND_OBJECT_IDX k_idx);
extern void cheat_death(player_type *creature_ptr);


#ifdef JP
/* japanese.c */
extern void sindarin_to_kana(char *kana, concptr sindarin);
extern void jverb( concptr in , char *out , int flag);
extern void sjis2euc(char *str);
extern void euc2sjis(char *str);
extern byte codeconv(char *str);
extern bool iskanji2(concptr s, int x);
extern void guess_convert_to_system_encoding(char* strbuf, int buflen);
#endif

#ifdef WORLD_SCORE
/* report.c */
extern errr report_score(void);
extern concptr make_screen_dump(void);
#endif

/* inet.c */
extern int soc_write(int sd, char *buf, size_t sz);
extern int soc_read(int sd, char *buf, size_t sz);
extern void set_proxy(char *default_url, int default_port);
extern int connect_server(int timeout, concptr host, int port);
extern int disconnect_server(int sd);
extern concptr soc_err(void);

extern void prepare_movie_hooks(void);
extern void prepare_browse_movie_aux(concptr filename);
extern void prepare_browse_movie(concptr filename);
extern void browse_movie(void);
extern bool browsing_movie;

#ifdef TRAVEL
/* for travel */
extern travel_type travel;
#endif

extern void kamaenaoshi(INVENTORY_IDX item);
