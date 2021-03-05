﻿#include "specific-object/chest.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-generator.h"
#include "perception/object-perception.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-equipment.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-sight.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "sv-definition/sv-other-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!< この値以降の小項目IDを持った箱は大型の箱としてドロップ数を増やす / Special "sval" limit -- first "large" chest */
#define SV_CHEST_MIN_LARGE 4

/*!
* @brief 箱からアイテムを引き出す /
* Allocates objects upon opening a chest    -BEN-
* @param scatter TRUEならばトラップによるアイテムの拡散処理
* @param y 箱の存在するマスのY座標
* @param x 箱の存在するマスのX座標
* @param o_idx 箱のオブジェクトID
* @return なし
* @details
* <pre>
* Disperse treasures from the given chest, centered at (x,y).
*
* Small chests often contain "gold", while Large chests always contain
* items.  Wooden chests contain 2 items, Iron chests contain 4 items,
* and Steel chests contain 6 items.  The "value" of the items in a
* chest is based on the "power" of the chest, which is in turn based
* on the level on which the chest is generated.
* </pre>
*/
void chest_death(player_type *owner_ptr, bool scatter, POSITION y, POSITION x, OBJECT_IDX o_idx)
{
	int number;

	bool small;
	BIT_FLAGS mode = AM_GOOD | AM_FORBID_CHEST;

	object_type forge;
	object_type *q_ptr;

	floor_type *floor_ptr = owner_ptr->current_floor_ptr;
	object_type *o_ptr = &floor_ptr->o_list[o_idx];

	/* Small chests often hold "gold" */
	small = (o_ptr->sval < SV_CHEST_MIN_LARGE);

	/* Determine how much to drop (see above) */
	number = (o_ptr->sval % SV_CHEST_MIN_LARGE) * 2;

	if (o_ptr->sval == SV_CHEST_KANDUME)
	{
		number = 5;
		small = FALSE;
		mode |= AM_GREAT;
		floor_ptr->object_level = o_ptr->xtra3;
	}
	else
	{
		/* Determine the "value" of the items */
		floor_ptr->object_level = ABS(o_ptr->pval) + 10;
	}

	/* Zero pval means empty chest */
	if (!o_ptr->pval) number = 0;


	/* Drop some objects (non-chests) */
	for (; number > 0; --number)
	{
		q_ptr = &forge;
		object_wipe(q_ptr);

		/* Small chests often drop gold */
		if (small && (randint0(100) < 25))
		{
			/* Make some gold */
			if (!make_gold(owner_ptr, q_ptr)) continue;
		}

		/* Otherwise drop an item */
		else
		{
			/* Make a good object */
			if (!make_object(owner_ptr, q_ptr, mode)) continue;
		}

		/* If chest scatters its contents, pick any floor square. */
		if (scatter)
		{
			int i;
			for (i = 0; i < 200; i++)
			{
				/* Pick a totally random spot. */
				y = randint0(MAX_HGT);
				x = randint0(MAX_WID);

				/* Must be an empty floor. */
				if (!is_cave_empty_bold(owner_ptr, y, x)) continue;

				/* Place the object there. */
				(void)drop_near(owner_ptr, q_ptr, -1, y, x);

				/* Done. */
				break;
			}
		}
		/* Normally, drop object near the chest. */
		else (void)drop_near(owner_ptr, q_ptr, -1, y, x);
	}

	/* Reset the object level */
	floor_ptr->object_level = floor_ptr->base_level;

	/* Empty */
	o_ptr->pval = 0;

	/* Known */
	object_known(o_ptr);
}


/*!
* @brief 箱のトラップ処理 /
* Chests have traps too.
* @param y 箱の存在するマスのY座標
* @param x 箱の存在するマスのX座標
* @param o_idx 箱のオブジェクトID
* @return なし
* @details
* <pre>
* Exploding chest destroys contents (and traps).
* Note that the chest itself is never destroyed.
* </pre>
*/
void chest_trap(player_type *target_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx)
{
	int i, trap;

	object_type *o_ptr = &target_ptr->current_floor_ptr->o_list[o_idx];

	int mon_level = o_ptr->xtra3;

	/* Ignore disarmed chests */
	if (o_ptr->pval <= 0) return;

	/* Obtain the traps */
	trap = chest_traps[o_ptr->pval];

	/* Lose strength */
	if (trap & (CHEST_LOSE_STR))
	{
		msg_print(_("仕掛けられていた小さな針に刺されてしまった！", "A small needle has pricked you!"));
		take_hit(target_ptr, DAMAGE_NOESCAPE, damroll(1, 4), _("毒針", "a poison needle"), -1);
		(void)do_dec_stat(target_ptr, A_STR);
	}

	/* Lose constitution */
	if (trap & (CHEST_LOSE_CON))
	{
		msg_print(_("仕掛けられていた小さな針に刺されてしまった！", "A small needle has pricked you!"));
		take_hit(target_ptr, DAMAGE_NOESCAPE, damroll(1, 4), _("毒針", "a poison needle"), -1);
		(void)do_dec_stat(target_ptr, A_CON);
	}

	/* Poison */
	if (trap & (CHEST_POISON))
	{
		msg_print(_("突如吹き出した緑色のガスに包み込まれた！", "A puff of green gas surrounds you!"));
            if (!(has_resist_pois(target_ptr) || is_oppose_pois(target_ptr)))
		{
			(void)set_poisoned(target_ptr, target_ptr->poisoned + 10 + randint1(20));
		}
	}

	/* Paralyze */
	if (trap & (CHEST_PARALYZE))
	{
		msg_print(_("突如吹き出した黄色いガスに包み込まれた！", "A puff of yellow gas surrounds you!"));
		if (!target_ptr->free_act)
		{
			(void)set_paralyzed(target_ptr, target_ptr->paralyzed + 10 + randint1(20));
		}
	}

	/* Summon monsters */
	if (trap & (CHEST_SUMMON))
	{
		int num = 2 + randint1(3);
		msg_print(_("突如吹き出した煙に包み込まれた！", "You are enveloped in a cloud of smoke!"));
		for (i = 0; i < num; i++)
		{
			if (randint1(100)<target_ptr->current_floor_ptr->dun_level)
				activate_hi_summon(target_ptr, target_ptr->y, target_ptr->x, FALSE);
			else
				(void)summon_specific(target_ptr, 0, y, x, mon_level, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
		}
	}

	/* Elemental summon. */
	if (trap & (CHEST_E_SUMMON))
	{
		msg_print(_("宝を守るためにエレメンタルが現れた！", "Elemental beings appear to protect their treasures!"));
		for (i = 0; i < randint1(3) + 5; i++)
		{
			(void)summon_specific(target_ptr, 0, y, x, mon_level, SUMMON_ELEMENTAL, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
		}
	}

	/* Force clouds, then summon birds. */
	if (trap & (CHEST_BIRD_STORM))
	{
		msg_print(_("鳥の群れがあなたを取り巻いた！", "A storm of birds swirls around you!"));

		for (i = 0; i < randint1(3) + 3; i++)
			(void)fire_meteor(target_ptr, -1, GF_FORCE, y, x, o_ptr->pval / 5, 7);

		for (i = 0; i < randint1(5) + o_ptr->pval / 5; i++)
		{
			(void)summon_specific(target_ptr, 0, y, x, mon_level, SUMMON_BIRD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
		}
	}

	/* Various colorful summonings. */
	if (trap & (CHEST_H_SUMMON))
	{
		/* Summon demons. */
		if (one_in_(4))
		{
			msg_print(_("炎と硫黄の雲の中に悪魔が姿を現した！", "Demons materialize in clouds of fire and brimstone!"));
			for (i = 0; i < randint1(3) + 2; i++)
			{
				(void)fire_meteor(target_ptr, -1, GF_FIRE, y, x, 10, 5);
				(void)summon_specific(target_ptr, 0, y, x, mon_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
			}
		}

		/* Summon dragons. */
		else if (one_in_(3))
		{
			msg_print(_("暗闇にドラゴンの影がぼんやりと現れた！", "Draconic forms loom out of the darkness!"));
			for (i = 0; i < randint1(3) + 2; i++)
			{
				(void)summon_specific(target_ptr, 0, y, x, mon_level, SUMMON_DRAGON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
			}
		}

		/* Summon hybrids. */
		else if (one_in_(2))
		{
			msg_print(_("奇妙な姿の怪物が襲って来た！", "Creatures strange and twisted assault you!"));
			for (i = 0; i < randint1(5) + 3; i++)
			{
				(void)summon_specific(target_ptr, 0, y, x, mon_level, SUMMON_HYBRID, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
			}
		}

		/* Summon vortices (scattered) */
		else
		{
			msg_print(_("渦巻が合体し、破裂した！", "Vortices coalesce and wreak destruction!"));
			for (i = 0; i < randint1(3) + 2; i++)
			{
				(void)summon_specific(target_ptr, 0, y, x, mon_level, SUMMON_VORTEX, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
			}
		}
	}

	/* Dispel player. */
	if ((trap & (CHEST_RUNES_OF_EVIL)) && o_ptr->k_idx)
	{
		/* Determine how many nasty tricks can be played. */
		int nasty_tricks_count = 4 + randint0(3);

		msg_print(_("恐ろしい声が響いた:  「暗闇が汝をつつまん！」", "Hideous voices bid:  'Let the darkness have thee!'"));
		/* This is gonna hurt... */
		for (; nasty_tricks_count > 0; nasty_tricks_count--)
		{
			/* ...but a high saving throw does help a little. */
			if (randint1(100 + o_ptr->pval * 2) > target_ptr->skill_sav)
			{
				if (one_in_(6)) take_hit(target_ptr, DAMAGE_NOESCAPE, damroll(5, 20), _("破滅のトラップの宝箱", "a chest dispel-player trap"), -1);
				else if (one_in_(5)) (void)set_cut(target_ptr,target_ptr->cut + 200);
				else if (one_in_(4))
				{
					if (!target_ptr->free_act)
						(void)set_paralyzed(target_ptr, target_ptr->paralyzed + 2 +
							randint0(6));
					else
						(void)set_stun(target_ptr, target_ptr->stun + 10 +
							randint0(100));
				}
				else if (one_in_(3)) apply_disenchant(target_ptr, 0);
				else if (one_in_(2))
				{
					(void)do_dec_stat(target_ptr, A_STR);
					(void)do_dec_stat(target_ptr, A_DEX);
					(void)do_dec_stat(target_ptr, A_CON);
					(void)do_dec_stat(target_ptr, A_INT);
					(void)do_dec_stat(target_ptr, A_WIS);
					(void)do_dec_stat(target_ptr, A_CHR);
				}
				else (void)fire_meteor(target_ptr, -1, GF_NETHER, y, x, 150, 1);
			}
		}
	}

	/* Aggravate monsters. */
	if (trap & (CHEST_ALARM))
	{
		msg_print(_("けたたましい音が鳴り響いた！", "An alarm sounds!"));
		aggravate_monsters(target_ptr, 0);
	}

	/* Explode */
	if ((trap & (CHEST_EXPLODE)) && o_ptr->k_idx)
	{
		msg_print(_("突然、箱が爆発した！", "There is a sudden explosion!"));
		msg_print(_("箱の中の物はすべて粉々に砕け散った！", "Everything inside the chest is destroyed!"));
		o_ptr->pval = 0;
		sound(SOUND_EXPLODE);
		take_hit(target_ptr, DAMAGE_ATTACK, damroll(5, 8), _("爆発する箱", "an exploding chest"), -1);
	}
	/* Scatter contents. */
	if ((trap & (CHEST_SCATTER)) && o_ptr->k_idx)
	{
		msg_print(_("宝箱の中身はダンジョンじゅうに散乱した！", "The contents of the chest scatter all over the dungeon!"));
		chest_death(target_ptr, TRUE, y, x, o_idx);
		o_ptr->pval = 0;
	}
}
