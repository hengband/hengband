#include "angband.h"
#include "effect-item.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "object-broken.h"
#include "autopick/autopick.h"

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるアイテムオブジェクトへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * <pre>
 * We are called from "project()" to "damage" objects
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Perhaps we should only SOMETIMES damage things on the ground.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We also "see" grids which are "memorized", probably a hack
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 * </pre>
 */
bool project_o(player_type *caster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ)
{
	grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

	OBJECT_IDX this_o_idx, next_o_idx = 0;

	bool obvious = FALSE;
	bool known = player_has_los_bold(caster_ptr, y, x);

	BIT_FLAGS flgs[TR_FLAG_SIZE];

	GAME_TEXT o_name[MAX_NLEN];

	KIND_OBJECT_IDX k_idx = 0;
	bool is_potion = FALSE;


	who = who ? who : 0;
	dam = (dam + r) / (r + 1);

	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[this_o_idx];

		bool is_art = FALSE;
		bool ignore = FALSE;
		bool do_kill = FALSE;

		concptr note_kill = NULL;

#ifdef JP
#else
		bool plural = (o_ptr->number > 1);
#endif
		next_o_idx = o_ptr->next_o_idx;
		object_flags(o_ptr, flgs);
		if (object_is_artifact(o_ptr)) is_art = TRUE;

		switch (typ)
		{
		case GF_ACID:
		{
			if (hates_acid(o_ptr))
			{
				do_kill = TRUE;
				note_kill = _("融けてしまった！", (plural ? " melt!" : " melts!"));
				if (have_flag(flgs, TR_IGNORE_ACID)) ignore = TRUE;
			}
			break;
		}
		case GF_ELEC:
		{
			if (hates_elec(o_ptr))
			{
				do_kill = TRUE;
				note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
				if (have_flag(flgs, TR_IGNORE_ELEC)) ignore = TRUE;
			}
			break;
		}
		case GF_FIRE:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
				if (have_flag(flgs, TR_IGNORE_FIRE)) ignore = TRUE;
			}
			break;
		}
		case GF_COLD:
		{
			if (hates_cold(o_ptr))
			{
				note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
				do_kill = TRUE;
				if (have_flag(flgs, TR_IGNORE_COLD)) ignore = TRUE;
			}
			break;
		}
		case GF_PLASMA:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
				if (have_flag(flgs, TR_IGNORE_FIRE)) ignore = TRUE;
			}
			if (hates_elec(o_ptr))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
				if (have_flag(flgs, TR_IGNORE_ELEC)) ignore = TRUE;
			}
			break;
		}
		case GF_METEOR:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
				if (have_flag(flgs, TR_IGNORE_FIRE)) ignore = TRUE;
			}

			if (hates_cold(o_ptr))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
				if (have_flag(flgs, TR_IGNORE_COLD)) ignore = TRUE;
			}

			break;
		}
		case GF_ICE:
		case GF_SHARDS:
		case GF_FORCE:
		case GF_SOUND:
		{
			if (hates_cold(o_ptr))
			{
				note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
				do_kill = TRUE;
			}

			break;
		}
		case GF_MANA:
		case GF_SEEKER:
		case GF_SUPER_RAY:
		{
			do_kill = TRUE;
			note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
			break;
		}
		case GF_DISINTEGRATE:
		{
			do_kill = TRUE;
			note_kill = _("蒸発してしまった！", (plural ? " evaporate!" : " evaporates!"));
			break;
		}
		case GF_CHAOS:
		{
			do_kill = TRUE;
			note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
			if (have_flag(flgs, TR_RES_CHAOS)) ignore = TRUE;
			else if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_CHAOS)) ignore = TRUE;
			break;
		}
		case GF_HOLY_FIRE:
		case GF_HELL_FIRE:
		{
			if (object_is_cursed(o_ptr))
			{
				do_kill = TRUE;
				note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
			}
			break;
		}
		case GF_IDENTIFY:
		{
			identify_item(caster_ptr, o_ptr);
			autopick_alter_item(caster_ptr, (-this_o_idx), FALSE);
			break;
		}
		case GF_KILL_TRAP:
		case GF_KILL_DOOR:
		{
			if (o_ptr->tval == TV_CHEST)
			{
				if (o_ptr->pval > 0)
				{
					o_ptr->pval = (0 - o_ptr->pval);
					object_known(o_ptr);
					if (known && (o_ptr->marked & OM_FOUND))
					{
						msg_print(_("カチッと音がした！", "Click!"));
						obvious = TRUE;
					}
				}
			}

			break;
		}
		case GF_ANIM_DEAD:
		{
			if (o_ptr->tval == TV_CORPSE)
			{
				int i;
				BIT_FLAGS mode = 0L;

				if (!who || is_pet(&caster_ptr->current_floor_ptr->m_list[who]))
					mode |= PM_FORCE_PET;

				for (i = 0; i < o_ptr->number; i++)
				{
					if (((o_ptr->sval == SV_CORPSE) && (randint1(100) > 80)) ||
						((o_ptr->sval == SV_SKELETON) && (randint1(100) > 60)))
					{
						if (!note_kill)
						{
							note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
						}
						continue;
					}
					else if (summon_named_creature(caster_ptr, who, y, x, o_ptr->pval, mode))
					{
						note_kill = _("生き返った。", " revived.");
					}
					else if (!note_kill)
					{
						note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
					}
				}

				do_kill = TRUE;
				obvious = TRUE;
			}

			break;
		}
		}

		if (do_kill)
		{
			if (known && (o_ptr->marked & OM_FOUND))
			{
				obvious = TRUE;
				object_desc(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			}

			if (is_art || ignore)
			{
				if (known && (o_ptr->marked & OM_FOUND))
				{
					msg_format(_("%sは影響を受けない！",
						(plural ? "The %s are unaffected!" : "The %s is unaffected!")), o_name);
				}
			}
			else
			{
				if (known && (o_ptr->marked & OM_FOUND) && note_kill)
				{
					msg_format(_("%sは%s", "The %s%s"), o_name, note_kill);
				}

				k_idx = o_ptr->k_idx;
				is_potion = object_is_potion(o_ptr);
				delete_object_idx(caster_ptr, this_o_idx);

				if (is_potion)
				{
					(void)potion_smash_effect(caster_ptr, who, y, x, k_idx);
				}

				lite_spot(caster_ptr, y, x);
			}
		}
	}

	return obvious;
}
