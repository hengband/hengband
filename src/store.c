/*!
 * @file store.c
 * @brief 店の処理 / Store commands
 * @date 2014/02/02
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "market/say-comments.h"
#include "market/store-owners.h"
#include "market/store-util.h"
#include "core.h"
#include "util.h"
#include "term.h"

#include "floor.h"
#include "io/write-diary.h"
#include "cmd/cmd-basic.h"
#include "cmd/cmd-diary.h"
#include "cmd/cmd-draw.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-help.h"
#include "cmd/cmd-item.h"
#include "cmd/cmd-macro.h"
#include "cmd/cmd-smith.h"
#include "cmd/cmd-visuals.h"
#include "cmd/cmd-zapwand.h"
#include "cmd/cmd-magiceat.h"
#include "spells.h"
#include "store.h"
#include "avatar.h"
#include "cmd-spell.h"
#include "rumor.h"
#include "player-status.h"
#include "player-class.h"
#include "player-inventory.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "floor-events.h"
#include "snipe.h"
#include "files.h"
#include "player-effects.h"
#include "player-race.h"
#include "mind.h"
#include "world.h"
#include "objectkind.h"
#include "autopick.h"
#include "floor-town.h"
#include "japanese.h"
#include "view-mainwindow.h"
#include "wild.h"

#define MIN_STOCK 12

static int store_top = 0;
static int store_bottom = 0;
static int xtra_stock = 0;
static store_type *st_ptr = NULL;
static const owner_type *ot_ptr = NULL;
static s16b old_town_num = 0;
static s16b inner_town_num = 0;

/*
 * We store the current "store feat" here so everyone can access it
 */
static int cur_store_feat;

/*
 * Buying and selling adjustments for race combinations.
 * Entry[owner][player] gives the basic "cost inflation".
 */
static byte rgold_adj[MAX_RACES][MAX_RACES] =
{
	/*Hum, HfE, Elf,  Hal, Gno, Dwa, HfO, HfT, Dun, HiE, Barbarian,
	 HfOg, HGn, HTn, Cyc, Yek, Klc, Kbd, Nbl, DkE, Drc, Mind Flayer,
	 Imp,  Glm, Skl, Zombie, Vampire, Spectre, Fairy, Beastman, Ent,
	 Angel, Demon, Kutar, Android, Merfolk */

	/* Human */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 125, 125, 105, 120, 105,  95, 140,
	  100, 120, 110, 105, 110 },

	/* Half-Elf */
	{ 110, 100, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  120, 115, 108, 115, 110, 110, 120, 120, 115, 115, 110,
	  120, 110, 110, 110, 120, 110, 100, 125, 100,  95, 140,
	  110, 115, 110, 110, 110 },

	/* Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  120, 120, 105, 120, 110, 105, 125, 125, 110, 115, 108,
	  120, 115, 110, 110, 120, 110, 100, 125, 100,  95, 140,
	  110, 110, 105, 110, 110 },

	/* Halfling */
	{ 115, 110, 105,  95, 105, 110, 115, 130, 115, 105, 115,
	  125, 120, 120, 125, 115, 110, 120, 120, 120, 115, 115,
	  120, 110, 120, 120, 130, 110, 110, 130, 110,  95, 140,
	  115, 120, 105, 115, 105 },

	/* Gnome */
	{ 115, 115, 110, 105,  95, 110, 115, 130, 115, 110, 115,
	  120, 125, 110, 120, 110, 105, 120, 110, 110, 105, 110,
	  120, 101, 110, 110, 120, 120, 115, 130, 115,  95, 140,
	  115, 110, 110, 115, 110 },

	/* Dwarf */
	{ 115, 120, 120, 110, 110,  95, 125, 135, 115, 120, 115,
	  125, 140, 130, 130, 120, 115, 115, 115, 135, 125, 120,
	  120, 105, 115, 115, 115, 115, 120, 130, 120,  95, 140,
	  115, 110, 115, 115, 120 },

	/* Half-Orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 115, 120, 125, 115, 125,  95, 140,
	  115, 110, 115, 115, 125 },

	/* Half-Troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 120, 110, 120,  95, 140,
	  110, 110, 115, 110, 130 },

	/* Amberite */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  120, 120, 105, 120, 115, 105, 115, 120, 110, 105, 105,
	  120, 105, 120, 120, 125, 120, 105, 135, 105,  95, 140,
	  100, 110, 110, 100, 110 },

	/* High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 125, 120, 100, 125, 100,  95, 140,
	  110, 110, 105, 110, 110 },

	/* Human / Barbarian (copied from human) */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 130, 125, 115, 120, 115,  95, 140,
	  100, 120, 110, 100, 110 },

	/* Half-Ogre: theoretical, copied from half-troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 120, 110, 120,  95, 140,
	  110, 110, 115, 110, 120 },

	/* Half-Giant: theoretical, copied from half-troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 130, 120, 130,  95, 140,
	  110, 110, 115, 110, 115 },

	/* Half-Titan: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 108 },

	/* Cyclops: theoretical, copied from half-troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 115 },

	/* Yeek: theoretical, copied from Half-Orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 110 },

	/* Klackon: theoretical, copied from Gnome */
	{ 115, 115, 110, 105,  95, 110, 115, 130, 115, 110, 115,
	  120, 125, 110, 120, 110, 105, 120, 110, 110, 105, 110,
	  120, 101, 110, 110, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 110 },

	/* Kobold: theoretical, copied from Half-Orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 120 },

	/* Nibelung: theoretical, copied from Dwarf */
	{ 115, 120, 120, 110, 110,  95, 125, 135, 115, 120, 115,
	  125, 140, 130, 130, 120, 115, 115, 115, 135, 125, 120,
	  120, 105, 115, 115, 120, 120, 130, 130, 130,  95, 140,
	  115, 135, 115, 115, 120 },

	/* Dark Elf */
	{ 110, 110, 110, 115, 120, 130, 115, 115, 120, 110, 115,
	  115, 115, 116, 115, 120, 120, 115, 115, 101, 110, 110,
	  110, 110, 112, 122, 110, 110, 110, 115, 110, 120, 120,
	  110, 101, 115, 110, 115 },

	/* Draconian: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 115 },

	/* Mind Flayer: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 110 },

	/* Imp: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  110, 110, 115, 110, 120 },

	/* Golem: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 110 },

	/* Skeleton: theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 110 },

	/* Zombie: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 110 },

	/* Vampire: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 120 },

	/* Spectre: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 110 },

	/* Sprite: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 105, 115, 110 },

	/* Beastman: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 125 },

	/* Ent */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  120, 120, 105, 120, 110, 105, 125, 125, 110, 115, 108,
	  120, 115, 110, 110, 120, 110, 100, 125, 100,  95, 140,
	  110, 110, 105, 110, 110 },

	/* Angel */
	{  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,
	   95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,
	   95,  95,  95,  95,  95,  95,  95,  95,  95,  95, 160,
	   95,  95,  95,  95,  95 },

	/* Demon */
	{ 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
	  140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
	  140, 140, 140, 140, 140, 140, 140, 140, 140, 160, 120,
	  140, 140, 140, 140, 140 },

	/* Dunadan */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 125, 125, 105, 120, 105,  95, 140,
	  100, 120, 110, 100, 110 },

	/* Shadow Fairy */
	{ 110, 110, 110, 115, 120, 130, 115, 115, 120, 110, 115,
	  115, 115, 116, 115, 120, 120, 115, 115, 101, 110, 110,
	  110, 110, 112, 122, 110, 110, 110, 115, 110, 120, 120,
	  110, 101, 115, 110, 115 },

	/* Kutar */
	{ 110, 110, 105, 105, 110, 115, 115, 115, 110, 105, 110,
	  115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
	  115, 115, 125, 125, 125, 125, 105, 115, 105,  95, 140,
	  110, 115, 100, 110, 110 },

	/* Android */
	{ 105, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 125, 125, 105, 120, 105,  95, 140,
	  100, 120, 110, 100, 110 },

	/* Merfolk */
	{ 110, 110, 110, 105, 110, 120, 125, 130, 110, 110, 110,
	  120, 115, 108, 115, 110, 110, 120, 120, 115, 115, 110,
	  120, 110, 110, 110, 120, 110, 110, 125, 110,  95, 140,
	  110, 115, 110, 110, 100 },
};


/*!
 * @brief 店舗価格を決定する. 無料にはならない /
 * Determine the price of an item (qty one) in a store.
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @param greed 店主の強欲度
 * @param flip TRUEならば店主にとっての買取価格、FALSEなら売出価格を計算
 * @return アイテムの店舗価格
 * @details
 * <pre>
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 * The "greed" value should exceed 100 when the player is "buying" the
 * item, and should be less than 100 when the player is "selling" it.
 * Hack -- the black market always charges twice as much as it should.
 * Charisma adjustment runs from 80 to 130
 * Racial adjustment runs from 95 to 130
 * Since greed/charisma/racial adjustments are centered at 100, we need
 * to adjust (by 200) to extract a usable multiplier.  Note that the
 * "greed" value is always something (?).
 * </pre>
 */
static PRICE price_item(player_type *player_ptr, object_type *o_ptr, int greed, bool flip)
{
	PRICE price = object_value(o_ptr);
	if (price <= 0) return (0L);

	int factor = rgold_adj[ot_ptr->owner_race][player_ptr->prace];
	factor += adj_chr_gold[player_ptr->stat_ind[A_CHR]];
	int adjust;
	if (flip)
	{
		adjust = 100 + (300 - (greed + factor));
		if (adjust > 100) adjust = 100;
		if (cur_store_num == STORE_BLACK)
			price = price / 2;

		price = (price * adjust + 50L) / 100L;
	}
	else
	{
		adjust = 100 + ((greed + factor) - 300);
		if (adjust < 100) adjust = 100;
		if (cur_store_num == STORE_BLACK)
			price = price * 2;

		price = (s32b)(((u32b)price * (u32b)adjust + 50UL) / 100UL);
	}

	if (price <= 0L) return (1L);
	return (price);
}


/*!
 * @brief 安価な消耗品の販売数を増やし、低確率で割引にする /
 * Certain "cheap" objects should be created in "piles"
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Some objects can be sold at a "discount" (in small piles)
 * </pre>
 */
static void mass_produce(object_type *o_ptr)
{
	int size = 1;
	PRICE cost = object_value(o_ptr);
	switch (o_ptr->tval)
	{
	case TV_FOOD:
	case TV_FLASK:
	case TV_LITE:
	{
		if (cost <= 5L) size += damroll(3, 5);
		if (cost <= 20L) size += damroll(3, 5);
		if (cost <= 50L) size += damroll(2, 2);
		break;
	}
	case TV_POTION:
	case TV_SCROLL:
	{
		if (cost <= 60L) size += damroll(3, 5);
		if (cost <= 240L) size += damroll(1, 5);
		if (o_ptr->sval == SV_SCROLL_STAR_IDENTIFY) size += damroll(3, 5);
		if (o_ptr->sval == SV_SCROLL_STAR_REMOVE_CURSE) size += damroll(1, 4);
		break;
	}
	case TV_LIFE_BOOK:
	case TV_SORCERY_BOOK:
	case TV_NATURE_BOOK:
	case TV_CHAOS_BOOK:
	case TV_DEATH_BOOK:
	case TV_TRUMP_BOOK:
	case TV_ARCANE_BOOK:
	case TV_CRAFT_BOOK:
	case TV_DAEMON_BOOK:
	case TV_CRUSADE_BOOK:
	case TV_MUSIC_BOOK:
	case TV_HISSATSU_BOOK:
	case TV_HEX_BOOK:
	{
		if (cost <= 50L) size += damroll(2, 3);
		if (cost <= 500L) size += damroll(1, 3);
		break;
	}
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_SHIELD:
	case TV_GLOVES:
	case TV_BOOTS:
	case TV_CLOAK:
	case TV_HELM:
	case TV_CROWN:
	case TV_SWORD:
	case TV_POLEARM:
	case TV_HAFTED:
	case TV_DIGGING:
	case TV_BOW:
	{
		if (object_is_artifact(o_ptr)) break;
		if (object_is_ego(o_ptr)) break;
		if (cost <= 10L) size += damroll(3, 5);
		if (cost <= 100L) size += damroll(3, 5);
		break;
	}
	case TV_SPIKE:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	{
		if (cost <= 5L) size += damroll(5, 5);
		if (cost <= 50L) size += damroll(5, 5);
		if (cost <= 500L) size += damroll(5, 5);
		break;
	}
	case TV_FIGURINE:
	{
		if (cost <= 100L) size += damroll(2, 2);
		if (cost <= 1000L) size += damroll(2, 2);
		break;
	}
	case TV_CAPTURE:
	case TV_STATUE:
	case TV_CARD:
	{
		size = 1;
		break;
	}

	/*
	 * Because many rods (and a few wands and staffs) are useful mainly
	 * in quantity, the Black Market will occasionally have a bunch of
	 * one kind. -LM-
	 */
	case TV_ROD:
	case TV_WAND:
	case TV_STAFF:
	{
		if ((cur_store_num == STORE_BLACK) && one_in_(3))
		{
			if (cost < 1601L) size += damroll(1, 5);
			else if (cost < 3201L) size += damroll(1, 3);
		}
		break;
	}
	}

	DISCOUNT_RATE discount = 0;
	if (cost < 5)
	{
		discount = 0;
	}
	else if (one_in_(25))
	{
		discount = 25;
	}
	else if (one_in_(150))
	{
		discount = 50;
	}
	else if (one_in_(300))
	{
		discount = 75;
	}
	else if (one_in_(500))
	{
		discount = 90;
	}

	if (o_ptr->art_name)
	{
		discount = 0;
	}

	o_ptr->discount = discount;
	o_ptr->number = size - (size * discount / 100);
	if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
	{
		o_ptr->pval *= (PARAMETER_VALUE)o_ptr->number;
	}
}


/*!
 * @brief 店舗に並べた品を同一品であるかどうか判定する /
 * Determine if a store item can "absorb" another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 同一扱いできるならTRUEを返す
 * @details
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
static bool store_object_similar(object_type *o_ptr, object_type *j_ptr)
{
	if (o_ptr == j_ptr) return 0;
	if (o_ptr->k_idx != j_ptr->k_idx) return 0;
	if ((o_ptr->pval != j_ptr->pval) && (o_ptr->tval != TV_WAND) && (o_ptr->tval != TV_ROD)) return 0;
	if (o_ptr->to_h != j_ptr->to_h) return 0;
	if (o_ptr->to_d != j_ptr->to_d) return 0;
	if (o_ptr->to_a != j_ptr->to_a) return 0;
	if (o_ptr->name2 != j_ptr->name2) return 0;
	if (object_is_artifact(o_ptr) || object_is_artifact(j_ptr)) return 0;
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		if (o_ptr->art_flags[i] != j_ptr->art_flags[i]) return 0;
	if (o_ptr->xtra1 || j_ptr->xtra1) return 0;
	if (o_ptr->timeout || j_ptr->timeout) return 0;
	if (o_ptr->ac != j_ptr->ac)   return 0;
	if (o_ptr->dd != j_ptr->dd)   return 0;
	if (o_ptr->ds != j_ptr->ds)   return 0;
	if (o_ptr->tval == TV_CHEST) return 0;
	if (o_ptr->tval == TV_STATUE) return 0;
	if (o_ptr->tval == TV_CAPTURE) return 0;
	if (o_ptr->discount != j_ptr->discount) return 0;
	return TRUE;
}


/*!
 * @brief 店舗に並べた品を重ね合わせできるかどうか判定する /
 * Allow a store item to absorb another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 重ね合わせできるならTRUEを返す
 * @details
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
static void store_object_absorb(object_type *o_ptr, object_type *j_ptr)
{
	int max_num = (o_ptr->tval == TV_ROD) ?
		MIN(99, MAX_SHORT / k_info[o_ptr->k_idx].pval) : 99;
	int total = o_ptr->number + j_ptr->number;
	int diff = (total > max_num) ? total - max_num : 0;
	o_ptr->number = (total > max_num) ? max_num : total;
	if (o_ptr->tval == TV_ROD)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
	}

	if (o_ptr->tval == TV_WAND)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
	}
}


/*!
 * @brief 店舗に品を置くスペースがあるかどうかの判定を返す /
 * Check to see if the shop will be carrying too many objects	-RAK-
 * @param o_ptr 店舗に置きたいオブジェクト構造体の参照ポインタ
 * @return 置き場がないなら0、重ね合わせできるアイテムがあるなら-1、スペースがあるなら1を返す。
 * @details
 * <pre>
 * Note that the shop, just like a player, will not accept things
 * it cannot hold.	Before, one could "nuke" potions this way.
 * Return value is now int:
 *  0 : No space
 * -1 : Can be combined to existing slot.
 *  1 : Cannot be combined but there are empty spaces.
 * </pre>
 */
static int store_check_num(object_type *o_ptr)
{
	object_type *j_ptr;
	if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM))
	{
		bool old_stack_force_notes = stack_force_notes;
		bool old_stack_force_costs = stack_force_costs;

		if (cur_store_num != STORE_HOME)
		{
			stack_force_notes = FALSE;
			stack_force_costs = FALSE;
		}

		for (int i = 0; i < st_ptr->stock_num; i++)
		{
			j_ptr = &st_ptr->stock[i];
			if (object_similar(j_ptr, o_ptr))
			{
				if (cur_store_num != STORE_HOME)
				{
					stack_force_notes = old_stack_force_notes;
					stack_force_costs = old_stack_force_costs;
				}

				return -1;
			}
		}

		if (cur_store_num != STORE_HOME)
		{
			stack_force_notes = old_stack_force_notes;
			stack_force_costs = old_stack_force_costs;
		}
	}
	else
	{
		for (int i = 0; i < st_ptr->stock_num; i++)
		{
			j_ptr = &st_ptr->stock[i];
			if (store_object_similar(j_ptr, o_ptr)) return -1;
		}
	}

	/* Free space is always usable */
	/*
	 * オプション powerup_home が設定されていると
	 * 我が家が 20 ページまで使える
	 */
	if ((cur_store_num == STORE_HOME) && (powerup_home == FALSE))
	{
		if (st_ptr->stock_num < ((st_ptr->stock_size) / 10))
		{
			return 1;
		}
	}
	else
	{
		if (st_ptr->stock_num < st_ptr->stock_size)
		{
			return 1;
		}
	}

	return 0;
}


/*!
 * @brief オブジェクトが祝福されているかの判定を返す /
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが祝福されたアイテムならばTRUEを返す
 */
static bool is_blessed_item(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);
	if (have_flag(flgs, TR_BLESSED)) return TRUE;
	else return FALSE;
}


/*!
 * @brief オブジェクトが所定の店舗で引き取れるかどうかを返す /
 * Determine if the current store will purchase the given item
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが買い取れるならばTRUEを返す
 * @note
 * Note that a shop-keeper must refuse to buy "worthless" items
 */
static bool store_will_buy(object_type *o_ptr)
{
	if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) return TRUE;
	switch (cur_store_num)
	{
	case STORE_GENERAL:
	{
		switch (o_ptr->tval)
		{
		case TV_POTION:
			if (o_ptr->sval != SV_POTION_WATER) return FALSE;

		case TV_WHISTLE:
		case TV_FOOD:
		case TV_LITE:
		case TV_FLASK:
		case TV_SPIKE:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_DIGGING:
		case TV_CLOAK:
		case TV_BOTTLE:
		case TV_FIGURINE:
		case TV_STATUE:
		case TV_CAPTURE:
		case TV_CARD:
			break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_ARMOURY:
	{
		switch (o_ptr->tval)
		{
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
			break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_WEAPON:
	{
		switch (o_ptr->tval)
		{
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		case TV_BOW:
		case TV_DIGGING:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_HISSATSU_BOOK:
			break;
		case TV_HAFTED:
		{
			if (o_ptr->sval == SV_WIZSTAFF) return FALSE;
		}
		break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_TEMPLE:
	{
		switch (o_ptr->tval)
		{
		case TV_LIFE_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_SCROLL:
		case TV_POTION:
		case TV_HAFTED:
		{
			break;
		}
		case TV_FIGURINE:
		case TV_STATUE:
		{
			monster_race *r_ptr = &r_info[o_ptr->pval];
			if (!(r_ptr->flags3 & RF3_EVIL))
			{
				if (r_ptr->flags3 & RF3_GOOD) break;
				if (r_ptr->flags3 & RF3_ANIMAL) break;
				if (my_strchr("?!", r_ptr->d_char)) break;
			}
		}
		case TV_POLEARM:
		case TV_SWORD:
		{
			if (is_blessed_item(o_ptr)) break;
		}
		default:
			return FALSE;
		}

		break;
	}
	case STORE_ALCHEMIST:
	{
		switch (o_ptr->tval)
		{
		case TV_SCROLL:
		case TV_POTION:
			break;
		default:
			return FALSE;
		}

		break;
	}
	case STORE_MAGIC:
	{
		switch (o_ptr->tval)
		{
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_TRUMP_BOOK:
		case TV_ARCANE_BOOK:
		case TV_CRAFT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HEX_BOOK:
		case TV_AMULET:
		case TV_RING:
		case TV_STAFF:
		case TV_WAND:
		case TV_ROD:
		case TV_SCROLL:
		case TV_POTION:
		case TV_FIGURINE:
			break;
		case TV_HAFTED:
		{
			if (o_ptr->sval == SV_WIZSTAFF) break;
			else return FALSE;
		}
		default:
			return FALSE;
		}

		break;
	}
	case STORE_BOOK:
	{
		switch (o_ptr->tval)
		{
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_LIFE_BOOK:
		case TV_TRUMP_BOOK:
		case TV_ARCANE_BOOK:
		case TV_CRAFT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HEX_BOOK:
			break;
		default:
			return FALSE;
		}

		break;
	}
	}

	if (object_value(o_ptr) <= 0) return FALSE;
	return TRUE;
}


/*!
 * @brief 現在の町の指定された店舗のアイテムを整理する /
 * Combine and reorder items in store.
 * @param store_num 店舗ID
 * @return 実際に整理が行われたならばTRUEを返す。
 */
bool combine_and_reorder_home(int store_num)
{
	store_type *old_st_ptr = st_ptr;
	st_ptr = &town_info[1].store[store_num];
	bool flag = FALSE;
	if (store_num != STORE_HOME)
	{
		stack_force_notes = FALSE;
		stack_force_costs = FALSE;
	}

	bool combined = TRUE;
	while (combined)
	{
		combined = FALSE;
		for (int i = st_ptr->stock_num - 1; i > 0; i--)
		{
			object_type *o_ptr;
			o_ptr = &st_ptr->stock[i];
			if (!o_ptr->k_idx) continue;
			for (int j = 0; j < i; j++)
			{
				object_type *j_ptr;
				j_ptr = &st_ptr->stock[j];
				if (!j_ptr->k_idx) continue;

				/*
				 * Get maximum number of the stack if these
				 * are similar, get zero otherwise.
				 */
				int max_num = object_similar_part(j_ptr, o_ptr);
				if (max_num == 0 || j_ptr->number >= max_num) continue;

				if (o_ptr->number + j_ptr->number <= max_num)
				{
					object_absorb(j_ptr, o_ptr);
					st_ptr->stock_num--;
					int k;
					for (k = i; k < st_ptr->stock_num; k++)
					{
						st_ptr->stock[k] = st_ptr->stock[k + 1];
					}

					object_wipe(&st_ptr->stock[k]);
					combined = TRUE;
					break;
				}

				ITEM_NUMBER old_num = o_ptr->number;
				ITEM_NUMBER remain = j_ptr->number + o_ptr->number - max_num;
				object_absorb(j_ptr, o_ptr);
				o_ptr->number = remain;
				if (o_ptr->tval == TV_ROD)
				{
					o_ptr->pval = o_ptr->pval * remain / old_num;
					o_ptr->timeout = o_ptr->timeout * remain / old_num;
				}
				else if (o_ptr->tval == TV_WAND)
				{
					o_ptr->pval = o_ptr->pval * remain / old_num;
				}

				combined = TRUE;
				break;
			}
		}

		flag |= combined;
	}

	for (int i = 0; i < st_ptr->stock_num; i++)
	{
		object_type *o_ptr;
		o_ptr = &st_ptr->stock[i];
		if (!o_ptr->k_idx) continue;

		s32b o_value = object_value(o_ptr);
		int j;
		for (j = 0; j < st_ptr->stock_num; j++)
		{
			if (object_sort_comp(o_ptr, o_value, &st_ptr->stock[j])) break;
		}

		if (j >= i) continue;

		flag = TRUE;
		object_type *j_ptr;
		object_type forge;
		j_ptr = &forge;
		object_copy(j_ptr, &st_ptr->stock[i]);
		for (int k = i; k > j; k--)
		{
			object_copy(&st_ptr->stock[k], &st_ptr->stock[k - 1]);
		}

		object_copy(&st_ptr->stock[j], j_ptr);
	}

	st_ptr = old_st_ptr;
	bool old_stack_force_notes = stack_force_notes;
	bool old_stack_force_costs = stack_force_costs;
	if (store_num != STORE_HOME)
	{
		stack_force_notes = old_stack_force_notes;
		stack_force_costs = old_stack_force_costs;
	}

	return flag;
}


/*!
 * @brief 我が家にオブジェクトを加える /
 * Add the item "o_ptr" to the inventory of the "Home"
 * @param o_ptr 加えたいオブジェクトの構造体参照ポインタ
 * @return 収めた先のID
 * @details
 * <pre>
 * In all cases, return the slot (or -1) where the object was placed
 * Note that this is a hacked up version of "inven_carry()".
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 * </pre>
 */
static int home_carry(player_type *player_ptr, object_type *o_ptr)
{
	if (cur_store_num != STORE_HOME)
	{
		stack_force_notes = FALSE;
		stack_force_costs = FALSE;
	}

	bool old_stack_force_notes = stack_force_notes;
	bool old_stack_force_costs = stack_force_costs;
	for (int slot = 0; slot < st_ptr->stock_num; slot++)
	{
		object_type *j_ptr;
		j_ptr = &st_ptr->stock[slot];
		if (object_similar(j_ptr, o_ptr))
		{
			object_absorb(j_ptr, o_ptr);
			if (cur_store_num != STORE_HOME)
			{
				stack_force_notes = old_stack_force_notes;
				stack_force_costs = old_stack_force_costs;
			}

			return (slot);
		}
	}

	if (cur_store_num != STORE_HOME)
	{
		stack_force_notes = old_stack_force_notes;
		stack_force_costs = old_stack_force_costs;
	}

	/* No space? */
	/*
	 * 隠し機能: オプション powerup_home が設定されていると
	 *           我が家が 20 ページまで使える
	 */
	if ((cur_store_num != STORE_HOME) || (powerup_home == TRUE))
	{
		if (st_ptr->stock_num >= st_ptr->stock_size)
		{
			return -1;
		}
	}
	else
	{
		if (st_ptr->stock_num >= ((st_ptr->stock_size) / 10))
		{
			return -1;
		}
	}

	PRICE value = object_value(o_ptr);
	int slot;
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		if (object_sort_comp(o_ptr, value, &st_ptr->stock[slot])) break;
	}

	for (int i = st_ptr->stock_num; i > slot; i--)
	{
		st_ptr->stock[i] = st_ptr->stock[i - 1];
	}

	st_ptr->stock_num++;
	st_ptr->stock[slot] = *o_ptr;
	chg_virtue(player_ptr, V_SACRIFICE, -1);
	(void)combine_and_reorder_home(cur_store_num);
	return slot;
}


/*!
 * @brief 店舗にオブジェクトを加える /
 * Add the item "o_ptr" to a real stores inventory.
 * @param o_ptr 加えたいオブジェクトの構造体参照ポインタ
 * @return 収めた先のID
 * @details
 * <pre>
 * In all cases, return the slot (or -1) where the object was placed
 * Note that this is a hacked up version of "inven_carry()".
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 * </pre>
 */
static int store_carry(object_type *o_ptr)
{
	PRICE value = object_value(o_ptr);
	if (value <= 0) return -1;
	o_ptr->ident |= IDENT_FULL_KNOWN;
	o_ptr->inscription = 0;
	o_ptr->feeling = FEEL_NONE;
	int slot;
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		object_type *j_ptr;
		j_ptr = &st_ptr->stock[slot];
		if (store_object_similar(j_ptr, o_ptr))
		{
			store_object_absorb(j_ptr, o_ptr);
			return slot;
		}
	}

	if (st_ptr->stock_num >= st_ptr->stock_size) return -1;

	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		object_type *j_ptr;
		j_ptr = &st_ptr->stock[slot];
		if (o_ptr->tval > j_ptr->tval) break;
		if (o_ptr->tval < j_ptr->tval) continue;
		if (o_ptr->sval < j_ptr->sval) break;
		if (o_ptr->sval > j_ptr->sval) continue;
		if (o_ptr->tval == TV_ROD)
		{
			if (o_ptr->pval < j_ptr->pval) break;
			if (o_ptr->pval > j_ptr->pval) continue;
		}

		PRICE j_value = object_value(j_ptr);
		if (value > j_value) break;
		if (value < j_value) continue;
	}

	for (int i = st_ptr->stock_num; i > slot; i--)
	{
		st_ptr->stock[i] = st_ptr->stock[i - 1];
	}

	st_ptr->stock_num++;
	st_ptr->stock[slot] = *o_ptr;
	return slot;
}


/*!
 * @brief 店舗のオブジェクト数を増やす /
 * Add the item "o_ptr" to a real stores inventory.
 * @param item 増やしたいアイテムのID
 * @param num 増やしたい数
 * @return なし
 * @details
 * <pre>
 * Increase, by a given amount, the number of a certain item
 * in a certain store.	This can result in zero items.
 * </pre>
 * @todo numは本来ITEM_NUMBER型にしたい。
 */
static void store_item_increase(INVENTORY_IDX item, int num)
{
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[item];
	int cnt = o_ptr->number + num;
	if (cnt > 255) cnt = 255;
	else if (cnt < 0) cnt = 0;

	num = cnt - o_ptr->number;
	o_ptr->number += (ITEM_NUMBER)num;
}


/*!
 * @brief 店舗のオブジェクト数を削除する /
 * Remove a slot if it is empty
 * @param item 削除したいアイテムのID
 * @return なし
 */
static void store_item_optimize(INVENTORY_IDX item)
{
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[item];
	if (!o_ptr->k_idx) return;
	if (o_ptr->number) return;

	st_ptr->stock_num--;
	for (int j = item; j < st_ptr->stock_num; j++)
	{
		st_ptr->stock[j] = st_ptr->stock[j + 1];
	}

	object_wipe(&st_ptr->stock[st_ptr->stock_num]);
}


/*!
 * @brief ブラックマーケット用の無価値品の排除判定 /
 * This function will keep 'crap' out of the black market.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return ブラックマーケットにとって無価値な品ならばTRUEを返す
 * @details
 * <pre>
 * Crap is defined as any item that is "available" elsewhere
 * Based on a suggestion by "Lee Vogt" <lvogt@cig.mcel.mot.com>
 * </pre>
 */
static bool black_market_crap(player_type *player_ptr, object_type *o_ptr)
{
	if (object_is_ego(o_ptr)) return FALSE;

	if (o_ptr->to_a > 0) return FALSE;
	if (o_ptr->to_h > 0) return FALSE;
	if (o_ptr->to_d > 0) return FALSE;

	for (int i = 0; i < MAX_STORES; i++)
	{
		if (i == STORE_HOME) continue;
		if (i == STORE_MUSEUM) continue;

		for (int j = 0; j < town_info[player_ptr->town_num].store[i].stock_num; j++)
		{
			object_type *j_ptr = &town_info[player_ptr->town_num].store[i].stock[j];
			if (o_ptr->k_idx == j_ptr->k_idx) return TRUE;
		}
	}

	return FALSE;
}


/*!
 * @brief 店舗の品揃え変化のためにアイテムを削除する /
 * Attempt to delete (some of) a random item from the store
 * @return なし
 * @details
 * <pre>
 * Hack -- we attempt to "maintain" piles of items when possible.
 * </pre>
 */
static void store_delete(void)
{
	INVENTORY_IDX what = (INVENTORY_IDX)randint0(st_ptr->stock_num);
	int num = st_ptr->stock[what].number;
	if (randint0(100) < 50) num = (num + 1) / 2;
	if (randint0(100) < 50) num = 1;
	if ((st_ptr->stock[what].tval == TV_ROD) || (st_ptr->stock[what].tval == TV_WAND))
	{
		st_ptr->stock[what].pval -= num * st_ptr->stock[what].pval / st_ptr->stock[what].number;
	}

	store_item_increase(what, -num);
	store_item_optimize(what);
}


/*!
 * @brief 店舗の品揃え変化のためにアイテムを追加する /
 * Creates a random item and gives it to a store
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 * Should we check for "permission" to have the given item?
 * </pre>
 */
static void store_create(player_type *player_ptr)
{
	if (st_ptr->stock_num >= st_ptr->stock_size) return;

	for (int tries = 0; tries < 4; tries++)
	{
		OBJECT_IDX i;
		DEPTH level;
		if (cur_store_num == STORE_BLACK)
		{
			/* Pick a level for object/magic */
			level = 25 + randint0(25);

			/* Random item (usually of given level) */
			i = get_obj_num(player_ptr, level, 0x00000000);

			/* Handle failure */
			if (i == 0) continue;
		}
		else
		{
			i = st_ptr->table[randint0(st_ptr->table_num)];
			level = rand_range(1, STORE_OBJ_LEVEL);
		}

		object_type forge;
		object_type *q_ptr;
		q_ptr = &forge;
		object_prep(q_ptr, i);
		apply_magic(player_ptr, q_ptr, level, AM_NO_FIXED_ART);
		if (!store_will_buy(q_ptr)) continue;

		if (q_ptr->tval == TV_LITE)
		{
			if (q_ptr->sval == SV_LITE_TORCH) q_ptr->xtra4 = FUEL_TORCH / 2;
			if (q_ptr->sval == SV_LITE_LANTERN) q_ptr->xtra4 = FUEL_LAMP / 2;
		}

		object_known(q_ptr);
		q_ptr->ident |= IDENT_STORE;
		if (q_ptr->tval == TV_CHEST) continue;

		if (cur_store_num == STORE_BLACK)
		{
			if (black_market_crap(player_ptr, q_ptr)) continue;
			if (object_value(q_ptr) < 10) continue;
		}
		else
		{
			if (object_value(q_ptr) <= 0) continue;
		}

		mass_produce(q_ptr);
		(void)store_carry(q_ptr);
		break;
	}
}


/*!
 * @brief 店舗の割引対象外にするかどうかを判定 /
 * Eliminate need to bargain if player has haggled well in the past
 * @param minprice アイテムの最低販売価格
 * @return 割引を禁止するならTRUEを返す。
 */
static bool noneedtobargain(PRICE minprice)
{
	PRICE good = st_ptr->good_buy;
	PRICE bad = st_ptr->bad_buy;
	if (minprice < 10L) return TRUE;
	if (good == MAX_SHORT) return TRUE;
	if (good > ((3 * bad) + (5 + (minprice / 50)))) return TRUE;

	return FALSE;
}


/*!
 * @brief 店主の持つプレイヤーに対する売買の良し悪し経験を記憶する /
 * Update the bargain info
 * @param price 実際の取引価格
 * @param minprice 店主の提示した価格
 * @param num 売買数
 * @return なし
 */
static void updatebargain(PRICE price, PRICE minprice, int num)
{
	if (!manual_haggle) return;
	if ((minprice / num) < 10L) return;
	if (price == minprice)
	{
		if (st_ptr->good_buy < MAX_SHORT)
		{
			st_ptr->good_buy++;
		}
	}
	else
	{
		if (st_ptr->bad_buy < MAX_SHORT)
		{
			st_ptr->bad_buy++;
		}
	}
}


/*!
 * @brief 店の商品リストを再表示する /
 * Re-displays a single store entry
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param pos 表示行
 * @return なし
 */
static void display_entry(player_type *player_ptr, int pos)
{
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[pos];
	int i = (pos % store_bottom);

	/* Label it, clear the line --(-- */
	char out_val[160];
	(void)sprintf(out_val, "%c) ", ((i > 25) ? toupper(I2A(i - 26)) : I2A(i)));
	prt(out_val, i + 6, 0);

	int cur_col = 3;
	if (show_item_graph)
	{
		TERM_COLOR a = object_attr(o_ptr);
		SYMBOL_CODE c = object_char(o_ptr);

		Term_queue_bigchar(cur_col, i + 6, a, c, 0, 0);
		if (use_bigtile) cur_col++;

		cur_col += 2;
	}

	/* Describe an item in the home */
	int maxwid = 75;
	if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM))
	{
		maxwid = 75;
		if (show_weights) maxwid -= 10;

		GAME_TEXT o_name[MAX_NLEN];
		object_desc(player_ptr, o_name, o_ptr, 0);
		o_name[maxwid] = '\0';
		c_put_str(tval_to_attr[o_ptr->tval], o_name, i + 6, cur_col);
		if (show_weights)
		{
			WEIGHT wgt = o_ptr->weight;
#ifdef JP
			sprintf(out_val, "%3d.%1d kg", lbtokg1(wgt), lbtokg2(wgt));
			put_str(out_val, i + 6, 67);
#else
			(void)sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
			put_str(out_val, i + 6, 68);
#endif

		}

		return;
	}

	maxwid = 65;
	if (show_weights) maxwid -= 7;

	GAME_TEXT o_name[MAX_NLEN];
	object_desc(player_ptr, o_name, o_ptr, 0);
	o_name[maxwid] = '\0';
	c_put_str(tval_to_attr[o_ptr->tval], o_name, i + 6, cur_col);

	if (show_weights)
	{
		int wgt = o_ptr->weight;
#ifdef JP
		sprintf(out_val, "%3d.%1d", lbtokg1(wgt), lbtokg2(wgt));
		put_str(out_val, i + 6, 60);
#else
		(void)sprintf(out_val, "%3d.%d", wgt / 10, wgt % 10);
		put_str(out_val, i + 6, 61);
#endif

	}

	s32b x;
	if (o_ptr->ident & (IDENT_FIXED))
	{
		x = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, FALSE);
#ifdef JP
		(void)sprintf(out_val, "%9ld固", (long)x);
#else
		(void)sprintf(out_val, "%9ld F", (long)x);
#endif
		put_str(out_val, i + 6, 68);
		return;
	}

	if (!manual_haggle)
	{
		x = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, FALSE);
		if (!noneedtobargain(x)) x += x / 10;

		(void)sprintf(out_val, "%9ld  ", (long)x);
		put_str(out_val, i + 6, 68);
		return;
	}

	x = price_item(player_ptr, o_ptr, ot_ptr->max_inflate, FALSE);
	(void)sprintf(out_val, "%9ld  ", (long)x);
	put_str(out_val, i + 6, 68);
}


/*!
 * @brief 店の商品リストを表示する /
 * Displays a store's inventory -RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * All prices are listed as "per individual object".  -BEN-
 */
static void display_store_inventory(player_type *player_ptr)
{
	int k;
	for (k = 0; k < store_bottom; k++)
	{
		if (store_top + k >= st_ptr->stock_num) break;

		display_entry(player_ptr, store_top + k);
	}

	for (int i = k; i < store_bottom + 1; i++)
		prt("", i + 6, 0);

#ifdef JP
	put_str("          ", 5, 20);
#else
	put_str("        ", 5, 22);
#endif

	if (st_ptr->stock_num > store_bottom)
	{
		prt(_("-続く-", "-more-"), k + 6, 3);
		put_str(format(_("(%dページ)  ", "(Page %d)  "), store_top / store_bottom + 1), 5, _(20, 22));
	}

	if (cur_store_num == STORE_HOME || cur_store_num == STORE_MUSEUM)
	{
		k = st_ptr->stock_size;
		if (cur_store_num == STORE_HOME && !powerup_home) k /= 10;
#ifdef JP
		put_str(format("アイテム数:  %4d/%4d", st_ptr->stock_num, k), 19 + xtra_stock, 27);
#else
		put_str(format("Objects:  %4d/%4d", st_ptr->stock_num, k), 19 + xtra_stock, 30);
#endif
	}
}


/*!
 * @brief プレイヤーの所持金を表示する /
 * Displays players gold					-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 */
static void store_prt_gold(player_type *player_ptr)
{
	prt(_("手持ちのお金: ", "Gold Remaining: "), 19 + xtra_stock, 53);
	char out_val[64];
	sprintf(out_val, "%9ld", (long)player_ptr->au);
	prt(out_val, 19 + xtra_stock, 68);
}


/*!
 * @brief 店舗情報全体を表示するメインルーチン /
 * Displays store (after clearing screen)		-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 */
static void display_store(player_type *player_ptr)
{
	Term_clear();
	if (cur_store_num == STORE_HOME)
	{
		put_str(_("我が家", "Your Home"), 3, 31);
		put_str(_("アイテムの一覧", "Item Description"), 5, 4);
		if (show_weights)
		{
			put_str(_("  重さ", "Weight"), 5, 70);
		}

		store_prt_gold(player_ptr);
		display_store_inventory(player_ptr);
		return;
	}

	if (cur_store_num == STORE_MUSEUM)
	{
		put_str(_("博物館", "Museum"), 3, 31);
		put_str(_("アイテムの一覧", "Item Description"), 5, 4);
		if (show_weights)
		{
			put_str(_("  重さ", "Weight"), 5, 70);
		}

		store_prt_gold(player_ptr);
		display_store_inventory(player_ptr);
		return;
	}

	concptr store_name = (f_name + f_info[cur_store_feat].name);
	concptr owner_name = (ot_ptr->owner_name);
	concptr race_name = race_info[ot_ptr->owner_race].title;
	char buf[80];
	sprintf(buf, "%s (%s)", owner_name, race_name);
	put_str(buf, 3, 10);

	sprintf(buf, "%s (%ld)", store_name, (long)(ot_ptr->max_cost));
	prt(buf, 3, 50);

	put_str(_("商品の一覧", "Item Description"), 5, 5);
	if (show_weights)
	{
		put_str(_("  重さ", "Weight"), 5, 60);
	}

	put_str(_(" 価格", "Price"), 5, 72);
	store_prt_gold(player_ptr);
	display_store_inventory(player_ptr);
}


/*!
 * @brief 店舗からアイテムを選択する /
 * Get the ID of a store item and return its value	-RAK-
 * @param com_val 選択IDを返す参照ポインタ
 * @param pmt メッセージキャプション
 * @param i 選択範囲の最小値
 * @param j 選択範囲の最大値
 * @return 実際に選択したらTRUE、キャンセルしたらFALSE
 */
static int get_stock(COMMAND_CODE *com_val, concptr pmt, int i, int j)
{
	if (repeat_pull(com_val) && (*com_val >= i) && (*com_val <= j))
	{
		return TRUE;
	}

	msg_print(NULL);
	*com_val = (-1);
	char lo = I2A(i);
	char hi = (j > 25) ? toupper(I2A(j - 26)) : I2A(j);
	char out_val[160];
#ifdef JP
	(void)sprintf(out_val, "(%s:%c-%c, ESCで中断) %s",
		(((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) ? "アイテム" : "商品"),
		lo, hi, pmt);
#else
	(void)sprintf(out_val, "(Items %c-%c, ESC to exit) %s",
		lo, hi, pmt);
#endif

	char command;
	while (TRUE)
	{
		if (!get_com(out_val, &command, FALSE)) break;

		COMMAND_CODE k;
		if (islower(command))
			k = A2I(command);
		else if (isupper(command))
			k = A2I(tolower(command)) + 26;
		else
			k = -1;

		if ((k >= i) && (k <= j))
		{
			*com_val = k;
			break;
		}

		bell();
	}

	prt("", 0, 0);
	if (command == ESCAPE) return FALSE;

	repeat_push(*com_val);
	return TRUE;
}


/*!
 * @brief 店主の不満度を増やし、プレイヤーを締め出す判定と処理を行う /
 * Increase the insult counter and get angry if too many -RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int increase_insults(void)
{
	st_ptr->insult_cur++;
	if (st_ptr->insult_cur <= ot_ptr->insult_max) return FALSE;

	say_comment_4();

	st_ptr->insult_cur = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;
	st_ptr->store_open = current_world_ptr->game_turn + TURNS_PER_TICK * TOWN_DAWN / 8 + randint1(TURNS_PER_TICK*TOWN_DAWN / 8);

	return TRUE;
}


/*!
 * @brief 店主の不満度を減らす /
 * Decrease insults 				-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static void decrease_insults(void)
{
	if (st_ptr->insult_cur) st_ptr->insult_cur--;
}


/*!
 * @brief 店主の不満度が増えた場合のみのメッセージを表示する /
 * Have insulted while haggling 			-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int haggle_insults(void)
{
	if (increase_insults()) return TRUE;

	say_comment_5();
	return FALSE;
}

/*
 * Mega-Hack -- Enable "increments"
 */
static bool allow_inc = FALSE;

/*
 * Mega-Hack -- Last "increment" during haggling
 */
static s32b last_inc = 0L;

/*!
 * @brief 交渉価格を確認と認証の是非を行う /
 * Get a haggle
 * @param pmt メッセージ
 * @param poffer 別途価格提示をした場合の値を返す参照ポインタ
 * @param price 現在の交渉価格
 * @param final 最終確定価格ならばTRUE
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int get_haggle(concptr pmt, s32b *poffer, PRICE price, int final)
{
	GAME_TEXT buf[128];
	if (!allow_inc) last_inc = 0L;

	if (final)
	{
		sprintf(buf, _("%s [承諾] ", "%s [accept] "), pmt);
	}
	else if (last_inc < 0)
	{
		sprintf(buf, _("%s [-$%ld] ", "%s [-%ld] "), pmt, (long)(ABS(last_inc)));
	}
	else if (last_inc > 0)
	{
		sprintf(buf, _("%s [+$%ld] ", "%s [+%ld] "), pmt, (long)(ABS(last_inc)));
	}
	else
	{
		sprintf(buf, "%s ", pmt);
	}

	msg_print(NULL);
	GAME_TEXT out_val[160];
	while (TRUE)
	{
		bool res;
		prt(buf, 0, 0);
		strcpy(out_val, "");

		/*
		 * Ask the user for a response.
		 * Don't allow to use numpad as cursor key.
		 */
		res = askfor_aux(out_val, 32, FALSE);
		prt("", 0, 0);
		if (!res) return FALSE;

		concptr p;
		for (p = out_val; *p == ' '; p++) /* loop */;

		if (*p == '\0')
		{
			if (final)
			{
				*poffer = price;
				last_inc = 0L;
				break;
			}

			if (allow_inc && last_inc)
			{
				*poffer += last_inc;
				break;
			}

			msg_print(_("値がおかしいです。", "Invalid response."));
			msg_print(NULL);
		}

		s32b i = atol(p);
		if ((*p == '+' || *p == '-'))
		{
			if (allow_inc)
			{
				*poffer += i;
				last_inc = i;
				break;
			}
		}
		else
		{
			*poffer = i;
			last_inc = 0L;
			break;
		}
	}

	return TRUE;
}


/*!
 * @brief 店主がプレイヤーからの交渉価格を判断する /
 * Receive an offer (from the player)
 * @param pmt メッセージ
 * @param poffer 店主からの交渉価格を返す参照ポインタ
 * @param last_offer 現在の交渉価格
 * @param factor 店主の価格基準倍率
 * @param price アイテムの実価値
 * @param final 最終価格確定ならばTRUE
 * @return プレイヤーの価格に対して不服ならばTRUEを返す /
 * Return TRUE if offer is NOT okay
 */
static bool receive_offer(concptr pmt, s32b *poffer, s32b last_offer, int factor, PRICE price, int final)
{
	while (TRUE)
	{
		if (!get_haggle(pmt, poffer, price, final)) return TRUE;
		if (((*poffer) * factor) >= (last_offer * factor)) break;
		if (haggle_insults()) return TRUE;

		(*poffer) = last_offer;
	}

	return FALSE;
}


/*!
 * @brief プレイヤーが購入する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static bool purchase_haggle(player_type *player_ptr, object_type *o_ptr, s32b *price)
{
	s32b cur_ask = price_item(player_ptr, o_ptr, ot_ptr->max_inflate, FALSE);
	s32b final_ask = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, FALSE);
	int noneed = noneedtobargain(final_ask);
	bool final = FALSE;
	concptr pmt = _("提示価格", "Asking");
	if (noneed || !manual_haggle)
	{
		if (noneed)
		{
			msg_print(_("結局この金額にまとまった。", "You eventually agree upon the price."));
			msg_print(NULL);
		}
		else
		{
			msg_print(_("すんなりとこの金額にまとまった。", "You quickly agree upon the price."));
			msg_print(NULL);
			final_ask += final_ask / 10;
		}

		cur_ask = final_ask;
		pmt = _("最終提示価格", "Final Offer");
		final = TRUE;
	}

	cur_ask *= o_ptr->number;
	final_ask *= o_ptr->number;
	s32b min_per = ot_ptr->haggle_per;
	s32b max_per = min_per * 3;
	s32b last_offer = object_value(o_ptr) * o_ptr->number;
	last_offer = last_offer * (200 - (int)(ot_ptr->max_inflate)) / 100L;
	if (last_offer <= 0) last_offer = 1;

	s32b offer = 0;
	allow_inc = FALSE;
	bool flag = FALSE;
	int annoyed = 0;
	bool cancel = FALSE;
	*price = 0;
	while (!flag)
	{
		bool loop_flag = TRUE;

		while (!flag && loop_flag)
		{
			char out_val[160];
			(void)sprintf(out_val, "%s :  %ld", pmt, (long)cur_ask);
			put_str(out_val, 1, 0);
			cancel = receive_offer(_("提示する金額? ", "What do you offer? "), &offer, last_offer, 1, cur_ask, final);
			if (cancel)
			{
				flag = TRUE;
			}
			else if (offer > cur_ask)
			{
				say_comment_6();
				offer = last_offer;
			}
			else if (offer == cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}
			else
			{
				loop_flag = FALSE;
			}
		}

		if (flag) continue;

		s32b x1 = 100 * (offer - last_offer) / (cur_ask - last_offer);
		if (x1 < min_per)
		{
			if (haggle_insults())
			{
				flag = TRUE;
				cancel = TRUE;
			}
		}
		else if (x1 > max_per)
		{
			x1 = x1 * 3 / 4;
			if (x1 < max_per) x1 = max_per;
		}

		s32b x2 = rand_range(x1 - 2, x1 + 2);
		s32b x3 = ((cur_ask - offer) * x2 / 100L) + 1;
		if (x3 < 0) x3 = 0;
		cur_ask -= x3;

		if (cur_ask < final_ask)
		{
			final = TRUE;
			cur_ask = final_ask;
			pmt = _("最終提示価格", "What do you offer? ");
			annoyed++;
			if (annoyed > 3)
			{
				(void)(increase_insults());
				cancel = TRUE;
				flag = TRUE;
			}
		}
		else if (offer >= cur_ask)
		{
			flag = TRUE;
			*price = offer;
		}

		last_offer = offer;
		allow_inc = TRUE;
		prt("", 1, 0);
		char out_val[160];
		(void)sprintf(out_val, _("前回の提示金額: $%ld", "Your last offer: %ld"), (long)last_offer);
		put_str(out_val, 1, 39);
		say_comment_2(cur_ask, annoyed);
	}

	if (cancel) return TRUE;

	updatebargain(*price, final_ask, o_ptr->number);
	return FALSE;
}


/*!
 * @brief プレイヤーが売却する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static bool sell_haggle(player_type *player_ptr, object_type *o_ptr, s32b *price)
{
	s32b cur_ask = price_item(player_ptr, o_ptr, ot_ptr->max_inflate, TRUE);
	s32b final_ask = price_item(player_ptr, o_ptr, ot_ptr->min_inflate, TRUE);
	int noneed = noneedtobargain(final_ask);
	s32b purse = (s32b)(ot_ptr->max_cost);
	bool final = FALSE;
	concptr pmt = _("提示金額", "Offer");
	if (noneed || !manual_haggle || (final_ask >= purse))
	{
		if (!manual_haggle && !noneed)
		{
			final_ask -= final_ask / 10;
		}

		if (final_ask >= purse)
		{
			msg_print(_("即座にこの金額にまとまった。", "You instantly agree upon the price."));
			msg_print(NULL);
			final_ask = purse;
		}
		else if (noneed)
		{
			msg_print(_("結局この金額にまとまった。", "You eventually agree upon the price."));
			msg_print(NULL);
		}
		else
		{
			msg_print(_("すんなりとこの金額にまとまった。", "You quickly agree upon the price."));
			msg_print(NULL);
		}

		cur_ask = final_ask;
		final = TRUE;
		pmt = _("最終提示金額", "Final Offer");
	}

	cur_ask *= o_ptr->number;
	final_ask *= o_ptr->number;

	s32b min_per = ot_ptr->haggle_per;
	s32b max_per = min_per * 3;
	s32b last_offer = object_value(o_ptr) * o_ptr->number;
	last_offer = last_offer * ot_ptr->max_inflate / 100L;
	s32b offer = 0;
	allow_inc = FALSE;
	bool flag = FALSE;
	bool loop_flag;
	int annoyed = 0;
	bool cancel = FALSE;
	*price = 0;
	while (!flag)
	{
		while (TRUE)
		{
			loop_flag = TRUE;

			char out_val[160];
			(void)sprintf(out_val, "%s :  %ld", pmt, (long)cur_ask);
			put_str(out_val, 1, 0);
			cancel = receive_offer(_("提示する価格? ", "What price do you ask? "),
				&offer, last_offer, -1, cur_ask, final);

			if (cancel)
			{
				flag = TRUE;
			}
			else if (offer < cur_ask)
			{
				say_comment_6();
				offer = last_offer;
			}
			else if (offer == cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}
			else
			{
				loop_flag = FALSE;
			}

			if (flag || !loop_flag) break;
		}

		if (flag) continue;

		s32b x1 = 100 * (last_offer - offer) / (last_offer - cur_ask);
		if (x1 < min_per)
		{
			if (haggle_insults())
			{
				flag = TRUE;
				cancel = TRUE;
			}
		}
		else if (x1 > max_per)
		{
			x1 = x1 * 3 / 4;
			if (x1 < max_per) x1 = max_per;
		}

		s32b x2 = rand_range(x1 - 2, x1 + 2);
		s32b x3 = ((offer - cur_ask) * x2 / 100L) + 1;
		if (x3 < 0) x3 = 0;
		cur_ask += x3;

		if (cur_ask > final_ask)
		{
			cur_ask = final_ask;
			final = TRUE;
			pmt = _("最終提示金額", "Final Offer");

			annoyed++;
			if (annoyed > 3)
			{
				flag = TRUE;
#ifdef JP
				/* 追加 $0 で買い取られてしまうのを防止 By FIRST*/
				cancel = TRUE;
#endif
				(void)(increase_insults());
			}
		}
		else if (offer <= cur_ask)
		{
			flag = TRUE;
			*price = offer;
		}

		last_offer = offer;
		allow_inc = TRUE;
		prt("", 1, 0);
		char out_val[160];
		(void)sprintf(out_val, _("前回の提示価格 $%ld", "Your last bid %ld"), (long)last_offer);
		put_str(out_val, 1, 39);
		say_comment_3(cur_ask, annoyed);
	}

	if (cancel) return TRUE;

	updatebargain(*price, final_ask, o_ptr->number);
	return FALSE;
}


/*!
 * @brief 店からの購入処理のメインルーチン /
 * Buy an item from a store 			-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void store_purchase(player_type *player_ptr)
{
	if (cur_store_num == STORE_MUSEUM)
	{
		msg_print(_("博物館から取り出すことはできません。", "Museum."));
		return;
	}

	if (st_ptr->stock_num <= 0)
	{
		if (cur_store_num == STORE_HOME)
			msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
		else
			msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
		return;
	}

	int i = (st_ptr->stock_num - store_top);
	if (i > store_bottom) i = store_bottom;

	char out_val[160];
#ifdef JP
	/* ブラックマーケットの時は別のメッセージ */
	switch (cur_store_num) {
	case 7:
		sprintf(out_val, "どのアイテムを取りますか? ");
		break;
	case 6:
		sprintf(out_val, "どれ? ");
		break;
	default:
		sprintf(out_val, "どの品物が欲しいんだい? ");
		break;
	}
#else
	if (cur_store_num == STORE_HOME)
	{
		sprintf(out_val, "Which item do you want to take? ");
	}
	else
	{
		sprintf(out_val, "Which item are you interested in? ");
	}
#endif

	COMMAND_CODE item;
	if (!get_stock(&item, out_val, 0, i - 1)) return;

	item = item + store_top;
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[item];
	ITEM_NUMBER amt = 1;
	object_type forge;
	object_type *j_ptr;
	j_ptr = &forge;
	object_copy(j_ptr, o_ptr);

	/*
	 * If a rod or wand, allocate total maximum timeouts or charges
	 * between those purchased and left on the shelf.
	 */
	reduce_charges(j_ptr, o_ptr->number - amt);
	j_ptr->number = amt;
	if (!inven_carry_okay(j_ptr))
	{
		msg_print(_("そんなにアイテムを持てない。", "You cannot carry that many different items."));
		return;
	}

	PRICE best = price_item(player_ptr, j_ptr, ot_ptr->min_inflate, FALSE);
	if (o_ptr->number > 1)
	{
		if ((cur_store_num != STORE_HOME) &&
			(o_ptr->ident & IDENT_FIXED))
		{
			msg_format(_("一つにつき $%ldです。", "That costs %ld gold per item."), (long)(best));
		}

		amt = get_quantity(NULL, o_ptr->number);
		if (amt <= 0) return;
	}

	j_ptr = &forge;
	object_copy(j_ptr, o_ptr);

	/*
	 * If a rod or wand, allocate total maximum timeouts or charges
	 * between those purchased and left on the shelf.
	 */
	reduce_charges(j_ptr, o_ptr->number - amt);
	j_ptr->number = amt;
	if (!inven_carry_okay(j_ptr))
	{
		msg_print(_("ザックにそのアイテムを入れる隙間がない。", "You cannot carry that many items."));
		return;
	}

	int choice;
	COMMAND_CODE item_new;
	PRICE price;
	if (cur_store_num == STORE_HOME)
	{
		bool combined_or_reordered;
		distribute_charges(o_ptr, j_ptr, amt);
		item_new = inven_carry(player_ptr, j_ptr);
		GAME_TEXT o_name[MAX_NLEN];
		object_desc(player_ptr, o_name, &player_ptr->inventory_list[item_new], 0);

		msg_format(_("%s(%c)を取った。", "You have %s (%c)."), o_name, index_to_label(item_new));
		handle_stuff(player_ptr);

		i = st_ptr->stock_num;
		store_item_increase(item, -amt);
		store_item_optimize(item);
		combined_or_reordered = combine_and_reorder_home(STORE_HOME);
		if (i == st_ptr->stock_num)
		{
			if (combined_or_reordered) display_store_inventory(player_ptr);
			else display_entry(player_ptr, item);
		}
		else
		{
			if (st_ptr->stock_num == 0) store_top = 0;
			else if (store_top >= st_ptr->stock_num) store_top -= store_bottom;
			display_store_inventory(player_ptr);

			chg_virtue(player_ptr, V_SACRIFICE, 1);
		}

		return;
	}

	if (o_ptr->ident & (IDENT_FIXED))
	{
		choice = 0;
		price = (best * j_ptr->number);
	}
	else
	{
		GAME_TEXT o_name[MAX_NLEN];
		object_desc(player_ptr, o_name, j_ptr, 0);
		msg_format(_("%s(%c)を購入する。", "Buying %s (%c)."), o_name, I2A(item));
		msg_print(NULL);
		choice = purchase_haggle(player_ptr, j_ptr, &price);
		if (st_ptr->store_open >= current_world_ptr->game_turn) return;
	}

	if (choice != 0) return;
	if (price == (best * j_ptr->number)) o_ptr->ident |= (IDENT_FIXED);
	if (player_ptr->au < price)
	{
		msg_print(_("お金が足りません。", "You do not have enough gold."));
		return;
	}

	say_comment_1(player_ptr);
	if (cur_store_num == STORE_BLACK)
		chg_virtue(player_ptr, V_JUSTICE, -1);
	if ((o_ptr->tval == TV_BOTTLE) && (cur_store_num != STORE_HOME))
		chg_virtue(player_ptr, V_NATURE, -1);

	sound(SOUND_BUY);
	decrease_insults();
	player_ptr->au -= price;
	store_prt_gold(player_ptr);
	object_aware(player_ptr, j_ptr);
	j_ptr->ident &= ~(IDENT_FIXED);
	GAME_TEXT o_name[MAX_NLEN];
	object_desc(player_ptr, o_name, j_ptr, 0);

	msg_format(_("%sを $%ldで購入しました。", "You bought %s for %ld gold."), o_name, (long)price);

	strcpy(record_o_name, o_name);
	record_turn = current_world_ptr->game_turn;

	if (record_buy) exe_write_diary(player_ptr, DIARY_BUY, 0, o_name);
	object_desc(player_ptr, o_name, o_ptr, OD_NAME_ONLY);
	if (record_rand_art && o_ptr->art_name)
		exe_write_diary(player_ptr, DIARY_ART, 0, o_name);

	j_ptr->inscription = 0;
	j_ptr->feeling = FEEL_NONE;
	j_ptr->ident &= ~(IDENT_STORE);
	item_new = inven_carry(player_ptr, j_ptr);

	object_desc(player_ptr, o_name, &player_ptr->inventory_list[item_new], 0);
	msg_format(_("%s(%c)を手に入れた。", "You have %s (%c)."), o_name, index_to_label(item_new));
	autopick_alter_item(player_ptr, item_new, FALSE);
	if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
	{
		o_ptr->pval -= j_ptr->pval;
	}

	handle_stuff(player_ptr);
	i = st_ptr->stock_num;
	store_item_increase(item, -amt);
	store_item_optimize(item);
	if (st_ptr->stock_num == 0)
	{
		if (one_in_(STORE_SHUFFLE))
		{
			char buf[80];
			msg_print(_("店主は引退した。", "The shopkeeper retires."));
			store_shuffle(player_ptr, cur_store_num);

			prt("", 3, 0);
			sprintf(buf, "%s (%s)",
				ot_ptr->owner_name, race_info[ot_ptr->owner_race].title);
			put_str(buf, 3, 10);
			sprintf(buf, "%s (%ld)",
				(f_name + f_info[cur_store_feat].name), (long)(ot_ptr->max_cost));
			prt(buf, 3, 50);
		}
		else
		{
			msg_print(_("店主は新たな在庫を取り出した。", "The shopkeeper brings out some new stock."));
		}

		for (i = 0; i < 10; i++)
		{
			store_maint(player_ptr, player_ptr->town_num, cur_store_num);
		}

		store_top = 0;
		display_store_inventory(player_ptr);
	}
	else if (st_ptr->stock_num != i)
	{
		if (store_top >= st_ptr->stock_num) store_top -= store_bottom;
		display_store_inventory(player_ptr);
	}
	else
	{
		display_entry(player_ptr, item);
	}
}


/*!
 * @brief 店からの売却処理のメインルーチン /
 * Sell an item to the store (or home)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void store_sell(player_type *owner_ptr)
{
	concptr q;
	if (cur_store_num == STORE_HOME)
		q = _("どのアイテムを置きますか? ", "Drop which item? ");
	else if (cur_store_num == STORE_MUSEUM)
		q = _("どのアイテムを寄贈しますか? ", "Give which item? ");
	else
		q = _("どのアイテムを売りますか? ", "Sell which item? ");

	item_tester_hook = store_will_buy;

	/* 我が家でおかしなメッセージが出るオリジナルのバグを修正 */
	concptr s;
	if (cur_store_num == STORE_HOME)
	{
		s = _("置けるアイテムを持っていません。", "You don't have any item to drop.");
	}
	else if (cur_store_num == STORE_MUSEUM)
	{
		s = _("寄贈できるアイテムを持っていません。", "You don't have any item to give.");
	}
	else
	{
		s = _("欲しい物がないですねえ。", "You have nothing that I want.");
	}

	OBJECT_IDX item;
	object_type *o_ptr;
	o_ptr = choose_object(owner_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	if ((item >= INVEN_RARM) && object_is_cursed(o_ptr))
	{
		msg_print(_("ふーむ、どうやらそれは呪われているようだね。", "Hmmm, it seems to be cursed."));
		return;
	}

	int amt = 1;
	if (o_ptr->number > 1)
	{
		amt = get_quantity(NULL, o_ptr->number);
		if (amt <= 0) return;
	}

	object_type forge;
	object_type *q_ptr;
	q_ptr = &forge;
	object_copy(q_ptr, o_ptr);
	q_ptr->number = amt;

	/*
	 * Hack -- If a rod or wand, allocate total maximum
	 * timeouts or charges to those being sold. -LM-
	 */
	if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
	{
		q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
	}

	GAME_TEXT o_name[MAX_NLEN];
	object_desc(owner_ptr, o_name, q_ptr, 0);

	/* Remove any inscription, feeling for stores */
	if ((cur_store_num != STORE_HOME) && (cur_store_num != STORE_MUSEUM))
	{
		q_ptr->inscription = 0;
		q_ptr->feeling = FEEL_NONE;
	}

	/* Is there room in the store (or the home?) */
	if (!store_check_num(q_ptr))
	{
		if (cur_store_num == STORE_HOME)
			msg_print(_("我が家にはもう置く場所がない。", "Your home is full."));

		else if (cur_store_num == STORE_MUSEUM)
			msg_print(_("博物館はもう満杯だ。", "Museum is full."));

		else
			msg_print(_("すいませんが、店にはもう置く場所がありません。", "I have not the room in my store to keep it."));

		return;
	}

	int choice;
	PRICE price, value, dummy;
	if ((cur_store_num != STORE_HOME) && (cur_store_num != STORE_MUSEUM))
	{
		msg_format(_("%s(%c)を売却する。", "Selling %s (%c)."), o_name, index_to_label(item));
		msg_print(NULL);

		choice = sell_haggle(owner_ptr, q_ptr, &price);
		if (st_ptr->store_open >= current_world_ptr->game_turn) return;

		if (choice == 0)
		{
			say_comment_1(owner_ptr);
			sound(SOUND_SELL);
			if (cur_store_num == STORE_BLACK)
				chg_virtue(owner_ptr, V_JUSTICE, -1);

			if ((o_ptr->tval == TV_BOTTLE) && (cur_store_num != STORE_HOME))
				chg_virtue(owner_ptr, V_NATURE, 1);
			decrease_insults();

			owner_ptr->au += price;
			store_prt_gold(owner_ptr);
			dummy = object_value(q_ptr) * q_ptr->number;

			identify_item(owner_ptr, o_ptr);
			q_ptr = &forge;
			object_copy(q_ptr, o_ptr);
			q_ptr->number = amt;
			q_ptr->ident |= IDENT_STORE;

			/*
			 * Hack -- If a rod or wand, let the shopkeeper know just
			 * how many charges he really paid for. -LM-
			 */
			if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
			{
				q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
			}

			value = object_value(q_ptr) * q_ptr->number;
			object_desc(owner_ptr, o_name, q_ptr, 0);
			msg_format(_("%sを $%ldで売却しました。", "You sold %s for %ld gold."), o_name, (long)price);

			if (record_sell) exe_write_diary(owner_ptr, DIARY_SELL, 0, o_name);

			if (!((o_ptr->tval == TV_FIGURINE) && (value > 0)))
			{
				purchase_analyze(owner_ptr, price, value, dummy);
			}

			/*
			 * Hack -- Allocate charges between those wands or rods sold
			 * and retained, unless all are being sold. -LM-
			 */
			distribute_charges(o_ptr, q_ptr, amt);
			q_ptr->timeout = 0;
			inven_item_increase(owner_ptr, item, -amt);
			inven_item_describe(owner_ptr, item);
			if (o_ptr->number > 0)
				autopick_alter_item(owner_ptr, item, FALSE);

			inven_item_optimize(owner_ptr, item);
			handle_stuff(owner_ptr);
			int item_pos = store_carry(q_ptr);
			if (item_pos >= 0)
			{
				store_top = (item_pos / store_bottom) * store_bottom;
				display_store_inventory(owner_ptr);
			}
		}
	}
	else if (cur_store_num == STORE_MUSEUM)
	{
		char o2_name[MAX_NLEN];
		object_desc(owner_ptr, o2_name, q_ptr, OD_NAME_ONLY);

		if (-1 == store_check_num(q_ptr))
		{
			msg_print(_("それと同じ品物は既に博物館にあるようです。", "The Museum already has one of those items."));
		}
		else
		{
			msg_print(_("博物館に寄贈したものは取り出すことができません！！", "You cannot take back items which have been donated to the Museum!!"));
		}

		if (!get_check(format(_("本当に%sを寄贈しますか？", "Really give %s to the Museum? "), o2_name))) return;

		identify_item(owner_ptr, q_ptr);
		q_ptr->ident |= IDENT_FULL_KNOWN;

		distribute_charges(o_ptr, q_ptr, amt);
		msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));
		choice = 0;

		vary_item(owner_ptr, item, -amt);
		handle_stuff(owner_ptr);

		int item_pos = home_carry(owner_ptr, q_ptr);
		if (item_pos >= 0)
		{
			store_top = (item_pos / store_bottom) * store_bottom;
			display_store_inventory(owner_ptr);
		}
	}
	else
	{
		distribute_charges(o_ptr, q_ptr, amt);
		msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));
		choice = 0;
		vary_item(owner_ptr, item, -amt);
		handle_stuff(owner_ptr);
		int item_pos = home_carry(owner_ptr, q_ptr);
		if (item_pos >= 0)
		{
			store_top = (item_pos / store_bottom) * store_bottom;
			display_store_inventory(owner_ptr);
		}
	}

	if ((choice == 0) && (item >= INVEN_RARM))
	{
		calc_android_exp(owner_ptr);
		verify_equip_slot(owner_ptr, item);
	}
}


/*!
 * @brief 店のアイテムを調べるコマンドのメインルーチン /
 * Examine an item in a store			   -JDL-
 * @return なし
 */
static void store_examine(player_type *player_ptr)
{
	if (st_ptr->stock_num <= 0)
	{
		if (cur_store_num == STORE_HOME)
			msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
		else if (cur_store_num == STORE_MUSEUM)
			msg_print(_("博物館には何も置いてありません。", "Museum is empty."));
		else
			msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
		return;
	}

	int i = (st_ptr->stock_num - store_top);
	if (i > store_bottom) i = store_bottom;

	char out_val[160];
	sprintf(out_val, _("どれを調べますか？", "Which item do you want to examine? "));

	COMMAND_CODE item;
	if (!get_stock(&item, out_val, 0, i - 1)) return;
	item = item + store_top;
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[item];
	if (!OBJECT_IS_FULL_KNOWN(o_ptr))
	{
		msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
		return;
	}

	GAME_TEXT o_name[MAX_NLEN];
	object_desc(player_ptr, o_name, o_ptr, 0);
	msg_format(_("%sを調べている...", "Examining %s..."), o_name);

	if (!screen_object(player_ptr, o_ptr, SCROBJ_FORCE_DETAIL))
		msg_print(_("特に変わったところはないようだ。", "You see nothing special."));

	return;
}


/*!
 * @brief 博物館のアイテムを除去するコマンドのメインルーチン /
 * Remove an item from museum (Originally from TOband)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void museum_remove_object(player_type *player_ptr)
{
	if (st_ptr->stock_num <= 0)
	{
		msg_print(_("博物館には何も置いてありません。", "Museum is empty."));
		return;
	}

	int i = st_ptr->stock_num - store_top;
	if (i > store_bottom) i = store_bottom;

	char out_val[160];
	sprintf(out_val, _("どのアイテムの展示をやめさせますか？", "Which item do you want to order to remove? "));

	COMMAND_CODE item;
	if (!get_stock(&item, out_val, 0, i - 1)) return;

	item = item + store_top;
	object_type *o_ptr;
	o_ptr = &st_ptr->stock[item];

	GAME_TEXT o_name[MAX_NLEN];
	object_desc(player_ptr, o_name, o_ptr, 0);

	msg_print(_("展示をやめさせたアイテムは二度と見ることはできません！", "Once removed from the Museum, an item will be gone forever!"));
	if (!get_check(format(_("本当に%sの展示をやめさせますか？", "Really order to remove %s from the Museum? "), o_name))) return;

	msg_format(_("%sの展示をやめさせた。", "You ordered to remove %s."), o_name);

	store_item_increase(item, -o_ptr->number);
	store_item_optimize(item);

	(void)combine_and_reorder_home(STORE_MUSEUM);
	if (st_ptr->stock_num == 0) store_top = 0;

	else if (store_top >= st_ptr->stock_num) store_top -= store_bottom;
	display_store_inventory(player_ptr);
}


/*
 * Hack -- set this to leave the store
 */
static bool leave_store = FALSE;


/*!
 * @brief 店舗処理コマンド選択のメインルーチン /
 * Process a command in a store
 * @param client_ptr 顧客となるクリーチャーの参照ポインタ
 * @return なし
 * @note
 * <pre>
 * Note that we must allow the use of a few "special" commands
 * in the stores which are not allowed in the dungeon, and we
 * must disable some commands which are allowed in the dungeon
 * but not in the stores, to prevent chaos.
 * </pre>
 */
static void store_process_command(player_type *client_ptr)
{
	repeat_check();
	if (rogue_like_commands && command_cmd == 'l')
	{
		command_cmd = 'x';
	}

	switch (command_cmd)
	{
	case ESCAPE:
	{
		leave_store = TRUE;
		break;
	}
	case '-':
	{
		/* 日本語版追加 */
		/* 1 ページ戻るコマンド: 我が家のページ数が多いので重宝するはず By BUG */
		if (st_ptr->stock_num <= store_bottom) {
			msg_print(_("これで全部です。", "Entire inventory is shown."));
		}
		else {
			store_top -= store_bottom;
			if (store_top < 0)
				store_top = ((st_ptr->stock_num - 1) / store_bottom) * store_bottom;
			if ((cur_store_num == STORE_HOME) && (powerup_home == FALSE))
				if (store_top >= store_bottom) store_top = store_bottom;
			display_store_inventory(client_ptr);
		}

		break;
	}
	case ' ':
	{
		if (st_ptr->stock_num <= store_bottom)
		{
			msg_print(_("これで全部です。", "Entire inventory is shown."));
		}
		else
		{
			store_top += store_bottom;
			/*
			 * 隠しオプション(powerup_home)がセットされていないときは
			 * 我が家では 2 ページまでしか表示しない
			 */
			if ((cur_store_num == STORE_HOME) &&
				(powerup_home == FALSE) &&
				(st_ptr->stock_num >= STORE_INVEN_MAX))
			{
				if (store_top >= (STORE_INVEN_MAX - 1))
				{
					store_top = 0;
				}
			}
			else
			{
				if (store_top >= st_ptr->stock_num) store_top = 0;
			}

			display_store_inventory(client_ptr);
		}

		break;
	}
	case KTRL('R'):
	{
		do_cmd_redraw(client_ptr);
		display_store(client_ptr);
		break;
	}
	case 'g':
	{
		store_purchase(client_ptr);
		break;
	}
	case 'd':
	{
		store_sell(client_ptr);
		break;
	}
	case 'x':
	{
		store_examine(client_ptr);
		break;
	}
	case '\r':
	{
		break;
	}
	case 'w':
	{
		do_cmd_wield(client_ptr);
		break;
	}
	case 't':
	{
		do_cmd_takeoff(client_ptr);
		break;
	}
	case 'k':
	{
		do_cmd_destroy(client_ptr);
		break;
	}
	case 'e':
	{
		do_cmd_equip(client_ptr);
		break;
	}
	case 'i':
	{
		do_cmd_inven(client_ptr);
		break;
	}
	case 'I':
	{
		do_cmd_observe(client_ptr);
		break;
	}
	case KTRL('I'):
	{
		toggle_inventory_equipment(client_ptr);
		break;
	}
	case 'b':
	{
		if ((client_ptr->pclass == CLASS_MINDCRAFTER) ||
			(client_ptr->pclass == CLASS_BERSERKER) ||
			(client_ptr->pclass == CLASS_NINJA) ||
			(client_ptr->pclass == CLASS_MIRROR_MASTER)
			) do_cmd_mind_browse(client_ptr);
		else if (client_ptr->pclass == CLASS_SMITH)
			do_cmd_kaji(client_ptr, TRUE);
		else if (client_ptr->pclass == CLASS_MAGIC_EATER)
			do_cmd_magic_eater(client_ptr, TRUE, FALSE);
		else if (client_ptr->pclass == CLASS_SNIPER)
			do_cmd_snipe_browse(client_ptr);
		else do_cmd_browse(client_ptr);
		break;
	}
	case '{':
	{
		do_cmd_inscribe(client_ptr);
		break;
	}
	case '}':
	{
		do_cmd_uninscribe(client_ptr);
		break;
	}
	case '?':
	{
		do_cmd_help(client_ptr);
		break;
	}
	case '/':
	{
		do_cmd_query_symbol(client_ptr);
		break;
	}
	case 'C':
	{
		client_ptr->town_num = old_town_num;
		do_cmd_player_status(client_ptr);
		client_ptr->town_num = inner_town_num;
		display_store(client_ptr);
		break;
	}
	case '!':
	{
		(void)Term_user(0);
		break;
	}
	case '"':
	{
		client_ptr->town_num = old_town_num;
		do_cmd_pref(client_ptr);
		client_ptr->town_num = inner_town_num;
		break;
	}
	case '@':
	{
		client_ptr->town_num = old_town_num;
		do_cmd_macros(client_ptr);
		client_ptr->town_num = inner_town_num;
		break;
	}
	case '%':
	{
		client_ptr->town_num = old_town_num;
		do_cmd_visuals(client_ptr);
		client_ptr->town_num = inner_town_num;
		break;
	}
	case '&':
	{
		client_ptr->town_num = old_town_num;
		do_cmd_colors(client_ptr);
		client_ptr->town_num = inner_town_num;
		break;
	}
	case '=':
	{
		do_cmd_options();
		(void)combine_and_reorder_home(STORE_HOME);
		do_cmd_redraw(client_ptr);
		display_store(client_ptr);
		break;
	}
	case ':':
	{
		do_cmd_note();
		break;
	}
	case 'V':
	{
		do_cmd_version();
		break;
	}
	case KTRL('F'):
	{
		do_cmd_feeling(client_ptr);
		break;
	}
	case KTRL('O'):
	{
		do_cmd_message_one();
		break;
	}
	case KTRL('P'):
	{
		do_cmd_messages(0);
		break;
	}
	case '|':
	{
		do_cmd_diary(client_ptr);
		break;
	}
	case '~':
	{
		do_cmd_knowledge(client_ptr);
		break;
	}
	case '(':
	{
		do_cmd_load_screen();
		break;
	}
	case ')':
	{
		do_cmd_save_screen(client_ptr);
		break;
	}
	default:
	{
		if ((cur_store_num == STORE_MUSEUM) && (command_cmd == 'r'))
		{
			museum_remove_object(client_ptr);
		}
		else
		{
			msg_print(_("そのコマンドは店の中では使えません。", "That command does not work in stores."));
		}

		break;
	}
	}
}


/*!
 * @brief 店舗処理全体のメインルーチン /
 * Enter a store, and interact with it. *
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note
 * <pre>
 * Note that we use the standard "request_command()" function
 * to get a command, allowing us to use "command_arg" and all
 * command macros and other nifty stuff, but we use the special
 * "shopping" argument, to force certain commands to be converted
 * into other commands, normally, we convert "p" (pray) and "m"
 * (cast magic) into "g" (get), and "s" (search) into "d" (drop).
 * </pre>
 */
void do_cmd_store(player_type *player_ptr)
{
	if (player_ptr->wild_mode) return;
	TERM_LEN w, h;
	Term_get_size(&w, &h);

	xtra_stock = MIN(14 + 26, ((h > 24) ? (h - 24) : 0));
	store_bottom = MIN_STOCK + xtra_stock;

	grid_type *g_ptr;
	g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];

	if (!cave_have_flag_grid(g_ptr, FF_STORE))
	{
		msg_print(_("ここには店がありません。", "You see no store here."));
		return;
	}

	int which = f_info[g_ptr->feat].subtype;
	old_town_num = player_ptr->town_num;
	if ((which == STORE_HOME) || (which == STORE_MUSEUM)) player_ptr->town_num = 1;
	if (player_ptr->current_floor_ptr->dun_level) player_ptr->town_num = NO_TOWN;
	inner_town_num = player_ptr->town_num;

	if ((town_info[player_ptr->town_num].store[which].store_open >= current_world_ptr->game_turn) ||
		(ironman_shops))
	{
		msg_print(_("ドアに鍵がかかっている。", "The doors are locked."));
		player_ptr->town_num = old_town_num;
		return;
	}

	int maintain_num = (current_world_ptr->game_turn - town_info[player_ptr->town_num].store[which].last_visit) / (TURNS_PER_TICK * STORE_TICKS);
	if (maintain_num > 10)
		maintain_num = 10;
	if (maintain_num)
	{
		for (int i = 0; i < maintain_num; i++)
			store_maint(player_ptr, player_ptr->town_num, which);

		town_info[player_ptr->town_num].store[which].last_visit = current_world_ptr->game_turn;
	}

	forget_lite(player_ptr->current_floor_ptr);
	forget_view(player_ptr->current_floor_ptr);
	current_world_ptr->character_icky = TRUE;
	command_arg = 0;
	command_rep = 0;
	command_new = 0;
	get_com_no_macros = TRUE;
	cur_store_num = which;
	cur_store_feat = g_ptr->feat;
	st_ptr = &town_info[player_ptr->town_num].store[cur_store_num];
	ot_ptr = &owners[cur_store_num][st_ptr->owner];
	store_top = 0;
	play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);
	display_store(player_ptr);
	leave_store = FALSE;

	while (!leave_store)
	{
		prt("", 1, 0);
		clear_from(20 + xtra_stock);
		prt(_(" ESC) 建物から出る", " ESC) Exit from Building."), 21 + xtra_stock, 0);
		if (st_ptr->stock_num > store_bottom)
		{
			prt(_(" -)前ページ", " -) Previous page"), 22 + xtra_stock, 0);
			prt(_(" スペース) 次ページ", " SPACE) Next page"), 23 + xtra_stock, 0);
		}

		if (cur_store_num == STORE_HOME)
		{
			prt(_("g) アイテムを取る", "g) Get an item."), 21 + xtra_stock, 27);
			prt(_("d) アイテムを置く", "d) Drop an item."), 22 + xtra_stock, 27);
			prt(_("x) 家のアイテムを調べる", "x) eXamine an item in the home."), 23 + xtra_stock, 27);
		}
		else if (cur_store_num == STORE_MUSEUM)
		{
			prt(_("d) アイテムを置く", "d) Drop an item."), 21 + xtra_stock, 27);
			prt(_("r) アイテムの展示をやめる", "r) order to Remove an item."), 22 + xtra_stock, 27);
			prt(_("x) 博物館のアイテムを調べる", "x) eXamine an item in the museum."), 23 + xtra_stock, 27);
		}
		else
		{
			prt(_("p) 商品を買う", "p) Purchase an item."), 21 + xtra_stock, 30);
			prt(_("s) アイテムを売る", "s) Sell an item."), 22 + xtra_stock, 30);
			prt(_("x) 商品を調べる", "x) eXamine an item in the shop"), 23 + xtra_stock, 30);
		}

		prt(_("i/e) 持ち物/装備の一覧", "i/e) Inventry/Equipment list"), 21 + xtra_stock, 56);
		if (rogue_like_commands)
		{
			prt(_("w/T) 装備する/はずす", "w/T) Wear/Take off equipment"), 22 + xtra_stock, 56);
		}
		else
		{
			prt(_("w/t) 装備する/はずす", "w/t) Wear/Take off equipment"), 22 + xtra_stock, 56);
		}

		prt(_("コマンド:", "You may: "), 20 + xtra_stock, 0);
		request_command(player_ptr, TRUE);
		store_process_command(player_ptr);

		/*
		 * Hack -- To redraw missiles damage and prices in store
		 * If player's charisma changes, or if player changes a bow, PU_BONUS is set
		 */
		bool need_redraw_store_inv = (player_ptr->update & PU_BONUS) ? TRUE : FALSE;
		current_world_ptr->character_icky = TRUE;
		handle_stuff(player_ptr);
		if (player_ptr->inventory_list[INVEN_PACK].k_idx)
		{
			INVENTORY_IDX item = INVEN_PACK;
			object_type *o_ptr = &player_ptr->inventory_list[item];
			if (cur_store_num != STORE_HOME)
			{
				if (cur_store_num == STORE_MUSEUM)
					msg_print(_("ザックからアイテムがあふれそうなので、あわてて博物館から出た...", "Your pack is so full that you flee the Museum..."));
				else
					msg_print(_("ザックからアイテムがあふれそうなので、あわてて店から出た...", "Your pack is so full that you flee the store..."));

				leave_store = TRUE;
			}
			else if (!store_check_num(o_ptr))
			{
				msg_print(_("ザックからアイテムがあふれそうなので、あわてて家から出た...", "Your pack is so full that you flee your home..."));
				leave_store = TRUE;
			}
			else
			{
				int item_pos;
				object_type forge;
				object_type *q_ptr;
				GAME_TEXT o_name[MAX_NLEN];
				msg_print(_("ザックからアイテムがあふれてしまった！", "Your pack overflows!"));
				q_ptr = &forge;
				object_copy(q_ptr, o_ptr);
				object_desc(player_ptr, o_name, q_ptr, 0);
				msg_format(_("%sが落ちた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));
				vary_item(player_ptr, item, -255);
				handle_stuff(player_ptr);

				item_pos = home_carry(player_ptr, q_ptr);
				if (item_pos >= 0)
				{
					store_top = (item_pos / store_bottom) * store_bottom;
					display_store_inventory(player_ptr);
				}
			}
		}

		if (need_redraw_store_inv) display_store_inventory(player_ptr);

		if (st_ptr->store_open >= current_world_ptr->game_turn) leave_store = TRUE;
	}

	select_floor_music(player_ptr);
	player_ptr->town_num = old_town_num;
	take_turn(player_ptr, 100);
	current_world_ptr->character_icky = FALSE;
	command_new = 0;
	command_see = FALSE;
	get_com_no_macros = FALSE;

	msg_erase();
	Term_clear();

	player_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
	player_ptr->update |= (PU_MONSTERS);
	player_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_EQUIPPY);
	player_ptr->redraw |= (PR_MAP);
	player_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}


/*!
 * @brief 現在の町の店主を交代させる /
 * Shuffle one of the stores.
 * @param which 店舗種類のID
 * @return なし
 */
void store_shuffle(player_type *player_ptr, int which)
{
	if (which == STORE_HOME) return;
	if (which == STORE_MUSEUM) return;

	cur_store_num = which;
	st_ptr = &town_info[player_ptr->town_num].store[cur_store_num];
	int j = st_ptr->owner;
	while (TRUE)
	{
		st_ptr->owner = (byte)randint0(MAX_OWNERS);
		if (j == st_ptr->owner) continue;
		int i;
		for (i = 1; i < max_towns; i++)
		{
			if (i == player_ptr->town_num) continue;
			if (st_ptr->owner == town_info[i].store[cur_store_num].owner) break;
		}

		if (i == max_towns) break;
	}

	ot_ptr = &owners[cur_store_num][st_ptr->owner];
	st_ptr->insult_cur = 0;
	st_ptr->store_open = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;
	for (int i = 0; i < st_ptr->stock_num; i++)
	{
		object_type *o_ptr;
		o_ptr = &st_ptr->stock[i];
		if (object_is_artifact(o_ptr)) continue;

		o_ptr->discount = 50;
		o_ptr->ident &= ~(IDENT_FIXED);
		o_ptr->inscription = quark_add(_("売出中", "on sale"));
	}
}


/*!
 * @brief 店の品揃えを変化させる /
 * Maintain the inventory at the stores.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 * @return なし
 */
void store_maint(player_type *player_ptr, int town_num, int store_num)
{
	cur_store_num = store_num;
	if (store_num == STORE_HOME) return;
	if (store_num == STORE_MUSEUM) return;

	st_ptr = &town_info[town_num].store[store_num];
	ot_ptr = &owners[store_num][st_ptr->owner];
	st_ptr->insult_cur = 0;
	if (store_num == STORE_BLACK)
	{
		for (INVENTORY_IDX j = st_ptr->stock_num - 1; j >= 0; j--)
		{
			object_type *o_ptr = &st_ptr->stock[j];
			if (black_market_crap(player_ptr, o_ptr))
			{
				store_item_increase(j, 0 - o_ptr->number);
				store_item_optimize(j);
			}
		}
	}

	INVENTORY_IDX j = st_ptr->stock_num;
	j = j - randint1(STORE_TURNOVER);
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;
	if (j < 0) j = 0;

	while (st_ptr->stock_num > j)
		store_delete();

	j = st_ptr->stock_num;
	j = j + randint1(STORE_TURNOVER);
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;
	if (j >= st_ptr->stock_size) j = st_ptr->stock_size - 1;

	while (st_ptr->stock_num < j) store_create(player_ptr);
}


/*!
 * @brief 店舗情報を初期化する /
 * Initialize the stores
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 * @return なし
 */
void store_init(int town_num, int store_num)
{
	cur_store_num = store_num;
	st_ptr = &town_info[town_num].store[store_num];
	while (TRUE)
	{
		st_ptr->owner = (byte)randint0(MAX_OWNERS);
		int i;
		for (i = 1; i < max_towns; i++)
		{
			if (i == town_num) continue;
			if (st_ptr->owner == town_info[i].store[store_num].owner) break;
		}

		if (i == max_towns) break;
	}

	ot_ptr = &owners[store_num][st_ptr->owner];

	st_ptr->store_open = 0;
	st_ptr->insult_cur = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;
	st_ptr->stock_num = 0;
	st_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
	for (int k = 0; k < st_ptr->stock_size; k++)
	{
		object_wipe(&st_ptr->stock[k]);
	}
}
