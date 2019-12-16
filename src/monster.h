#pragma once
#include "monsterrace.h"

typedef bool(*monsterrace_hook_type)(MONRACE_IDX r_idx);

extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;

/* Maximum "Nazguls" number */
#define MAX_NAZGUL_NUM 5

#define MTIMED_CSLEEP   0 /* Monster is sleeping */
#define MTIMED_FAST     1 /* Monster is temporarily fast */
#define MTIMED_SLOW     2 /* Monster is temporarily slow */
#define MTIMED_STUNNED  3 /* Monster is stunned */
#define MTIMED_CONFUSED 4 /* Monster is confused */
#define MTIMED_MONFEAR  5 /* Monster is afraid */
#define MTIMED_INVULNER 6 /* Monster is temporarily invulnerable */

#define MAX_MTIMED      7

#define MON_CSLEEP(M_PTR)   ((M_PTR)->mtimed[MTIMED_CSLEEP])
#define MON_FAST(M_PTR)     ((M_PTR)->mtimed[MTIMED_FAST])
#define MON_SLOW(M_PTR)     ((M_PTR)->mtimed[MTIMED_SLOW])
#define MON_STUNNED(M_PTR)  ((M_PTR)->mtimed[MTIMED_STUNNED])
#define MON_CONFUSED(M_PTR) ((M_PTR)->mtimed[MTIMED_CONFUSED])
#define MON_MONFEAR(M_PTR)  ((M_PTR)->mtimed[MTIMED_MONFEAR])
#define MON_INVULNER(M_PTR) ((M_PTR)->mtimed[MTIMED_INVULNER])

/*
 * Monster information, for a specific monster.
 * Note: fy, fx constrain dungeon size to 256x256
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */

typedef struct monster_type monster_type;


struct monster_type
{
	MONRACE_IDX r_idx;		/* Monster race index 0 = dead. */
	MONRACE_IDX ap_r_idx;	/* Monster race appearance index */
	struct floor_type *current_floor_ptr;

	/* Sub-alignment flags for neutral monsters */
	#define SUB_ALIGN_NEUTRAL 0x0000
	#define SUB_ALIGN_EVIL    0x0001
	#define SUB_ALIGN_GOOD    0x0002
	BIT_FLAGS8 sub_align;		/* Sub-alignment for a neutral monster */

	POSITION fy;		/* Y location on map */
	POSITION fx;		/* X location on map */

	HIT_POINT hp;		/* Current Hit points */
	HIT_POINT maxhp;		/* Max Hit points */
	HIT_POINT max_maxhp;		/* Max Max Hit points */
	HIT_POINT dealt_damage;		/* Sum of damages dealt by player */

	TIME_EFFECT mtimed[MAX_MTIMED];	/* Timed status counter */

	SPEED mspeed;	        /* Monster "speed" */
	ACTION_ENERGY energy_need;	/* Monster "energy" */

	POSITION cdis;		/* Current dis from player */

	BIT_FLAGS8 mflag;	/* Extra monster flags */
#define MFLAG_VIEW      0x01    /* Monster is in line of sight */
#define MFLAG_LOS       0x02    /* Monster is marked for project_all_los(caster_ptr, ) */
#define MFLAG_XXX2      0x04    /* (unused) */
#define MFLAG_ETF       0x08    /* Monster is entering the field. */
#define MFLAG_BORN      0x10    /* Monster is still being born */
#define MFLAG_NICE      0x20    /* Monster is still being nice */

	BIT_FLAGS8 mflag2;	/* Extra monster flags */
#define MFLAG2_KAGE      0x01    /* Monster is kage */
#define MFLAG2_NOPET     0x02    /* Cannot make monster pet */
#define MFLAG2_NOGENO    0x04    /* Cannot genocide */
#define MFLAG2_CHAMELEON 0x08    /* Monster is chameleon */
#define MFLAG2_NOFLOW    0x10    /* Monster is in no_flow_by_smell mode */
#define MFLAG2_SHOW      0x20    /* Monster is recently memorized */
#define MFLAG2_MARK      0x40    /* Monster is currently memorized */

	bool ml;		/* Monster is "visible" */

	OBJECT_IDX hold_o_idx;	/* Object being held (if any) */

	POSITION target_y;		/* Can attack !los player */
	POSITION target_x;		/* Can attack !los player */

	STR_OFFSET nickname;		/* Monster's Nickname */

	EXP exp;

	/* TODO: クローン、ペット、有効化は意義が異なるので別変数に切り離すこと。save/loadのバージョン更新が面倒そうだけど */
	BIT_FLAGS smart; /*!< Field for "smart_learn" - Some bit-flags for the "smart" field */
#define SM_RES_ACID             0x00000001 /*!< モンスターの学習フラグ: プレイヤーに酸耐性あり */
#define SM_RES_ELEC             0x00000002 /*!< モンスターの学習フラグ: プレイヤーに電撃耐性あり */
#define SM_RES_FIRE             0x00000004 /*!< モンスターの学習フラグ: プレイヤーに火炎耐性あり */
#define SM_RES_COLD             0x00000008 /*!< モンスターの学習フラグ: プレイヤーに冷気耐性あり */
#define SM_RES_POIS             0x00000010 /*!< モンスターの学習フラグ: プレイヤーに毒耐性あり */
#define SM_RES_NETH             0x00000020 /*!< モンスターの学習フラグ: プレイヤーに地獄耐性あり */
#define SM_RES_LITE             0x00000040 /*!< モンスターの学習フラグ: プレイヤーに閃光耐性あり */
#define SM_RES_DARK             0x00000080 /*!< モンスターの学習フラグ: プレイヤーに暗黒耐性あり */
#define SM_RES_FEAR             0x00000100 /*!< モンスターの学習フラグ: プレイヤーに恐怖耐性あり */
#define SM_RES_CONF             0x00000200 /*!< モンスターの学習フラグ: プレイヤーに混乱耐性あり */
#define SM_RES_CHAOS            0x00000400 /*!< モンスターの学習フラグ: プレイヤーにカオス耐性あり */
#define SM_RES_DISEN            0x00000800 /*!< モンスターの学習フラグ: プレイヤーに劣化耐性あり */
#define SM_RES_BLIND            0x00001000 /*!< モンスターの学習フラグ: プレイヤーに盲目耐性あり */
#define SM_RES_NEXUS            0x00002000 /*!< モンスターの学習フラグ: プレイヤーに因果混乱耐性あり */
#define SM_RES_SOUND            0x00004000 /*!< モンスターの学習フラグ: プレイヤーに轟音耐性あり */
#define SM_RES_SHARD            0x00008000 /*!< モンスターの学習フラグ: プレイヤーに破片耐性あり */
#define SM_OPP_ACID             0x00010000 /*!< モンスターの学習フラグ: プレイヤーに二重酸耐性あり */
#define SM_OPP_ELEC             0x00020000 /*!< モンスターの学習フラグ: プレイヤーに二重電撃耐性あり */
#define SM_OPP_FIRE             0x00040000 /*!< モンスターの学習フラグ: プレイヤーに二重火炎耐性あり */
#define SM_OPP_COLD             0x00080000 /*!< モンスターの学習フラグ: プレイヤーに二重冷気耐性あり */
#define SM_OPP_POIS             0x00100000 /*!< モンスターの学習フラグ: プレイヤーに二重毒耐性あり */
#define SM_OPP_XXX1             0x00200000 /*!< 未使用 / (unused) */
#define SM_CLONED               0x00400000 /*!< クローンである / Cloned */
#define SM_PET                  0x00800000 /*!< ペットである / Pet */
#define SM_IMM_ACID             0x01000000 /*!< モンスターの学習フラグ: プレイヤーに酸免疫あり */
#define SM_IMM_ELEC             0x02000000 /*!< モンスターの学習フラグ: プレイヤーに電撃免疫あり */
#define SM_IMM_FIRE             0x04000000 /*!< モンスターの学習フラグ: プレイヤーに火炎免疫あり */
#define SM_IMM_COLD             0x08000000 /*!< モンスターの学習フラグ: プレイヤーに冷気免疫あり */
#define SM_FRIENDLY             0x10000000 /*!< 友好的である / Friendly */
#define SM_IMM_REFLECT          0x20000000 /*!< モンスターの学習フラグ: プレイヤーに反射あり */
#define SM_IMM_FREE             0x40000000 /*!< モンスターの学習フラグ: プレイヤーに麻痺耐性あり */
#define SM_IMM_MANA             0x80000000 /*!< モンスターの学習フラグ: プレイヤーにMPがない */

	MONSTER_IDX parent_m_idx;
};

#define MON_BEGGAR        12
#define MON_LEPER         13
#define MON_BLACK_MARKET  14
#define MON_LION_HEART    19
#define MON_GHB           39
#define MON_NOV_PRIEST    45
#define MON_GRIP          53
#define MON_WOLF          54
#define MON_FANG          55
#define MON_LOUSE         69
#define MON_PIRANHA       70
#define MON_COPPER_COINS  85
#define MON_NOV_PALADIN   97
#define MON_GREEN_G       100
#define MON_NOV_PRIEST_G  109
#define MON_SILVER_COINS  117
#define MON_D_ELF         122
#define MON_MANES         128
#define MON_LOST_SOUL     133
#define MON_ROBIN_HOOD    138
#define MON_NOV_PALADIN_G 147
#define MON_PHANTOM_W     152
#define MON_WOUNDED_BEAR  159
#define MON_D_ELF_MAGE    178
#define MON_D_ELF_WARRIOR 182
#define MON_BLUE_HORROR   189
#define MON_GOLD_COINS    195
#define MON_VORPAL_BUNNY  205
#define MON_MASTER_YEEK   224
#define MON_PRIEST        225
#define MON_D_ELF_PRIEST  226
#define MON_TIGER         230
#define MON_MITHRIL_COINS 239
#define MON_DRUID         241
#define MON_PINK_HORROR   242
#define MON_HILL_GIANT    255
#define MON_WERERAT       270
#define MON_UMBER_HULK    283
#define MON_ORC_CAPTAIN   285
#define MON_BERSERKER     293
#define MON_IMP           296
#define MON_SHAGRAT       314
#define MON_GORBAG        315
#define MON_STONE_GIANT   321
#define MON_LIZARD_KING   332
#define MON_WYVERN        334
#define MON_SABRE_TIGER   339
#define MON_D_ELF_LORD    348
#define MON_FIRE_VOR      354
#define MON_WATER_VOR     355
#define MON_ARCH_VILE     357
#define MON_COLD_VOR      358
#define MON_ENERGY_VOR    359
#define MON_IRON_GOLEM    367
#define MON_JADE_MONK     370
#define MON_D_ELF_WARLOCK 375
#define MON_HAGEN         383
#define MON_MENELDOR      384
#define MON_PHANTOM_B     385
#define MON_C_CRAWLER     395
#define MON_XICLOTLAN     396
#define MON_D_ELF_DRUID   400
#define MON_TROLL_PRIEST  403
#define MON_GWAIHIR       410
#define MON_ANGEL         417
#define MON_ADAMANT_COINS 423
#define MON_COLBRAN       435
#define MON_SPIRIT_NAGA   436
#define MON_GACHAPIN      441
#define MON_BASILISK      453
#define MON_ARCHANGEL     456
#define MON_MITHRIL_GOLEM 464
#define MON_THORONDOR     468
#define MON_SHADOW_DRAKE  471
#define MON_GHOST         477
#define MON_OGRE_SHAMAN   479
#define MON_GHOUL_KING    483
#define MON_NINJA         485
#define MON_BICLOPS       490
#define MON_IVORY_MONK    492
#define MON_LOG_MASTER    498
#define MON_ETHER_DRAKE   504
#define MON_GOEMON        505
#define MON_CHERUB        511
#define MON_WATER_ELEM    512
#define MON_JURT          517
#define MON_LICH          518
#define MON_BLOODLETTER   523
#define MON_HALFLING_S    539
#define MON_GRAV_HOUND    540
#define MON_REVENANT      555
#define MON_RAAL          557
#define MON_COLOSSUS      558
#define MON_NIGHTBLADE    564
#define MON_ELDER_THING   569
#define MON_CRYPT_THING   577
#define MON_NEXUS_VOR     587
#define MON_PLASMA_VOR    588
#define MON_TIME_VOR      589
#define MON_M_MH_DRAGON   593
#define MON_MANDOR        598
#define MON_SHIM_VOR      600
#define MON_SERAPH        605
#define MON_BARON_HELL    609
#define MON_KAVLAX        616
#define MON_ETTIN         621
#define MON_VAMPIRE_LORD  623
#define MON_JUBJUB        640
#define MON_G_C_DRAKE     646
#define MON_CLUB_DEMON    648
#define MON_F_ANGEL       652
#define MON_D_ELF_SORC    657
#define MON_MASTER_LICH   658
#define MON_RINALDO       660
#define MON_ARCHON        661
#define MON_UND_BEHOLDER  664
#define MON_IRON_LICH     666
#define MON_JACK_SHADOWS  670
#define MON_LLOIGOR       682
#define MON_DREADMASTER   690
#define MON_DROLEM        691
#define MON_DAWN          693
#define MON_NAZGUL        696
#define MON_SMAUG         697
#define MON_STORMBRINGER  698
#define MON_ULTRA_PALADIN 699
#define MON_G_TITAN       702
#define MON_S_TYRANNO     705
#define MON_FAFNER        712
#define MON_G_BALROG      720
#define MON_TIME_HOUND    725
#define MON_PLASMA_HOUND  726
#define MON_BULLGATES     732
#define MON_SANTACLAUS    733
#define MON_LORD_CHAOS    737
#define MON_TINDALOS      739
#define MON_DEMILICH      742
#define MON_NIGHTCRAWLER  744
#define MON_CHAOS_VOR     751
#define MON_AETHER_VOR    752
#define MON_FUNDIN        762
#define MON_DWORKIN       763
#define MON_NIGHTWALKER   768
#define MON_RAPHAEL       769
#define MON_SARUMAN       771
#define MON_GANDALF       772
#define MON_BRAND         773
#define MON_SHADOWLORD    774
#define MON_ARCHLICH      776
#define MON_JABBERWOCK    778
#define MON_CHAOS_HOUND   779
#define MON_ULT_BEHOLDER  781
#define MON_SHAMBLER      786
#define MON_BLEYS         789
#define MON_FIONA         791
#define MON_SKY_DRAKE     793
#define MON_JULIAN        794
#define MON_BLACK_REAVER  798
#define MON_CAINE         799
#define MON_GERARD        807
#define MON_UNGOLIANT     808
#define MON_ATLACH_NACHA  809
#define MON_Y_GOLONAC     810
#define MON_AETHER_HOUND  811
#define MON_WARP_DEMON    812
#define MON_ERIC          813
#define MON_UNMAKER       815
#define MON_CYBER         816
#define MON_KLING         819
#define MON_CORWIN        820
#define MON_ANGMAR        825
#define MON_CANTORAS      830
#define MON_GODZILLA      832
#define MON_SPAWN_CTH     836
#define MON_SURTUR        837
#define MON_TARRASQUE     838
#define MON_LUNGORTHIN    839
#define MON_CYBER_KING    843
#define MON_WYRM_POWER    847
#define MON_NODENS        849
#define MON_JORMUNGAND    854
#define MON_DESTROYER     855
#define MON_GOTHMOG       856
#define MON_G_CTHULHU     857
#define MON_SAURON        858
#define MON_UNICORN_ORD   859
#define MON_OBERON        860
#define MON_MORGOTH       861
#define MON_SERPENT       862
#define MON_ONE_RING      864
#define MON_CAAWS         866
#define MON_CULVERIN      867
#define MON_EBONY_MONK    870
#define MON_HAGURE        871
#define MON_OROCHI        872
#define MON_ECHIZEN       873
#define MON_SPECT_WYRM    874
#define MON_DIO           878
#define MON_OHMU          879
#define MON_WONG          880
#define MON_ZOMBI_SERPENT 883
#define MON_D_ELF_SHADE   886
#define MON_MANA_HOUND    887
#define MON_VENOM_WYRM    890
#define MON_TROLL_KING    894
#define MON_BAZOOKER      896
#define MON_SHARD_VOR     897
#define MON_ELF_LORD      900
#define MON_MASTER_MYS    916
#define MON_G_MASTER_MYS  917
#define MON_IE            921
#define MON_TSUCHINOKO    926
#define MON_GCWADL        929
#define MON_LOCKE_CLONE   930
#define MON_CALDARM       931
#define MON_BANORLUPART   932
#define MON_BANOR         933
#define MON_LUPART        934
#define MON_KENSHIROU     936
#define MON_W_KNIGHT      938
#define MON_PLANETAR      942
#define MON_SOLAR         943
#define MON_BIKETAL       945
#define MON_RICH          948
#define MON_IKETA         949
#define MON_B_DEATH_SWORD 953
#define MON_YASE_HORSE    955
#define MON_HORSE         956
#define MON_BOTEI         963
#define MON_KAGE          964
#define MON_JAIAN         967
#define MON_BELD          973
#define MON_THAT_BAT      975
#define MON_SHUTEN        979
#define MON_FENGHUANG     988
#define MON_KIRIN         989
#define MON_BAHAMUT       1000
#define MON_SUKE          1001
#define MON_KAKU          1002
#define MON_GHOST_Q       1003
#define MON_PIP           1004
#define MON_A_GOLD        1010
#define MON_A_SILVER      1011
#define MON_ROLENTO       1013
#define MON_RAOU          1018
#define MON_SHURYUUDAN    1023
#define MON_WAHHA         1031
#define MON_DEBBY         1032
#define MON_KNI_TEMPLAR   1037
#define MON_PALADIN       1038
#define MON_CHAMELEON     1040
#define MON_CHAMELEON_K   1041
#define MON_TOPAZ_MONK    1047
#define MON_M_MINDCRAFTER 1056
#define MON_ELDER_VAMPIRE 1058
#define MON_NOBORTA       1059
#define MON_MORI_TROLL    1060
#define MON_BARNEY        1061
#define MON_GROO          1062
#define MON_LOUSY         1063
#define MON_WYRM_SPACE    1064
#define MON_JIZOTAKO      1065
#define MON_TANUKI        1067
#define MON_ALIEN_JURAL   1082
#define MON_HATOPOPPO     1083
#define MON_KOGAN         1096

/*
 * Bit flags for the place_monster_???() (etc)
 */
#define PM_ALLOW_SLEEP    0x00000001    /*!< モンスター生成フラグ: 眠っている状態で生成されても良い */
#define PM_ALLOW_GROUP    0x00000002    /*!< モンスター生成フラグ: 集団生成されても良い */
#define PM_FORCE_FRIENDLY 0x00000004    /*!< モンスター生成フラグ: 必ず友好的に生成される */
#define PM_FORCE_PET      0x00000008    /*!< モンスター生成フラグ: 必ずペットとして生成される */
#define PM_NO_KAGE        0x00000010    /*!< モンスター生成フラグ: 必ずあやしい影としては生成されない */
#define PM_NO_PET         0x00000020    /*!< モンスター生成フラグ: 必ずペットとして生成されない */
#define PM_ALLOW_UNIQUE   0x00000040    /*!< モンスター生成フラグ: ユニークの選択生成を許可する */
#define PM_IGNORE_TERRAIN 0x00000080    /*!< モンスター生成フラグ: 侵入可能地形を考慮せずに生成する */
#define PM_HASTE          0x00000100    /*!< モンスター生成フラグ: 加速状態で生成する */
#define PM_KAGE           0x00000200    /*!< モンスター生成フラグ: 必ずあやしい影として生成する */
#define PM_MULTIPLY       0x00000400    /*!< モンスター生成フラグ: 増殖処理時として生成する */

extern bool place_monster_aux(MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
extern bool place_monster(POSITION y, POSITION x, BIT_FLAGS mode);
extern bool alloc_horde(POSITION y, POSITION x);
extern bool alloc_guardian(bool def_val);
extern bool alloc_monster(POSITION dis, BIT_FLAGS mode);

extern void monster_desc(char *desc, monster_type *m_ptr, BIT_FLAGS mode);
/* Bit flags for monster_desc() */
#define MD_OBJECTIVE      0x00000001 /* Objective (or Reflexive) */
#define MD_POSSESSIVE     0x00000002 /* Possessive (or Reflexive) */
#define MD_INDEF_HIDDEN   0x00000004 /* Use indefinites for hidden monsters ("something") */
#define MD_INDEF_VISIBLE  0x00000008 /* Use indefinites for visible monsters ("a kobold") */
#define MD_PRON_HIDDEN    0x00000010 /* Pronominalize hidden monsters */
#define MD_PRON_VISIBLE   0x00000020 /* Pronominalize visible monsters */
#define MD_ASSUME_HIDDEN  0x00000040 /* Assume the monster is hidden */
#define MD_ASSUME_VISIBLE 0x00000080 /* Assume the monster is visible */
#define MD_TRUE_NAME      0x00000100 /* Chameleon's true name */
#define MD_IGNORE_HALLU   0x00000200 /* Ignore hallucination, and penetrate shape change */

#define MD_WRONGDOER_NAME (MD_IGNORE_HALLU | MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE) /* 加害明記向け */

extern void monster_name(MONSTER_IDX m_idx, char* m_name);

extern void roff_top(MONRACE_IDX r_idx);
extern void screen_roff(MONRACE_IDX r_idx, BIT_FLAGS mode);
extern void display_roff(MONRACE_IDX r_idx);
extern void output_monster_spoiler(MONRACE_IDX r_idx, void(*roff_func)(TERM_COLOR attr, concptr str));
extern concptr extract_note_dies(MONRACE_IDX r_idx);
extern void monster_death(MONSTER_IDX m_idx, bool drop_item);
extern monsterrace_hook_type get_monster_hook(void);
extern monsterrace_hook_type get_monster_hook2(POSITION y, POSITION x);
extern void set_friendly(monster_type *m_ptr);
extern void set_pet(monster_type *m_ptr);
extern void set_hostile(monster_type *m_ptr);
extern void anger_monster(monster_type *m_ptr);

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING              0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
extern bool monster_can_cross_terrain(FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode);
extern bool monster_can_enter(POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode);

extern bool are_enemies(monster_type *m_ptr1, monster_type *m_ptr2);
extern bool monster_has_hostile_align(monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr);
extern void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char* msg);
extern concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode);
extern int get_monster_crowd_number(MONSTER_IDX m_idx);
extern void message_pain(MONSTER_IDX m_idx, HIT_POINT dam);

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
extern void update_monster(player_type *subject_ptr, MONSTER_IDX m_idx, bool full);
extern void update_monsters(bool full);
extern bool multiply_monster(MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);


/*
 * Some things which induce learning
 */
#define DRS_ACID         1
#define DRS_ELEC         2
#define DRS_FIRE         3
#define DRS_COLD         4
#define DRS_POIS         5
#define DRS_NETH         6
#define DRS_LITE         7
#define DRS_DARK         8
#define DRS_FEAR         9
#define DRS_CONF        10
#define DRS_CHAOS       11
#define DRS_DISEN       12
#define DRS_BLIND       13
#define DRS_NEXUS       14
#define DRS_SOUND       15
#define DRS_SHARD       16
#define DRS_FREE        30
#define DRS_MANA        31
#define DRS_REFLECT     32
extern void update_smart_learn(MONSTER_IDX m_idx, int what);

extern void choose_new_monster(MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx);
extern SPEED get_mspeed(monster_race *r_ptr);
extern void monster_drop_carried_objects(monster_type *m_ptr);

#define is_friendly(A) \
	 (bool)(((A)->smart & SM_FRIENDLY) ? TRUE : FALSE)

#define is_friendly_idx(MONSTER_IDX) \
	 (bool)((MONSTER_IDX) > 0 && is_friendly(&p_ptr->current_floor_ptr->m_list[(MONSTER_IDX)]))

#define is_pet(A) \
	 (bool)(((A)->smart & SM_PET) ? TRUE : FALSE)

#define is_hostile(A) \
	 (bool)((is_friendly(A) || is_pet(A)) ? FALSE : TRUE)

/* Hack -- Determine monster race appearance index is same as race index */
#define is_original_ap(A) \
	 (bool)(((A)->ap_r_idx == (A)->r_idx) ? TRUE : FALSE)

#define is_original_ap_and_seen(A) \
	 (bool)((A)->ml && !p_ptr->image && ((A)->ap_r_idx == (A)->r_idx))
