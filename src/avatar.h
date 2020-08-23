/* avatar.c */
extern bool compare_virtue(int type, int num, int tekitou);
extern int virtue_number(int type);
extern concptr virtue[MAX_VIRTUE];
extern void get_virtues(void);
extern void chg_virtue(int virtue, int amount);
extern void set_virtue(int virtue, int amount);
extern void dump_virtues(FILE * OutFile);
