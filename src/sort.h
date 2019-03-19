extern void ang_sort_aux(vptr u, vptr v, int p, int q);
extern void ang_sort(vptr u, vptr v, int n);

extern bool ang_sort_comp_distance(vptr u, vptr v, int a, int b);
extern bool ang_sort_comp_importance(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_distance(vptr u, vptr v, int a, int b);

extern bool ang_sort_art_comp(vptr u, vptr v, int a, int b);
extern void ang_sort_art_swap(vptr u, vptr v, int a, int b);

extern bool ang_sort_comp_quest_num(vptr u, vptr v, int a, int b);
extern void ang_sort_swap_quest_num(vptr u, vptr v, int a, int b);
