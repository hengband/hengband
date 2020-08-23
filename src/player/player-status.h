#pragma once

#include "system/angband.h"

/* 人畜無害なenumヘッダを先に読み込む */
#include "player-info/base-status-types.h"
#include "player/player-classes-types.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
#include "spell/spells-util.h"

/*
 * Most of the "player" information goes here.
 *
 * This stucture gives us a large collection of player variables.
 *
 * This structure contains several "blocks" of information.
 *   (1) the "permanent" info
 *   (2) the "variable" info
 *   (3) the "transient" info
 *
 * All of the "permanent" info, and most of the "variable" info,
 * is saved in the savefile.  The "transient" info is recomputed
 * whenever anything important changes.
 */

/*
 * Player constants
 */
#define PY_MAX_EXP 99999999L /*!< プレイヤー経験値の最大値 / Maximum exp */
#define PY_MAX_GOLD 999999999L /*!< プレイヤー所持金の最大値 / Maximum gold */
#define PY_MAX_LEVEL 50 /*!< プレイヤーレベルの最大値 / Maximum level */

#define GINOU_MAX 10
#define MAX_MANE 16

#define MAGIC_GLOVE_REDUCE_MANA 0x0001
#define MAGIC_FAIL_5PERCENT 0x0002
#define MAGIC_GAIN_EXP 0x0004

/* no_flowed 判定対象となるスペル */
#define SPELL_DD_S 27
#define SPELL_DD_T 13
#define SPELL_SW 22
#define SPELL_WALL 20

/*!< Empty hand status */
enum empty_hand_status {
    EMPTY_HAND_NONE = 0x0000, /*!<Both hands are used */
    EMPTY_HAND_LARM = 0x0001, /*!<Left hand is empty */
    EMPTY_HAND_RARM = 0x0002 /*!<Right hand is empty */
};

/*
 * Player sex constants (hard-coded by save-files, arrays, etc)
 */
#define SEX_FEMALE 0
#define SEX_MALE 1
#define MAX_SEXES 2 /*!< 性別の定義最大数 / Maximum number of player "sex" types (see "table.c", etc) */

typedef struct floor_type floor_type;
typedef struct object_type object_type;
typedef struct player_type {
    int player_uid;
    int player_euid;
    int player_egid;

    floor_type *current_floor_ptr;
    POSITION oldpy; /* Previous player location -KMW- */
    POSITION oldpx; /* Previous player location -KMW- */

    SEX_IDX psex; /* Sex index */
    player_race_type prace; /* Race index */
    player_class_type pclass; /* Class index */
    player_personality_type pseikaku; /* Seikaku index */
    REALM_IDX realm1; /* First magic realm */
    REALM_IDX realm2; /* Second magic realm */
    player_personality_type oops; /* Unused */

    DICE_SID hitdie; /* Hit dice (sides) */
    u16b expfact; /* Experience factor
                   * Note: was byte, causing overflow for Amberite
                   * characters (such as Amberite Paladins)
                   */

    s16b age; /* Characters age */
    s16b ht; /* Height */
    s16b wt; /* Weight */
    s16b sc; /* Social Class */

    PRICE au; /* Current Gold */

    EXP max_max_exp; /* Max max experience (only to calculate score) */
    EXP max_exp; /* Max experience */
    EXP exp; /* Cur experience */
    u32b exp_frac; /* Cur exp frac (times 2^16) */

    PLAYER_LEVEL lev; /* Level */

    TOWN_IDX town_num; /* Current town number */
    s16b arena_number; /* monster number in on_defeat_arena_monster -KMW- */
    bool phase_out; /*!< フェイズアウト状態(闘技場観戦状態などに利用、NPCの処理の対象にならず自身もほとんどの行動ができない) */

    DUNGEON_IDX dungeon_idx; /* current dungeon index */
    POSITION wilderness_x; /* Coordinates in the wilderness */
    POSITION wilderness_y;
    bool wild_mode;

    HIT_POINT mhp; /* Max hit pts */
    HIT_POINT chp; /* Cur hit pts */
    u32b chp_frac; /* Cur hit frac (times 2^16) */
    PERCENTAGE mutant_regenerate_mod;

    MANA_POINT msp; /* Max mana pts */
    MANA_POINT csp; /* Cur mana pts */
    u32b csp_frac; /* Cur mana frac (times 2^16) */

    s16b max_plv; /* Max Player Level */

    BASE_STATUS stat_max[A_MAX]; /* Current "maximal" stat values */
    BASE_STATUS stat_max_max[A_MAX]; /* Maximal "maximal" stat values */
    BASE_STATUS stat_cur[A_MAX]; /* Current "natural" stat values */

    s16b learned_spells;
    s16b add_spells;

    u32b count;

    TIME_EFFECT fast; /* Timed -- Fast */
    TIME_EFFECT slow; /* Timed -- Slow */
    TIME_EFFECT blind; /* Timed -- Blindness */
    TIME_EFFECT paralyzed; /* Timed -- Paralysis */
    TIME_EFFECT confused; /* Timed -- Confusion */
    TIME_EFFECT afraid; /* Timed -- Fear */
    TIME_EFFECT image; /* Timed -- Hallucination */
    TIME_EFFECT poisoned; /* Timed -- Poisoned */
    TIME_EFFECT cut; /* Timed -- Cut */
    TIME_EFFECT stun; /* Timed -- Stun */

    TIME_EFFECT protevil; /* Timed -- Protection */
    TIME_EFFECT invuln; /* Timed -- Invulnerable */
    TIME_EFFECT ult_res; /* Timed -- Ultimate Resistance */
    TIME_EFFECT hero; /* Timed -- Heroism */
    TIME_EFFECT shero; /* Timed -- Super Heroism */
    TIME_EFFECT shield; /* Timed -- Shield Spell */
    TIME_EFFECT blessed; /* Timed -- Blessed */
    TIME_EFFECT tim_invis; /* Timed -- See Invisible */
    TIME_EFFECT tim_infra; /* Timed -- Infra Vision */
    TIME_EFFECT tsuyoshi; /* Timed -- Tsuyoshi Special */
    TIME_EFFECT ele_attack; /* Timed -- Elemental Attack */
    TIME_EFFECT ele_immune; /* Timed -- Elemental Immune */

    TIME_EFFECT oppose_acid; /* Timed -- oppose acid */
    TIME_EFFECT oppose_elec; /* Timed -- oppose lightning */
    TIME_EFFECT oppose_fire; /* Timed -- oppose heat */
    TIME_EFFECT oppose_cold; /* Timed -- oppose cold */
    TIME_EFFECT oppose_pois; /* Timed -- oppose poison */

    TIME_EFFECT tim_esp; /* Timed ESP */
    TIME_EFFECT wraith_form; /* Timed wraithform */

    TIME_EFFECT resist_magic; /* Timed Resist Magic (later) */
    TIME_EFFECT tim_regen;
    TIME_EFFECT tim_pass_wall;
    TIME_EFFECT tim_stealth;
    TIME_EFFECT tim_levitation;
    TIME_EFFECT tim_sh_touki;
    TIME_EFFECT lightspeed;
    TIME_EFFECT tsubureru;
    TIME_EFFECT magicdef;
    TIME_EFFECT tim_res_nether; /* Timed -- Nether resistance */
    TIME_EFFECT tim_res_time; /* Timed -- Time resistance */
    MIMIC_RACE_IDX mimic_form;
    TIME_EFFECT tim_mimic;
    TIME_EFFECT tim_sh_fire;
    TIME_EFFECT tim_sh_holy;
    TIME_EFFECT tim_eyeeye;

    /* for mirror master */
    TIME_EFFECT tim_reflect; /* Timed -- Reflect */
    TIME_EFFECT multishadow; /* Timed -- Multi-shadow */
    TIME_EFFECT dustrobe; /* Timed -- Robe of dust */

    bool timewalk;

#define COMMAND_ARG_REST_UNTIL_DONE -2 /*!<休憩コマンド引数 … 必要な分だけ回復 */
#define COMMAND_ARG_REST_FULL_HEALING -1 /*!<休憩コマンド引数 … HPとMPが全回復するまで */
    GAME_TURN resting; /* Current counter for resting, if any */

    PATRON_IDX chaos_patron;

    BIT_FLAGS muta1; /*!< レイシャル型の変異 / "Activatable" mutations must be in MUT1_* */
    BIT_FLAGS muta2; /*!< 常時効果つきの変異1 / Randomly activating mutations must be MUT2_* */
    BIT_FLAGS muta3; /*!< 常時効果つきの変異2 / Other mutations will be mainly in MUT3_* */

    s16b virtues[8];
    s16b vir_types[8];

    TIME_EFFECT word_recall; /* Word of recall counter */
    TIME_EFFECT alter_reality; /* Alter reality counter */
    DUNGEON_IDX recall_dungeon; /* Dungeon set to be recalled */

    ENERGY energy_need; /* Energy needed for next move */
    ENERGY enchant_energy_need; /* Energy needed for next upkeep effect	 */

    FEED food; /* Current nutrition */

    /*
     * p_ptr->special_attackによるプレイヤーの攻撃状態の定義 / Bit flags for the "p_ptr->special_attack" variable. -LM-
     *
     * Note:  The elemental and poison attacks should be managed using the
     * function "set_ele_attack", in spell2.c.  This provides for timeouts and
     * prevents the player from getting more than one at a time.
     */
    BIT_FLAGS special_attack;

    /* プレイヤーの防御状態の定義 / Bit flags for the "p_ptr->special_defense" variable. -LM- */
    BIT_FLAGS special_defense;
    ACTION_IDX action; /* Currently action */
    BIT_FLAGS spell_learned1; /* bit mask of spells learned */
    BIT_FLAGS spell_learned2; /* bit mask of spells learned */
    BIT_FLAGS spell_worked1; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_worked2; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_forgotten1; /* bit mask of spells learned but forgotten */
    BIT_FLAGS spell_forgotten2; /* bit mask of spells learned but forgotten */
    SPELL_IDX spell_order[64]; /* order spells learned/remembered/forgotten */

    SUB_EXP spell_exp[64]; /* Proficiency of spells */
    SUB_EXP weapon_exp[5][64]; /* Proficiency of weapons */
    SUB_EXP skill_exp[GINOU_MAX]; /* Proficiency of misc. skill */

    MAGIC_NUM1 magic_num1[MAX_SPELLS]; /*!< Array for non-spellbook type magic */
    MAGIC_NUM2 magic_num2[MAX_SPELLS]; /*!< 魔道具術師の取り込み済魔道具使用回数 / Flags for non-spellbook type magics */

    SPELL_IDX mane_spell[MAX_MANE];
    HIT_POINT mane_dam[MAX_MANE];
    s16b mane_num;
    bool new_mane;

#define CONCENT_RADAR_THRESHOLD 2
#define CONCENT_TELE_THRESHOLD 5
    s16b concent; /* Sniper's concentration level */

    HIT_POINT player_hp[PY_MAX_LEVEL];
    char died_from[80]; /* What killed the player */
    concptr last_message; /* Last message on death or retirement */
    char history[4][60]; /* Textual "history" for the Player */

    u16b panic_save; /* Panic save */

    bool wait_report_score; /* Waiting to report score */
    bool is_dead; /* Player is dead */
    bool now_damaged;
    bool ambush_flag;
    BIT_FLAGS change_floor_mode; /*!<フロア移行処理に関するフラグ / Mode flags for changing floor */

    bool reset_concent; /* Concentration reset flag */

    MONSTER_IDX riding; /* Riding on a monster of this index */

#define KNOW_STAT 0x01
#define KNOW_HPRATE 0x02
    BIT_FLAGS8 knowledge; /* Knowledge about yourself */
    BIT_FLAGS visit; /* Visited towns */

    player_race_type start_race; /* Race at birth */
    BIT_FLAGS old_race1; /* Record of race changes */
    BIT_FLAGS old_race2; /* Record of race changes */
    s16b old_realm; /* Record of realm changes */

    s16b pet_follow_distance; /* Length of the imaginary "leash" for pets */
    s16b pet_extra_flags; /* Various flags for controling pets */

    s16b today_mon; /* Wanted monster */

    bool dtrap; /* Whether you are on trap-safe grids */
    FLOOR_IDX floor_id; /* Current floor location */

    bool autopick_autoregister; /* auto register is in-use or not */

    byte feeling; /* Most recent dungeon feeling */
    s32b feeling_turn; /* The turn of the last dungeon feeling */

    object_type *inventory_list; /* The player's inventory */
    s16b inven_cnt; /* Number of items in inventory */
    s16b equip_cnt; /* Number of items in equipment */

    /*** Temporary fields ***/

    bool playing; /* True if player is playing */
    bool leaving; /* True if player is leaving */

    bool monk_notify_aux;

    byte leave_bldg;
    byte exit_bldg; /* Goal obtained in on_defeat_arena_monster? -KMW- */

    bool leaving_dungeon; /* True if player is leaving the dungeon */
    bool teleport_town;
    bool enter_dungeon; /* Just enter the dungeon */

    IDX health_who; /* Health bar trackee */

    MONRACE_IDX monster_race_idx; /* Monster race trackee */

    KIND_OBJECT_IDX object_kind_idx; /* Object kind trackee */

    s16b new_spells; /* Number of spells available */
    s16b old_spells;

    s16b old_food_aux; /* Old value of food */

    bool old_cumber_armor;
    bool old_cumber_glove;
    bool old_heavy_wield[2];
    bool old_heavy_shoot;
    bool old_icky_wield[2];
    bool old_riding_wield[2];
    bool old_riding_ryoute;
    bool old_monlite;
    int extra_blows[2];

    POSITION old_lite; /* Old radius of lite (if any) */

    bool cumber_armor; /* Mana draining armor */
    bool cumber_glove; /* Mana draining gloves */
    bool heavy_wield[2]; /* Heavy weapon */
    bool icky_wield[2]; /* Icky weapon */
    bool riding_wield[2]; /* Riding weapon */
    bool riding_ryoute; /* Riding weapon */
    bool monlite;
    bool yoiyami;
    bool easy_2weapon;
    bool down_saving;

    POSITION cur_lite; /* Radius of lite (if any) */

    BIT_FLAGS update; /* Pending Updates */
    BIT_FLAGS redraw; /* Normal Redraws */
    BIT_FLAGS window; /* Window Redraws */
    s16b stat_use[A_MAX]; /* Current modified stats */
    s16b stat_top[A_MAX]; /* Maximal modified stats */

    bool sutemi;
    bool counter;

    ALIGNMENT align; /* Good/evil/neutral */
    POSITION run_py;
    POSITION run_px;
    DIRECTION fishing_dir;

    MONSTER_IDX pet_t_m_idx;
    MONSTER_IDX riding_t_m_idx;

    /*** Extracted fields ***/

    s16b running; /* Current counter for running, if any */
    bool suppress_multi_reward; /*!< 複数レベルアップ時のパトロンからの報酬多重受け取りを防止 */

    WEIGHT total_weight; /*!< 所持品と装備品の計算総重量 / Total weight being carried */

    s16b stat_add[A_MAX]; /* Modifiers to stat values */
    s16b stat_ind[A_MAX]; /* Indexes into stat tables */

    bool hack_mutation;
    bool is_fired;
    bool level_up_message;

    bool immune_acid; /* Immunity to acid */
    bool immune_elec; /* Immunity to lightning */
    bool immune_fire; /* Immunity to fire */
    bool immune_cold; /* Immunity to cold */

    bool resist_acid; /* Resist acid */
    bool resist_elec; /* Resist lightning */
    bool resist_fire; /* Resist fire */
    bool resist_cold; /* Resist cold */
    bool resist_pois; /* Resist poison */

    bool resist_conf; /* Resist confusion */
    bool resist_sound; /* Resist sound */
    bool resist_lite; /* Resist light */
    bool resist_dark; /* Resist darkness */
    bool resist_chaos; /* Resist chaos */
    bool resist_disen; /* Resist disenchant */
    bool resist_shard; /* Resist shards */
    bool resist_nexus; /* Resist nexus */
    bool resist_blind; /* Resist blindness */
    bool resist_neth; /* Resist nether */
    bool resist_fear; /* Resist fear */
    bool resist_time; /* Resist time */
    bool resist_water; /* Resist water */

    bool reflect; /* Reflect 'bolt' attacks */
    bool sh_fire; /* Fiery 'immolation' effect */
    bool sh_elec; /* Electric 'immolation' effect */
    bool sh_cold; /* Cold 'immolation' effect */

    BIT_FLAGS anti_magic; /* Anti-magic */
    BIT_FLAGS anti_tele; /* Prevent teleportation */

    bool sustain_str; /* Keep strength */
    bool sustain_int; /* Keep intelligence */
    bool sustain_wis; /* Keep wisdom */
    bool sustain_dex; /* Keep dexterity */
    bool sustain_con; /* Keep constitution */
    bool sustain_chr; /* Keep charisma */

    BIT_FLAGS cursed; /* Player is cursed */

    bool can_swim; /* No damage falling */
    bool levitation; /* No damage falling */
    bool lite; /* Permanent light */
    bool free_act; /* Never paralyzed */
    bool see_inv; /* Can see invisible */
    bool regenerate; /* Regenerate hit pts */
    bool hold_exp; /* Resist exp draining */

    bool telepathy; /* Telepathy */
    BIT_FLAGS esp_animal;
    BIT_FLAGS esp_undead;
    BIT_FLAGS esp_demon;
    BIT_FLAGS esp_orc;
    BIT_FLAGS esp_troll;
    BIT_FLAGS esp_giant;
    BIT_FLAGS esp_dragon;
    BIT_FLAGS esp_human;
    BIT_FLAGS esp_evil;
    BIT_FLAGS esp_good;
    BIT_FLAGS esp_nonliving;
    BIT_FLAGS esp_unique;

    bool slow_digest; /* Slower digestion */
    BIT_FLAGS bless_blade; /* Blessed blade */
    BIT_FLAGS xtra_might; /* Extra might bow */
    bool impact[2]; /* Earthquake blows */
    bool pass_wall; /* Permanent wraithform */
    bool kill_wall;
    BIT_FLAGS dec_mana;
    BIT_FLAGS easy_spell;
    bool heavy_spell;
    bool warning;
    bool mighty_throw;
    bool see_nocto; /* Noctovision */
    bool invoking_midnight_curse;

    DICE_NUMBER to_dd[2]; /* Extra dice/sides */
    DICE_SID to_ds[2];

    HIT_PROB dis_to_h[2]; /*!< 判明している現在の表記上の近接武器命中修正値 /  Known bonus to hit (wield) */
    HIT_PROB dis_to_h_b; /*!< 判明している現在の表記上の射撃武器命中修正値 / Known bonus to hit (bow) */
    HIT_POINT dis_to_d[2]; /*!< 判明している現在の表記上の近接武器ダメージ修正値 / Known bonus to dam (wield) */
    ARMOUR_CLASS dis_to_a; /*!< 判明している現在の表記上の装備AC修正値 / Known bonus to ac */
    ARMOUR_CLASS dis_ac; /*!< 判明している現在の表記上の装備AC基礎値 / Known base ac */

    s16b to_h[2]; /* Bonus to hit (wield) */
    s16b to_h_b; /* Bonus to hit (bow) */
    s16b to_h_m; /* Bonus to hit (misc) */
    s16b to_d[2]; /* Bonus to dam (wield) */
    s16b to_d_m; /* Bonus to dam (misc) */
    ARMOUR_CLASS to_a; /* Bonus to ac */

    s16b to_m_chance; /* Minusses to cast chance */

    bool no_flowed;

    ARMOUR_CLASS ac; /*!< 装備無しの基本AC / Base ac */

    ACTION_SKILL_POWER see_infra; /*!< 赤外線視能力の強さ /Infravision range */
    ACTION_SKILL_POWER skill_dis; /*!< 行動技能値:解除能力 / Skill: Disarming */
    ACTION_SKILL_POWER skill_dev; /*!< 行動技能値:魔道具使用 / Skill: Magic Devices */
    ACTION_SKILL_POWER skill_sav; /*!< 行動技能値:魔法防御 / Skill: Saving throw */
    ACTION_SKILL_POWER skill_stl; /*!< 行動技能値:隠密 / Skill: Stealth factor */

    /*!
     * 行動技能値:知覚 / Skill: Searching ability
     * この値はsearch()による地形の隠し要素発見処理などで混乱、盲目、幻覚、無光源などの
     * 状態異常がない限り、難易度修正などがないままそのままパーセンテージ値として使われる。
     * 100以上ならば必ず全てのトラップなどを見つけることが出来る。
     */
    ACTION_SKILL_POWER skill_srh;

    ACTION_SKILL_POWER skill_fos; /*!< 行動技能値:探索 / Skill: Searching frequency */
    ACTION_SKILL_POWER skill_thn; /*!< 行動技能値:打撃命中能力 / Skill: To hit (normal) */
    ACTION_SKILL_POWER skill_thb; /*!< 行動技能値:射撃命中能力 / Skill: To hit (shooting) */
    ACTION_SKILL_POWER skill_tht; /*!< 行動技能値:投射命中能力 / Skill: To hit (throwing) */
    ACTION_SKILL_POWER skill_dig; /*!< 行動技能値:掘削 / Skill: Digging */

    s16b num_blow[2]; /* Number of blows */
    s16b num_fire; /* Number of shots */

    byte tval_xtra; /* (Unused)Correct xtra tval */
    byte tval_ammo; /* Correct ammo tval */

    s16b pspeed; /*!< 現在の速度 / Current speed */

    ENERGY energy_use; /*!< 直近のターンに消費したエネルギー / Energy use this turn */

    POSITION y; /*!< ダンジョンの現在Y座標 / Player location in dungeon */
    POSITION x; /*!< ダンジョンの現在X座標 / Player location in dungeon */
    GAME_TEXT name[32]; /*!< 現在のプレイヤー名 / Current player's character name */
    char base_name[32]; /*!< Stripped version of "player_name" */

} player_type;

extern player_type *p_ptr;

extern concptr your_alignment(player_type *creature_ptr);
extern int weapon_exp_level(int weapon_exp);
extern int riding_exp_level(int riding_exp);
extern int spell_exp_level(int spell_exp);

extern int calc_weapon_weight_limit(player_type *creature_ptr);

extern s16b calc_num_fire(player_type *creature_ptr, object_type *o_ptr);
extern void calc_bonuses(player_type *creature_ptr);
extern WEIGHT weight_limit(player_type *creature_ptr);
extern bool has_melee_weapon(player_type *creature_ptr, int i);

extern bool heavy_armor(player_type *creature_ptr);
extern void update_creature(player_type *creature_ptr);
extern BIT_FLAGS16 empty_hands(player_type *creature_ptr, bool riding_control);
extern bool player_has_no_spellbooks(player_type *creature_ptr);

extern void take_turn(player_type *creature_ptr, PERCENTAGE need_cost);
extern void free_turn(player_type *creature_ptr);

extern bool player_place(player_type *creature_ptr, POSITION y, POSITION x);

extern void check_experience(player_type *creature_ptr);
extern void wreck_the_pattern(player_type *creature_ptr);
extern void cnv_stat(int val, char *out_val);
extern s16b modify_stat_value(int value, int amount);
extern long calc_score(player_type *creature_ptr);

extern bool is_blessed(player_type *creature_ptr);
extern bool is_time_limit_esp(player_type *creature_ptr);
extern bool is_time_limit_stealth(player_type *creature_ptr);
extern bool can_two_hands_wielding(player_type *creature_ptr);
bool is_fast(player_type *creature_ptr);
bool is_invuln(player_type *creature_ptr);
bool is_hero(player_type *creature_ptr);
bool is_echizen(player_type *creature_ptr);

/*
 * Player "food" crucial values
 */
#define PY_FOOD_MAX 15000 /*!< 食べ過ぎ～満腹の閾値 / Food value (Bloated) */
#define PY_FOOD_FULL 10000 /*!< 満腹～平常の閾値 / Food value (Normal) */
#define PY_FOOD_ALERT 2000 /*!< 平常～空腹の閾値 / Food value (Hungry) */
#define PY_FOOD_WEAK 1000 /*!< 空腹～衰弱の閾値 / Food value (Weak) */
#define PY_FOOD_FAINT 500 /*!< 衰弱～衰弱(赤表示/麻痺)の閾値 / Food value (Fainting) */
#define PY_FOOD_STARVE 100 /*!< 衰弱(赤表示/麻痺)～飢餓ダメージの閾値 / Food value (Starving) */

/*
 * Player regeneration constants
 */
#define PY_REGEN_NORMAL 197 /* Regen factor*2^16 when full */
#define PY_REGEN_WEAK 98 /* Regen factor*2^16 when weak */
#define PY_REGEN_FAINT 33 /* Regen factor*2^16 when fainting */
#define PY_REGEN_HPBASE 1442 /* Min amount hp regen*2^16 */
#define PY_REGEN_MNBASE 524 /* Min amount mana regen*2^16 */

extern void cheat_death(player_type *creature_ptr);

extern void stop_singing(player_type *creature_ptr);
extern void stop_mouth(player_type *caster_ptr);
extern PERCENTAGE calculate_upkeep(player_type *creature_ptr);
extern bool music_singing(player_type *caster_ptr, int music_songs);

#define SINGING_SONG_EFFECT(P_PTR) ((P_PTR)->magic_num1[0])
#define INTERUPTING_SONG_EFFECT(P_PTR) ((P_PTR)->magic_num1[1])
#define SINGING_COUNT(P_PTR) ((P_PTR)->magic_num1[2])
#define SINGING_SONG_ID(P_PTR) ((P_PTR)->magic_num2[0])
#define music_singing_any(CREATURE_PTR) (((CREATURE_PTR)->pclass == CLASS_BARD) && (CREATURE_PTR)->magic_num1[0])
