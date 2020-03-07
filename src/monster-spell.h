#pragma once


/* Imitator */
typedef struct monster_power monster_power;
struct monster_power
{
	PLAYER_LEVEL level;
	MANA_POINT smana;
	PERCENTAGE fail;
	int     manedam;
	int     manefail;
	int     use_stat;
	concptr    name;
};

/* Spell Type flag */
#define MONSTER_TO_PLAYER     0x01
#define MONSTER_TO_MONSTER    0x02

/* summoning number */
#define S_NUM_6     (easy_band ? 2 : 6)
#define S_NUM_4     (easy_band ? 1 : 4)

/* monster spell number */
#define RF4_SPELL_START 32 * 3
#define RF5_SPELL_START 32 * 4
#define RF6_SPELL_START 32 * 5

#define RF4_SPELL_SIZE 32
#define RF5_SPELL_SIZE 32
#define RF6_SPELL_SIZE 32

/* Spell Damage Calc Flag*/
#define DAM_ROLL 1
#define DAM_MAX 2
#define DAM_MIN 3
#define DICE_NUM 4
#define DICE_SIDE 5
#define DICE_MULT 6
#define DICE_DIV 7
#define BASE_DAM 8

#define MAX_MONSPELLS 96
#define MONSPELL_TYPE_BOLT 1
#define MONSPELL_TYPE_BALL 2
#define MONSPELL_TYPE_BREATH 3
#define MONSPELL_TYPE_SUMMON 4
#define MONSPELL_TYPE_OTHER 5

/* Monster Spells */
#define MS_SHRIEK         0
#define MS_XXX1           1
#define MS_DISPEL         2
#define MS_ROCKET         3
#define MS_SHOOT          4
#define MS_XXX2           5
#define MS_XXX3           6
#define MS_XXX4           7
#define MS_BR_ACID        8
#define MS_BR_ELEC        9
#define MS_BR_FIRE        10
#define MS_BR_COLD        11
#define MS_BR_POIS        12
#define MS_BR_NETHER      13
#define MS_BR_LITE        14
#define MS_BR_DARK        15
#define MS_BR_CONF        16
#define MS_BR_SOUND       17
#define MS_BR_CHAOS       18
#define MS_BR_DISEN       19
#define MS_BR_NEXUS       20
#define MS_BR_TIME        21
#define MS_BR_INERTIA     22
#define MS_BR_GRAVITY     23
#define MS_BR_SHARDS      24
#define MS_BR_PLASMA      25
#define MS_BR_FORCE       26
#define MS_BR_MANA        27
#define MS_BALL_NUKE      28
#define MS_BR_NUKE        29
#define MS_BALL_CHAOS     30
#define MS_BR_DISI        31
#define MS_BALL_ACID      32
#define MS_BALL_ELEC      33
#define MS_BALL_FIRE      34
#define MS_BALL_COLD      35
#define MS_BALL_POIS      36
#define MS_BALL_NETHER    37
#define MS_BALL_WATER     38
#define MS_BALL_MANA      39
#define MS_BALL_DARK      40
#define MS_DRAIN_MANA     41
#define MS_MIND_BLAST     42
#define MS_BRAIN_SMASH    43
#define MS_CAUSE_1        44
#define MS_CAUSE_2        45
#define MS_CAUSE_3        46
#define MS_CAUSE_4        47
#define MS_BOLT_ACID      48
#define MS_BOLT_ELEC      49
#define MS_BOLT_FIRE      50
#define MS_BOLT_COLD      51
#define MS_STARBURST      52
#define MS_BOLT_NETHER    53
#define MS_BOLT_WATER     54
#define MS_BOLT_MANA      55
#define MS_BOLT_PLASMA    56
#define MS_BOLT_ICE       57
#define MS_MAGIC_MISSILE  58
#define MS_SCARE          59
#define MS_BLIND          60
#define MS_CONF           61
#define MS_SLOW           62
#define MS_SLEEP          63
#define MS_SPEED          64
#define MS_HAND_DOOM      65
#define MS_HEAL           66
#define MS_INVULNER       67
#define MS_BLINK          68
#define MS_TELEPORT       69
#define MS_WORLD          70
#define MS_SPECIAL        71
#define MS_TELE_TO        72
#define MS_TELE_AWAY      73
#define MS_TELE_LEVEL     74
#define MS_PSY_SPEAR      75
#define MS_DARKNESS       76
#define MS_MAKE_TRAP      77
#define MS_FORGET         78
#define MS_RAISE_DEAD     79
#define MS_S_KIN          80
#define MS_S_CYBER        81
#define MS_S_MONSTER      82
#define MS_S_MONSTERS     83
#define MS_S_ANT          84
#define MS_S_SPIDER       85
#define MS_S_HOUND        86
#define MS_S_HYDRA        87
#define MS_S_ANGEL        88
#define MS_S_DEMON        89
#define MS_S_UNDEAD       90
#define MS_S_DRAGON       91
#define MS_S_HI_UNDEAD    92
#define MS_S_HI_DRAGON    93
#define MS_S_AMBERITE     94
#define MS_S_UNIQUE       95

/*
 * Hack -- choose "intelligent" spells when desperate
 * Including "summon" spells
 */
#define RF4_INT_MASK \
	(RF4_SUMMON_MASK | RF4_DISPEL)

#define RF5_INT_MASK \
	(RF5_SUMMON_MASK | \
	 RF5_HOLD | RF5_SLOW | RF5_CONF | RF5_BLIND | RF5_SCARE)

#define RF6_INT_MASK \
	(RF6_SUMMON_MASK | \
	 RF6_BLINK | RF6_TPORT | RF6_TELE_LEVEL | RF6_TELE_AWAY | \
	 RF6_HEAL | RF6_INVULNER | RF6_HASTE | RF6_TRAPS)

 /*
  * Hack -- spells that cannot be used while player riding on the monster
  */
#define RF4_RIDING_MASK \
	(RF4_SHRIEK)

#define RF5_RIDING_MASK 0UL

#define RF6_RIDING_MASK \
	(RF6_BLINK | RF6_TPORT | RF6_TRAPS | RF6_DARKNESS | RF6_SPECIAL)

  /*
   * Hack -- "bolt" spells that may hurt fellow monsters
   * Currently "bolt" spells are included in "attack"
   */
#define RF4_BOLT_MASK \
	(RF4_ROCKET | RF4_SHOOT)

#define RF5_BOLT_MASK \
	(RF5_BO_ACID | RF5_BO_ELEC | RF5_BO_FIRE | RF5_BO_COLD | \
	 RF5_BO_NETH | RF5_BO_WATE | RF5_BO_MANA | RF5_BO_PLAS | \
	 RF5_BO_ICEE | RF5_MISSILE)

#define RF6_BOLT_MASK 0UL

   /*
	* Hack -- "beam" spells that may hurt fellow monsters
	* Currently "beam" spells are included in "attack"
	*/
#define RF4_BEAM_MASK 0UL

#define RF5_BEAM_MASK 0UL

#define RF6_BEAM_MASK (RF6_PSY_SPEAR)

	/*
	 * Hack -- "ball" spells that may hurt friends
	 * Including "radius 4 ball" and "breath" spells
	 * Currently "ball" spells are included in "attack"
	 */
#define RF4_BALL_MASK \
	(RF4_BIG_BALL_MASK | RF4_BREATH_MASK | \
	 RF4_ROCKET | RF4_BA_NUKE)

#define RF5_BALL_MASK \
	(RF5_BIG_BALL_MASK | RF5_BREATH_MASK | \
	 RF5_BA_ACID | RF5_BA_ELEC | RF5_BA_FIRE | RF5_BA_COLD | \
	 RF5_BA_POIS | RF5_BA_NETH)

#define RF6_BALL_MASK \
	(RF6_BIG_BALL_MASK | RF6_BREATH_MASK)

	 /*
	  * Hack -- "ball" spells with radius 4 that may hurt friends
	  * Currently "radius 4 ball" spells are included in "ball"
	  */
#define RF4_BIG_BALL_MASK \
	(RF4_BA_CHAO)

#define RF5_BIG_BALL_MASK \
	(RF5_BA_LITE | RF5_BA_DARK | RF5_BA_WATE | RF5_BA_MANA)

#define RF6_BIG_BALL_MASK 0UL

	  /*
	   * Hack -- "breath" spells that may hurt friends
	   * Currently "breath" spells are included in "ball" and "non-magic"
	   */
#define RF4_BREATH_MASK \
	(RF4_BR_ACID | RF4_BR_ELEC | RF4_BR_FIRE | RF4_BR_COLD | \
	 RF4_BR_POIS | RF4_BR_NETH | RF4_BR_LITE | RF4_BR_DARK | \
	 RF4_BR_CONF | RF4_BR_SOUN | RF4_BR_CHAO | RF4_BR_DISE | \
	 RF4_BR_NEXU | RF4_BR_SHAR | RF4_BR_TIME | RF4_BR_INER | \
	 RF4_BR_GRAV | RF4_BR_PLAS | RF4_BR_WALL | RF4_BR_MANA | \
	 RF4_BR_NUKE | RF4_BR_DISI)

#define RF5_BREATH_MASK 0UL

#define RF6_BREATH_MASK 0UL

	   /*
		* Hack -- "summon" spells
		* Currently "summon" spells are included in "intelligent" and "indirect"
		*/
#define RF4_SUMMON_MASK 0UL

#define RF5_SUMMON_MASK 0UL

#define RF6_SUMMON_MASK \
	(RF6_S_KIN | RF6_S_CYBER | RF6_S_MONSTER | RF6_S_MONSTERS | RF6_S_ANT | \
	 RF6_S_SPIDER | RF6_S_HOUND | RF6_S_HYDRA | RF6_S_ANGEL | RF6_S_DEMON | \
	 RF6_S_UNDEAD | RF6_S_DRAGON | RF6_S_HI_UNDEAD | RF6_S_HI_DRAGON | \
	 RF6_S_AMBERITES | RF6_S_UNIQUE)

		/*
		 * Hack -- "attack" spells
		 * Including "bolt", "beam" and "ball" spells
		 */
#define RF4_ATTACK_MASK \
	(RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK | RF4_DISPEL)

#define RF5_ATTACK_MASK \
	(RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | \
	 RF5_DRAIN_MANA | RF5_MIND_BLAST | RF5_BRAIN_SMASH | \
	 RF5_CAUSE_1 | RF5_CAUSE_2 | RF5_CAUSE_3 | RF5_CAUSE_4 | \
	 RF5_SCARE | RF5_BLIND | RF5_CONF | RF5_SLOW | RF5_HOLD)

#define RF6_ATTACK_MASK \
	(RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK | \
	 RF6_HAND_DOOM | RF6_TELE_TO | RF6_TELE_AWAY | RF6_TELE_LEVEL | \
	 RF6_DARKNESS | RF6_TRAPS | RF6_FORGET)

		 /*
		  * Hack -- "indirect" spells
		  * Including "summon" spells
		  */
#define RF4_INDIRECT_MASK \
	(RF4_SUMMON_MASK | RF4_SHRIEK)

#define RF5_INDIRECT_MASK \
	(RF5_SUMMON_MASK)

#define RF6_INDIRECT_MASK \
	(RF6_SUMMON_MASK | \
	 RF6_HASTE | RF6_HEAL | RF6_INVULNER | RF6_BLINK | RF6_WORLD | \
	 RF6_TPORT | RF6_RAISE_DEAD)

		  /*
		   * Hack -- "non-magic" spells
		   * Including "breath" spells
		   */
#define RF4_NOMAGIC_MASK \
	(RF4_BREATH_MASK | RF4_SHRIEK | RF4_ROCKET | RF4_SHOOT)

#define RF5_NOMAGIC_MASK \
	(RF5_BREATH_MASK)

#define RF6_NOMAGIC_MASK \
	(RF6_BREATH_MASK | RF6_SPECIAL)

extern const monster_power monster_powers[MAX_MONSPELLS];
extern const concptr monster_powers_short[MAX_MONSPELLS];

/* mspells1.c */
extern bool clean_shot(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
extern bool summon_possible(player_type *target_ptr, POSITION y1, POSITION x1);
extern bool raise_possible(player_type *target_ptr, monster_type *m_ptr);
extern bool dispel_check(player_type *creature_ptr, MONSTER_IDX m_idx);
extern bool spell_is_inate(SPELL_IDX spell);
extern bool make_attack_spell(MONSTER_IDX m_idx, player_type *target_ptr);
extern void beam(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type);
extern void bolt(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type);
extern void breath(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, EFFECT_ID typ, int dam_hp, POSITION rad, bool breath, int monspell, int target_type);

/* mspells2.c */
extern void get_project_point(player_type *target_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg);
extern bool monst_spell_monst(player_type *target_ptr, MONSTER_IDX m_idx);

/* mspells3.c */
extern bool do_cmd_cast_learned(player_type *caster_ptr);
extern void learn_spell(player_type *learner_ptr, int monspell);
extern void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, BIT_FLAGS mode);

/* mspells4.c */
extern bool spell_learnable(player_type *target_ptr, MONSTER_IDX m_idx);
extern HIT_POINT monspell_to_player(player_type *target_ptr, int SPELL_NUM, POSITION y, POSITION x, MONSTER_IDX m_idx);
extern HIT_POINT monspell_to_monster(player_type *target_ptr, int SPELL_NUM, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell);
extern HIT_POINT monspell_damage(player_type *target_ptr, int SPELL_NUM, MONSTER_IDX m_idx, int TYPE);
extern HIT_POINT monspell_race_damage(player_type *target_ptr, int SPELL_NUM, MONRACE_IDX r_idx, int TYPE);
extern HIT_POINT monspell_bluemage_damage(player_type *target_ptr, int SPELL_NUM, PLAYER_LEVEL plev, int TYPE);
