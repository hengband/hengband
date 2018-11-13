
extern void init_normal_traps(void);
extern FEAT_IDX choose_random_trap(void);
extern void disclose_grid(POSITION y, POSITION x);
extern void place_trap(POSITION y, POSITION x);
extern void hit_trap(bool break_trap);

#define MAX_NORMAL_TRAPS 18

/* See init_feat_variables() in init2.c */
