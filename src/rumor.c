#include "angband.h"
#include "util.h"

#include "files.h"
#include "object-flavor.h"
#include "artifact.h"
#include "dungeon.h"
#include "monsterrace.h"
#include "floor-town.h"

/*
 * Display a rumor and apply its effects
 */

IDX rumor_num(char *zz, IDX max_idx)
{
	if (strcmp(zz, "*") == 0) return randint1(max_idx - 1);
	return (IDX)atoi(zz);
}

concptr rumor_bind_name(char *base, concptr fullname)
{
	char *s, *v;

	s = strstr(base, "{Name}");
	if (s)
	{
		s[0] = '\0';
		v = format("%s%s%s", base, fullname, (s + 6));
	}
	else
	{
		v = base;
	}

	return v;
}

void display_rumor(bool ex)
{
	errr err;
	int section = 0;
	char Rumor[1024];

	if (ex)
	{
		if (randint0(3) == 0) section = 1;
	}

	err = _(get_rnd_line_jonly("rumors_j.txt", section, Rumor, 10),
		get_rnd_line("rumors.txt", section, Rumor));
	if (err) strcpy(Rumor, _("嘘の噂もある。", "Some rumors are wrong."));

	err = TRUE;

	if (strncmp(Rumor, "R:", 2) == 0)
	{
		char *zz[4];
		concptr rumor_msg = NULL;
		concptr rumor_eff_format = NULL;
		char fullname[1024] = "";

		if (tokenize(Rumor + 2, 3, zz, TOKENIZE_CHECKQUOTE) == 3)
		{
			if (strcmp(zz[0], "ARTIFACT") == 0)
			{
				IDX a_idx, k_idx;
				object_type forge;
				object_type *q_ptr = &forge;
				artifact_type *a_ptr;

				while (1)
				{
					a_idx = rumor_num(zz[1], max_a_idx);

					a_ptr = &a_info[a_idx];
					if (a_ptr->name) break;
				}

				k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
				object_prep(q_ptr, k_idx);
				q_ptr->name1 = a_idx;
				q_ptr->ident = IDENT_STORE;
				object_desc(fullname, q_ptr, OD_NAME_ONLY);
			}
			else if (strcmp(zz[0], "MONSTER") == 0)
			{
				MONRACE_IDX r_idx;
				monster_race *r_ptr;

				while (1)
				{
					r_idx = rumor_num(zz[1], max_r_idx);
					r_ptr = &r_info[r_idx];
					if (r_ptr->name) break;
				}

				strcpy(fullname, r_name + r_ptr->name);

				/* Remember this monster */
				if (!r_ptr->r_sights)
				{
					r_ptr->r_sights++;
				}
			}
			else if (strcmp(zz[0], "DUNGEON") == 0)
			{
				DUNGEON_IDX d_idx;
				dungeon_type *d_ptr;

				while (1)
				{
					d_idx = rumor_num(zz[1], max_d_idx);
					d_ptr = &d_info[d_idx];
					if (d_ptr->name) break;
				}

				strcpy(fullname, d_name + d_ptr->name);

				if (!max_dlv[d_idx])
				{
					max_dlv[d_idx] = d_ptr->mindepth;
					rumor_eff_format = _("%sに帰還できるようになった。", "You can recall to %s.");
				}
			}
			else if (strcmp(zz[0], "TOWN") == 0)
			{
				IDX t_idx;
				s32b visit;

				while (1)
				{
					t_idx = rumor_num(zz[1], NO_TOWN);
					if (town_info[t_idx].name) break;
				}

				strcpy(fullname, town_info[t_idx].name);

				visit = (1L << (t_idx - 1));
				if ((t_idx != SECRET_TOWN) && !(p_ptr->visit & visit))
				{
					p_ptr->visit |= visit;
					rumor_eff_format = _("%sに行ったことがある気がする。", "You feel you have been to %s.");
				}
			}

			rumor_msg = rumor_bind_name(zz[2], fullname);
			msg_print(rumor_msg);
			if (rumor_eff_format)
			{
				msg_print(NULL);
				msg_format(rumor_eff_format, fullname);
			}
			err = FALSE;
		}
		/* error */
		if (err) msg_print(_("この情報は間違っている。", "This information is wrong."));
	}
	else
	{
		msg_format("%s", Rumor);
	}
}
