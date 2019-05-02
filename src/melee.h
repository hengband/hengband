
typedef int COMBAT_OPTION_IDX; // py_attack()用コンバットオプション型定義
#define HISSATSU_NONE   0
#define HISSATSU_2      1
#define HISSATSU_3WAY   2
#define HISSATSU_SUTEMI 3
#define HISSATSU_FIRE   4
#define HISSATSU_COLD   5
#define HISSATSU_POISON 6
#define HISSATSU_ELEC   7
#define HISSATSU_NYUSIN 8
#define HISSATSU_FUKI   9
#define HISSATSU_MAJIN  10
#define HISSATSU_BOOMER 11
#define HISSATSU_DRAIN  12
#define HISSATSU_SEKIRYUKA 13
#define HISSATSU_OTAKEBI 14
#define HISSATSU_SHOUGE 15
#define HISSATSU_CONF   16
#define HISSATSU_ISSEN  17
#define HISSATSU_KYUSHO 18
#define HISSATSU_KONSIN 19
#define HISSATSU_HYAKU  20
#define HISSATSU_MINEUCHI 21
#define HISSATSU_MEKIKI 22
#define HISSATSU_ZANMA  23
#define HISSATSU_UNDEAD 24
#define HISSATSU_HAGAN  25
#define HISSATSU_QUAKE  26
#define HISSATSU_COUNTER 27
#define HISSATSU_HARAI  28
#define HISSATSU_3DAN   29
#define HISSATSU_100NIN 30
#define HISSATSU_IAI    100

extern const concptr silly_attacks[MAX_SILLY_ATTACK];
#ifdef JP
extern const concptr silly_attacks2[MAX_SILLY_ATTACK];
#endif

/* For Monk martial arts */
typedef struct martial_arts martial_arts;
struct martial_arts
{
	concptr desc;       /* A verbose attack description */
	PLAYER_LEVEL min_level;  /* Minimum level to use */
	int chance;     /* Chance of 'success' */
	int dd;         /* Damage dice */
	int ds;         /* Damage sides */
	int effect;     /* Special effects */
};

extern const martial_arts ma_blows[MAX_MA];
extern const int monk_ave_damage[PY_MAX_LEVEL + 1][3];

extern bool test_hit_norm(HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible);
extern PERCENTAGE hit_chance(HIT_RELIABILITY chance, ARMOUR_CLASS ac);
extern HIT_POINT tot_dam_aux(object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, BIT_FLAGS mode, bool thrown);
extern HIT_POINT critical_norm(WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, BIT_FLAGS mode);
extern bool py_attack(POSITION y, POSITION x, COMBAT_OPTION_IDX mode);
extern bool make_attack_normal(MONSTER_IDX m_idx);
extern void mon_take_hit_mon(MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who);

