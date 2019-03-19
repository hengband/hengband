#include "angband.h"
#include "sort.h"


/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort_aux(vptr u, vptr v, int p, int q)
{
	int z, a, b;

	/* Done sort */
	if (p >= q) return;

	/* Pivot */
	z = p;

	/* Begin */
	a = p;
	b = q;

	/* Partition */
	while (TRUE)
	{
		/* Slide i2 */
		while (!(*ang_sort_comp)(u, v, b, z)) b--;

		/* Slide i1 */
		while (!(*ang_sort_comp)(u, v, z, a)) a++;

		/* Done partition */
		if (a >= b) break;

		/* Swap */
		(*ang_sort_swap)(u, v, a, b);

		/* Advance */
		a++, b--;
	}

	/* Recurse left side */
	ang_sort_aux(u, v, p, b);

	/* Recurse right side */
	ang_sort_aux(u, v, b + 1, q);
}


/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort(vptr u, vptr v, int n)
{
	/* Sort the array */
	ang_sort_aux(u, v, 0, n - 1);
}



/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
bool ang_sort_comp_distance(vptr u, vptr v, int a, int b)
{
	POSITION *x = (POSITION*)(u);
	POSITION *y = (POSITION*)(v);

	POSITION da, db, kx, ky;

	/* Absolute distance components */
	kx = x[a]; kx -= p_ptr->x; kx = ABS(kx);
	ky = y[a]; ky -= p_ptr->y; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Absolute distance components */
	kx = x[b]; kx -= p_ptr->x; kx = ABS(kx);
	ky = y[b]; ky -= p_ptr->y; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Compare the distances */
	return (da <= db);
}


/*
 * Sorting hook -- comp function -- by importance level of grids
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by level of monster
 */
bool ang_sort_comp_importance(vptr u, vptr v, int a, int b)
{
	POSITION *x = (POSITION*)(u);
	POSITION *y = (POSITION*)(v);
	grid_type *ca_ptr = &current_floor_ptr->grid_array[y[a]][x[a]];
	grid_type *cb_ptr = &current_floor_ptr->grid_array[y[b]][x[b]];
	monster_type *ma_ptr = &current_floor_ptr->m_list[ca_ptr->m_idx];
	monster_type *mb_ptr = &current_floor_ptr->m_list[cb_ptr->m_idx];
	monster_race *ap_ra_ptr, *ap_rb_ptr;

	/* The player grid */
	if (y[a] == p_ptr->y && x[a] == p_ptr->x) return TRUE;
	if (y[b] == p_ptr->y && x[b] == p_ptr->x) return FALSE;

	/* Extract monster race */
	if (ca_ptr->m_idx && ma_ptr->ml) ap_ra_ptr = &r_info[ma_ptr->ap_r_idx];
	else ap_ra_ptr = NULL;
	if (cb_ptr->m_idx && mb_ptr->ml) ap_rb_ptr = &r_info[mb_ptr->ap_r_idx];
	else ap_rb_ptr = NULL;

	if (ap_ra_ptr && !ap_rb_ptr) return TRUE;
	if (!ap_ra_ptr && ap_rb_ptr) return FALSE;

	/* Compare two monsters */
	if (ap_ra_ptr && ap_rb_ptr)
	{
		/* Unique monsters first */
		if ((ap_ra_ptr->flags1 & RF1_UNIQUE) && !(ap_rb_ptr->flags1 & RF1_UNIQUE)) return TRUE;
		if (!(ap_ra_ptr->flags1 & RF1_UNIQUE) && (ap_rb_ptr->flags1 & RF1_UNIQUE)) return FALSE;

		/* Shadowers first (あやしい影) */
		if ((ma_ptr->mflag2 & MFLAG2_KAGE) && !(mb_ptr->mflag2 & MFLAG2_KAGE)) return TRUE;
		if (!(ma_ptr->mflag2 & MFLAG2_KAGE) && (mb_ptr->mflag2 & MFLAG2_KAGE)) return FALSE;

		/* Unknown monsters first */
		if (!ap_ra_ptr->r_tkills && ap_rb_ptr->r_tkills) return TRUE;
		if (ap_ra_ptr->r_tkills && !ap_rb_ptr->r_tkills) return FALSE;

		/* Higher level monsters first (if known) */
		if (ap_ra_ptr->r_tkills && ap_rb_ptr->r_tkills)
		{
			if (ap_ra_ptr->level > ap_rb_ptr->level) return TRUE;
			if (ap_ra_ptr->level < ap_rb_ptr->level) return FALSE;
		}

		/* Sort by index if all conditions are same */
		if (ma_ptr->ap_r_idx > mb_ptr->ap_r_idx) return TRUE;
		if (ma_ptr->ap_r_idx < mb_ptr->ap_r_idx) return FALSE;
	}

	/* An object get higher priority */
	if (current_floor_ptr->grid_array[y[a]][x[a]].o_idx && !current_floor_ptr->grid_array[y[b]][x[b]].o_idx) return TRUE;
	if (!current_floor_ptr->grid_array[y[a]][x[a]].o_idx && current_floor_ptr->grid_array[y[b]][x[b]].o_idx) return FALSE;

	/* Priority from the terrain */
	if (f_info[ca_ptr->feat].priority > f_info[cb_ptr->feat].priority) return TRUE;
	if (f_info[ca_ptr->feat].priority < f_info[cb_ptr->feat].priority) return FALSE;

	/* If all conditions are same, compare distance */
	return ang_sort_comp_distance(u, v, a, b);
}


/*
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
void ang_sort_swap_distance(vptr u, vptr v, int a, int b)
{
	POSITION *x = (POSITION*)(u);
	POSITION *y = (POSITION*)(v);

	POSITION temp;

	/* Swap "x" */
	temp = x[a];
	x[a] = x[b];
	x[b] = temp;

	/* Swap "y" */
	temp = y[a];
	y[a] = y[b];
	y[b] = temp;
}



/*
 * Sorting hook -- Comp function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
bool ang_sort_art_comp(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);
	u16b *why = (u16b*)(v);

	int w1 = who[a];
	int w2 = who[b];

	int z1, z2;

	/* Sort by total kills */
	if (*why >= 3)
	{
		/* Extract total kills */
		z1 = a_info[w1].tval;
		z2 = a_info[w2].tval;

		/* Compare total kills */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by monster level */
	if (*why >= 2)
	{
		/* Extract levels */
		z1 = a_info[w1].sval;
		z2 = a_info[w2].sval;

		/* Compare levels */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by monster experience */
	if (*why >= 1)
	{
		/* Extract experience */
		z1 = a_info[w1].level;
		z2 = a_info[w2].level;

		/* Compare experience */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Compare indexes */
	return (w1 <= w2);
}


/*
 * Sorting hook -- Swap function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void ang_sort_art_swap(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b holder;

	/* Unused */
	(void)v;

	/* Swap */
	holder = who[a];
	who[a] = who[b];
	who[b] = holder;
}

bool ang_sort_comp_quest_num(vptr u, vptr v, int a, int b)
{
	QUEST_IDX *q_num = (QUEST_IDX *)u;
	quest_type *qa = &quest[q_num[a]];
	quest_type *qb = &quest[q_num[b]];

	/* Unused */
	(void)v;

	return (qa->comptime != qb->comptime) ?
		(qa->comptime < qb->comptime) :
		(qa->level <= qb->level);
}

void ang_sort_swap_quest_num(vptr u, vptr v, int a, int b)
{
	QUEST_IDX *q_num = (QUEST_IDX *)u;
	QUEST_IDX tmp;

	/* Unused */
	(void)v;

	tmp = q_num[a];
	q_num[a] = q_num[b];
	q_num[b] = tmp;
}

