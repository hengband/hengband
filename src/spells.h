#pragma once

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

extern const magic_type technic_info[NUM_TECHNIC][32];

/* spells3.c */
extern bool teleport_away(MONSTER_IDX m_idx, POSITION dis, BIT_FLAGS mode);
extern void teleport_monster_to(MONSTER_IDX m_idx, POSITION ty, POSITION tx, int power, BIT_FLAGS mode);
extern bool teleport_player_aux(POSITION dis, BIT_FLAGS mode);
extern void teleport_player(POSITION dis, BIT_FLAGS mode);
extern void teleport_player_away(MONSTER_IDX m_idx, POSITION dis);
extern void teleport_player_to(POSITION ny, POSITION nx, BIT_FLAGS mode);
extern void teleport_away_followable(MONSTER_IDX m_idx);
extern bool teleport_level_other(player_type *creature_ptr);
extern void teleport_level(MONSTER_IDX m_idx);
extern DUNGEON_IDX choose_dungeon(concptr note, POSITION y, POSITION x);
extern bool recall_player(player_type *creature_ptr, TIME_EFFECT turns);
extern bool free_level_recall(player_type *creature_ptr);
extern bool reset_recall(void);
extern bool apply_disenchant(BIT_FLAGS mode);
extern void brand_weapon(int brand_type);
extern void call_the_(void);
extern void fetch(DIRECTION dir, WEIGHT wgt, bool require_los);
extern void alter_reality(void);
extern void identify_pack(void);
extern int remove_curse(void);
extern int remove_all_curse(void);
extern bool alchemy(void);
extern bool enchant(object_type *o_ptr, int n, int eflag);
extern bool enchant_spell(HIT_PROB num_hit, HIT_POINT num_dam, ARMOUR_CLASS num_ac);
extern bool artifact_scroll(void);
extern bool ident_spell(bool only_equip);
extern bool mundane_spell(bool only_equip);
extern bool identify_item(object_type *o_ptr);
extern bool identify_fully(bool only_equip);
extern bool recharge(int power);
extern void display_spell_list(void);
extern EXP experience_of_spell(SPELL_IDX spell, REALM_IDX use_realm);
extern MANA_POINT mod_need_mana(MANA_POINT need_mana, SPELL_IDX spell, REALM_IDX realm);
extern PERCENTAGE mod_spell_chance_1(PERCENTAGE chance);
extern PERCENTAGE mod_spell_chance_2(PERCENTAGE chance);
extern PERCENTAGE spell_chance(SPELL_IDX spell, REALM_IDX realm);
extern void print_spells(SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, REALM_IDX realm);
extern bool polymorph_monster(POSITION y, POSITION x);
extern bool dimension_door(void);
extern bool mirror_tunnel(void);
extern void massacre(void);
extern bool eat_lock(void);
extern bool shock_power(void);
extern bool booze(player_type *creature_ptr);
extern bool detonation(player_type *creature_ptr);
extern void blood_curse_to_enemy(MONSTER_IDX m_idx);
extern bool fire_crimson(void);
extern bool tele_town(void);
