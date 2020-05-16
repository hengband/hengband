extern void ang_sort_aux(vptr u, vptr v, int p, int q, bool(*ang_sort_comp)(vptr u, vptr v, int a, int b), void(*ang_sort_swap)(vptr u, vptr v, int a, int b));
extern void ang_sort(vptr u, vptr v, int n, bool(*ang_sort_comp)(vptr u, vptr v, int a, int b), void(*ang_sort_swap)(vptr u, vptr v, int a, int b));

extern bool ang_sort_comp_distance(vptr u, vptr v, int a, int b);
extern bool ang_sort_comp_importance(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_distance(vptr u, vptr v, int a, int b);

extern bool ang_sort_art_comp(vptr u, vptr v, int a, int b);
extern void ang_sort_art_swap(vptr u, vptr v, int a, int b);

extern bool ang_sort_comp_quest_num(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_quest_num(vptr u, vptr v, int a, int b);

extern bool ang_sort_comp_pet(vptr u, vptr v, int a, int b);

extern bool ang_sort_comp_hook(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_hook(vptr u, vptr v, int a, int b);

extern bool ang_sort_comp_monster_level(vptr u, vptr v, int a, int b);
extern bool ang_sort_comp_pet_dismiss(vptr u, vptr v, int a, int b);

extern bool ang_sort_comp_cave_temp(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_cave_temp(vptr u, vptr v, int a, int b);

extern bool ang_sort_comp_evol_tree(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_evol_tree(vptr u, vptr v, int a, int b);
