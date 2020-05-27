#pragma once

#include "object/tr-types.h"

extern void do_cmd_activate(player_type *user_ptr);
extern void exe_activate(player_type *user_ptr, INVENTORY_IDX item);
extern bool activate_artifact(player_type *user_ptr, object_type *o_ptr);

typedef struct {
	concptr flag;
	byte index;
	byte level;
	s32b value;
	struct {
		int constant;
		DICE_NUMBER dice;
	} timeout;
	concptr desc;
} activation_type;

extern const activation_type activation_info[];

typedef struct {
	tr_type flag;
	int type;
	concptr name;
} dragonbreath_type;

extern const dragonbreath_type dragonbreath_info[];

/* Activation effects for random artifacts */
#define ACT_SUNLIGHT            1
#define ACT_BO_MISS_1           2
#define ACT_BA_POIS_1           3
#define ACT_BO_ELEC_1           4
#define ACT_BO_ACID_1           5
#define ACT_BO_COLD_1           6
#define ACT_BO_FIRE_1           7
#define ACT_BA_COLD_1           8
#define ACT_BA_FIRE_1           9
#define ACT_HYPODYNAMIA_1       10
#define ACT_BA_COLD_2           11
#define ACT_BA_ELEC_2           12
#define ACT_HYPODYNAMIA_2       13
#define ACT_DRAIN_1             14
#define ACT_BO_MISS_2           15
#define ACT_BA_FIRE_3           16
#define ACT_BA_COLD_3           17
#define ACT_BA_ELEC_3           18
#define ACT_WHIRLWIND           19
#define ACT_DRAIN_2             20
#define ACT_CALL_CHAOS          21
#define ACT_ROCKET              22
#define ACT_DISP_EVIL           23
#define ACT_BA_MISS_3           24
#define ACT_DISP_GOOD           25
#define ACT_BO_MANA             26
#define ACT_BA_FIRE_2           27
#define ACT_BA_WATER            28
#define ACT_BA_STAR             29
#define ACT_BA_DARK             30
#define ACT_BA_MANA             31
#define ACT_PESTICIDE           32
#define ACT_BLINDING_LIGHT      33
#define ACT_BIZARRE             34
#define ACT_CAST_BA_STAR        35
#define ACT_BLADETURNER         36
#define ACT_BA_ACID_1           37
#define ACT_BR_FIRE             38
#define ACT_BR_COLD             39
#define ACT_BR_DRAGON           40
#define ACT_BA_FIRE_4           41
#define ACT_BA_NUKE_1           42
/* 42 - 50 unused */
#define ACT_CONFUSE             51
#define ACT_SLEEP               52
#define ACT_QUAKE               53
#define ACT_TERROR              54
#define ACT_TELE_AWAY           55
#define ACT_BANISH_EVIL         56
#define ACT_GENOCIDE            57
#define ACT_MASS_GENO           58
#define ACT_SCARE_AREA          59
#define ACT_AGGRAVATE           60
/* 59 - 64 unused */
#define ACT_CHARM_ANIMAL        65
#define ACT_CHARM_UNDEAD        66
#define ACT_CHARM_OTHER         67
#define ACT_CHARM_ANIMALS       68
#define ACT_CHARM_OTHERS        69
#define ACT_SUMMON_ANIMAL       70
#define ACT_SUMMON_PHANTOM      71
#define ACT_SUMMON_ELEMENTAL    72
#define ACT_SUMMON_DEMON        73
#define ACT_SUMMON_UNDEAD       74
#define ACT_SUMMON_HOUND        75
#define ACT_SUMMON_DAWN         76
#define ACT_SUMMON_OCTOPUS      77
/* 76 - 80 unused */
#define ACT_CHOIR_SINGS         80
#define ACT_CURE_LW             81
#define ACT_CURE_MW             82
#define ACT_CURE_POISON         83
#define ACT_REST_EXP            84
#define ACT_REST_ALL            85
#define ACT_CURE_700            86
#define ACT_CURE_1000           87
#define ACT_CURING              88
#define ACT_CURE_MANA_FULL      89
/* 90 unused */
#define ACT_ESP                 91
#define ACT_BERSERK             92
#define ACT_PROT_EVIL           93
#define ACT_RESIST_ALL          94
#define ACT_SPEED               95
#define ACT_XTRA_SPEED          96
#define ACT_WRAITH              97
#define ACT_INVULN              98
#define ACT_HERO                99
#define ACT_HERO_SPEED          100
#define ACT_RESIST_ACID         101
#define ACT_RESIST_FIRE         102
#define ACT_RESIST_COLD         103
#define ACT_RESIST_ELEC         104
#define ACT_RESIST_POIS         105
/* 106 - 110 unused */
#define ACT_LIGHT               111
#define ACT_MAP_LIGHT           112
#define ACT_DETECT_ALL          113
#define ACT_DETECT_XTRA         114
#define ACT_ID_FULL             115
#define ACT_ID_PLAIN            116
#define ACT_RUNE_EXPLO          117
#define ACT_RUNE_PROT           118
#define ACT_SATIATE             119
#define ACT_DEST_DOOR           120
#define ACT_STONE_MUD           121
#define ACT_RECHARGE            122
#define ACT_ALCHEMY             123
#define ACT_DIM_DOOR            124
#define ACT_TELEPORT            125
#define ACT_RECALL              126
#define ACT_JUDGE               127
#define ACT_TELEKINESIS         128
#define ACT_DETECT_UNIQUE       129
#define ACT_ESCAPE              130
#define ACT_DISP_CURSE_XTRA     131
#define ACT_BRAND_FIRE_BOLTS    132
#define ACT_RECHARGE_XTRA       133
#define ACT_LORE                134
#define ACT_SHIKOFUMI           135
#define ACT_PHASE_DOOR          136
#define ACT_DETECT_ALL_MONS     137
#define ACT_ULTIMATE_RESIST     138
/* 127 -> unused */
#define ACT_FALLING_STAR        246
#define ACT_STRAIN_HASTE        247
#define ACT_TELEPORT_LEVEL      248
#define ACT_GRAND_CROSS         249
#define ACT_CAST_OFF            250
#define ACT_FISHING             251
#define ACT_INROU               252
#define ACT_MURAMASA            253
#define ACT_BLOODY_MOON         254
#define ACT_CRIMSON             255

