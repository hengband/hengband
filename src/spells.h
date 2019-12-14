#pragma once
#include "realm.h"

#define DETECT_RAD_DEFAULT 30
#define DETECT_RAD_MAP     30
#define DETECT_RAD_ALL     255

#define SPOP_DISPLAY_MES    0x0001 // !< スペル処理オプション … メッセージを表示する
#define SPOP_NO_UPDATE      0x0002 // !< スペル処理オプション … ステータス更新を解決後行う
#define SPOP_DEBUG          0x8000 // !< スペル処理オプション … デバッグ処理あり

/*
 * Spell types used by project(), and related functions.
 */
#define GF_ELEC         1			/*!< 魔法効果: 電撃*/
#define GF_POIS         2			/*!< 魔法効果: 毒*/
#define GF_ACID         3			/*!< 魔法効果: 酸*/
#define GF_COLD         4			/*!< 魔法効果: 冷気*/
#define GF_FIRE         5			/*!< 魔法効果: 火炎*/
#define GF_PSY_SPEAR    9			/*!< 魔法効果: 光の剣*/
#define GF_MISSILE      10			/*!< 魔法効果: 弱魔力*/
#define GF_ARROW        11			/*!< 魔法効果: 射撃*/
#define GF_PLASMA       12			/*!< 魔法効果: プラズマ*/
 /* Replaced with GF_HOLY_FIRE and GF_HELL_FIRE */
 /* #define GF_HOLY_ORB     13 */
#define GF_WATER        14			/*!< 魔法効果: 水流*/
#define GF_LITE         15			/*!< 魔法効果: 閃光*/
#define GF_DARK         16			/*!< 魔法効果: 暗黒*/
#define GF_LITE_WEAK    17			/*!< 魔法効果: 弱光*/
#define GF_DARK_WEAK    18			/*!< 魔法効果: 弱暗*/
#define GF_SHARDS       20			/*!< 魔法効果: 破片*/
#define GF_SOUND        21			/*!< 魔法効果: 轟音*/
#define GF_CONFUSION    22			/*!< 魔法効果: 混乱*/
#define GF_FORCE        23			/*!< 魔法効果: フォース*/
#define GF_INERTIAL     24			/*!< 魔法効果: 遅鈍*/
#define GF_MANA         26			/*!< 魔法効果: 純粋魔力*/
#define GF_METEOR       27			/*!< 魔法効果: 隕石*/
#define GF_ICE          28			/*!< 魔法効果: 極寒*/
#define GF_CHAOS        30			/*!< 魔法効果: カオス*/
#define GF_NETHER       31			/*!< 魔法効果: 地獄*/
#define GF_DISENCHANT   32			/*!< 魔法効果: 劣化*/
#define GF_NEXUS        33			/*!< 魔法効果: 因果混乱*/
#define GF_TIME         34			/*!< 魔法効果: 時間逆転*/
#define GF_GRAVITY      35			/*!< 魔法効果: 重力*/
#define GF_KILL_WALL    40			/*!< 魔法効果: 岩石溶解*/
#define GF_KILL_DOOR    41			/*!< 魔法効果: ドア破壊*/
#define GF_KILL_TRAP    42			/*!< 魔法効果: トラップ破壊*/
#define GF_MAKE_WALL    45			/*!< 魔法効果: 壁生成*/
#define GF_MAKE_DOOR    46			/*!< 魔法効果: ドア生成*/
#define GF_MAKE_TRAP    47			/*!< 魔法効果: トラップ生成*/
#define GF_MAKE_TREE    48			/*!< 魔法効果: 森林生成*/
#define GF_OLD_CLONE    51			/*!< 魔法効果: クローン・モンスター*/
#define GF_OLD_POLY     52			/*!< 魔法効果: チェンジ・モンスター*/
#define GF_OLD_HEAL     53			/*!< 魔法効果: 回復モンスター*/
#define GF_OLD_SPEED    54			/*!< 魔法効果: スピード・モンスター*/
#define GF_OLD_SLOW     55			/*!< 魔法効果: スロウ・モンスター*/
#define GF_OLD_CONF     56			/*!< 魔法効果: パニック・モンスター*/
#define GF_OLD_SLEEP    57			/*!< 魔法効果: スリープ・モンスター*/
#define GF_HYPODYNAMIA  58			/*!< 魔法効果: 衰弱*/
#define GF_AWAY_UNDEAD  61			/*!< 魔法効果: アンデッド・アウェイ*/
#define GF_AWAY_EVIL    62			/*!< 魔法効果: 邪悪飛ばし*/
#define GF_AWAY_ALL     63			/*!< 魔法効果: テレポート・アウェイ*/
#define GF_TURN_UNDEAD  64			/*!< 魔法効果: アンデッド恐慌*/
#define GF_TURN_EVIL    65			/*!< 魔法効果: 邪悪恐慌*/
#define GF_TURN_ALL     66			/*!< 魔法効果: モンスター恐慌*/
#define GF_DISP_UNDEAD  67			/*!< 魔法効果: アンデッド退散*/
#define GF_DISP_EVIL    68			/*!< 魔法効果: 邪悪退散*/
#define GF_DISP_ALL     69			/*!< 魔法効果: モンスター退散*/
/* New types for Zangband begin here... */
#define GF_DISP_DEMON      70		/*!< 魔法効果: 悪魔退散*/
#define GF_DISP_LIVING     71		/*!< 魔法効果: 生命退散*/
#define GF_ROCKET          72		/*!< 魔法効果: ロケット*/
#define GF_NUKE            73		/*!< 魔法効果: 放射性廃棄物*/
#define GF_MAKE_GLYPH      74		/*!< 魔法効果: 守りのルーン生成*/
#define GF_STASIS          75		/*!< 魔法効果: モンスター拘束*/
#define GF_STONE_WALL      76		/*!< 魔法効果: 壁生成*/
#define GF_DEATH_RAY       77		/*!< 魔法効果: 死の光線*/
#define GF_STUN            78		/*!< 魔法効果: 朦朧*/
#define GF_HOLY_FIRE       79		/*!< 魔法効果: 聖光*/
#define GF_HELL_FIRE       80		/*!< 魔法効果: 地獄の劫火*/
#define GF_DISINTEGRATE    81		/*!< 魔法効果: 分解*/
#define GF_CHARM           82		/*!< 魔法効果: モンスター魅了*/
#define GF_CONTROL_UNDEAD  83		/*!< 魔法効果: アンデッド支配*/
#define GF_CONTROL_ANIMAL  84		/*!< 魔法効果: 動物支配*/
#define GF_PSI             85		/*!< 魔法効果: サイキック攻撃*/
#define GF_PSI_DRAIN       86		/*!< 魔法効果: 精神吸収*/
#define GF_TELEKINESIS     87		/*!< 魔法効果: テレキシネス*/
#define GF_JAM_DOOR        88		/*!< 魔法効果: 施錠*/
#define GF_DOMINATION      89		/*!< 魔法効果: 精神支配*/
#define GF_DISP_GOOD       90		/*!< 魔法効果: 善良退散*/
#define GF_DRAIN_MANA      91		/*!< 魔法効果: 魔力吸収*/
#define GF_MIND_BLAST      92		/*!< 魔法効果: 精神攻撃*/
#define GF_BRAIN_SMASH     93		/*!< 魔法効果: 脳攻撃*/
#define GF_CAUSE_1         94		/*!< 魔法効果: 軽傷の呪い*/
#define GF_CAUSE_2         95		/*!< 魔法効果: 重傷の呪い*/
#define GF_CAUSE_3         96		/*!< 魔法効果: 致命傷の呪い*/
#define GF_CAUSE_4         97		/*!< 魔法効果: 秘孔を突く*/
#define GF_HAND_DOOM       98		/*!< 魔法効果: 破滅の手*/
#define GF_CAPTURE         99		/*!< 魔法効果: 捕縛*/
#define GF_ANIM_DEAD      100		/*!< 魔法効果: 死者復活*/
#define GF_CHARM_LIVING   101		/*!< 魔法効果: 生命魅了*/
#define GF_IDENTIFY       102		/*!< 魔法効果: 鑑定*/
#define GF_ATTACK         103		/*!< 魔法効果: 白兵*/
#define GF_ENGETSU        104		/*!< 魔法効果: 円月*/
#define GF_GENOCIDE       105		/*!< 魔法効果: 抹殺*/
#define GF_PHOTO          106		/*!< 魔法効果: 撮影*/
#define GF_CONTROL_DEMON  107		/*!< 魔法効果: 悪魔支配*/
#define GF_LAVA_FLOW      108		/*!< 魔法効果: 溶岩噴出*/
#define GF_BLOOD_CURSE    109		/*!< 魔法効果: 血の呪い*/
#define GF_SEEKER         110		/*!< 魔法効果: シーカーレイ*/
#define GF_SUPER_RAY      111		/*!< 魔法効果: スーパーレイ*/
#define GF_STAR_HEAL      112		/*!< 魔法効果: 星の癒し*/
#define GF_WATER_FLOW     113		/*!< 魔法効果: 流水*/
#define GF_CRUSADE        114		/*!< 魔法効果: 聖戦*/
#define GF_STASIS_EVIL    115		/*!< 魔法効果: 邪悪拘束*/
#define GF_WOUNDS         116		/*!< 魔法効果: 創傷*/

#define MAX_GF         117

/*
 * The "name" of spell 'N' is stored as spell_names[X][N],
 * where X is 0 for mage-spells and 1 for priest-spells.
 */
typedef struct magic_type magic_type;

struct magic_type
{
	PLAYER_LEVEL slevel;	/* Required level (to learn) */
	MANA_POINT smana;		/* Required mana (to cast) */
	PERCENTAGE sfail;		/* Minimum chance of failure */
	EXP sexp;				/* Encoded experience bonus */
};

extern int cap_mon;
extern int cap_mspeed;
extern HIT_POINT cap_hp;
extern HIT_POINT cap_maxhp;
extern STR_OFFSET cap_nickname;
extern bool sukekaku;

extern const magic_type technic_info[NUM_TECHNIC][32];

/* spells1.c */
extern PERCENTAGE beam_chance(void);
extern bool in_disintegration_range(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern void breath_shape(u16b *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ);
extern POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);

#define PROJECT_WHO_UNCTRL_POWER -1 /*!< 魔法効果の自然発生要因: 名状し難い力の解放 */
#define PROJECT_WHO_GLASS_SHARDS -2 /*!< 魔法効果の自然発生要因: 破壊されたガラス地形の破片 */
extern bool project(player_type *caster_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flg, int monspell);

extern int project_length;
extern bool binding_field(player_type *caster_ptr, HIT_POINT dam);
extern void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam);
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
extern bool genocide_aux(player_type *caster_ptr, MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, concptr spell_name);
extern bool symbol_genocide(player_type *caster_ptr, int power, bool player_cast);
extern bool mass_genocide(player_type *caster_ptr, int power, bool player_cast);
extern bool mass_genocide_undead(player_type *caster_ptr, int power, bool player_cast);
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
extern void lite_room(POSITION y1, POSITION x1);
extern bool starlight(bool magic);
extern void unlite_room(POSITION y1, POSITION x1);
extern bool lite_area(HIT_POINT dam, POSITION rad);
extern bool unlite_area(HIT_POINT dam, POSITION rad);
extern bool fire_ball(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_breath(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_rocket(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_ball_hide(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
extern bool fire_meteor(MONSTER_IDX who, EFFECT_ID typ, POSITION x, POSITION y, HIT_POINT dam, POSITION rad);
extern bool fire_bolt(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
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
extern bool teleport_swap(DIRECTION dir);
extern bool project_hook(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, BIT_FLAGS flg);
extern bool project_all_los(EFFECT_ID typ, HIT_POINT dam);
extern bool eat_magic(player_type *caster_ptr, int power);
extern void discharge_minion(void);
extern bool kawarimi(bool success);
extern bool rush_attack(player_type *attacker_ptr, bool *mdeath);
extern void remove_all_mirrors(bool explode);
extern void ring_of_power(player_type *caster_ptr, DIRECTION dir);
extern void wild_magic(player_type *caster_ptr, int spell);
extern void cast_meteor(HIT_POINT dam, POSITION rad);
extern bool cast_wrath_of_the_god(player_type *caster_ptr, HIT_POINT dam, POSITION rad);
extern void cast_wonder(player_type *caster_ptr, DIRECTION dir);
extern void cast_invoke_spirits(player_type *caster_ptr, DIRECTION dir);
extern void cast_shuffle(player_type *caster_ptr);
extern void stop_mouth(void);
extern bool_hack vampirism(player_type *caster_ptr);
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
extern bool vanish_dungeon(player_type *caster_ptr);

/*
 * Bit flags for teleportation
 */
#define TELEPORT_NONMAGICAL 0x00000001
#define TELEPORT_PASSIVE    0x00000002
#define TELEPORT_DEC_VALOUR 0x00000004

/* spells3.c */
extern bool teleport_away(player_type *caster_ptr, MONSTER_IDX m_idx, POSITION dis, BIT_FLAGS mode);
extern void teleport_monster_to(MONSTER_IDX m_idx, POSITION ty, POSITION tx, int power, BIT_FLAGS mode);
extern bool teleport_player_aux(player_type *creature_ptr, POSITION dis, BIT_FLAGS mode);
extern void teleport_player(player_type *creature_ptr, POSITION dis, BIT_FLAGS mode);
extern void teleport_player_away(MONSTER_IDX m_idx, POSITION dis);
extern void teleport_player_to(player_type *creature_ptr, POSITION ny, POSITION nx, BIT_FLAGS mode);
extern void teleport_away_followable(MONSTER_IDX m_idx);
extern bool teleport_level_other(player_type *caster_ptr);
extern void teleport_level(player_type *creature_ptr, MONSTER_IDX m_idx);
extern bool recall_player(player_type *creature_ptr, TIME_EFFECT turns);
extern bool free_level_recall(player_type *creature_ptr);
extern bool reset_recall(void);
extern bool apply_disenchant(player_type *target_ptr, BIT_FLAGS mode);
extern void call_the_void(player_type *caster_ptr);
extern void fetch(player_type *caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los);
extern void reserve_alter_reality(player_type *caster_ptr);
extern void identify_pack(void);
extern int remove_curse(player_type *caster_ptr);
extern int remove_all_curse(player_type *caster_ptr);
extern bool alchemy(void);

extern bool artifact_scroll(void);
extern bool ident_spell(player_type *caster_ptr, bool only_equip);
extern bool mundane_spell(bool only_equip);
extern bool identify_item(player_type *owner_ptr, object_type *o_ptr);
extern bool identify_fully(bool only_equip);
extern bool recharge(int power);
extern void display_spell_list(player_type *caster_ptr);
extern EXP experience_of_spell(player_type *caster_ptr, SPELL_IDX spell, REALM_IDX use_realm);
extern MANA_POINT mod_need_mana(MANA_POINT need_mana, SPELL_IDX spell, REALM_IDX realm);
extern PERCENTAGE mod_spell_chance_1(player_type *caster_ptr, PERCENTAGE chance);
extern PERCENTAGE mod_spell_chance_2(player_type *caster_ptr, PERCENTAGE chance);
extern PERCENTAGE spell_chance(player_type *caster_ptr, SPELL_IDX spell, REALM_IDX realm);
extern void print_spells(player_type* caster_ptr, SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, REALM_IDX realm);
extern bool polymorph_monster(player_type *caster_ptr, POSITION y, POSITION x);
extern bool dimension_door(void);
extern bool mirror_tunnel(void);
extern void massacre(player_type *caster_ptr);
extern bool eat_lock(player_type *caster_ptr);
extern bool shock_power(player_type *caster_ptr);
extern bool booze(player_type *creature_ptr);
extern bool detonation(player_type *creature_ptr);
extern void blood_curse_to_enemy(player_type *caster_ptr, MONSTER_IDX m_idx);
extern bool fire_crimson(player_type *shooter_ptr);
extern bool tele_town(player_type *caster_ptr);
extern int project_length;

/* Is "teleport level" ineffective to this target? */
#define TELE_LEVEL_IS_INEFF(TARGET) \
	(p_ptr->current_floor_ptr->inside_arena || p_ptr->phase_out || \
	 (p_ptr->current_floor_ptr->inside_quest && !random_quest_number(p_ptr->current_floor_ptr->dun_level)) || \
	 (((TARGET) <= 0) && (quest_number(p_ptr->current_floor_ptr->dun_level) || (p_ptr->current_floor_ptr->dun_level >= d_info[p_ptr->dungeon_idx].maxdepth)) && \
	  (p_ptr->current_floor_ptr->dun_level >= 1) && ironman_downward))

