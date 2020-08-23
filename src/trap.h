
extern void init_normal_traps(void);
extern FEAT_IDX choose_random_trap(void);
extern void disclose_grid(POSITION y, POSITION x);
extern void place_trap(POSITION y, POSITION x);
extern void hit_trap(bool break_trap);

extern const int chest_traps[64];

/* Types of normal traps */
#define NOT_TRAP        -1
#define TRAP_TRAPDOOR    0
#define TRAP_PIT         1
#define TRAP_SPIKED_PIT  2
#define TRAP_POISON_PIT  3
#define TRAP_TY_CURSE    4
#define TRAP_TELEPORT    5
#define TRAP_FIRE        6
#define TRAP_ACID        7
#define TRAP_SLOW        8

#define TRAP_LOSE_STR    9
#define TRAP_LOSE_DEX   10
#define TRAP_LOSE_CON   11
#define TRAP_BLIND      12
#define TRAP_CONFUSE    13
#define TRAP_POISON     14
#define TRAP_SLEEP      15
#define TRAP_TRAPS      16
#define TRAP_ALARM      17

#define MAX_NORMAL_TRAPS 18


/* Types of special traps */
#define TRAP_OPEN       18
#define TRAP_ARMAGEDDON 19
#define TRAP_PIRANHA    20



/* See init_feat_variables() in init2.c */
