/* mutation.c */
extern bool gain_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut);
extern bool lose_mutation(MUTATION_IDX choose_mut);
extern void lose_all_mutations(player_type *creature_ptr);
extern void dump_mutations(player_type *creature_ptr, FILE *OutFile);
extern void do_cmd_knowledge_mutations(void);
extern int calc_mutant_regenerate_mod(player_type *creature_ptr);
extern bool mutation_power_aux(player_type *creature_ptr, int power);
extern void become_living_trump(player_type *creature_ptr);
