/* mutation.c */
extern bool gain_random_mutation(MUTATION_IDX choose_mut);
extern bool lose_mutation(MUTATION_IDX choose_mut);
extern void lose_all_mutations(void);
extern void dump_mutations(FILE *OutFile);
extern void do_cmd_knowledge_mutations(void);
extern int calc_mutant_regenerate_mod(void);
extern bool mutation_power_aux(int power);
