#include "angband.h"
#include "util.h"

#include "player-move.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-skill.h"
#include "floor.h"
#include "floor-events.h"
#include "feature.h"
#include "artifact.h"
#include "avatar.h"
#include "spells-status.h"
#include "object.h"
#include "object-hook.h"
#include "monster.h"
#include "monster-status.h"
#include "monsterrace-hook.h"
#include "mutation.h"
#include "patron.h"
#include "realm-hex.h"
#include "cmd-pet.h"
#include "cmd-spell.h"
#include "dungeon.h"

/*!
 * @var horror_desc
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ（通常時）
 */
static concptr horror_desc[MAX_SAN_HORROR] =
{
#ifdef JP
	"忌まわしい",
	"底知れぬ",
	"ぞっとする",
	"破滅的な",
	"冒涜的な",

	"いやな",
	"恐ろしい",
	"不潔な",
	"容赦のない",
	"おぞましい",

	"地獄の",
	"身の毛もよだつ",
	"地獄の",
	"忌まわしい",
	"悪夢のような",

	"嫌悪を感じる",
	"罰当たりな",
	"恐い",
	"不浄な",
	"言うもおぞましい",
#else
	"abominable",
	"abysmal",
	"appalling",
	"baleful",
	"blasphemous",

	"disgusting",
	"dreadful",
	"filthy",
	"grisly",
	"hideous",

	"hellish",
	"horrible",
	"infernal",
	"loathsome",
	"nightmarish",

	"repulsive",
	"sacrilegious",
	"terrible",
	"unclean",
	"unspeakable",
#endif

};

/*!
 * @var funny_desc
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ(幻覚状態時)
 */
static concptr funny_desc[MAX_SAN_FUNNY] =
{
#ifdef JP
	"間抜けな",
	"滑稽な",
	"ばからしい",
	"無味乾燥な",
	"馬鹿げた",

	"笑える",
	"ばかばかしい",
	"ぶっとんだ",
	"いかした",
	"ポストモダンな",

	"ファンタスティックな",
	"ダダイズム的な",
	"キュビズム的な",
	"宇宙的な",
	"卓越した",

	"理解不能な",
	"ものすごい",
	"驚くべき",
	"信じられない",
	"カオティックな",

	"野性的な",
	"非常識な",
#else
	"silly",
	"hilarious",
	"absurd",
	"insipid",
	"ridiculous",

	"laughable",
	"ludicrous",
	"far-out",
	"groovy",
	"postmodern",

	"fantastic",
	"dadaistic",
	"cubistic",
	"cosmic",
	"awesome",

	"incomprehensible",
	"fabulous",
	"amazing",
	"incredible",
	"chaotic",

	"wild",
	"preposterous",
#endif

};

/*!
 * @var funny_comments
 * @brief ELDRITCH HORROR効果時の幻覚時間延長を示す錯乱表現
 */
static concptr funny_comments[MAX_SAN_COMMENT] =
{
#ifdef JP
	/* nuke me */
	  "最高だぜ！",
	  "うひょー！",
	  "いかすぜ！",
	  "すんばらしい！",
	  "ぶっとびー！"
  #else
	  "Wow, cosmic, man!",
	  "Rad!",
	  "Groovy!",
	  "Cool!",
	  "Far out!"
  #endif

};


/*!
 * @brief 基本必要経験値テーブル /
 * Base experience levels, may be adjusted up for race and/or class
 */
const s32b player_exp[PY_MAX_LEVEL] =
{
	10,
	25,
	45,
	70,
	100,
	140,
	200,
	280,
	380,/*10*/
	500,
	650,
	850,
	1100,
	1400,
	1800,
	2300,
	2900,
	3600,
	4400,/*20*/
	5400,
	6800,
	8400,
	10200,
	12500,
	17500,
	25000,
	35000L,
	50000L,
	75000L,/*30*/
	100000L,
	150000L,
	200000L,
	275000L,
	350000L,
	450000L,
	550000L,
	700000L,
	850000L,
	1000000L,/*40*/
	1250000L,
	1500000L,
	1800000L,
	2100000L,
	2400000L,
	2700000L,
	3000000L,
	3500000L,
	4000000L,
	4500000L,/*50*/
	5000000L
};


/*!
 * @brief 基本必要強化値テーブル(アンドロイド専用)
 */
const s32b player_exp_a[PY_MAX_LEVEL] =
{
	20,
	50,
	100,
	170,
	280,
	430,
	650,
	950,
	1400,/*10*/
	1850,
	2300,
	2900,
	3600,
	4400,
	5400,
	6800,
	8400,
	10400,
	12500,/*20*/
	17500,
	25000,
	35000,
	50000L,
	75000L,
	100000L,
	150000L,
	200000L,
	275000L,
	350000L,/*30*/
	450000L,
	550000L,
	650000L,
	800000L,
	950000L,
	1100000L,
	1250000L,
	1400000L,
	1550000L,
	1700000L,/*40*/
	1900000L,
	2100000L,
	2300000L,
	2550000L,
	2800000L,
	3050000L,
	3300000L,
	3700000L,
	4100000L,
	4500000L,/*50*/
	5000000L
};


/*
 * Return alignment title
 */
concptr your_alignment(void)
{
	if (p_ptr->align > 150) return _("大善", "Lawful");
	else if (p_ptr->align > 50) return _("中善", "Good");
	else if (p_ptr->align > 10) return _("小善", "Neutral Good");
	else if (p_ptr->align > -11) return _("中立", "Neutral");
	else if (p_ptr->align > -51) return _("小悪", "Neutral Evil");
	else if (p_ptr->align > -151) return _("中悪", "Evil");
	else return _("大悪", "Chaotic");
}


/*
 * Return proficiency level of weapons and misc. skills (except riding)
 */
int weapon_exp_level(int weapon_exp)
{
	if (weapon_exp < WEAPON_EXP_BEGINNER) return EXP_LEVEL_UNSKILLED;
	else if (weapon_exp < WEAPON_EXP_SKILLED) return EXP_LEVEL_BEGINNER;
	else if (weapon_exp < WEAPON_EXP_EXPERT) return EXP_LEVEL_SKILLED;
	else if (weapon_exp < WEAPON_EXP_MASTER) return EXP_LEVEL_EXPERT;
	else return EXP_LEVEL_MASTER;
}


/*
 * Return proficiency level of riding
 */
int riding_exp_level(int riding_exp)
{
	if (riding_exp < RIDING_EXP_BEGINNER) return EXP_LEVEL_UNSKILLED;
	else if (riding_exp < RIDING_EXP_SKILLED) return EXP_LEVEL_BEGINNER;
	else if (riding_exp < RIDING_EXP_EXPERT) return EXP_LEVEL_SKILLED;
	else if (riding_exp < RIDING_EXP_MASTER) return EXP_LEVEL_EXPERT;
	else return EXP_LEVEL_MASTER;
}


/*
 * Return proficiency level of spells
 */
int spell_exp_level(int spell_exp)
{
	if (spell_exp < SPELL_EXP_BEGINNER) return EXP_LEVEL_UNSKILLED;
	else if (spell_exp < SPELL_EXP_SKILLED) return EXP_LEVEL_BEGINNER;
	else if (spell_exp < SPELL_EXP_EXPERT) return EXP_LEVEL_SKILLED;
	else if (spell_exp < SPELL_EXP_MASTER) return EXP_LEVEL_EXPERT;
	else return EXP_LEVEL_MASTER;
}

/*!
 * @brief プレイヤーの全ステータスを更新する /
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 * @return なし
 * @details
 * <pre>
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 * </pre>
 */
void calc_bonuses(void)
{
	int i, j, hold;
	int new_speed;
	int default_hand = 0;
	int empty_hands_status = empty_hands(TRUE);
	int extra_blows[2];
	object_type *o_ptr;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	bool omoi = FALSE;
	bool yoiyami = FALSE;
	bool down_saving = FALSE;

#if 0
	bool have_dd_s = FALSE, have_dd_t = FALSE;
#endif
	bool have_sw = FALSE, have_kabe = FALSE;
	bool easy_2weapon = FALSE;
	bool riding_levitation = FALSE;
	OBJECT_IDX this_o_idx, next_o_idx = 0;
	const player_race *tmp_rp_ptr;

	/* Save the old vision stuff */
	bool old_telepathy = p_ptr->telepathy;
	bool old_esp_animal = p_ptr->esp_animal;
	bool old_esp_undead = p_ptr->esp_undead;
	bool old_esp_demon = p_ptr->esp_demon;
	bool old_esp_orc = p_ptr->esp_orc;
	bool old_esp_troll = p_ptr->esp_troll;
	bool old_esp_giant = p_ptr->esp_giant;
	bool old_esp_dragon = p_ptr->esp_dragon;
	bool old_esp_human = p_ptr->esp_human;
	bool old_esp_evil = p_ptr->esp_evil;
	bool old_esp_good = p_ptr->esp_good;
	bool old_esp_nonliving = p_ptr->esp_nonliving;
	bool old_esp_unique = p_ptr->esp_unique;
	bool old_see_inv = p_ptr->see_inv;
	bool old_mighty_throw = p_ptr->mighty_throw;

	/* Current feature under player. */
	feature_type *f_ptr = &f_info[current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].feat];

	/* Save the old armor class */
	ARMOUR_CLASS old_dis_ac = p_ptr->dis_ac;
	ARMOUR_CLASS old_dis_to_a = p_ptr->dis_to_a;


	/* Clear extra blows/shots */
	extra_blows[0] = extra_blows[1] = 0;

	/* Clear the stat modifiers */
	for (i = 0; i < A_MAX; i++) p_ptr->stat_add[i] = 0;


	/* Clear the Displayed/Real armor class */
	p_ptr->dis_ac = p_ptr->ac = 0;

	/* Clear the Displayed/Real Bonuses */
	p_ptr->dis_to_h[0] = p_ptr->to_h[0] = 0;
	p_ptr->dis_to_h[1] = p_ptr->to_h[1] = 0;
	p_ptr->dis_to_d[0] = p_ptr->to_d[0] = 0;
	p_ptr->dis_to_d[1] = p_ptr->to_d[1] = 0;
	p_ptr->dis_to_h_b = p_ptr->to_h_b = 0;
	p_ptr->dis_to_a = p_ptr->to_a = 0;
	p_ptr->to_h_m = 0;
	p_ptr->to_d_m = 0;

	p_ptr->to_m_chance = 0;

	/* Clear the Extra Dice Bonuses */
	p_ptr->to_dd[0] = p_ptr->to_ds[0] = 0;
	p_ptr->to_dd[1] = p_ptr->to_ds[1] = 0;

	/* Start with "normal" speed */
	new_speed = 110;

	/* Start with a single blow per current_world_ptr->game_turn */
	p_ptr->num_blow[0] = 1;
	p_ptr->num_blow[1] = 1;

	/* Start with a single shot per current_world_ptr->game_turn */
	p_ptr->num_fire = 100;

	/* Reset the "xtra" tval */
	p_ptr->tval_xtra = 0;

	/* Reset the "ammo" tval */
	p_ptr->tval_ammo = 0;

	/* Clear all the flags */
	p_ptr->cursed = 0L;
	p_ptr->bless_blade = FALSE;
	p_ptr->xtra_might = FALSE;
	p_ptr->impact[0] = FALSE;
	p_ptr->impact[1] = FALSE;
	p_ptr->pass_wall = FALSE;
	p_ptr->kill_wall = FALSE;
	p_ptr->dec_mana = FALSE;
	p_ptr->easy_spell = FALSE;
	p_ptr->heavy_spell = FALSE;
	p_ptr->see_inv = FALSE;
	p_ptr->free_act = FALSE;
	p_ptr->slow_digest = FALSE;
	p_ptr->regenerate = FALSE;
	p_ptr->can_swim = FALSE;
	p_ptr->levitation = FALSE;
	p_ptr->hold_exp = FALSE;
	p_ptr->telepathy = FALSE;
	p_ptr->esp_animal = FALSE;
	p_ptr->esp_undead = FALSE;
	p_ptr->esp_demon = FALSE;
	p_ptr->esp_orc = FALSE;
	p_ptr->esp_troll = FALSE;
	p_ptr->esp_giant = FALSE;
	p_ptr->esp_dragon = FALSE;
	p_ptr->esp_human = FALSE;
	p_ptr->esp_evil = FALSE;
	p_ptr->esp_good = FALSE;
	p_ptr->esp_nonliving = FALSE;
	p_ptr->esp_unique = FALSE;
	p_ptr->lite = FALSE;
	p_ptr->sustain_str = FALSE;
	p_ptr->sustain_int = FALSE;
	p_ptr->sustain_wis = FALSE;
	p_ptr->sustain_con = FALSE;
	p_ptr->sustain_dex = FALSE;
	p_ptr->sustain_chr = FALSE;
	p_ptr->resist_acid = FALSE;
	p_ptr->resist_elec = FALSE;
	p_ptr->resist_fire = FALSE;
	p_ptr->resist_cold = FALSE;
	p_ptr->resist_pois = FALSE;
	p_ptr->resist_conf = FALSE;
	p_ptr->resist_sound = FALSE;
	p_ptr->resist_lite = FALSE;
	p_ptr->resist_dark = FALSE;
	p_ptr->resist_chaos = FALSE;
	p_ptr->resist_disen = FALSE;
	p_ptr->resist_shard = FALSE;
	p_ptr->resist_nexus = FALSE;
	p_ptr->resist_blind = FALSE;
	p_ptr->resist_neth = FALSE;
	p_ptr->resist_time = FALSE;
	p_ptr->resist_water = FALSE;
	p_ptr->resist_fear = FALSE;
	p_ptr->reflect = FALSE;
	p_ptr->sh_fire = FALSE;
	p_ptr->sh_elec = FALSE;
	p_ptr->sh_cold = FALSE;
	p_ptr->anti_magic = FALSE;
	p_ptr->anti_tele = FALSE;
	p_ptr->warning = FALSE;
	p_ptr->mighty_throw = FALSE;
	p_ptr->see_nocto = FALSE;

	p_ptr->immune_acid = FALSE;
	p_ptr->immune_elec = FALSE;
	p_ptr->immune_fire = FALSE;
	p_ptr->immune_cold = FALSE;

	p_ptr->ryoute = FALSE;
	p_ptr->migite = FALSE;
	p_ptr->hidarite = FALSE;
	p_ptr->no_flowed = FALSE;

	if (p_ptr->mimic_form) tmp_rp_ptr = &mimic_info[p_ptr->mimic_form];
	else tmp_rp_ptr = &race_info[p_ptr->prace];

	/* Base infravision (purely racial) */
	p_ptr->see_infra = tmp_rp_ptr->infra;

	/* Base skill -- disarming */
	p_ptr->skill_dis = tmp_rp_ptr->r_dis + cp_ptr->c_dis + ap_ptr->a_dis;

	/* Base skill -- magic devices */
	p_ptr->skill_dev = tmp_rp_ptr->r_dev + cp_ptr->c_dev + ap_ptr->a_dev;

	/* Base skill -- saving throw */
	p_ptr->skill_sav = tmp_rp_ptr->r_sav + cp_ptr->c_sav + ap_ptr->a_sav;

	/* Base skill -- stealth */
	p_ptr->skill_stl = tmp_rp_ptr->r_stl + cp_ptr->c_stl + ap_ptr->a_stl;

	/* Base skill -- searching ability */
	p_ptr->skill_srh = tmp_rp_ptr->r_srh + cp_ptr->c_srh + ap_ptr->a_srh;

	/* Base skill -- searching frequency */
	p_ptr->skill_fos = tmp_rp_ptr->r_fos + cp_ptr->c_fos + ap_ptr->a_fos;

	/* Base skill -- combat (normal) */
	p_ptr->skill_thn = tmp_rp_ptr->r_thn + cp_ptr->c_thn + ap_ptr->a_thn;

	/* Base skill -- combat (shooting) */
	p_ptr->skill_thb = tmp_rp_ptr->r_thb + cp_ptr->c_thb + ap_ptr->a_thb;

	/* Base skill -- combat (throwing) */
	p_ptr->skill_tht = tmp_rp_ptr->r_thb + cp_ptr->c_thb + ap_ptr->a_thb;

	/* Base skill -- digging */
	p_ptr->skill_dig = 0;

	if (has_melee_weapon(INVEN_RARM)) p_ptr->migite = TRUE;
	if (has_melee_weapon(INVEN_LARM))
	{
		p_ptr->hidarite = TRUE;
		if (!p_ptr->migite) default_hand = 1;
	}

	if (CAN_TWO_HANDS_WIELDING())
	{
		if (p_ptr->migite && (empty_hands(FALSE) == EMPTY_HAND_LARM) &&
			object_allow_two_hands_wielding(&p_ptr->inventory_list[INVEN_RARM]))
		{
			p_ptr->ryoute = TRUE;
		}
		else if (p_ptr->hidarite && (empty_hands(FALSE) == EMPTY_HAND_RARM) &&
			object_allow_two_hands_wielding(&p_ptr->inventory_list[INVEN_LARM]))
		{
			p_ptr->ryoute = TRUE;
		}
		else
		{
			switch (p_ptr->pclass)
			{
			case CLASS_MONK:
			case CLASS_FORCETRAINER:
			case CLASS_BERSERKER:
				if (empty_hands(FALSE) == (EMPTY_HAND_RARM | EMPTY_HAND_LARM))
				{
					p_ptr->migite = TRUE;
					p_ptr->ryoute = TRUE;
				}
				break;
			}
		}
	}

	if (!p_ptr->migite && !p_ptr->hidarite)
	{
		if (empty_hands_status & EMPTY_HAND_RARM) p_ptr->migite = TRUE;
		else if (empty_hands_status == EMPTY_HAND_LARM)
		{
			p_ptr->hidarite = TRUE;
			default_hand = 1;
		}
	}

	if (p_ptr->special_defense & KAMAE_MASK)
	{
		if (!(empty_hands_status & EMPTY_HAND_RARM))
		{
			set_action(ACTION_NONE);
		}
	}

	switch (p_ptr->pclass)
	{
	case CLASS_WARRIOR:
		if (p_ptr->lev > 29) p_ptr->resist_fear = TRUE;
		if (p_ptr->lev > 44) p_ptr->regenerate = TRUE;
		break;
	case CLASS_PALADIN:
		if (p_ptr->lev > 39) p_ptr->resist_fear = TRUE;
		break;
	case CLASS_CHAOS_WARRIOR:
		if (p_ptr->lev > 29) p_ptr->resist_chaos = TRUE;
		if (p_ptr->lev > 39) p_ptr->resist_fear = TRUE;
		break;
	case CLASS_MINDCRAFTER:
		if (p_ptr->lev > 9) p_ptr->resist_fear = TRUE;
		if (p_ptr->lev > 19) p_ptr->sustain_wis = TRUE;
		if (p_ptr->lev > 29) p_ptr->resist_conf = TRUE;
		if (p_ptr->lev > 39) p_ptr->telepathy = TRUE;
		break;
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		/* Unencumbered Monks become faster every 10 levels */
		if (!(heavy_armor()))
		{
			if (!(prace_is_(RACE_KLACKON) ||
				prace_is_(RACE_SPRITE) ||
				(p_ptr->pseikaku == SEIKAKU_MUNCHKIN)))
				new_speed += (p_ptr->lev) / 10;

			/* Free action if unencumbered at level 25 */
			if (p_ptr->lev > 24)
				p_ptr->free_act = TRUE;
		}
		break;
	case CLASS_SORCERER:
		p_ptr->to_a -= 50;
		p_ptr->dis_to_a -= 50;
		break;
	case CLASS_BARD:
		p_ptr->resist_sound = TRUE;
		break;
	case CLASS_SAMURAI:
		if (p_ptr->lev > 29) p_ptr->resist_fear = TRUE;
		break;
	case CLASS_BERSERKER:
		p_ptr->shero = 1;
		p_ptr->sustain_str = TRUE;
		p_ptr->sustain_dex = TRUE;
		p_ptr->sustain_con = TRUE;
		p_ptr->regenerate = TRUE;
		p_ptr->free_act = TRUE;
		new_speed += 2;
		if (p_ptr->lev > 29) new_speed++;
		if (p_ptr->lev > 39) new_speed++;
		if (p_ptr->lev > 44) new_speed++;
		if (p_ptr->lev > 49) new_speed++;
		p_ptr->to_a += 10 + p_ptr->lev / 2;
		p_ptr->dis_to_a += 10 + p_ptr->lev / 2;
		p_ptr->skill_dig += (100 + p_ptr->lev * 8);
		if (p_ptr->lev > 39) p_ptr->reflect = TRUE;
		p_ptr->redraw |= PR_STATUS;
		break;
	case CLASS_MIRROR_MASTER:
		if (p_ptr->lev > 39) p_ptr->reflect = TRUE;
		break;
	case CLASS_NINJA:
		/* Unencumbered Ninjas become faster every 10 levels */
		if (heavy_armor())
		{
			new_speed -= (p_ptr->lev) / 10;
			p_ptr->skill_stl -= (p_ptr->lev) / 10;
		}
		else if ((!p_ptr->inventory_list[INVEN_RARM].k_idx || p_ptr->migite) &&
			(!p_ptr->inventory_list[INVEN_LARM].k_idx || p_ptr->hidarite))
		{
			new_speed += 3;
			if (!(prace_is_(RACE_KLACKON) ||
				prace_is_(RACE_SPRITE) ||
				(p_ptr->pseikaku == SEIKAKU_MUNCHKIN)))
				new_speed += (p_ptr->lev) / 10;
			p_ptr->skill_stl += (p_ptr->lev) / 10;

			/* Free action if unencumbered at level 25 */
			if (p_ptr->lev > 24)
				p_ptr->free_act = TRUE;
		}
		if ((!p_ptr->inventory_list[INVEN_RARM].k_idx || p_ptr->migite) &&
			(!p_ptr->inventory_list[INVEN_LARM].k_idx || p_ptr->hidarite))
		{
			p_ptr->to_a += p_ptr->lev / 2 + 5;
			p_ptr->dis_to_a += p_ptr->lev / 2 + 5;
		}
		p_ptr->slow_digest = TRUE;
		p_ptr->resist_fear = TRUE;
		if (p_ptr->lev > 19) p_ptr->resist_pois = TRUE;
		if (p_ptr->lev > 24) p_ptr->sustain_dex = TRUE;
		if (p_ptr->lev > 29) p_ptr->see_inv = TRUE;
		if (p_ptr->lev > 44)
		{
			p_ptr->oppose_pois = 1;
			p_ptr->redraw |= PR_STATUS;
		}
		p_ptr->see_nocto = TRUE;
		break;
	}

	/***** Races ****/
	if (p_ptr->mimic_form)
	{
		switch (p_ptr->mimic_form)
		{
		case MIMIC_DEMON:
			p_ptr->hold_exp = TRUE;
			p_ptr->resist_chaos = TRUE;
			p_ptr->resist_neth = TRUE;
			p_ptr->resist_fire = TRUE;
			p_ptr->oppose_fire = 1;
			p_ptr->see_inv = TRUE;
			new_speed += 3;
			p_ptr->redraw |= PR_STATUS;
			p_ptr->to_a += 10;
			p_ptr->dis_to_a += 10;
			break;
		case MIMIC_DEMON_LORD:
			p_ptr->hold_exp = TRUE;
			p_ptr->resist_chaos = TRUE;
			p_ptr->resist_neth = TRUE;
			p_ptr->immune_fire = TRUE;
			p_ptr->resist_acid = TRUE;
			p_ptr->resist_fire = TRUE;
			p_ptr->resist_cold = TRUE;
			p_ptr->resist_elec = TRUE;
			p_ptr->resist_pois = TRUE;
			p_ptr->resist_conf = TRUE;
			p_ptr->resist_disen = TRUE;
			p_ptr->resist_nexus = TRUE;
			p_ptr->resist_fear = TRUE;
			p_ptr->sh_fire = TRUE;
			p_ptr->see_inv = TRUE;
			p_ptr->telepathy = TRUE;
			p_ptr->levitation = TRUE;
			p_ptr->kill_wall = TRUE;
			new_speed += 5;
			p_ptr->to_a += 20;
			p_ptr->dis_to_a += 20;
			break;
		case MIMIC_VAMPIRE:
			p_ptr->resist_dark = TRUE;
			p_ptr->hold_exp = TRUE;
			p_ptr->resist_neth = TRUE;
			p_ptr->resist_cold = TRUE;
			p_ptr->resist_pois = TRUE;
			p_ptr->see_inv = TRUE;
			new_speed += 3;
			p_ptr->to_a += 10;
			p_ptr->dis_to_a += 10;
			if (p_ptr->pclass != CLASS_NINJA) p_ptr->lite = TRUE;
			break;
		}
	}
	else
	{
		switch (p_ptr->prace)
		{
		case RACE_ELF:
			p_ptr->resist_lite = TRUE;
			break;
		case RACE_HOBBIT:
			p_ptr->hold_exp = TRUE;
			break;
		case RACE_GNOME:
			p_ptr->free_act = TRUE;
			break;
		case RACE_DWARF:
			p_ptr->resist_blind = TRUE;
			break;
		case RACE_HALF_ORC:
			p_ptr->resist_dark = TRUE;
			break;
		case RACE_HALF_TROLL:
			p_ptr->sustain_str = TRUE;

			if (p_ptr->lev > 14)
			{
				/* High level trolls heal fast... */
				p_ptr->regenerate = TRUE;

				if (p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_BERSERKER)
				{
					p_ptr->slow_digest = TRUE;
					/* Let's not make Regeneration
					 * a disadvantage for the poor warriors who can
					 * never learn a spell that satisfies hunger (actually
					 * neither can rogues, but half-trolls are not
					 * supposed to play rogues) */
				}
			}
			break;
		case RACE_AMBERITE:
			p_ptr->sustain_con = TRUE;
			p_ptr->regenerate = TRUE;  /* Amberites heal fast... */
			break;
		case RACE_HIGH_ELF:
			p_ptr->resist_lite = TRUE;
			p_ptr->see_inv = TRUE;
			break;
		case RACE_BARBARIAN:
			p_ptr->resist_fear = TRUE;
			break;
		case RACE_HALF_OGRE:
			p_ptr->resist_dark = TRUE;
			p_ptr->sustain_str = TRUE;
			break;
		case RACE_HALF_GIANT:
			p_ptr->sustain_str = TRUE;
			p_ptr->resist_shard = TRUE;
			break;
		case RACE_HALF_TITAN:
			p_ptr->resist_chaos = TRUE;
			break;
		case RACE_CYCLOPS:
			p_ptr->resist_sound = TRUE;
			break;
		case RACE_YEEK:
			p_ptr->resist_acid = TRUE;
			if (p_ptr->lev > 19) p_ptr->immune_acid = TRUE;
			break;
		case RACE_KLACKON:
			p_ptr->resist_conf = TRUE;
			p_ptr->resist_acid = TRUE;

			/* Klackons become faster */
			new_speed += (p_ptr->lev) / 10;
			break;
		case RACE_KOBOLD:
			p_ptr->resist_pois = TRUE;
			break;
		case RACE_NIBELUNG:
			p_ptr->resist_disen = TRUE;
			p_ptr->resist_dark = TRUE;
			break;
		case RACE_DARK_ELF:
			p_ptr->resist_dark = TRUE;
			if (p_ptr->lev > 19) p_ptr->see_inv = TRUE;
			break;
		case RACE_DRACONIAN:
			p_ptr->levitation = TRUE;
			if (p_ptr->lev > 4) p_ptr->resist_fire = TRUE;
			if (p_ptr->lev > 9) p_ptr->resist_cold = TRUE;
			if (p_ptr->lev > 14) p_ptr->resist_acid = TRUE;
			if (p_ptr->lev > 19) p_ptr->resist_elec = TRUE;
			if (p_ptr->lev > 34) p_ptr->resist_pois = TRUE;
			break;
		case RACE_MIND_FLAYER:
			p_ptr->sustain_int = TRUE;
			p_ptr->sustain_wis = TRUE;
			if (p_ptr->lev > 14) p_ptr->see_inv = TRUE;
			if (p_ptr->lev > 29) p_ptr->telepathy = TRUE;
			break;
		case RACE_IMP:
			p_ptr->resist_fire = TRUE;
			if (p_ptr->lev > 9) p_ptr->see_inv = TRUE;
			break;
		case RACE_GOLEM:
			p_ptr->slow_digest = TRUE;
			p_ptr->free_act = TRUE;
			p_ptr->see_inv = TRUE;
			p_ptr->resist_pois = TRUE;
			if (p_ptr->lev > 34) p_ptr->hold_exp = TRUE;
			break;
		case RACE_SKELETON:
			p_ptr->resist_shard = TRUE;
			p_ptr->hold_exp = TRUE;
			p_ptr->see_inv = TRUE;
			p_ptr->resist_pois = TRUE;
			if (p_ptr->lev > 9) p_ptr->resist_cold = TRUE;
			break;
		case RACE_ZOMBIE:
			p_ptr->resist_neth = TRUE;
			p_ptr->hold_exp = TRUE;
			p_ptr->see_inv = TRUE;
			p_ptr->resist_pois = TRUE;
			p_ptr->slow_digest = TRUE;
			if (p_ptr->lev > 4) p_ptr->resist_cold = TRUE;
			break;
		case RACE_VAMPIRE:
			p_ptr->resist_dark = TRUE;
			p_ptr->hold_exp = TRUE;
			p_ptr->resist_neth = TRUE;
			p_ptr->resist_cold = TRUE;
			p_ptr->resist_pois = TRUE;
			if (p_ptr->pclass != CLASS_NINJA) p_ptr->lite = TRUE;
			break;
		case RACE_SPECTRE:
			p_ptr->levitation = TRUE;
			p_ptr->free_act = TRUE;
			p_ptr->resist_neth = TRUE;
			p_ptr->hold_exp = TRUE;
			p_ptr->see_inv = TRUE;
			p_ptr->resist_pois = TRUE;
			p_ptr->slow_digest = TRUE;
			p_ptr->resist_cold = TRUE;
			p_ptr->pass_wall = TRUE;
			if (p_ptr->lev > 34) p_ptr->telepathy = TRUE;
			break;
		case RACE_SPRITE:
			p_ptr->levitation = TRUE;
			p_ptr->resist_lite = TRUE;

			/* Sprites become faster */
			new_speed += (p_ptr->lev) / 10;
			break;
		case RACE_BEASTMAN:
			p_ptr->resist_conf = TRUE;
			p_ptr->resist_sound = TRUE;
			break;
		case RACE_ENT:
			/* Ents dig like maniacs, but only with their hands. */
			if (!p_ptr->inventory_list[INVEN_RARM].k_idx)
				p_ptr->skill_dig += p_ptr->lev * 10;
			/* Ents get tougher and stronger as they age, but lose dexterity. */
			if (p_ptr->lev > 25) p_ptr->stat_add[A_STR]++;
			if (p_ptr->lev > 40) p_ptr->stat_add[A_STR]++;
			if (p_ptr->lev > 45) p_ptr->stat_add[A_STR]++;

			if (p_ptr->lev > 25) p_ptr->stat_add[A_DEX]--;
			if (p_ptr->lev > 40) p_ptr->stat_add[A_DEX]--;
			if (p_ptr->lev > 45) p_ptr->stat_add[A_DEX]--;

			if (p_ptr->lev > 25) p_ptr->stat_add[A_CON]++;
			if (p_ptr->lev > 40) p_ptr->stat_add[A_CON]++;
			if (p_ptr->lev > 45) p_ptr->stat_add[A_CON]++;
			break;
		case RACE_ANGEL:
			p_ptr->levitation = TRUE;
			p_ptr->see_inv = TRUE;
			break;
		case RACE_DEMON:
			p_ptr->resist_fire = TRUE;
			p_ptr->resist_neth = TRUE;
			p_ptr->hold_exp = TRUE;
			if (p_ptr->lev > 9) p_ptr->see_inv = TRUE;
			if (p_ptr->lev > 44)
			{
				p_ptr->oppose_fire = 1;
				p_ptr->redraw |= PR_STATUS;
			}
			break;
		case RACE_DUNADAN:
			p_ptr->sustain_con = TRUE;
			break;
		case RACE_S_FAIRY:
			p_ptr->levitation = TRUE;
			break;
		case RACE_KUTAR:
			p_ptr->resist_conf = TRUE;
			break;
		case RACE_ANDROID:
			p_ptr->slow_digest = TRUE;
			p_ptr->free_act = TRUE;
			p_ptr->resist_pois = TRUE;
			p_ptr->hold_exp = TRUE;
			break;
		case RACE_MERFOLK:
			p_ptr->resist_water = TRUE;
			break;
		default:
			/* Do nothing */
			;
		}
	}

	if (p_ptr->ult_res || (p_ptr->special_defense & KATA_MUSOU))
	{
		p_ptr->see_inv = TRUE;
		p_ptr->free_act = TRUE;
		p_ptr->slow_digest = TRUE;
		p_ptr->regenerate = TRUE;
		p_ptr->levitation = TRUE;
		p_ptr->hold_exp = TRUE;
		p_ptr->telepathy = TRUE;
		p_ptr->lite = TRUE;
		p_ptr->sustain_str = TRUE;
		p_ptr->sustain_int = TRUE;
		p_ptr->sustain_wis = TRUE;
		p_ptr->sustain_con = TRUE;
		p_ptr->sustain_dex = TRUE;
		p_ptr->sustain_chr = TRUE;
		p_ptr->resist_acid = TRUE;
		p_ptr->resist_elec = TRUE;
		p_ptr->resist_fire = TRUE;
		p_ptr->resist_cold = TRUE;
		p_ptr->resist_pois = TRUE;
		p_ptr->resist_conf = TRUE;
		p_ptr->resist_sound = TRUE;
		p_ptr->resist_lite = TRUE;
		p_ptr->resist_dark = TRUE;
		p_ptr->resist_chaos = TRUE;
		p_ptr->resist_disen = TRUE;
		p_ptr->resist_shard = TRUE;
		p_ptr->resist_nexus = TRUE;
		p_ptr->resist_blind = TRUE;
		p_ptr->resist_neth = TRUE;
		p_ptr->resist_fear = TRUE;
		p_ptr->reflect = TRUE;
		p_ptr->sh_fire = TRUE;
		p_ptr->sh_elec = TRUE;
		p_ptr->sh_cold = TRUE;
		p_ptr->to_a += 100;
		p_ptr->dis_to_a += 100;
	}
	/* Temporary shield */
	else if (p_ptr->tsubureru || p_ptr->shield || p_ptr->magicdef)
	{
		p_ptr->to_a += 50;
		p_ptr->dis_to_a += 50;
	}

	if (p_ptr->tim_res_nether)
	{
		p_ptr->resist_neth = TRUE;
	}
	if (p_ptr->tim_sh_fire)
	{
		p_ptr->sh_fire = TRUE;
	}
	if (p_ptr->tim_res_time)
	{
		p_ptr->resist_time = TRUE;
	}

	/* Sexy Gal */
	if (p_ptr->pseikaku == SEIKAKU_SEXY) p_ptr->cursed |= (TRC_AGGRAVATE);

	if (p_ptr->pseikaku == SEIKAKU_NAMAKE) p_ptr->to_m_chance += 10;
	if (p_ptr->pseikaku == SEIKAKU_KIREMONO) p_ptr->to_m_chance -= 3;
	if ((p_ptr->pseikaku == SEIKAKU_GAMAN) || (p_ptr->pseikaku == SEIKAKU_CHIKARA)) p_ptr->to_m_chance++;

	if (p_ptr->pseikaku == SEIKAKU_CHARGEMAN)
	{
		p_ptr->to_m_chance += 5;
		p_ptr->resist_conf = TRUE;
	}

	/* Lucky man */
	if (p_ptr->pseikaku == SEIKAKU_LUCKY) p_ptr->muta3 |= MUT3_GOOD_LUCK;

	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN)
	{
		p_ptr->resist_blind = TRUE;
		p_ptr->resist_conf = TRUE;
		p_ptr->hold_exp = TRUE;
		if (p_ptr->pclass != CLASS_NINJA) p_ptr->lite = TRUE;

		if ((p_ptr->prace != RACE_KLACKON) && (p_ptr->prace != RACE_SPRITE))
			/* Munchkin become faster */
			new_speed += (p_ptr->lev) / 10 + 5;
	}

	if (music_singing(MUSIC_WALL))
	{
		p_ptr->kill_wall = TRUE;
	}

	/* Hack -- apply racial/class stat maxes */
	/* Apply the racial modifiers */
	for (i = 0; i < A_MAX; i++)
	{
		/* Modify the stats for "race" */
		p_ptr->stat_add[i] += (tmp_rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i]);
	}


	/* I'm adding the mutations here for the lack of a better place... */
	if (p_ptr->muta3)
	{
		/* Hyper Strength */
		if (p_ptr->muta3 & MUT3_HYPER_STR)
		{
			p_ptr->stat_add[A_STR] += 4;
		}

		/* Puny */
		if (p_ptr->muta3 & MUT3_PUNY)
		{
			p_ptr->stat_add[A_STR] -= 4;
		}

		/* Living computer */
		if (p_ptr->muta3 & MUT3_HYPER_INT)
		{
			p_ptr->stat_add[A_INT] += 4;
			p_ptr->stat_add[A_WIS] += 4;
		}

		/* Moronic */
		if (p_ptr->muta3 & MUT3_MORONIC)
		{
			p_ptr->stat_add[A_INT] -= 4;
			p_ptr->stat_add[A_WIS] -= 4;
		}

		if (p_ptr->muta3 & MUT3_RESILIENT)
		{
			p_ptr->stat_add[A_CON] += 4;
		}

		if (p_ptr->muta3 & MUT3_XTRA_FAT)
		{
			p_ptr->stat_add[A_CON] += 2;
			new_speed -= 2;
		}

		if (p_ptr->muta3 & MUT3_ALBINO)
		{
			p_ptr->stat_add[A_CON] -= 4;
		}

		if (p_ptr->muta3 & MUT3_FLESH_ROT)
		{
			p_ptr->stat_add[A_CON] -= 2;
			p_ptr->stat_add[A_CHR] -= 1;
			p_ptr->regenerate = FALSE;
			/* Cancel innate regeneration */
		}

		if (p_ptr->muta3 & MUT3_SILLY_VOI)
		{
			p_ptr->stat_add[A_CHR] -= 4;
		}

		if (p_ptr->muta3 & MUT3_BLANK_FAC)
		{
			p_ptr->stat_add[A_CHR] -= 1;
		}

		if (p_ptr->muta3 & MUT3_XTRA_EYES)
		{
			p_ptr->skill_fos += 15;
			p_ptr->skill_srh += 15;
		}

		if (p_ptr->muta3 & MUT3_MAGIC_RES)
		{
			p_ptr->skill_sav += (15 + (p_ptr->lev / 5));
		}

		if (p_ptr->muta3 & MUT3_XTRA_NOIS)
		{
			p_ptr->skill_stl -= 3;
		}

		if (p_ptr->muta3 & MUT3_INFRAVIS)
		{
			p_ptr->see_infra += 3;
		}

		if (p_ptr->muta3 & MUT3_XTRA_LEGS)
		{
			new_speed += 3;
		}

		if (p_ptr->muta3 & MUT3_SHORT_LEG)
		{
			new_speed -= 3;
		}

		if (p_ptr->muta3 & MUT3_ELEC_TOUC)
		{
			p_ptr->sh_elec = TRUE;
		}

		if (p_ptr->muta3 & MUT3_FIRE_BODY)
		{
			p_ptr->sh_fire = TRUE;
			p_ptr->lite = TRUE;
		}

		if (p_ptr->muta3 & MUT3_WART_SKIN)
		{
			p_ptr->stat_add[A_CHR] -= 2;
			p_ptr->to_a += 5;
			p_ptr->dis_to_a += 5;
		}

		if (p_ptr->muta3 & MUT3_SCALES)
		{
			p_ptr->stat_add[A_CHR] -= 1;
			p_ptr->to_a += 10;
			p_ptr->dis_to_a += 10;
		}

		if (p_ptr->muta3 & MUT3_IRON_SKIN)
		{
			p_ptr->stat_add[A_DEX] -= 1;
			p_ptr->to_a += 25;
			p_ptr->dis_to_a += 25;
		}

		if (p_ptr->muta3 & MUT3_WINGS)
		{
			p_ptr->levitation = TRUE;
		}

		if (p_ptr->muta3 & MUT3_FEARLESS)
		{
			p_ptr->resist_fear = TRUE;
		}

		if (p_ptr->muta3 & MUT3_REGEN)
		{
			p_ptr->regenerate = TRUE;
		}

		if (p_ptr->muta3 & MUT3_ESP)
		{
			p_ptr->telepathy = TRUE;
		}

		if (p_ptr->muta3 & MUT3_LIMBER)
		{
			p_ptr->stat_add[A_DEX] += 3;
		}

		if (p_ptr->muta3 & MUT3_ARTHRITIS)
		{
			p_ptr->stat_add[A_DEX] -= 3;
		}

		if (p_ptr->muta3 & MUT3_MOTION)
		{
			p_ptr->free_act = TRUE;
			p_ptr->skill_stl += 1;
		}

		if (p_ptr->muta3 & MUT3_ILL_NORM)
		{
			p_ptr->stat_add[A_CHR] = 0;
		}
	}

	if (p_ptr->tsuyoshi)
	{
		p_ptr->stat_add[A_STR] += 4;
		p_ptr->stat_add[A_CON] += 4;
	}

	/* Scan the usable p_ptr->inventory_list */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		int bonus_to_h, bonus_to_d;
		o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		object_flags(o_ptr, flgs);

		p_ptr->cursed |= (o_ptr->curse_flags & (0xFFFFFFF0L));
		if (o_ptr->name1 == ART_CHAINSWORD) p_ptr->cursed |= TRC_CHAINSWORD;

		/* Affect stats */
		if (have_flag(flgs, TR_STR)) p_ptr->stat_add[A_STR] += o_ptr->pval;
		if (have_flag(flgs, TR_INT)) p_ptr->stat_add[A_INT] += o_ptr->pval;
		if (have_flag(flgs, TR_WIS)) p_ptr->stat_add[A_WIS] += o_ptr->pval;
		if (have_flag(flgs, TR_DEX)) p_ptr->stat_add[A_DEX] += o_ptr->pval;
		if (have_flag(flgs, TR_CON)) p_ptr->stat_add[A_CON] += o_ptr->pval;
		if (have_flag(flgs, TR_CHR)) p_ptr->stat_add[A_CHR] += o_ptr->pval;

		if (have_flag(flgs, TR_MAGIC_MASTERY))    p_ptr->skill_dev += 8 * o_ptr->pval;

		/* Affect stealth */
		if (have_flag(flgs, TR_STEALTH)) p_ptr->skill_stl += o_ptr->pval;

		/* Affect searching ability (factor of five) */
		if (have_flag(flgs, TR_SEARCH)) p_ptr->skill_srh += (o_ptr->pval * 5);

		/* Affect searching frequency (factor of five) */
		if (have_flag(flgs, TR_SEARCH)) p_ptr->skill_fos += (o_ptr->pval * 5);

		/* Affect infravision */
		if (have_flag(flgs, TR_INFRA)) p_ptr->see_infra += o_ptr->pval;

		/* Affect digging (factor of 20) */
		if (have_flag(flgs, TR_TUNNEL)) p_ptr->skill_dig += (o_ptr->pval * 20);

		/* Affect speed */
		if (have_flag(flgs, TR_SPEED)) new_speed += o_ptr->pval;

		/* Affect blows */
		if (have_flag(flgs, TR_BLOWS))
		{
			if ((i == INVEN_RARM || i == INVEN_RIGHT) && !p_ptr->ryoute) extra_blows[0] += o_ptr->pval;
			else if ((i == INVEN_LARM || i == INVEN_LEFT) && !p_ptr->ryoute) extra_blows[1] += o_ptr->pval;
			else { extra_blows[0] += o_ptr->pval; extra_blows[1] += o_ptr->pval; }
		}

		/* Hack -- cause earthquakes */
		if (have_flag(flgs, TR_IMPACT)) p_ptr->impact[(i == INVEN_RARM) ? 0 : 1] = TRUE;

		/* Various flags */
		if (have_flag(flgs, TR_AGGRAVATE))   p_ptr->cursed |= TRC_AGGRAVATE;
		if (have_flag(flgs, TR_DRAIN_EXP))   p_ptr->cursed |= TRC_DRAIN_EXP;
		if (have_flag(flgs, TR_TY_CURSE))    p_ptr->cursed |= TRC_TY_CURSE;
		if (have_flag(flgs, TR_ADD_L_CURSE)) p_ptr->cursed |= TRC_ADD_L_CURSE;
		if (have_flag(flgs, TR_ADD_H_CURSE)) p_ptr->cursed |= TRC_ADD_H_CURSE;
		if (have_flag(flgs, TR_DRAIN_HP))    p_ptr->cursed |= TRC_DRAIN_HP;
		if (have_flag(flgs, TR_DRAIN_MANA))  p_ptr->cursed |= TRC_DRAIN_MANA;
		if (have_flag(flgs, TR_CALL_ANIMAL)) p_ptr->cursed |= TRC_CALL_ANIMAL;
		if (have_flag(flgs, TR_CALL_DEMON))  p_ptr->cursed |= TRC_CALL_DEMON;
		if (have_flag(flgs, TR_CALL_DRAGON)) p_ptr->cursed |= TRC_CALL_DRAGON;
		if (have_flag(flgs, TR_CALL_UNDEAD)) p_ptr->cursed |= TRC_CALL_UNDEAD;
		if (have_flag(flgs, TR_COWARDICE))   p_ptr->cursed |= TRC_COWARDICE;
		if (have_flag(flgs, TR_LOW_MELEE))   p_ptr->cursed |= TRC_LOW_MELEE;
		if (have_flag(flgs, TR_LOW_AC))      p_ptr->cursed |= TRC_LOW_AC;
		if (have_flag(flgs, TR_LOW_MAGIC))   p_ptr->cursed |= TRC_LOW_MAGIC;
		if (have_flag(flgs, TR_FAST_DIGEST)) p_ptr->cursed |= TRC_FAST_DIGEST;
		if (have_flag(flgs, TR_SLOW_REGEN))  p_ptr->cursed |= TRC_SLOW_REGEN;
		if (have_flag(flgs, TR_DEC_MANA))    p_ptr->dec_mana = TRUE;
		if (have_flag(flgs, TR_BLESSED))     p_ptr->bless_blade = TRUE;
		if (have_flag(flgs, TR_XTRA_MIGHT))  p_ptr->xtra_might = TRUE;
		if (have_flag(flgs, TR_SLOW_DIGEST)) p_ptr->slow_digest = TRUE;
		if (have_flag(flgs, TR_REGEN))       p_ptr->regenerate = TRUE;
		if (have_flag(flgs, TR_TELEPATHY))   p_ptr->telepathy = TRUE;
		if (have_flag(flgs, TR_ESP_ANIMAL))  p_ptr->esp_animal = TRUE;
		if (have_flag(flgs, TR_ESP_UNDEAD))  p_ptr->esp_undead = TRUE;
		if (have_flag(flgs, TR_ESP_DEMON))   p_ptr->esp_demon = TRUE;
		if (have_flag(flgs, TR_ESP_ORC))     p_ptr->esp_orc = TRUE;
		if (have_flag(flgs, TR_ESP_TROLL))   p_ptr->esp_troll = TRUE;
		if (have_flag(flgs, TR_ESP_GIANT))   p_ptr->esp_giant = TRUE;
		if (have_flag(flgs, TR_ESP_DRAGON))  p_ptr->esp_dragon = TRUE;
		if (have_flag(flgs, TR_ESP_HUMAN))   p_ptr->esp_human = TRUE;
		if (have_flag(flgs, TR_ESP_EVIL))    p_ptr->esp_evil = TRUE;
		if (have_flag(flgs, TR_ESP_GOOD))    p_ptr->esp_good = TRUE;
		if (have_flag(flgs, TR_ESP_NONLIVING)) p_ptr->esp_nonliving = TRUE;
		if (have_flag(flgs, TR_ESP_UNIQUE))  p_ptr->esp_unique = TRUE;

		if (have_flag(flgs, TR_SEE_INVIS))   p_ptr->see_inv = TRUE;
		if (have_flag(flgs, TR_LEVITATION))     p_ptr->levitation = TRUE;
		if (have_flag(flgs, TR_FREE_ACT))    p_ptr->free_act = TRUE;
		if (have_flag(flgs, TR_HOLD_EXP))   p_ptr->hold_exp = TRUE;
		if (have_flag(flgs, TR_WARNING)) {
			if (!o_ptr->inscription || !(my_strchr(quark_str(o_ptr->inscription), '$')))
				p_ptr->warning = TRUE;
		}

		if (have_flag(flgs, TR_TELEPORT))
		{
			if (object_is_cursed(o_ptr)) p_ptr->cursed |= TRC_TELEPORT;
			else
			{
				concptr insc = quark_str(o_ptr->inscription);

				if (o_ptr->inscription && my_strchr(insc, '.'))
				{
					/*
					 * {.} will stop random teleportation.
					 */
				}
				else
				{
					/* Controlled random teleportation */
					p_ptr->cursed |= TRC_TELEPORT_SELF;
				}
			}
		}

		/* Immunity flags */
		if (have_flag(flgs, TR_IM_FIRE)) p_ptr->immune_fire = TRUE;
		if (have_flag(flgs, TR_IM_ACID)) p_ptr->immune_acid = TRUE;
		if (have_flag(flgs, TR_IM_COLD)) p_ptr->immune_cold = TRUE;
		if (have_flag(flgs, TR_IM_ELEC)) p_ptr->immune_elec = TRUE;

		/* Resistance flags */
		if (have_flag(flgs, TR_RES_ACID))   p_ptr->resist_acid = TRUE;
		if (have_flag(flgs, TR_RES_ELEC))   p_ptr->resist_elec = TRUE;
		if (have_flag(flgs, TR_RES_FIRE))   p_ptr->resist_fire = TRUE;
		if (have_flag(flgs, TR_RES_COLD))   p_ptr->resist_cold = TRUE;
		if (have_flag(flgs, TR_RES_POIS))   p_ptr->resist_pois = TRUE;
		if (have_flag(flgs, TR_RES_FEAR))   p_ptr->resist_fear = TRUE;
		if (have_flag(flgs, TR_RES_CONF))   p_ptr->resist_conf = TRUE;
		if (have_flag(flgs, TR_RES_SOUND))  p_ptr->resist_sound = TRUE;
		if (have_flag(flgs, TR_RES_LITE))   p_ptr->resist_lite = TRUE;
		if (have_flag(flgs, TR_RES_DARK))   p_ptr->resist_dark = TRUE;
		if (have_flag(flgs, TR_RES_CHAOS))  p_ptr->resist_chaos = TRUE;
		if (have_flag(flgs, TR_RES_DISEN))  p_ptr->resist_disen = TRUE;
		if (have_flag(flgs, TR_RES_SHARDS)) p_ptr->resist_shard = TRUE;
		if (have_flag(flgs, TR_RES_NEXUS))  p_ptr->resist_nexus = TRUE;
		if (have_flag(flgs, TR_RES_BLIND))  p_ptr->resist_blind = TRUE;
		if (have_flag(flgs, TR_RES_NETHER)) p_ptr->resist_neth = TRUE;

		if (have_flag(flgs, TR_REFLECT))  p_ptr->reflect = TRUE;
		if (have_flag(flgs, TR_SH_FIRE))  p_ptr->sh_fire = TRUE;
		if (have_flag(flgs, TR_SH_ELEC))  p_ptr->sh_elec = TRUE;
		if (have_flag(flgs, TR_SH_COLD))  p_ptr->sh_cold = TRUE;
		if (have_flag(flgs, TR_NO_MAGIC)) p_ptr->anti_magic = TRUE;
		if (have_flag(flgs, TR_NO_TELE))  p_ptr->anti_tele = TRUE;

		/* Sustain flags */
		if (have_flag(flgs, TR_SUST_STR)) p_ptr->sustain_str = TRUE;
		if (have_flag(flgs, TR_SUST_INT)) p_ptr->sustain_int = TRUE;
		if (have_flag(flgs, TR_SUST_WIS)) p_ptr->sustain_wis = TRUE;
		if (have_flag(flgs, TR_SUST_DEX)) p_ptr->sustain_dex = TRUE;
		if (have_flag(flgs, TR_SUST_CON)) p_ptr->sustain_con = TRUE;
		if (have_flag(flgs, TR_SUST_CHR)) p_ptr->sustain_chr = TRUE;

		if (o_ptr->name2 == EGO_YOIYAMI) yoiyami = TRUE;
		if (o_ptr->name2 == EGO_2WEAPON) easy_2weapon = TRUE;
		if (o_ptr->name2 == EGO_RING_RES_TIME) p_ptr->resist_time = TRUE;
		if (o_ptr->name2 == EGO_RING_THROW) p_ptr->mighty_throw = TRUE;
		if (have_flag(flgs, TR_EASY_SPELL)) p_ptr->easy_spell = TRUE;
		if (o_ptr->name2 == EGO_AMU_FOOL) p_ptr->heavy_spell = TRUE;
		if (o_ptr->name2 == EGO_AMU_NAIVETY) down_saving = TRUE;

		if (o_ptr->curse_flags & TRC_LOW_MAGIC)
		{
			if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
			{
				p_ptr->to_m_chance += 10;
			}
			else
			{
				p_ptr->to_m_chance += 3;
			}
		}

		if (o_ptr->tval == TV_CAPTURE) continue;

		/* Modify the base armor class */
		p_ptr->ac += o_ptr->ac;

		/* The base armor class is always known */
		p_ptr->dis_ac += o_ptr->ac;

		/* Apply the bonuses to armor class */
		p_ptr->to_a += o_ptr->to_a;

		/* Apply the mental bonuses to armor class, if known */
		if (object_is_known(o_ptr)) p_ptr->dis_to_a += o_ptr->to_a;

		if (o_ptr->curse_flags & TRC_LOW_MELEE)
		{
			int slot = i - INVEN_RARM;
			if (slot < 2)
			{
				if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
				{
					p_ptr->to_h[slot] -= 15;
					if (o_ptr->ident & IDENT_MENTAL) p_ptr->dis_to_h[slot] -= 15;
				}
				else
				{
					p_ptr->to_h[slot] -= 5;
					if (o_ptr->ident & IDENT_MENTAL) p_ptr->dis_to_h[slot] -= 5;
				}
			}
			else
			{
				if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
				{
					p_ptr->to_h_b -= 15;
					if (o_ptr->ident & IDENT_MENTAL) p_ptr->dis_to_h_b -= 15;
				}
				else
				{
					p_ptr->to_h_b -= 5;
					if (o_ptr->ident & IDENT_MENTAL) p_ptr->dis_to_h_b -= 5;
				}
			}
		}

		if (o_ptr->curse_flags & TRC_LOW_AC)
		{
			if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
			{
				p_ptr->to_a -= 30;
				if (o_ptr->ident & IDENT_MENTAL) p_ptr->dis_to_a -= 30;
			}
			else
			{
				p_ptr->to_a -= 10;
				if (o_ptr->ident & IDENT_MENTAL) p_ptr->dis_to_a -= 10;
			}
		}

		/* Hack -- do not apply "weapon" bonuses */
		if (i == INVEN_RARM && has_melee_weapon(i)) continue;
		if (i == INVEN_LARM && has_melee_weapon(i)) continue;

		/* Hack -- do not apply "bow" bonuses */
		if (i == INVEN_BOW) continue;

		bonus_to_h = o_ptr->to_h;
		bonus_to_d = o_ptr->to_d;

		if (p_ptr->pclass == CLASS_NINJA)
		{
			if (o_ptr->to_h > 0) bonus_to_h = (o_ptr->to_h + 1) / 2;
			if (o_ptr->to_d > 0) bonus_to_d = (o_ptr->to_d + 1) / 2;
		}

		/* To Bow and Natural attack */

		/* Apply the bonuses to hit/damage */
		p_ptr->to_h_b += (s16b)bonus_to_h;
		p_ptr->to_h_m += (s16b)bonus_to_h;
		p_ptr->to_d_m += (s16b)bonus_to_d;

		/* Apply the mental bonuses tp hit/damage, if known */
		if (object_is_known(o_ptr)) p_ptr->dis_to_h_b += (s16b)bonus_to_h;

		/* To Melee */
		if ((i == INVEN_LEFT || i == INVEN_RIGHT) && !p_ptr->ryoute)
		{
			/* Apply the bonuses to hit/damage */
			p_ptr->to_h[i - INVEN_RIGHT] += (s16b)bonus_to_h;
			p_ptr->to_d[i - INVEN_RIGHT] += (s16b)bonus_to_d;

			/* Apply the mental bonuses tp hit/damage, if known */
			if (object_is_known(o_ptr))
			{
				p_ptr->dis_to_h[i - INVEN_RIGHT] += (s16b)bonus_to_h;
				p_ptr->dis_to_d[i - INVEN_RIGHT] += (s16b)bonus_to_d;
			}
		}
		else if (p_ptr->migite && p_ptr->hidarite)
		{
			/* Apply the bonuses to hit/damage */
			p_ptr->to_h[0] += (bonus_to_h > 0) ? (bonus_to_h + 1) / 2 : bonus_to_h;
			p_ptr->to_h[1] += (bonus_to_h > 0) ? bonus_to_h / 2 : bonus_to_h;
			p_ptr->to_d[0] += (bonus_to_d > 0) ? (bonus_to_d + 1) / 2 : bonus_to_d;
			p_ptr->to_d[1] += (bonus_to_d > 0) ? bonus_to_d / 2 : bonus_to_d;

			/* Apply the mental bonuses tp hit/damage, if known */
			if (object_is_known(o_ptr))
			{
				p_ptr->dis_to_h[0] += (bonus_to_h > 0) ? (bonus_to_h + 1) / 2 : bonus_to_h;
				p_ptr->dis_to_h[1] += (bonus_to_h > 0) ? bonus_to_h / 2 : bonus_to_h;
				p_ptr->dis_to_d[0] += (bonus_to_d > 0) ? (bonus_to_d + 1) / 2 : bonus_to_d;
				p_ptr->dis_to_d[1] += (bonus_to_d > 0) ? bonus_to_d / 2 : bonus_to_d;
			}
		}
		else
		{
			/* Apply the bonuses to hit/damage */
			p_ptr->to_h[default_hand] += (s16b)bonus_to_h;
			p_ptr->to_d[default_hand] += (s16b)bonus_to_d;

			/* Apply the mental bonuses to hit/damage, if known */
			if (object_is_known(o_ptr))
			{
				p_ptr->dis_to_h[default_hand] += (s16b)bonus_to_h;
				p_ptr->dis_to_d[default_hand] += (s16b)bonus_to_d;
			}
		}
	}

	/* Shield skill bonus */
	if (object_is_armour(&p_ptr->inventory_list[INVEN_RARM]) || object_is_armour(&p_ptr->inventory_list[INVEN_LARM]))
	{
		p_ptr->ac += p_ptr->skill_exp[GINOU_SHIELD] * (1 + p_ptr->lev / 22) / 2000;
		p_ptr->dis_ac += p_ptr->skill_exp[GINOU_SHIELD] * (1 + p_ptr->lev / 22) / 2000;
	}

	if (old_mighty_throw != p_ptr->mighty_throw)
	{
		/* Redraw average damege display of Shuriken */
		p_ptr->window |= PW_INVEN;
	}

	if (p_ptr->cursed & TRC_TELEPORT) p_ptr->cursed &= ~(TRC_TELEPORT_SELF);

	/* Monks get extra ac for armour _not worn_ */
	if (((p_ptr->pclass == CLASS_MONK) || (p_ptr->pclass == CLASS_FORCETRAINER)) && !heavy_armor())
	{
		if (!(p_ptr->inventory_list[INVEN_BODY].k_idx))
		{
			p_ptr->to_a += (p_ptr->lev * 3) / 2;
			p_ptr->dis_to_a += (p_ptr->lev * 3) / 2;
		}
		if (!(p_ptr->inventory_list[INVEN_OUTER].k_idx) && (p_ptr->lev > 15))
		{
			p_ptr->to_a += ((p_ptr->lev - 13) / 3);
			p_ptr->dis_to_a += ((p_ptr->lev - 13) / 3);
		}
		if (!(p_ptr->inventory_list[INVEN_LARM].k_idx) && (p_ptr->lev > 10))
		{
			p_ptr->to_a += ((p_ptr->lev - 8) / 3);
			p_ptr->dis_to_a += ((p_ptr->lev - 8) / 3);
		}
		if (!(p_ptr->inventory_list[INVEN_HEAD].k_idx) && (p_ptr->lev > 4))
		{
			p_ptr->to_a += (p_ptr->lev - 2) / 3;
			p_ptr->dis_to_a += (p_ptr->lev - 2) / 3;
		}
		if (!(p_ptr->inventory_list[INVEN_HANDS].k_idx))
		{
			p_ptr->to_a += (p_ptr->lev / 2);
			p_ptr->dis_to_a += (p_ptr->lev / 2);
		}
		if (!(p_ptr->inventory_list[INVEN_FEET].k_idx))
		{
			p_ptr->to_a += (p_ptr->lev / 3);
			p_ptr->dis_to_a += (p_ptr->lev / 3);
		}
		if (p_ptr->special_defense & KAMAE_BYAKKO)
		{
			p_ptr->stat_add[A_STR] += 2;
			p_ptr->stat_add[A_DEX] += 2;
			p_ptr->stat_add[A_CON] -= 3;
		}
		else if (p_ptr->special_defense & KAMAE_SEIRYU)
		{
		}
		else if (p_ptr->special_defense & KAMAE_GENBU)
		{
			p_ptr->stat_add[A_INT] -= 1;
			p_ptr->stat_add[A_WIS] -= 1;
			p_ptr->stat_add[A_DEX] -= 2;
			p_ptr->stat_add[A_CON] += 3;
		}
		else if (p_ptr->special_defense & KAMAE_SUZAKU)
		{
			p_ptr->stat_add[A_STR] -= 2;
			p_ptr->stat_add[A_INT] += 1;
			p_ptr->stat_add[A_WIS] += 1;
			p_ptr->stat_add[A_DEX] += 2;
			p_ptr->stat_add[A_CON] -= 2;
		}
	}

	if (p_ptr->special_defense & KATA_KOUKIJIN)
	{
		for (i = 0; i < A_MAX; i++)
			p_ptr->stat_add[i] += 5;
		p_ptr->to_a -= 50;
		p_ptr->dis_to_a -= 50;
	}

	/* Hack -- aura of fire also provides light */
	if (p_ptr->sh_fire) p_ptr->lite = TRUE;

	/* Golems also get an intrinsic AC bonus */
	if (prace_is_(RACE_GOLEM) || prace_is_(RACE_ANDROID))
	{
		p_ptr->to_a += 10 + (p_ptr->lev * 2 / 5);
		p_ptr->dis_to_a += 10 + (p_ptr->lev * 2 / 5);
	}

	/* Hex bonuses */
	if (p_ptr->realm1 == REALM_HEX)
	{
		if (hex_spelling_any()) p_ptr->skill_stl -= (1 + CASTING_HEX_NUM(p_ptr));
		if (hex_spelling(HEX_DETECT_EVIL)) p_ptr->esp_evil = TRUE;
		if (hex_spelling(HEX_XTRA_MIGHT)) p_ptr->stat_add[A_STR] += 4;
		if (hex_spelling(HEX_BUILDING))
		{
			p_ptr->stat_add[A_STR] += 4;
			p_ptr->stat_add[A_DEX] += 4;
			p_ptr->stat_add[A_CON] += 4;
		}
		if (hex_spelling(HEX_DEMON_AURA))
		{
			p_ptr->sh_fire = TRUE;
			p_ptr->regenerate = TRUE;
		}
		if (hex_spelling(HEX_ICE_ARMOR))
		{
			p_ptr->sh_cold = TRUE;
			p_ptr->to_a += 30;
			p_ptr->dis_to_a += 30;
		}
		if (hex_spelling(HEX_SHOCK_CLOAK))
		{
			p_ptr->sh_elec = TRUE;
			new_speed += 3;
		}
		for (i = INVEN_RARM; i <= INVEN_FEET; i++)
		{
			ARMOUR_CLASS ac = 0;
			o_ptr = &p_ptr->inventory_list[i];
			if (!o_ptr->k_idx) continue;
			if (!object_is_armour(o_ptr)) continue;
			if (!object_is_cursed(o_ptr)) continue;
			ac += 5;
			if (o_ptr->curse_flags & TRC_HEAVY_CURSE) ac += 7;
			if (o_ptr->curse_flags & TRC_PERMA_CURSE) ac += 13;
			p_ptr->to_a += (s16b)ac;
			p_ptr->dis_to_a += (s16b)ac;
		}
	}

	/* Calculate stats */
	for (i = 0; i < A_MAX; i++)
	{
		int top, use, ind;

		/* Extract the new "stat_use" value for the stat */
		top = modify_stat_value(p_ptr->stat_max[i], p_ptr->stat_add[i]);

		/* Notice changes */
		if (p_ptr->stat_top[i] != top)
		{
			/* Save the new value */
			p_ptr->stat_top[i] = (s16b)top;
			p_ptr->redraw |= (PR_STATS);
			p_ptr->window |= (PW_PLAYER);
		}


		/* Extract the new "stat_use" value for the stat */
		use = modify_stat_value(p_ptr->stat_cur[i], p_ptr->stat_add[i]);

		if ((i == A_CHR) && (p_ptr->muta3 & MUT3_ILL_NORM))
		{
			/* 10 to 18/90 charisma, guaranteed, based on level */
			if (use < 8 + 2 * p_ptr->lev)
			{
				use = 8 + 2 * p_ptr->lev;
			}
		}

		/* Notice changes */
		if (p_ptr->stat_use[i] != use)
		{
			/* Save the new value */
			p_ptr->stat_use[i] = (s16b)use;
			p_ptr->redraw |= (PR_STATS);
			p_ptr->window |= (PW_PLAYER);
		}


		/* Values: 3, 4, ..., 17 */
		if (use <= 18) ind = (use - 3);

		/* Ranges: 18/00-18/09, ..., 18/210-18/219 */
		else if (use <= 18 + 219) ind = (15 + (use - 18) / 10);

		/* Range: 18/220+ */
		else ind = (37);

		/* Notice changes */
		if (p_ptr->stat_ind[i] != ind)
		{
			/* Save the new index */
			p_ptr->stat_ind[i] = (s16b)ind;

			/* Change in CON affects Hitpoints */
			if (i == A_CON)
			{
				p_ptr->update |= (PU_HP);
			}

			/* Change in INT may affect Mana/Spells */
			else if (i == A_INT)
			{
				if (mp_ptr->spell_stat == A_INT)
				{
					p_ptr->update |= (PU_MANA | PU_SPELLS);
				}
			}

			/* Change in WIS may affect Mana/Spells */
			else if (i == A_WIS)
			{
				if (mp_ptr->spell_stat == A_WIS)
				{
					p_ptr->update |= (PU_MANA | PU_SPELLS);
				}
			}

			/* Change in WIS may affect Mana/Spells */
			else if (i == A_CHR)
			{
				if (mp_ptr->spell_stat == A_CHR)
				{
					p_ptr->update |= (PU_MANA | PU_SPELLS);
				}
			}

			p_ptr->window |= (PW_PLAYER);
		}
	}


	/* Apply temporary "stun" */
	if (p_ptr->stun > 50)
	{
		p_ptr->to_h[0] -= 20;
		p_ptr->to_h[1] -= 20;
		p_ptr->to_h_b -= 20;
		p_ptr->to_h_m -= 20;
		p_ptr->dis_to_h[0] -= 20;
		p_ptr->dis_to_h[1] -= 20;
		p_ptr->dis_to_h_b -= 20;
		p_ptr->to_d[0] -= 20;
		p_ptr->to_d[1] -= 20;
		p_ptr->to_d_m -= 20;
		p_ptr->dis_to_d[0] -= 20;
		p_ptr->dis_to_d[1] -= 20;
	}
	else if (p_ptr->stun)
	{
		p_ptr->to_h[0] -= 5;
		p_ptr->to_h[1] -= 5;
		p_ptr->to_h_b -= 5;
		p_ptr->to_h_m -= 5;
		p_ptr->dis_to_h[0] -= 5;
		p_ptr->dis_to_h[1] -= 5;
		p_ptr->dis_to_h_b -= 5;
		p_ptr->to_d[0] -= 5;
		p_ptr->to_d[1] -= 5;
		p_ptr->to_d_m -= 5;
		p_ptr->dis_to_d[0] -= 5;
		p_ptr->dis_to_d[1] -= 5;
	}

	/* Wraith form */
	if (p_ptr->wraith_form)
	{
		p_ptr->reflect = TRUE;
		p_ptr->pass_wall = TRUE;
	}

	if (p_ptr->kabenuke)
	{
		p_ptr->pass_wall = TRUE;
	}

	/* Temporary blessing */
	if (IS_BLESSED())
	{
		p_ptr->to_a += 5;
		p_ptr->dis_to_a += 5;
		p_ptr->to_h[0] += 10;
		p_ptr->to_h[1] += 10;
		p_ptr->to_h_b += 10;
		p_ptr->to_h_m += 10;
		p_ptr->dis_to_h[0] += 10;
		p_ptr->dis_to_h[1] += 10;
		p_ptr->dis_to_h_b += 10;
	}

	if (p_ptr->magicdef)
	{
		p_ptr->resist_blind = TRUE;
		p_ptr->resist_conf = TRUE;
		p_ptr->reflect = TRUE;
		p_ptr->free_act = TRUE;
		p_ptr->levitation = TRUE;
	}

	/* Temporary "Hero" */
	if (IS_HERO())
	{
		p_ptr->to_h[0] += 12;
		p_ptr->to_h[1] += 12;
		p_ptr->to_h_b += 12;
		p_ptr->to_h_m += 12;
		p_ptr->dis_to_h[0] += 12;
		p_ptr->dis_to_h[1] += 12;
		p_ptr->dis_to_h_b += 12;
	}

	/* Temporary "Beserk" */
	if (p_ptr->shero)
	{
		p_ptr->to_h[0] += 12;
		p_ptr->to_h[1] += 12;
		p_ptr->to_h_b -= 12;
		p_ptr->to_h_m += 12;
		p_ptr->to_d[0] += 3 + (p_ptr->lev / 5);
		p_ptr->to_d[1] += 3 + (p_ptr->lev / 5);
		p_ptr->to_d_m += 3 + (p_ptr->lev / 5);
		p_ptr->dis_to_h[0] += 12;
		p_ptr->dis_to_h[1] += 12;
		p_ptr->dis_to_h_b -= 12;
		p_ptr->dis_to_d[0] += 3 + (p_ptr->lev / 5);
		p_ptr->dis_to_d[1] += 3 + (p_ptr->lev / 5);
		p_ptr->to_a -= 10;
		p_ptr->dis_to_a -= 10;
		p_ptr->skill_stl -= 7;
		p_ptr->skill_dev -= 20;
		p_ptr->skill_sav -= 30;
		p_ptr->skill_srh -= 15;
		p_ptr->skill_fos -= 15;
		p_ptr->skill_tht -= 20;
		p_ptr->skill_dig += 30;
	}

	/* Temporary "fast" */
	if (IS_FAST())
	{
		new_speed += 10;
	}

	/* Temporary "slow" */
	if (p_ptr->slow)
	{
		new_speed -= 10;
	}

	/* Temporary "telepathy" */
	if (IS_TIM_ESP())
	{
		p_ptr->telepathy = TRUE;
	}

	if (p_ptr->ele_immune)
	{
		if (p_ptr->special_defense & DEFENSE_ACID)
			p_ptr->immune_acid = TRUE;
		else if (p_ptr->special_defense & DEFENSE_ELEC)
			p_ptr->immune_elec = TRUE;
		else if (p_ptr->special_defense & DEFENSE_FIRE)
			p_ptr->immune_fire = TRUE;
		else if (p_ptr->special_defense & DEFENSE_COLD)
			p_ptr->immune_cold = TRUE;
	}

	/* Temporary see invisible */
	if (p_ptr->tim_invis)
	{
		p_ptr->see_inv = TRUE;
	}

	/* Temporary infravision boost */
	if (p_ptr->tim_infra)
	{
		p_ptr->see_infra += 3;
	}

	/* Temporary regeneration boost */
	if (p_ptr->tim_regen)
	{
		p_ptr->regenerate = TRUE;
	}

	/* Temporary levitation */
	if (p_ptr->tim_levitation)
	{
		p_ptr->levitation = TRUE;
	}

	/* Temporary reflection */
	if (p_ptr->tim_reflect)
	{
		p_ptr->reflect = TRUE;
	}

	/* Hack -- Hero/Shero -> Res fear */
	if (IS_HERO() || p_ptr->shero)
	{
		p_ptr->resist_fear = TRUE;
	}


	/* Hack -- Telepathy Change */
	if (p_ptr->telepathy != old_telepathy)
	{
		p_ptr->update |= (PU_MONSTERS);
	}

	if ((p_ptr->esp_animal != old_esp_animal) ||
		(p_ptr->esp_undead != old_esp_undead) ||
		(p_ptr->esp_demon != old_esp_demon) ||
		(p_ptr->esp_orc != old_esp_orc) ||
		(p_ptr->esp_troll != old_esp_troll) ||
		(p_ptr->esp_giant != old_esp_giant) ||
		(p_ptr->esp_dragon != old_esp_dragon) ||
		(p_ptr->esp_human != old_esp_human) ||
		(p_ptr->esp_evil != old_esp_evil) ||
		(p_ptr->esp_good != old_esp_good) ||
		(p_ptr->esp_nonliving != old_esp_nonliving) ||
		(p_ptr->esp_unique != old_esp_unique))
	{
		p_ptr->update |= (PU_MONSTERS);
	}

	/* Hack -- See Invis Change */
	if (p_ptr->see_inv != old_see_inv)
	{
		p_ptr->update |= (PU_MONSTERS);
	}

	/* Bloating slows the player down (a little) */
	if (p_ptr->food >= PY_FOOD_MAX) new_speed -= 10;

	if (p_ptr->special_defense & KAMAE_SUZAKU) new_speed += 10;

	if ((p_ptr->migite && (empty_hands_status & EMPTY_HAND_RARM)) ||
		(p_ptr->hidarite && (empty_hands_status & EMPTY_HAND_LARM)))
	{
		p_ptr->to_h[default_hand] += (p_ptr->skill_exp[GINOU_SUDE] - WEAPON_EXP_BEGINNER) / 200;
		p_ptr->dis_to_h[default_hand] += (p_ptr->skill_exp[GINOU_SUDE] - WEAPON_EXP_BEGINNER) / 200;
	}

	if (has_melee_weapon(INVEN_RARM) && has_melee_weapon(INVEN_LARM))
	{
		int penalty1, penalty2;
		penalty1 = ((100 - p_ptr->skill_exp[GINOU_NITOURYU] / 160) - (130 - p_ptr->inventory_list[INVEN_RARM].weight) / 8);
		penalty2 = ((100 - p_ptr->skill_exp[GINOU_NITOURYU] / 160) - (130 - p_ptr->inventory_list[INVEN_LARM].weight) / 8);
		if ((p_ptr->inventory_list[INVEN_RARM].name1 == ART_QUICKTHORN) && (p_ptr->inventory_list[INVEN_LARM].name1 == ART_TINYTHORN))
		{
			penalty1 = penalty1 / 2 - 5;
			penalty2 = penalty2 / 2 - 5;
			new_speed += 7;
			p_ptr->to_a += 10;
			p_ptr->dis_to_a += 10;
		}
		if (easy_2weapon)
		{
			if (penalty1 > 0) penalty1 /= 2;
			if (penalty2 > 0) penalty2 /= 2;
		}
		else if ((p_ptr->inventory_list[INVEN_LARM].tval == TV_SWORD) && ((p_ptr->inventory_list[INVEN_LARM].sval == SV_MAIN_GAUCHE) || (p_ptr->inventory_list[INVEN_LARM].sval == SV_WAKIZASHI)))
		{
			penalty1 = MAX(0, penalty1 - 10);
			penalty2 = MAX(0, penalty2 - 10);
		}
		if ((p_ptr->inventory_list[INVEN_RARM].name1 == ART_MUSASI_KATANA) && (p_ptr->inventory_list[INVEN_LARM].name1 == ART_MUSASI_WAKIZASI))
		{
			penalty1 = MIN(0, penalty1);
			penalty2 = MIN(0, penalty2);
			p_ptr->to_a += 10;
			p_ptr->dis_to_a += 10;
		}
		else
		{
			if ((p_ptr->inventory_list[INVEN_RARM].name1 == ART_MUSASI_KATANA) && (penalty1 > 0))
				penalty1 /= 2;
			if ((p_ptr->inventory_list[INVEN_LARM].name1 == ART_MUSASI_WAKIZASI) && (penalty2 > 0))
				penalty2 /= 2;
		}
		if (p_ptr->inventory_list[INVEN_RARM].tval == TV_POLEARM) penalty1 += 10;
		if (p_ptr->inventory_list[INVEN_LARM].tval == TV_POLEARM) penalty2 += 10;
		p_ptr->to_h[0] -= (s16b)penalty1;
		p_ptr->to_h[1] -= (s16b)penalty2;
		p_ptr->dis_to_h[0] -= (s16b)penalty1;
		p_ptr->dis_to_h[1] -= (s16b)penalty2;
	}

	/* Extract the current weight (in tenth pounds) */
	j = p_ptr->total_weight;

	if (!p_ptr->riding)
	{
		/* Extract the "weight limit" (in tenth pounds) */
		i = (int)weight_limit();
	}
	else
	{
		monster_type *riding_m_ptr = &current_floor_ptr->m_list[p_ptr->riding];
		monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
		SPEED speed = riding_m_ptr->mspeed;

		if (riding_m_ptr->mspeed > 110)
		{
			new_speed = 110 + (s16b)((speed - 110) * (p_ptr->skill_exp[GINOU_RIDING] * 3 + p_ptr->lev * 160L - 10000L) / (22000L));
			if (new_speed < 110) new_speed = 110;
		}
		else
		{
			new_speed = speed;
		}
		new_speed += (p_ptr->skill_exp[GINOU_RIDING] + p_ptr->lev * 160L) / 3200;
		if (MON_FAST(riding_m_ptr)) new_speed += 10;
		if (MON_SLOW(riding_m_ptr)) new_speed -= 10;
		riding_levitation = (riding_r_ptr->flags7 & RF7_CAN_FLY) ? TRUE : FALSE;
		if (riding_r_ptr->flags7 & (RF7_CAN_SWIM | RF7_AQUATIC)) p_ptr->can_swim = TRUE;

		if (!(riding_r_ptr->flags2 & RF2_PASS_WALL)) p_ptr->pass_wall = FALSE;
		if (riding_r_ptr->flags2 & RF2_KILL_WALL) p_ptr->kill_wall = TRUE;

		if (p_ptr->skill_exp[GINOU_RIDING] < RIDING_EXP_SKILLED) j += (p_ptr->wt * 3 * (RIDING_EXP_SKILLED - p_ptr->skill_exp[GINOU_RIDING])) / RIDING_EXP_SKILLED;

		/* Extract the "weight limit" */
		i = 1500 + riding_r_ptr->level * 25;
	}

	/* Apply "encumbrance" from weight */
	if (j > i) new_speed -= ((j - i) / (i / 5));

	/* Searching slows the player down */
	if (p_ptr->action == ACTION_SEARCH) new_speed -= 10;

	/* Feature bonus */
	if (p_ptr->prace == RACE_MERFOLK)
	{
		if (have_flag(f_ptr->flags, FF_WATER))
		{
			new_speed += (2 + p_ptr->lev / 10);
		}
		else if (!p_ptr->levitation)
		{
			new_speed -= 2;
		}
	}


	/* Actual Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_d[0] += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->to_d[1] += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->to_d_m += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->to_h[0] += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_h[1] += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_h_b += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_h_m += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->to_h[0] += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->to_h[1] += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->to_h_b += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->to_h_m += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->dis_to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->dis_to_d[0] += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->dis_to_d[1] += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->dis_to_h[0] += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->dis_to_h[1] += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->dis_to_h_b += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->dis_to_h[0] += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->dis_to_h[1] += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);
	p_ptr->dis_to_h_b += ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);


	/* Obtain the "hold" value */
	hold = adj_str_hold[p_ptr->stat_ind[A_STR]];


	/* Examine the "current bow" */
	o_ptr = &p_ptr->inventory_list[INVEN_BOW];

	/* It is hard to carholdry a heavy bow */
	p_ptr->heavy_shoot = is_heavy_shoot(o_ptr);
	if (p_ptr->heavy_shoot)
	{
		/* Hard to wield a heavy bow */
		p_ptr->to_h_b += 2 * (hold - o_ptr->weight / 10);
		p_ptr->dis_to_h_b += 2 * (hold - o_ptr->weight / 10);
	}

	/* Compute "extra shots" if needed */
	if (o_ptr->k_idx)
	{
		p_ptr->tval_ammo = (byte_hack)bow_tval_ammo(o_ptr);

		/* Apply special flags */
		if (o_ptr->k_idx && !p_ptr->heavy_shoot)
		{
			/* Extra shots */
			p_ptr->num_fire = calc_num_fire(o_ptr);

			/* Snipers love Cross bows */
			if ((p_ptr->pclass == CLASS_SNIPER) &&
				(p_ptr->tval_ammo == TV_BOLT))
			{
				p_ptr->to_h_b += (10 + (p_ptr->lev / 5));
				p_ptr->dis_to_h_b += (10 + (p_ptr->lev / 5));
			}
		}
	}

	if (p_ptr->ryoute) hold *= 2;

	for (i = 0; i < 2; i++)
	{
		/* Examine the "main weapon" */
		o_ptr = &p_ptr->inventory_list[INVEN_RARM + i];

		object_flags(o_ptr, flgs);

		/* Assume not heavy */
		p_ptr->heavy_wield[i] = FALSE;
		p_ptr->icky_wield[i] = FALSE;
		p_ptr->riding_wield[i] = FALSE;

		if (!has_melee_weapon(INVEN_RARM + i))
		{
			p_ptr->num_blow[i] = 1;
			continue;
		}
		/* It is hard to hold a heavy weapon */
		if (hold < o_ptr->weight / 10)
		{
			/* Hard to wield a heavy weapon */
			p_ptr->to_h[i] += 2 * (hold - o_ptr->weight / 10);
			p_ptr->dis_to_h[i] += 2 * (hold - o_ptr->weight / 10);

			/* Heavy weapon */
			p_ptr->heavy_wield[i] = TRUE;
		}
		else if (p_ptr->ryoute && (hold < o_ptr->weight / 5)) omoi = TRUE;

		if ((i == 1) && (o_ptr->tval == TV_SWORD) && ((o_ptr->sval == SV_MAIN_GAUCHE) || (o_ptr->sval == SV_WAKIZASHI)))
		{
			p_ptr->to_a += 5;
			p_ptr->dis_to_a += 5;
		}

		/* Normal weapons */
		if (o_ptr->k_idx && !p_ptr->heavy_wield[i])
		{
			int str_index, dex_index;

			int num = 0, wgt = 0, mul = 0, div = 0;

			/* Analyze the class */
			switch (p_ptr->pclass)
			{
			case CLASS_WARRIOR:
				num = 6; wgt = 70; mul = 5; break;

			case CLASS_BERSERKER:
				num = 6; wgt = 70; mul = 7; break;

			case CLASS_MAGE:
			case CLASS_HIGH_MAGE:
			case CLASS_BLUE_MAGE:
				num = 3; wgt = 100; mul = 2; break;

			case CLASS_PRIEST:
			case CLASS_MAGIC_EATER:
			case CLASS_MINDCRAFTER:
				num = 5; wgt = 100; mul = 3; break;

			case CLASS_ROGUE:
				num = 5; wgt = 40; mul = 3; break;

			case CLASS_RANGER:
				num = 5; wgt = 70; mul = 4; break;

			case CLASS_PALADIN:
			case CLASS_SAMURAI:
				num = 5; wgt = 70; mul = 4; break;

			case CLASS_SMITH:
				num = 5; wgt = 150; mul = 5; break;

			case CLASS_WARRIOR_MAGE:
			case CLASS_RED_MAGE:
				num = 5; wgt = 70; mul = 3; break;

			case CLASS_CHAOS_WARRIOR:
				num = 5; wgt = 70; mul = 4; break;

			case CLASS_MONK:
				num = 5; wgt = 60; mul = 3; break;

			case CLASS_TOURIST:
				num = 4; wgt = 100; mul = 3; break;

			case CLASS_IMITATOR:
				num = 5; wgt = 70; mul = 4; break;

			case CLASS_BEASTMASTER:
				num = 5; wgt = 70; mul = 3; break;

			case CLASS_CAVALRY:
				if ((p_ptr->riding) && (have_flag(flgs, TR_RIDING))) { num = 5; wgt = 70; mul = 4; }
				else { num = 5; wgt = 100; mul = 3; }
				break;

			case CLASS_SORCERER:
				num = 1; wgt = 1; mul = 1; break;

			case CLASS_ARCHER:
			case CLASS_BARD:
			case CLASS_SNIPER:
				num = 4; wgt = 70; mul = 2; break;

			case CLASS_FORCETRAINER:
				num = 4; wgt = 60; mul = 2; break;

			case CLASS_MIRROR_MASTER:
				num = 3; wgt = 100; mul = 3; break;

			case CLASS_NINJA:
				num = 4; wgt = 20; mul = 1; break;
			}

			/* Hex - extra mights gives +1 bonus to max blows */
			if (hex_spelling(HEX_XTRA_MIGHT) || hex_spelling(HEX_BUILDING)) { num++; wgt /= 2; mul += 2; }

			/* Enforce a minimum "weight" (tenth pounds) */
			div = ((o_ptr->weight < wgt) ? wgt : o_ptr->weight);

			/* Access the strength vs weight */
			str_index = (adj_str_blow[p_ptr->stat_ind[A_STR]] * mul / div);

			if (p_ptr->ryoute && !omoi) str_index++;
			if (p_ptr->pclass == CLASS_NINJA) str_index = MAX(0, str_index - 1);

			/* Maximal value */
			if (str_index > 11) str_index = 11;

			/* Index by dexterity */
			dex_index = (adj_dex_blow[p_ptr->stat_ind[A_DEX]]);

			/* Maximal value */
			if (dex_index > 11) dex_index = 11;

			/* Use the blows table */
			p_ptr->num_blow[i] = blows_table[str_index][dex_index];

			/* Maximal value */
			if (p_ptr->num_blow[i] > num) p_ptr->num_blow[i] = (s16b)num;

			/* Add in the "bonus blows" */
			p_ptr->num_blow[i] += (s16b)extra_blows[i];


			if (p_ptr->pclass == CLASS_WARRIOR) p_ptr->num_blow[i] += (p_ptr->lev / 40);
			else if (p_ptr->pclass == CLASS_BERSERKER)
			{
				p_ptr->num_blow[i] += (p_ptr->lev / 23);
			}
			else if ((p_ptr->pclass == CLASS_ROGUE) && (o_ptr->weight < 50) && (p_ptr->stat_ind[A_DEX] >= 30)) p_ptr->num_blow[i] ++;

			if (p_ptr->special_defense & KATA_FUUJIN) p_ptr->num_blow[i] -= 1;

			if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI)) p_ptr->num_blow[i] = 1;


			/* Require at least one blow */
			if (p_ptr->num_blow[i] < 1) p_ptr->num_blow[i] = 1;

			/* Boost digging skill by weapon weight */
			p_ptr->skill_dig += (o_ptr->weight / 10);
		}

		/* Assume okay */
		/* Priest weapon penalty for non-blessed edged weapons */
		if ((p_ptr->pclass == CLASS_PRIEST) && (!(have_flag(flgs, TR_BLESSED))) &&
			((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM)))
		{
			/* Reduce the real bonuses */
			p_ptr->to_h[i] -= 2;
			p_ptr->to_d[i] -= 2;

			/* Reduce the mental bonuses */
			p_ptr->dis_to_h[i] -= 2;
			p_ptr->dis_to_d[i] -= 2;

			/* Icky weapon */
			p_ptr->icky_wield[i] = TRUE;
		}
		else if (p_ptr->pclass == CLASS_BERSERKER)
		{
			p_ptr->to_h[i] += p_ptr->lev / 5;
			p_ptr->to_d[i] += p_ptr->lev / 6;
			p_ptr->dis_to_h[i] += p_ptr->lev / 5;
			p_ptr->dis_to_d[i] += p_ptr->lev / 6;
			if (((i == 0) && !p_ptr->hidarite) || p_ptr->ryoute)
			{
				p_ptr->to_h[i] += p_ptr->lev / 5;
				p_ptr->to_d[i] += p_ptr->lev / 6;
				p_ptr->dis_to_h[i] += p_ptr->lev / 5;
				p_ptr->dis_to_d[i] += p_ptr->lev / 6;
			}
		}
		else if (p_ptr->pclass == CLASS_SORCERER)
		{
			if (!((o_ptr->tval == TV_HAFTED) && ((o_ptr->sval == SV_WIZSTAFF) || (o_ptr->sval == SV_NAMAKE_HAMMER))))
			{
				/* Reduce the real bonuses */
				p_ptr->to_h[i] -= 200;
				p_ptr->to_d[i] -= 200;

				/* Reduce the mental bonuses */
				p_ptr->dis_to_h[i] -= 200;
				p_ptr->dis_to_d[i] -= 200;

				/* Icky weapon */
				p_ptr->icky_wield[i] = TRUE;
			}
			else
			{
				/* Reduce the real bonuses */
				p_ptr->to_h[i] -= 30;
				p_ptr->to_d[i] -= 10;

				/* Reduce the mental bonuses */
				p_ptr->dis_to_h[i] -= 30;
				p_ptr->dis_to_d[i] -= 10;
			}
		}
		/* Hex bonuses */
		if (p_ptr->realm1 == REALM_HEX)
		{
			if (object_is_cursed(o_ptr))
			{
				if (o_ptr->curse_flags & (TRC_CURSED)) { p_ptr->to_h[i] += 5; p_ptr->dis_to_h[i] += 5; }
				if (o_ptr->curse_flags & (TRC_HEAVY_CURSE)) { p_ptr->to_h[i] += 7; p_ptr->dis_to_h[i] += 7; }
				if (o_ptr->curse_flags & (TRC_PERMA_CURSE)) { p_ptr->to_h[i] += 13; p_ptr->dis_to_h[i] += 13; }
				if (o_ptr->curse_flags & (TRC_TY_CURSE)) { p_ptr->to_h[i] += 5; p_ptr->dis_to_h[i] += 5; }
				if (hex_spelling(HEX_RUNESWORD))
				{
					if (o_ptr->curse_flags & (TRC_CURSED)) { p_ptr->to_d[i] += 5; p_ptr->dis_to_d[i] += 5; }
					if (o_ptr->curse_flags & (TRC_HEAVY_CURSE)) { p_ptr->to_d[i] += 7; p_ptr->dis_to_d[i] += 7; }
					if (o_ptr->curse_flags & (TRC_PERMA_CURSE)) { p_ptr->to_d[i] += 13; p_ptr->dis_to_d[i] += 13; }
				}
			}
		}
		if (p_ptr->riding)
		{
			if ((o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE)))
			{
				p_ptr->to_h[i] += 15;
				p_ptr->dis_to_h[i] += 15;
				p_ptr->to_dd[i] += 2;
			}
			else if (!(have_flag(flgs, TR_RIDING)))
			{
				int penalty;
				if ((p_ptr->pclass == CLASS_BEASTMASTER) || (p_ptr->pclass == CLASS_CAVALRY))
				{
					penalty = 5;
				}
				else
				{
					penalty = r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].level - p_ptr->skill_exp[GINOU_RIDING] / 80;
					penalty += 30;
					if (penalty < 30) penalty = 30;
				}
				p_ptr->to_h[i] -= (s16b)penalty;
				p_ptr->dis_to_h[i] -= (s16b)penalty;

				/* Riding weapon */
				p_ptr->riding_wield[i] = TRUE;
			}
		}
	}

	if (p_ptr->riding)
	{
		int penalty = 0;

		p_ptr->riding_ryoute = FALSE;

		if (p_ptr->ryoute || (empty_hands(FALSE) == EMPTY_HAND_NONE)) p_ptr->riding_ryoute = TRUE;
		else if (p_ptr->pet_extra_flags & PF_RYOUTE)
		{
			switch (p_ptr->pclass)
			{
			case CLASS_MONK:
			case CLASS_FORCETRAINER:
			case CLASS_BERSERKER:
				if ((empty_hands(FALSE) != EMPTY_HAND_NONE) && !has_melee_weapon(INVEN_RARM) && !has_melee_weapon(INVEN_LARM))
					p_ptr->riding_ryoute = TRUE;
				break;
			}
		}

		if ((p_ptr->pclass == CLASS_BEASTMASTER) || (p_ptr->pclass == CLASS_CAVALRY))
		{
			if (p_ptr->tval_ammo != TV_ARROW) penalty = 5;
		}
		else
		{
			penalty = r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx].level - p_ptr->skill_exp[GINOU_RIDING] / 80;
			penalty += 30;
			if (penalty < 30) penalty = 30;
		}
		if (p_ptr->tval_ammo == TV_BOLT) penalty *= 2;
		p_ptr->to_h_b -= (s16b)penalty;
		p_ptr->dis_to_h_b -= (s16b)penalty;
	}

	/* Different calculation for monks with empty hands */
	if (((p_ptr->pclass == CLASS_MONK) || (p_ptr->pclass == CLASS_FORCETRAINER) || (p_ptr->pclass == CLASS_BERSERKER)) &&
		(empty_hands_status & EMPTY_HAND_RARM) && !p_ptr->hidarite)
	{
		int blow_base = p_ptr->lev + adj_dex_blow[p_ptr->stat_ind[A_DEX]];
		p_ptr->num_blow[0] = 0;

		if (p_ptr->pclass == CLASS_FORCETRAINER)
		{
			if (blow_base > 18) p_ptr->num_blow[0]++;
			if (blow_base > 31) p_ptr->num_blow[0]++;
			if (blow_base > 44) p_ptr->num_blow[0]++;
			if (blow_base > 58) p_ptr->num_blow[0]++;
			if (P_PTR_KI)
			{
				p_ptr->to_d[0] += P_PTR_KI / 5;
				p_ptr->dis_to_d[0] += P_PTR_KI / 5;
			}
		}
		else
		{
			if (blow_base > 12) p_ptr->num_blow[0]++;
			if (blow_base > 22) p_ptr->num_blow[0]++;
			if (blow_base > 31) p_ptr->num_blow[0]++;
			if (blow_base > 39) p_ptr->num_blow[0]++;
			if (blow_base > 46) p_ptr->num_blow[0]++;
			if (blow_base > 53) p_ptr->num_blow[0]++;
			if (blow_base > 59) p_ptr->num_blow[0]++;
		}

		if (heavy_armor() && (p_ptr->pclass != CLASS_BERSERKER))
			p_ptr->num_blow[0] /= 2;
		else
		{
			p_ptr->to_h[0] += (p_ptr->lev / 3);
			p_ptr->dis_to_h[0] += (p_ptr->lev / 3);

			p_ptr->to_d[0] += (p_ptr->lev / 6);
			p_ptr->dis_to_d[0] += (p_ptr->lev / 6);
		}

		if (p_ptr->special_defense & KAMAE_BYAKKO)
		{
			p_ptr->to_a -= 40;
			p_ptr->dis_to_a -= 40;

		}
		else if (p_ptr->special_defense & KAMAE_SEIRYU)
		{
			p_ptr->to_a -= 50;
			p_ptr->dis_to_a -= 50;
			p_ptr->resist_acid = TRUE;
			p_ptr->resist_fire = TRUE;
			p_ptr->resist_elec = TRUE;
			p_ptr->resist_cold = TRUE;
			p_ptr->resist_pois = TRUE;
			p_ptr->sh_fire = TRUE;
			p_ptr->sh_elec = TRUE;
			p_ptr->sh_cold = TRUE;
			p_ptr->levitation = TRUE;
		}
		else if (p_ptr->special_defense & KAMAE_GENBU)
		{
			p_ptr->to_a += (p_ptr->lev*p_ptr->lev) / 50;
			p_ptr->dis_to_a += (p_ptr->lev*p_ptr->lev) / 50;
			p_ptr->reflect = TRUE;
			p_ptr->num_blow[0] -= 2;
			if ((p_ptr->pclass == CLASS_MONK) && (p_ptr->lev > 42)) p_ptr->num_blow[0]--;
			if (p_ptr->num_blow[0] < 0) p_ptr->num_blow[0] = 0;
		}
		else if (p_ptr->special_defense & KAMAE_SUZAKU)
		{
			p_ptr->to_h[0] -= (p_ptr->lev / 3);
			p_ptr->to_d[0] -= (p_ptr->lev / 6);

			p_ptr->dis_to_h[0] -= (p_ptr->lev / 3);
			p_ptr->dis_to_d[0] -= (p_ptr->lev / 6);
			p_ptr->num_blow[0] /= 2;
			p_ptr->levitation = TRUE;
		}

		p_ptr->num_blow[0] += 1 + extra_blows[0];
	}

	if (p_ptr->riding) p_ptr->levitation = riding_levitation;

	p_ptr->monk_armour_aux = FALSE;

	if (heavy_armor())
	{
		p_ptr->monk_armour_aux = TRUE;
	}

	for (i = 0; i < 2; i++)
	{
		if (has_melee_weapon(INVEN_RARM + i))
		{
			OBJECT_TYPE_VALUE tval = p_ptr->inventory_list[INVEN_RARM + i].tval - TV_WEAPON_BEGIN;
			OBJECT_SUBTYPE_VALUE sval = p_ptr->inventory_list[INVEN_RARM + i].sval;

			p_ptr->to_h[i] += (p_ptr->weapon_exp[tval][sval] - WEAPON_EXP_BEGINNER) / 200;
			p_ptr->dis_to_h[i] += (p_ptr->weapon_exp[tval][sval] - WEAPON_EXP_BEGINNER) / 200;
			if ((p_ptr->pclass == CLASS_MONK) || (p_ptr->pclass == CLASS_FORCETRAINER))
			{
				if (!s_info[p_ptr->pclass].w_max[tval][sval])
				{
					p_ptr->to_h[i] -= 40;
					p_ptr->dis_to_h[i] -= 40;
					p_ptr->icky_wield[i] = TRUE;
				}
			}
			else if (p_ptr->pclass == CLASS_NINJA)
			{
				if ((s_info[CLASS_NINJA].w_max[tval][sval] <= WEAPON_EXP_BEGINNER) || (p_ptr->inventory_list[INVEN_LARM - i].tval == TV_SHIELD))
				{
					p_ptr->to_h[i] -= 40;
					p_ptr->dis_to_h[i] -= 40;
					p_ptr->icky_wield[i] = TRUE;
					p_ptr->num_blow[i] /= 2;
					if (p_ptr->num_blow[i] < 1) p_ptr->num_blow[i] = 1;
				}
			}
		}
	}

	/* Maximum speed is (+99). (internally it's 110 + 99) */
	/* Temporary lightspeed forces to be maximum speed */
	if ((p_ptr->lightspeed && !p_ptr->riding) || (new_speed > 209))
	{
		new_speed = 209;
	}

	/* Minimum speed is (-99). (internally it's 110 - 99) */
	if (new_speed < 11) new_speed = 11;

	/* Display the speed (if needed) */
	if (p_ptr->pspeed != (byte)new_speed)
	{
		p_ptr->pspeed = (byte)new_speed;
		p_ptr->redraw |= (PR_SPEED);
	}

	if (yoiyami)
	{
		if (p_ptr->to_a > (0 - p_ptr->ac))
			p_ptr->to_a = 0 - p_ptr->ac;
		if (p_ptr->dis_to_a > (0 - p_ptr->dis_ac))
			p_ptr->dis_to_a = 0 - p_ptr->dis_ac;
	}

	/* Redraw armor (if needed) */
	if ((p_ptr->dis_ac != old_dis_ac) || (p_ptr->dis_to_a != old_dis_to_a))
	{
		p_ptr->redraw |= (PR_ARMOR);
		p_ptr->window |= (PW_PLAYER);
	}

	if (p_ptr->ryoute && !omoi)
	{
		int bonus_to_h = 0, bonus_to_d = 0;
		bonus_to_d = ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128) / 2;
		bonus_to_h = ((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128) + ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);

		p_ptr->to_h[default_hand] += MAX(bonus_to_h, 1);
		p_ptr->dis_to_h[default_hand] += MAX(bonus_to_h, 1);
		p_ptr->to_d[default_hand] += MAX(bonus_to_d, 1);
		p_ptr->dis_to_d[default_hand] += MAX(bonus_to_d, 1);
	}

	if (((p_ptr->pclass == CLASS_MONK) || (p_ptr->pclass == CLASS_FORCETRAINER) || (p_ptr->pclass == CLASS_BERSERKER)) && (empty_hands(FALSE) == (EMPTY_HAND_RARM | EMPTY_HAND_LARM))) p_ptr->ryoute = FALSE;

	/* Affect Skill -- stealth (bonus one) */
	p_ptr->skill_stl += 1;

	if (IS_TIM_STEALTH()) p_ptr->skill_stl += 99;

	/* Affect Skill -- disarming (DEX and INT) */
	p_ptr->skill_dis += adj_dex_dis[p_ptr->stat_ind[A_DEX]];
	p_ptr->skill_dis += adj_int_dis[p_ptr->stat_ind[A_INT]];

	/* Affect Skill -- magic devices (INT) */
	p_ptr->skill_dev += adj_int_dev[p_ptr->stat_ind[A_INT]];

	/* Affect Skill -- saving throw (WIS) */
	p_ptr->skill_sav += adj_wis_sav[p_ptr->stat_ind[A_WIS]];

	/* Affect Skill -- digging (STR) */
	p_ptr->skill_dig += adj_str_dig[p_ptr->stat_ind[A_STR]];

	/* Affect Skill -- disarming (Level, by Class) */
	p_ptr->skill_dis += ((cp_ptr->x_dis * p_ptr->lev / 10) + (ap_ptr->a_dis * p_ptr->lev / 50));

	/* Affect Skill -- magic devices (Level, by Class) */
	p_ptr->skill_dev += ((cp_ptr->x_dev * p_ptr->lev / 10) + (ap_ptr->a_dev * p_ptr->lev / 50));

	/* Affect Skill -- saving throw (Level, by Class) */
	p_ptr->skill_sav += ((cp_ptr->x_sav * p_ptr->lev / 10) + (ap_ptr->a_sav * p_ptr->lev / 50));

	/* Affect Skill -- stealth (Level, by Class) */
	p_ptr->skill_stl += (cp_ptr->x_stl * p_ptr->lev / 10);

	/* Affect Skill -- search ability (Level, by Class) */
	p_ptr->skill_srh += (cp_ptr->x_srh * p_ptr->lev / 10);

	/* Affect Skill -- search frequency (Level, by Class) */
	p_ptr->skill_fos += (cp_ptr->x_fos * p_ptr->lev / 10);

	/* Affect Skill -- combat (normal) (Level, by Class) */
	p_ptr->skill_thn += ((cp_ptr->x_thn * p_ptr->lev / 10) + (ap_ptr->a_thn * p_ptr->lev / 50));

	/* Affect Skill -- combat (shooting) (Level, by Class) */
	p_ptr->skill_thb += ((cp_ptr->x_thb * p_ptr->lev / 10) + (ap_ptr->a_thb * p_ptr->lev / 50));

	/* Affect Skill -- combat (throwing) (Level, by Class) */
	p_ptr->skill_tht += ((cp_ptr->x_thb * p_ptr->lev / 10) + (ap_ptr->a_thb * p_ptr->lev / 50));


	if ((prace_is_(RACE_S_FAIRY)) && (p_ptr->pseikaku != SEIKAKU_SEXY) && (p_ptr->cursed & TRC_AGGRAVATE))
	{
		p_ptr->cursed &= ~(TRC_AGGRAVATE);
		p_ptr->skill_stl = MIN(p_ptr->skill_stl - 3, (p_ptr->skill_stl + 2) / 2);
	}

	/* Limit Skill -- stealth from 0 to 30 */
	if (p_ptr->skill_stl > 30) p_ptr->skill_stl = 30;
	if (p_ptr->skill_stl < 0) p_ptr->skill_stl = 0;

	/* Limit Skill -- digging from 1 up */
	if (p_ptr->skill_dig < 1) p_ptr->skill_dig = 1;

	if (p_ptr->anti_magic && (p_ptr->skill_sav < (90 + p_ptr->lev))) p_ptr->skill_sav = 90 + p_ptr->lev;

	if (p_ptr->tsubureru) p_ptr->skill_sav = 10;

	if ((p_ptr->ult_res || p_ptr->resist_magic || p_ptr->magicdef) && (p_ptr->skill_sav < (95 + p_ptr->lev))) p_ptr->skill_sav = 95 + p_ptr->lev;

	if (down_saving) p_ptr->skill_sav /= 2;

	/* Hack -- Each elemental immunity includes resistance */
	if (p_ptr->immune_acid) p_ptr->resist_acid = TRUE;
	if (p_ptr->immune_elec) p_ptr->resist_elec = TRUE;
	if (p_ptr->immune_fire) p_ptr->resist_fire = TRUE;
	if (p_ptr->immune_cold) p_ptr->resist_cold = TRUE;


	/* Hack -- handle "xtra" mode */
	if (character_xtra) return;

	/* Take note when "heavy bow" changes */
	if (p_ptr->old_heavy_shoot != p_ptr->heavy_shoot)
	{
		if (p_ptr->heavy_shoot)
		{
			msg_print(_("こんな重い弓を装備しているのは大変だ。", "You have trouble wielding such a heavy bow."));
		}
		else if (p_ptr->inventory_list[INVEN_BOW].k_idx)
		{
			msg_print(_("この弓なら装備していても辛くない。", "You have no trouble wielding your bow."));
		}
		else
		{
			msg_print(_("重い弓を装備からはずして体が楽になった。", "You feel relieved to put down your heavy bow."));
		}

		/* Save it */
		p_ptr->old_heavy_shoot = p_ptr->heavy_shoot;
	}

	for (i = 0; i < 2; i++)
	{
		/* Take note when "heavy weapon" changes */
		if (p_ptr->old_heavy_wield[i] != p_ptr->heavy_wield[i])
		{
			if (p_ptr->heavy_wield[i])
			{
				msg_print(_("こんな重い武器を装備しているのは大変だ。", "You have trouble wielding such a heavy weapon."));
			}
			else if (has_melee_weapon(INVEN_RARM + i))
			{
				msg_print(_("これなら装備していても辛くない。", "You have no trouble wielding your weapon."));
			}
			else if (p_ptr->heavy_wield[1 - i])
			{
				msg_print(_("まだ武器が重い。", "You have still trouble wielding a heavy weapon."));
			}
			else
			{
				msg_print(_("重い武器を装備からはずして体が楽になった。", "You feel relieved to put down your heavy weapon."));
			}

			/* Save it */
			p_ptr->old_heavy_wield[i] = p_ptr->heavy_wield[i];
		}

		/* Take note when "heavy weapon" changes */
		if (p_ptr->old_riding_wield[i] != p_ptr->riding_wield[i])
		{
			if (p_ptr->riding_wield[i])
			{
				msg_print(_("この武器は乗馬中に使うにはむかないようだ。", "This weapon is not suitable for use while riding."));
			}
			else if (!p_ptr->riding)
			{
				msg_print(_("この武器は徒歩で使いやすい。", "This weapon was not suitable for use while riding."));
			}
			else if (has_melee_weapon(INVEN_RARM + i))
			{
				msg_print(_("これなら乗馬中にぴったりだ。", "This weapon is suitable for use while riding."));
			}
			/* Save it */
			p_ptr->old_riding_wield[i] = p_ptr->riding_wield[i];
		}

		/* Take note when "illegal weapon" changes */
		if (p_ptr->old_icky_wield[i] != p_ptr->icky_wield[i])
		{
			if (p_ptr->icky_wield[i])
			{
				msg_print(_("今の装備はどうも自分にふさわしくない気がする。", "You do not feel comfortable with your weapon."));
				if (is_loading_now)
				{
					chg_virtue(V_FAITH, -1);
				}
			}
			else if (has_melee_weapon(INVEN_RARM + i))
			{
				msg_print(_("今の装備は自分にふさわしい気がする。", "You feel comfortable with your weapon."));
			}
			else
			{
				msg_print(_("装備をはずしたら随分と気が楽になった。", "You feel more comfortable after removing your weapon."));
			}

			/* Save it */
			p_ptr->old_icky_wield[i] = p_ptr->icky_wield[i];
		}
	}

	if (p_ptr->riding && (p_ptr->old_riding_ryoute != p_ptr->riding_ryoute))
	{
		if (p_ptr->riding_ryoute)
		{
#ifdef JP
			msg_format("%s馬を操れない。", (empty_hands(FALSE) == EMPTY_HAND_NONE) ? "両手がふさがっていて" : "");
#else
			msg_print("You are using both hand for fighting, and you can't control a riding pet.");
#endif
		}
		else
		{
#ifdef JP
			msg_format("%s馬を操れるようになった。", (empty_hands(FALSE) == EMPTY_HAND_NONE) ? "手が空いて" : "");
#else
			msg_print("You began to control riding pet with one hand.");
#endif
		}

		p_ptr->old_riding_ryoute = p_ptr->riding_ryoute;
	}

	if (((p_ptr->pclass == CLASS_MONK) || (p_ptr->pclass == CLASS_FORCETRAINER) || (p_ptr->pclass == CLASS_NINJA)) && (p_ptr->monk_armour_aux != p_ptr->monk_notify_aux))
	{
		if (heavy_armor())
		{
			msg_print(_("装備が重くてバランスを取れない。", "The weight of your armor disrupts your balance."));
			if (is_loading_now)
			{
				chg_virtue(V_HARMONY, -1);
			}
		}
		else
		{
			msg_print(_("バランスがとれるようになった。", "You regain your balance."));
		}

		p_ptr->monk_notify_aux = p_ptr->monk_armour_aux;
	}

	for (i = 0; i < INVEN_PACK; i++)
	{
#if 0
		if ((p_ptr->inventory_list[i].tval == TV_SORCERY_BOOK) && (p_ptr->inventory_list[i].sval == 2)) have_dd_s = TRUE;
		if ((p_ptr->inventory_list[i].tval == TV_TRUMP_BOOK) && (p_ptr->inventory_list[i].sval == 1)) have_dd_t = TRUE;
#endif
		if ((p_ptr->inventory_list[i].tval == TV_NATURE_BOOK) && (p_ptr->inventory_list[i].sval == 2)) have_sw = TRUE;
		if ((p_ptr->inventory_list[i].tval == TV_CRAFT_BOOK) && (p_ptr->inventory_list[i].sval == 2)) have_kabe = TRUE;
	}

	for (this_o_idx = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

#if 0
		if ((o_ptr->tval == TV_SORCERY_BOOK) && (o_ptr->sval == 3)) have_dd_s = TRUE;
		if ((o_ptr->tval == TV_TRUMP_BOOK) && (o_ptr->sval == 1)) have_dd_t = TRUE;
#endif
		if ((o_ptr->tval == TV_NATURE_BOOK) && (o_ptr->sval == 2)) have_sw = TRUE;
		if ((o_ptr->tval == TV_CRAFT_BOOK) && (o_ptr->sval == 2)) have_kabe = TRUE;
	}

	if (p_ptr->pass_wall && !p_ptr->kill_wall) p_ptr->no_flowed = TRUE;
#if 0
	if (have_dd_s && ((p_ptr->realm1 == REALM_SORCERY) || (p_ptr->realm2 == REALM_SORCERY) || (p_ptr->pclass == CLASS_SORCERER)))
	{
		const magic_type *s_ptr = &mp_ptr->info[REALM_SORCERY - 1][SPELL_DD_S];
		if (p_ptr->lev >= s_ptr->slevel) p_ptr->no_flowed = TRUE;
	}

	if (have_dd_t && ((p_ptr->realm1 == REALM_TRUMP) || (p_ptr->realm2 == REALM_TRUMP) || (p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE)))
	{
		const magic_type *s_ptr = &mp_ptr->info[REALM_TRUMP - 1][SPELL_DD_T];
		if (p_ptr->lev >= s_ptr->slevel) p_ptr->no_flowed = TRUE;
	}
#endif
	if (have_sw && ((p_ptr->realm1 == REALM_NATURE) || (p_ptr->realm2 == REALM_NATURE) || (p_ptr->pclass == CLASS_SORCERER)))
	{
		const magic_type *s_ptr = &mp_ptr->info[REALM_NATURE - 1][SPELL_SW];
		if (p_ptr->lev >= s_ptr->slevel) p_ptr->no_flowed = TRUE;
	}

	if (have_kabe && ((p_ptr->realm1 == REALM_CRAFT) || (p_ptr->realm2 == REALM_CRAFT) || (p_ptr->pclass == CLASS_SORCERER)))
	{
		const magic_type *s_ptr = &mp_ptr->info[REALM_CRAFT - 1][SPELL_KABE];
		if (p_ptr->lev >= s_ptr->slevel) p_ptr->no_flowed = TRUE;
	}
}


static void calc_alignment(void)
{
	MONSTER_IDX m_idx;
	p_ptr->align = 0;
	int i, j, neutral[2];

	for (m_idx = m_max - 1; m_idx >= 1; m_idx--)
	{
		monster_type *m_ptr;
		monster_race *r_ptr;
		m_ptr = &current_floor_ptr->m_list[m_idx];
		if (!monster_is_valid(m_ptr)) continue;
		r_ptr = &r_info[m_ptr->r_idx];

		if (is_pet(m_ptr))
		{
			if (r_ptr->flags3 & RF3_GOOD) p_ptr->align += r_ptr->level;
			if (r_ptr->flags3 & RF3_EVIL) p_ptr->align -= r_ptr->level;
		}
	}

	if (p_ptr->mimic_form)
	{
		switch (p_ptr->mimic_form)
		{
		case MIMIC_DEMON:
			p_ptr->align -= 200;
			break;
		case MIMIC_DEMON_LORD:
			p_ptr->align -= 200;
			break;
		}
	}
	else
	{
		switch (p_ptr->prace)
		{
		case RACE_ANGEL:
			p_ptr->align += 200;
			break;
		case RACE_DEMON:
			p_ptr->align -= 200;
			break;
		}
	}

	for (i = 0; i < 2; i++)
	{
		if (has_melee_weapon(INVEN_RARM + i))
		{
			if (p_ptr->inventory_list[INVEN_RARM + i].name1 == ART_IRON_BALL) p_ptr->align -= 1000;
		}
	}

	/* Determine player alignment */
	for (i = 0, j = 0; i < 8; i++)
	{
		switch (p_ptr->vir_types[i])
		{
		case V_JUSTICE:
			p_ptr->align += p_ptr->virtues[i] * 2;
			break;
		case V_CHANCE:
			/* Do nothing */
			break;
		case V_NATURE:
		case V_HARMONY:
			neutral[j++] = i;
			break;
		case V_UNLIFE:
			p_ptr->align -= p_ptr->virtues[i];
			break;
		default:
			p_ptr->align += p_ptr->virtues[i];
			break;
		}
	}

	for (i = 0; i < j; i++)
	{
		if (p_ptr->align > 0)
		{
			p_ptr->align -= p_ptr->virtues[neutral[i]] / 2;
			if (p_ptr->align < 0) p_ptr->align = 0;
		}
		else if (p_ptr->align < 0)
		{
			p_ptr->align += p_ptr->virtues[neutral[i]] / 2;
			if (p_ptr->align > 0) p_ptr->align = 0;
		}
	}
}

/*!
 * @brief プレイヤーの最大HPを計算する /
 * Calculate the players (maximal) hit points
 * Adjust current hitpoints if necessary
 * @return なし
 * @details
 */
static void calc_hitpoints(void)
{
	int bonus, mhp;
	byte tmp_hitdie;

	/* Un-inflate "half-hitpoint bonus per level" value */
	bonus = ((int)(adj_con_mhp[p_ptr->stat_ind[A_CON]]) - 128) * p_ptr->lev / 4;

	/* Calculate hitpoints */
	mhp = p_ptr->player_hp[p_ptr->lev - 1];

	if (p_ptr->mimic_form)
	{
		if (p_ptr->pclass == CLASS_SORCERER)
			tmp_hitdie = mimic_info[p_ptr->mimic_form].r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
		else
			tmp_hitdie = mimic_info[p_ptr->mimic_form].r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
		mhp = mhp * tmp_hitdie / p_ptr->hitdie;
	}

	if (p_ptr->pclass == CLASS_SORCERER)
	{
		if (p_ptr->lev < 30)
			mhp = (mhp * (45 + p_ptr->lev) / 100);
		else
			mhp = (mhp * 75 / 100);
		bonus = (bonus * 65 / 100);
	}

	mhp += bonus;

	if (p_ptr->pclass == CLASS_BERSERKER)
	{
		mhp = mhp * (110 + (((p_ptr->lev + 40) * (p_ptr->lev + 40) - 1550) / 110)) / 100;
	}

	/* Always have at least one hitpoint per level */
	if (mhp < p_ptr->lev + 1) mhp = p_ptr->lev + 1;

	/* Factor in the hero / superhero settings */
	if (IS_HERO()) mhp += 10;
	if (p_ptr->shero && (p_ptr->pclass != CLASS_BERSERKER)) mhp += 30;
	if (p_ptr->tsuyoshi) mhp += 50;

	/* Factor in the hex spell settings */
	if (hex_spelling(HEX_XTRA_MIGHT)) mhp += 15;
	if (hex_spelling(HEX_BUILDING)) mhp += 60;

	/* New maximum hitpoints */
	if (p_ptr->mhp != mhp)
	{
		/* Enforce maximum */
		if (p_ptr->chp >= mhp)
		{
			p_ptr->chp = mhp;
			p_ptr->chp_frac = 0;
		}

#ifdef JP
		/* レベルアップの時は上昇量を表示する */
		if ((level_up == 1) && (mhp > p_ptr->mhp))
		{
			msg_format("最大ヒット・ポイントが %d 増加した！", (mhp - p_ptr->mhp));
		}
#endif
		/* Save the new max-hitpoints */
		p_ptr->mhp = mhp;

		/* Display hitpoints (later) */
		p_ptr->redraw |= (PR_HP);
		p_ptr->window |= (PW_PLAYER);
	}
}

/*!
 * @brief プレイヤーの光源半径を計算する / Extract and set the current "lite radius"
 * @return なし
 * @details
 * SWD: Experimental modification: multiple light sources have additive effect.
 */
static void calc_torch(void)
{
	int i;
	POSITION rad;
	object_type *o_ptr;
	BIT_FLAGS flgs[TR_FLAG_SIZE];

	/* Assume no light */
	p_ptr->cur_lite = 0;

	/* Loop through all wielded items */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		if (o_ptr->name2 == EGO_LITE_SHINE) p_ptr->cur_lite++;

		/* Need Fuels */
		if (o_ptr->name2 != EGO_LITE_DARKNESS)
		{
			if (o_ptr->tval == TV_LITE)
			{
				if ((o_ptr->sval == SV_LITE_TORCH) && !(o_ptr->xtra4 > 0)) continue;
				if ((o_ptr->sval == SV_LITE_LANTERN) && !(o_ptr->xtra4 > 0)) continue;
			}
		}
		object_flags(o_ptr, flgs);

		/* calc the lite_radius */

		rad = 0;
		if (have_flag(flgs, TR_LITE_1) && o_ptr->name2 != EGO_LITE_DARKNESS)  rad += 1;
		if (have_flag(flgs, TR_LITE_2) && o_ptr->name2 != EGO_LITE_DARKNESS)  rad += 2;
		if (have_flag(flgs, TR_LITE_3) && o_ptr->name2 != EGO_LITE_DARKNESS)  rad += 3;
		if (have_flag(flgs, TR_LITE_M1)) rad -= 1;
		if (have_flag(flgs, TR_LITE_M2)) rad -= 2;
		if (have_flag(flgs, TR_LITE_M3)) rad -= 3;
		p_ptr->cur_lite += rad;
	}

	/* max radius is 14 (was 5) without rewriting other code -- */
	/* see current_floor_ptr->grid_array.c:update_lite() and defines.h:LITE_MAX */
	if (d_info[p_ptr->dungeon_idx].flags1 & DF1_DARKNESS && p_ptr->cur_lite > 1)
		p_ptr->cur_lite = 1;

	/*
	 * check if the player doesn't have light radius,
	 * but does weakly glow as an intrinsic.
	 */
	if (p_ptr->cur_lite <= 0 && p_ptr->lite) p_ptr->cur_lite++;

	if (p_ptr->cur_lite > 14) p_ptr->cur_lite = 14;
	if (p_ptr->cur_lite < 0) p_ptr->cur_lite = 0;

	/* end experimental mods */

	/* Notice changes in the "lite radius" */
	if (p_ptr->old_lite != p_ptr->cur_lite)
	{
		/* Hack -- PU_MON_LITE for monsters' darkness */
		p_ptr->update |= (PU_LITE | PU_MON_LITE | PU_MONSTERS);

		/* Remember the old lite */
		p_ptr->old_lite = p_ptr->cur_lite;

		if ((p_ptr->cur_lite > 0) && (p_ptr->special_defense & NINJA_S_STEALTH))
			set_superstealth(FALSE);
	}
}

/*!
 * @brief プレイヤーの現在学習可能な魔法数を計算し、増減に応じて魔法の忘却、再学習を処置する。 /
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 * @return なし
 * @details
 * Note that this function induces various "status" messages,
 * which must be bypasses until the character is created.
 */
static void calc_spells(void)
{
	int i, j, k, levels;
	int num_allowed;
	int num_boukyaku = 0;

	const magic_type	*s_ptr;
	REALM_IDX which;
	int bonus = 0;

	concptr p;

	/* Hack -- must be literate */
	if (!mp_ptr->spell_book) return;

	/* Hack -- wait for creation */
	if (!character_generated) return;

	/* Hack -- handle "xtra" mode */
	if (character_xtra) return;

	if ((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE))
	{
		p_ptr->new_spells = 0;
		return;
	}

	p = spell_category_name(mp_ptr->spell_book);

	/* Determine the number of spells allowed */
	levels = p_ptr->lev - mp_ptr->spell_first + 1;

	/* Hack -- no negative spells */
	if (levels < 0) levels = 0;

	/* Extract total allowed spells */
	num_allowed = (adj_mag_study[p_ptr->stat_ind[mp_ptr->spell_stat]] * levels / 2);

	if ((p_ptr->pclass != CLASS_SAMURAI) && (mp_ptr->spell_book != TV_LIFE_BOOK))
	{
		bonus = 4;
	}
	if (p_ptr->pclass == CLASS_SAMURAI)
	{
		num_allowed = 32;
	}
	else if (p_ptr->realm2 == REALM_NONE)
	{
		num_allowed = (num_allowed + 1) / 2;
		if (num_allowed > (32 + bonus)) num_allowed = 32 + bonus;
	}
	else if ((p_ptr->pclass == CLASS_MAGE) || (p_ptr->pclass == CLASS_PRIEST))
	{
		if (num_allowed > (96 + bonus)) num_allowed = 96 + bonus;
	}
	else
	{
		if (num_allowed > (80 + bonus)) num_allowed = 80 + bonus;
	}

	/* Count the number of spells we know */
	for (j = 0; j < 64; j++)
	{
		/* Count known spells */
		if ((j < 32) ?
			(p_ptr->spell_forgotten1 & (1L << j)) :
			(p_ptr->spell_forgotten2 & (1L << (j - 32))))
		{
			num_boukyaku++;
		}
	}

	/* See how many spells we must forget or may learn */
	p_ptr->new_spells = num_allowed + p_ptr->add_spells + num_boukyaku - p_ptr->learned_spells;

	/* Forget spells which are too hard */
	for (i = 63; i >= 0; i--)
	{
		/* Efficiency -- all done */
		if (!p_ptr->spell_learned1 && !p_ptr->spell_learned2) break;

		/* Access the spell */
		j = p_ptr->spell_order[i];

		/* Skip non-spells */
		if (j >= 99) continue;


		/* Get the spell */
		if (!is_magic((j < 32) ? p_ptr->realm1 : p_ptr->realm2))
		{
			if (j < 32)
				s_ptr = &technic_info[p_ptr->realm1 - MIN_TECHNIC][j];
			else
				s_ptr = &technic_info[p_ptr->realm2 - MIN_TECHNIC][j % 32];
		}
		else if (j < 32)
			s_ptr = &mp_ptr->info[p_ptr->realm1 - 1][j];
		else
			s_ptr = &mp_ptr->info[p_ptr->realm2 - 1][j % 32];

		/* Skip spells we are allowed to know */
		if (s_ptr->slevel <= p_ptr->lev) continue;

		/* Is it known? */
		if ((j < 32) ?
			(p_ptr->spell_learned1 & (1L << j)) :
			(p_ptr->spell_learned2 & (1L << (j - 32))))
		{
			/* Mark as forgotten */
			if (j < 32)
			{
				p_ptr->spell_forgotten1 |= (1L << j);
				which = p_ptr->realm1;
			}
			else
			{
				p_ptr->spell_forgotten2 |= (1L << (j - 32));
				which = p_ptr->realm2;
			}

			/* No longer known */
			if (j < 32)
			{
				p_ptr->spell_learned1 &= ~(1L << j);
				which = p_ptr->realm1;
			}
			else
			{
				p_ptr->spell_learned2 &= ~(1L << (j - 32));
				which = p_ptr->realm2;
			}

#ifdef JP
			msg_format("%sの%sを忘れてしまった。", do_spell(which, j % 32, SPELL_NAME), p);
#else
			msg_format("You have forgotten the %s of %s.", p, do_spell(which, j % 32, SPELL_NAME));
#endif


			/* One more can be learned */
			p_ptr->new_spells++;
		}
	}


	/* Forget spells if we know too many spells */
	for (i = 63; i >= 0; i--)
	{
		/* Stop when possible */
		if (p_ptr->new_spells >= 0) break;

		/* Efficiency -- all done */
		if (!p_ptr->spell_learned1 && !p_ptr->spell_learned2) break;

		/* Get the (i+1)th spell learned */
		j = p_ptr->spell_order[i];

		/* Skip unknown spells */
		if (j >= 99) continue;

		/* Forget it (if learned) */
		if ((j < 32) ?
			(p_ptr->spell_learned1 & (1L << j)) :
			(p_ptr->spell_learned2 & (1L << (j - 32))))
		{
			/* Mark as forgotten */
			if (j < 32)
			{
				p_ptr->spell_forgotten1 |= (1L << j);
				which = p_ptr->realm1;
			}
			else
			{
				p_ptr->spell_forgotten2 |= (1L << (j - 32));
				which = p_ptr->realm2;
			}

			/* No longer known */
			if (j < 32)
			{
				p_ptr->spell_learned1 &= ~(1L << j);
				which = p_ptr->realm1;
			}
			else
			{
				p_ptr->spell_learned2 &= ~(1L << (j - 32));
				which = p_ptr->realm2;
			}

#ifdef JP
			msg_format("%sの%sを忘れてしまった。", do_spell(which, j % 32, SPELL_NAME), p);
#else
			msg_format("You have forgotten the %s of %s.", p, do_spell(which, j % 32, SPELL_NAME));
#endif


			/* One more can be learned */
			p_ptr->new_spells++;
		}
	}


	/* Check for spells to remember */
	for (i = 0; i < 64; i++)
	{
		/* None left to remember */
		if (p_ptr->new_spells <= 0) break;

		/* Efficiency -- all done */
		if (!p_ptr->spell_forgotten1 && !p_ptr->spell_forgotten2) break;

		/* Get the next spell we learned */
		j = p_ptr->spell_order[i];

		/* Skip unknown spells */
		if (j >= 99) break;

		/* Access the spell */
		if (!is_magic((j < 32) ? p_ptr->realm1 : p_ptr->realm2))
		{
			if (j < 32)
				s_ptr = &technic_info[p_ptr->realm1 - MIN_TECHNIC][j];
			else
				s_ptr = &technic_info[p_ptr->realm2 - MIN_TECHNIC][j % 32];
		}
		else if (j < 32)
			s_ptr = &mp_ptr->info[p_ptr->realm1 - 1][j];
		else
			s_ptr = &mp_ptr->info[p_ptr->realm2 - 1][j % 32];

		/* Skip spells we cannot remember */
		if (s_ptr->slevel > p_ptr->lev) continue;

		/* First set of spells */
		if ((j < 32) ?
			(p_ptr->spell_forgotten1 & (1L << j)) :
			(p_ptr->spell_forgotten2 & (1L << (j - 32))))
		{
			/* No longer forgotten */
			if (j < 32)
			{
				p_ptr->spell_forgotten1 &= ~(1L << j);
				which = p_ptr->realm1;
			}
			else
			{
				p_ptr->spell_forgotten2 &= ~(1L << (j - 32));
				which = p_ptr->realm2;
			}

			/* Known once more */
			if (j < 32)
			{
				p_ptr->spell_learned1 |= (1L << j);
				which = p_ptr->realm1;
			}
			else
			{
				p_ptr->spell_learned2 |= (1L << (j - 32));
				which = p_ptr->realm2;
			}

#ifdef JP
			msg_format("%sの%sを思い出した。", do_spell(which, j % 32, SPELL_NAME), p);
#else
			msg_format("You have remembered the %s of %s.", p, do_spell(which, j % 32, SPELL_NAME));
#endif


			/* One less can be learned */
			p_ptr->new_spells--;
		}
	}

	k = 0;

	if (p_ptr->realm2 == REALM_NONE)
	{
		/* Count spells that can be learned */
		for (j = 0; j < 32; j++)
		{
			if (!is_magic(p_ptr->realm1)) s_ptr = &technic_info[p_ptr->realm1 - MIN_TECHNIC][j];
			else s_ptr = &mp_ptr->info[p_ptr->realm1 - 1][j];

			/* Skip spells we cannot remember */
			if (s_ptr->slevel > p_ptr->lev) continue;

			/* Skip spells we already know */
			if (p_ptr->spell_learned1 & (1L << j))
			{
				continue;
			}

			/* Count it */
			k++;
		}
		if (k > 32) k = 32;
		if ((p_ptr->new_spells > k) &&
			((mp_ptr->spell_book == TV_LIFE_BOOK) || (mp_ptr->spell_book == TV_HISSATSU_BOOK)))
		{
			p_ptr->new_spells = (s16b)k;
		}
	}

	if (p_ptr->new_spells < 0) p_ptr->new_spells = 0;

	/* Spell count changed */
	if (p_ptr->old_spells != p_ptr->new_spells)
	{
		/* Message if needed */
		if (p_ptr->new_spells)
		{
#ifdef JP
			if (p_ptr->new_spells < 10) {
				msg_format("あと %d つの%sを学べる。", p_ptr->new_spells, p);
			}
			else {
				msg_format("あと %d 個の%sを学べる。", p_ptr->new_spells, p);
			}
#else
			msg_format("You can learn %d more %s%s.",
				p_ptr->new_spells, p,
				(p_ptr->new_spells != 1) ? "s" : "");
#endif

		}

		/* Save the new_spells value */
		p_ptr->old_spells = p_ptr->new_spells;

		/* Redraw Study Status */
		p_ptr->redraw |= (PR_STUDY);

		/* Redraw object recall */
		p_ptr->window |= (PW_OBJECT);
	}
}

/*!
 * @brief プレイヤーの最大MPを計算する /
 * Calculate maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 * @return なし
 * @details
 * This function induces status messages.
 */
static void calc_mana(void)
{
	int msp, levels, cur_wgt, max_wgt;

	object_type *o_ptr;


	/* Hack -- Must be literate */
	if (!mp_ptr->spell_book) return;

	if ((p_ptr->pclass == CLASS_MINDCRAFTER) ||
		(p_ptr->pclass == CLASS_MIRROR_MASTER) ||
		(p_ptr->pclass == CLASS_BLUE_MAGE))
	{
		levels = p_ptr->lev;
	}
	else
	{
		if (mp_ptr->spell_first > p_ptr->lev)
		{
			/* Save new mana */
			p_ptr->msp = 0;

			/* Display mana later */
			p_ptr->redraw |= (PR_MANA);
			return;
		}

		/* Extract "effective" player level */
		levels = (p_ptr->lev - mp_ptr->spell_first) + 1;
	}

	if (p_ptr->pclass == CLASS_SAMURAI)
	{
		msp = (adj_mag_mana[p_ptr->stat_ind[mp_ptr->spell_stat]] + 10) * 2;
		if (msp) msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);
	}
	else
	{
		/* Extract total mana */
		msp = adj_mag_mana[p_ptr->stat_ind[mp_ptr->spell_stat]] * (levels + 3) / 4;

		/* Hack -- usually add one mana */
		if (msp) msp++;

		if (msp) msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);

		if (msp && (p_ptr->pseikaku == SEIKAKU_MUNCHKIN)) msp += msp / 2;

		/* Hack: High mages have a 25% mana bonus */
		if (msp && (p_ptr->pclass == CLASS_HIGH_MAGE)) msp += msp / 4;

		if (msp && (p_ptr->pclass == CLASS_SORCERER)) msp += msp * (25 + p_ptr->lev) / 100;
	}

	/* Only mages are affected */
	if (mp_ptr->spell_xtra & MAGIC_GLOVE_REDUCE_MANA)
	{
		BIT_FLAGS flgs[TR_FLAG_SIZE];

		/* Assume player is not encumbered by gloves */
		p_ptr->cumber_glove = FALSE;

		/* Get the gloves */
		o_ptr = &p_ptr->inventory_list[INVEN_HANDS];

		/* Examine the gloves */
		object_flags(o_ptr, flgs);

		/* Normal gloves hurt mage-type spells */
		if (o_ptr->k_idx &&
			!(have_flag(flgs, TR_FREE_ACT)) &&
			!(have_flag(flgs, TR_DEC_MANA)) &&
			!(have_flag(flgs, TR_EASY_SPELL)) &&
			!((have_flag(flgs, TR_MAGIC_MASTERY)) && (o_ptr->pval > 0)) &&
			!((have_flag(flgs, TR_DEX)) && (o_ptr->pval > 0)))
		{
			/* Encumbered */
			p_ptr->cumber_glove = TRUE;

			/* Reduce mana */
			msp = (3 * msp) / 4;
		}
	}


	/* Assume player not encumbered by armor */
	p_ptr->cumber_armor = FALSE;

	/* Weigh the armor */
	cur_wgt = 0;
	if (p_ptr->inventory_list[INVEN_RARM].tval > TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_RARM].weight;
	if (p_ptr->inventory_list[INVEN_LARM].tval > TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_LARM].weight;
	cur_wgt += p_ptr->inventory_list[INVEN_BODY].weight;
	cur_wgt += p_ptr->inventory_list[INVEN_HEAD].weight;
	cur_wgt += p_ptr->inventory_list[INVEN_OUTER].weight;
	cur_wgt += p_ptr->inventory_list[INVEN_HANDS].weight;
	cur_wgt += p_ptr->inventory_list[INVEN_FEET].weight;

	/* Subtract a percentage of maximum mana. */
	switch (p_ptr->pclass)
	{
		/* For these classes, mana is halved if armour
		 * is 30 pounds over their weight limit. */
	case CLASS_MAGE:
	case CLASS_HIGH_MAGE:
	case CLASS_BLUE_MAGE:
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
	case CLASS_SORCERER:
	{
		if (p_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_RARM].weight;
		if (p_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_LARM].weight;
		break;
	}

	/* Mana halved if armour is 40 pounds over weight limit. */
	case CLASS_PRIEST:
	case CLASS_BARD:
	case CLASS_TOURIST:
	{
		if (p_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_RARM].weight * 2 / 3;
		if (p_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_LARM].weight * 2 / 3;
		break;
	}

	case CLASS_MINDCRAFTER:
	case CLASS_BEASTMASTER:
	case CLASS_MIRROR_MASTER:
	{
		if (p_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_RARM].weight / 2;
		if (p_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_LARM].weight / 2;
		break;
	}

	/* Mana halved if armour is 50 pounds over weight limit. */
	case CLASS_ROGUE:
	case CLASS_RANGER:
	case CLASS_RED_MAGE:
	case CLASS_WARRIOR_MAGE:
	{
		if (p_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_RARM].weight / 3;
		if (p_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_LARM].weight / 3;
		break;
	}

	/* Mana halved if armour is 60 pounds over weight limit. */
	case CLASS_PALADIN:
	case CLASS_CHAOS_WARRIOR:
	{
		if (p_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_RARM].weight / 5;
		if (p_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD) cur_wgt += p_ptr->inventory_list[INVEN_LARM].weight / 5;
		break;
	}

	/* For new classes created, but not yet added to this formula. */
	default:
	{
		break;
	}
	}

	/* Determine the weight allowance */
	max_wgt = mp_ptr->spell_weight;

	/* Heavy armor penalizes mana by a percentage.  -LM- */
	if ((cur_wgt - max_wgt) > 0)
	{
		/* Encumbered */
		p_ptr->cumber_armor = TRUE;

		/* Subtract a percentage of maximum mana. */
		switch (p_ptr->pclass)
		{
			/* For these classes, mana is halved if armour
			 * is 30 pounds over their weight limit. */
		case CLASS_MAGE:
		case CLASS_HIGH_MAGE:
		case CLASS_BLUE_MAGE:
		{
			msp -= msp * (cur_wgt - max_wgt) / 600;
			break;
		}

		/* Mana halved if armour is 40 pounds over weight limit. */
		case CLASS_PRIEST:
		case CLASS_MINDCRAFTER:
		case CLASS_BEASTMASTER:
		case CLASS_BARD:
		case CLASS_FORCETRAINER:
		case CLASS_TOURIST:
		case CLASS_MIRROR_MASTER:
		{
			msp -= msp * (cur_wgt - max_wgt) / 800;
			break;
		}

		case CLASS_SORCERER:
		{
			msp -= msp * (cur_wgt - max_wgt) / 900;
			break;
		}

		/* Mana halved if armour is 50 pounds over weight limit. */
		case CLASS_ROGUE:
		case CLASS_RANGER:
		case CLASS_MONK:
		case CLASS_RED_MAGE:
		{
			msp -= msp * (cur_wgt - max_wgt) / 1000;
			break;
		}

		/* Mana halved if armour is 60 pounds over weight limit. */
		case CLASS_PALADIN:
		case CLASS_CHAOS_WARRIOR:
		case CLASS_WARRIOR_MAGE:
		{
			msp -= msp * (cur_wgt - max_wgt) / 1200;
			break;
		}

		case CLASS_SAMURAI:
		{
			p_ptr->cumber_armor = FALSE;
			break;
		}

		/* For new classes created, but not yet added to this formula. */
		default:
		{
			msp -= msp * (cur_wgt - max_wgt) / 800;
			break;
		}
		}
	}

	/* Mana can never be negative */
	if (msp < 0) msp = 0;


	/* Maximum mana has changed */
	if (p_ptr->msp != msp)
	{
		/* Enforce maximum */
		if ((p_ptr->csp >= msp) && (p_ptr->pclass != CLASS_SAMURAI))
		{
			p_ptr->csp = msp;
			p_ptr->csp_frac = 0;
		}

#ifdef JP
		/* レベルアップの時は上昇量を表示する */
		if ((level_up == 1) && (msp > p_ptr->msp))
		{
			msg_format("最大マジック・ポイントが %d 増加した！", (msp - p_ptr->msp));
		}
#endif
		/* Save new mana */
		p_ptr->msp = msp;

		/* Display mana later */
		p_ptr->redraw |= (PR_MANA);
		p_ptr->window |= (PW_PLAYER | PW_SPELL);
	}


	/* Hack -- handle "xtra" mode */
	if (character_xtra) return;

	/* Take note when "glove state" changes */
	if (p_ptr->old_cumber_glove != p_ptr->cumber_glove)
	{
		if (p_ptr->cumber_glove)
		{
			msg_print(_("手が覆われて呪文が唱えにくい感じがする。", "Your covered hands feel unsuitable for spellcasting."));
		}
		else
		{
			msg_print(_("この手の状態なら、ぐっと呪文が唱えやすい感じだ。", "Your hands feel more suitable for spellcasting."));
		}

		/* Save it */
		p_ptr->old_cumber_glove = p_ptr->cumber_glove;
	}


	/* Take note when "armor state" changes */
	if (p_ptr->old_cumber_armor != p_ptr->cumber_armor)
	{
		if (p_ptr->cumber_armor)
		{
			msg_print(_("装備の重さで動きが鈍くなってしまっている。", "The weight of your equipment encumbers your movement."));
		}
		else
		{
			msg_print(_("ぐっと楽に体を動かせるようになった。", "You feel able to move more freely."));
		}

		/* Save it */
		p_ptr->old_cumber_armor = p_ptr->cumber_armor;
	}
}

/*!
 * @brief 装備中の射撃武器の威力倍率を返す /
 * calcurate the fire rate of target object
 * @param o_ptr 計算する射撃武器のアイテム情報参照ポインタ
 * @return 射撃倍率の値(100で1.00倍)
 */
s16b calc_num_fire(object_type *o_ptr)
{
	int extra_shots = 0;
	int i;
	int num = 0;
	OBJECT_TYPE_VALUE tval_ammo = bow_tval_ammo(o_ptr);
	object_type *q_ptr;
	BIT_FLAGS flgs[TR_FLAG_SIZE];

	/* Scan the usable p_ptr->inventory_list */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		q_ptr = &p_ptr->inventory_list[i];
		if (!q_ptr->k_idx) continue;

		/* Do not apply current equip */
		if (i == INVEN_BOW) continue;

		object_flags(q_ptr, flgs);

		/* Boost shots */
		if (have_flag(flgs, TR_XTRA_SHOTS)) extra_shots++;
	}

	object_flags(o_ptr, flgs);
	if (have_flag(flgs, TR_XTRA_SHOTS)) extra_shots++;

	if (o_ptr->k_idx && !is_heavy_shoot(o_ptr))
	{
		num = 100;
		/* Extra shots */
		num += (extra_shots * 100);

		/* Hack -- Rangers love Bows */
		if ((p_ptr->pclass == CLASS_RANGER) &&
			(tval_ammo == TV_ARROW))
		{
			num += (p_ptr->lev * 4);
		}

		if ((p_ptr->pclass == CLASS_CAVALRY) &&
			(tval_ammo == TV_ARROW))
		{
			num += (p_ptr->lev * 3);
		}

		if (p_ptr->pclass == CLASS_ARCHER)
		{
			if (tval_ammo == TV_ARROW)
				num += ((p_ptr->lev * 5) + 50);
			else if ((tval_ammo == TV_BOLT) || (tval_ammo == TV_SHOT))
				num += (p_ptr->lev * 4);
		}

		/*
		 * Addendum -- also "Reward" high level warriors,
		 * with _any_ missile weapon -- TY
		 */
		if (p_ptr->pclass == CLASS_WARRIOR &&
			(tval_ammo <= TV_BOLT) &&
			(tval_ammo >= TV_SHOT))
		{
			num += (p_ptr->lev * 2);
		}
		if ((p_ptr->pclass == CLASS_ROGUE) &&
			(tval_ammo == TV_SHOT))
		{
			num += (p_ptr->lev * 4);
		}
	}
	return (s16b)num;
}

/*!
 * @brief プレイヤーの所持重量制限を計算する /
 * Computes current weight limit.
 * @return 制限重量(ポンド)
 */
WEIGHT weight_limit(void)
{
	WEIGHT i;

	/* Weight limit based only on strength */
	i = (WEIGHT)adj_str_wgt[p_ptr->stat_ind[A_STR]] * 50; /* Constant was 100 */
	if (p_ptr->pclass == CLASS_BERSERKER) i = i * 3 / 2;

	/* Return the result */
	return i;
}

/*!
 * @brief プレイヤーが現在右手/左手に武器を持っているか判定する /
 * @param i 判定する手のID(右手:0 左手:1)
 * @return 持っているならばTRUE
 */
bool has_melee_weapon(int i)
{
	return ((p_ptr->inventory_list[i].k_idx && object_is_melee_weapon(&p_ptr->inventory_list[i])) ? TRUE : FALSE);
}

/*!
 * @brief プレイヤーの現在開いている手の状態を返す
 * @param riding_control 乗馬中により片手を必要としている状態ならばTRUEを返す。
 * @return 開いている手のビットフラグ
 */
BIT_FLAGS16 empty_hands(bool riding_control)
{
	BIT_FLAGS16 status = EMPTY_HAND_NONE;

	if (!p_ptr->inventory_list[INVEN_RARM].k_idx) status |= EMPTY_HAND_RARM;
	if (!p_ptr->inventory_list[INVEN_LARM].k_idx) status |= EMPTY_HAND_LARM;

	if (riding_control && (status != EMPTY_HAND_NONE) && p_ptr->riding && !(p_ptr->pet_extra_flags & PF_RYOUTE))
	{
		if (status & EMPTY_HAND_LARM) status &= ~(EMPTY_HAND_LARM);
		else if (status & EMPTY_HAND_RARM) status &= ~(EMPTY_HAND_RARM);
	}

	return status;
}


/*!
 * @brief プレイヤーが防具重量制限のある職業時にペナルティを受ける状態にあるかどうかを返す。
 * @return ペナルティが適用されるならばTRUE。
 */
bool heavy_armor(void)
{
	WEIGHT monk_arm_wgt = 0;

	if ((p_ptr->pclass != CLASS_MONK) && (p_ptr->pclass != CLASS_FORCETRAINER) && (p_ptr->pclass != CLASS_NINJA)) return FALSE;

	/* Weight the armor */
	if (p_ptr->inventory_list[INVEN_RARM].tval > TV_SWORD) monk_arm_wgt += p_ptr->inventory_list[INVEN_RARM].weight;
	if (p_ptr->inventory_list[INVEN_LARM].tval > TV_SWORD) monk_arm_wgt += p_ptr->inventory_list[INVEN_LARM].weight;
	monk_arm_wgt += p_ptr->inventory_list[INVEN_BODY].weight;
	monk_arm_wgt += p_ptr->inventory_list[INVEN_HEAD].weight;
	monk_arm_wgt += p_ptr->inventory_list[INVEN_OUTER].weight;
	monk_arm_wgt += p_ptr->inventory_list[INVEN_HANDS].weight;
	monk_arm_wgt += p_ptr->inventory_list[INVEN_FEET].weight;

	return (monk_arm_wgt > (100 + (p_ptr->lev * 4)));
}

/*!
 * @brief p_ptr->update のフラグに応じた更新をまとめて行う / Handle "p_ptr->update"
 * @return なし
 * @details 更新処理の対象はプレイヤーの能力修正/光源寿命/HP/MP/魔法の学習状態、他多数の外界の状態判定。
 */
void update_creature(player_type *creature_ptr)
{
	if (!creature_ptr->update) return;

	/* Actually do auto-destroy */
	if (creature_ptr->update & (PU_AUTODESTROY))
	{
		creature_ptr->update &= ~(PU_AUTODESTROY);
		autopick_delayed_alter();
	}
	if (creature_ptr->update & (PU_COMBINE))
	{
		creature_ptr->update &= ~(PU_COMBINE);
		combine_pack();
	}

	/* Reorder the pack */
	if (creature_ptr->update & (PU_REORDER))
	{
		creature_ptr->update &= ~(PU_REORDER);
		reorder_pack();
	}

	if (creature_ptr->update & (PU_BONUS))
	{
		creature_ptr->update &= ~(PU_BONUS);
		calc_alignment();
		calc_bonuses();
	}

	if (creature_ptr->update & (PU_TORCH))
	{
		creature_ptr->update &= ~(PU_TORCH);
		calc_torch();
	}

	if (creature_ptr->update & (PU_HP))
	{
		creature_ptr->update &= ~(PU_HP);
		calc_hitpoints();
	}

	if (creature_ptr->update & (PU_MANA))
	{
		creature_ptr->update &= ~(PU_MANA);
		calc_mana();
	}

	if (creature_ptr->update & (PU_SPELLS))
	{
		creature_ptr->update &= ~(PU_SPELLS);
		calc_spells();
	}

	/* Character is not ready yet, no screen updates */
	if (!character_generated) return;

	/* Character is in "icky" mode, no screen updates */
	if (character_icky) return;

	if (creature_ptr->update & (PU_UN_LITE))
	{
		creature_ptr->update &= ~(PU_UN_LITE);
		forget_lite();
	}

	if (creature_ptr->update & (PU_UN_VIEW))
	{
		creature_ptr->update &= ~(PU_UN_VIEW);
		forget_view();
	}

	if (creature_ptr->update & (PU_VIEW))
	{
		creature_ptr->update &= ~(PU_VIEW);
		update_view();
	}

	if (creature_ptr->update & (PU_LITE))
	{
		creature_ptr->update &= ~(PU_LITE);
		update_lite();
	}


	if (creature_ptr->update & (PU_FLOW))
	{
		creature_ptr->update &= ~(PU_FLOW);
		update_flow();
	}

	if (creature_ptr->update & (PU_DISTANCE))
	{
		creature_ptr->update &= ~(PU_DISTANCE);

		/* Still need to call update_monsters(FALSE) after update_mon_lite() */
		/* creature_ptr->update &= ~(PU_MONSTERS); */

		update_monsters(TRUE);
	}

	if (creature_ptr->update & (PU_MON_LITE))
	{
		creature_ptr->update &= ~(PU_MON_LITE);
		update_mon_lite();
	}

	/*
	 * Mega-Hack -- Delayed visual update
	 * Only used if update_view(), update_lite() or update_mon_lite() was called
	 */
	if (creature_ptr->update & (PU_DELAY_VIS))
	{
		creature_ptr->update &= ~(PU_DELAY_VIS);
		delayed_visual_update();
	}

	if (creature_ptr->update & (PU_MONSTERS))
	{
		creature_ptr->update &= ~(PU_MONSTERS);
		update_monsters(FALSE);
	}
}

/*!
 * @brief プレイヤーが魔道書を一冊も持っていないかを判定する
 * @return 魔道書を一冊も持っていないならTRUEを返す
 */
bool player_has_no_spellbooks(void)
{
	int i;
	object_type *o_ptr;

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		if (o_ptr->k_idx && check_book_realm(o_ptr->tval, o_ptr->sval)) return FALSE;
	}

	for (i = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; i; i = o_ptr->next_o_idx)
	{
		o_ptr = &current_floor_ptr->o_list[i];
		if (o_ptr->k_idx && (o_ptr->marked & OM_FOUND) && check_book_realm(o_ptr->tval, o_ptr->sval)) return FALSE;
	}

	return TRUE;
}

void take_turn(player_type *creature_ptr, PERCENTAGE need_cost)
{
	creature_ptr->energy_use = (ENERGY)need_cost;
}

void free_turn(player_type *creature_ptr)
{
	creature_ptr->energy_use = 0;
}

/*!
 * @brief プレイヤーを指定座標に配置する / Place the player in the dungeon XXX XXX
 * @param x 配置先X座標
 * @param y 配置先Y座標
 * @return 配置に成功したらTRUE
 */
bool player_place(POSITION y, POSITION x)
{
	/* Paranoia XXX XXX */
	if (current_floor_ptr->grid_array[y][x].m_idx != 0) return FALSE;

	/* Save player location */
	p_ptr->y = y;
	p_ptr->x = x;

	/* Success */
	return TRUE;
}

/*!
 * @brief 種族アンバライトが出血時パターンの上に乗った際のペナルティ処理
 * @return なし
 */
void wreck_the_pattern(void)
{
	int to_ruin = 0;
	POSITION r_y, r_x;
	int pattern_type = f_info[current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].feat].subtype;

	if (pattern_type == PATTERN_TILE_WRECKED)
	{
		/* Ruined already */
		return;
	}

	msg_print(_("パターンを血で汚してしまった！", "You bleed on the Pattern!"));
	msg_print(_("何か恐ろしい事が起こった！", "Something terrible happens!"));

	if (!IS_INVULN()) take_hit(DAMAGE_NOESCAPE, damroll(10, 8), _("パターン損壊", "corrupting the Pattern"), -1);
	to_ruin = randint1(45) + 35;

	while (to_ruin--)
	{
		scatter(&r_y, &r_x, p_ptr->y, p_ptr->x, 4, 0);

		if (pattern_tile(r_y, r_x) &&
			(f_info[current_floor_ptr->grid_array[r_y][r_x].feat].subtype != PATTERN_TILE_WRECKED))
		{
			cave_set_feat(r_y, r_x, feat_pattern_corrupted);
		}
	}

	cave_set_feat(p_ptr->y, p_ptr->x, feat_pattern_corrupted);
}


/*!
 * @brief ELDRITCH_HORRORによるプレイヤーの精神破壊処理
 * @param m_ptr ELDRITCH_HORRORを引き起こしたモンスターの参照ポインタ
 * @param necro 暗黒領域魔法の詠唱失敗によるものならばTRUEを返す
 * @return なし
 */
void sanity_blast(monster_type *m_ptr, bool necro)
{
	int power = 100;

	if (p_ptr->inside_battle || !character_dungeon) return;

	if (!necro && m_ptr)
	{
		GAME_TEXT m_name[MAX_NLEN];
		monster_race *r_ptr = &r_info[m_ptr->ap_r_idx];

		power = r_ptr->level / 2;

		monster_desc(m_name, m_ptr, 0);

		if (!(r_ptr->flags1 & RF1_UNIQUE))
		{
			if (r_ptr->flags1 & RF1_FRIENDS)
				power /= 2;
		}
		else power *= 2;

		if (!is_loading_now)
			return; /* No effect yet, just loaded... */

		if (!m_ptr->ml)
			return; /* Cannot see it for some reason */

		if (!(r_ptr->flags2 & RF2_ELDRITCH_HORROR))
			return;

		if (is_pet(m_ptr))
			return; /* Pet eldritch horrors are safe most of the time */

		if (randint1(100) > power) return;

		if (saving_throw(p_ptr->skill_sav - power))
		{
			return; /* Save, no adverse effects */
		}

		if (p_ptr->image)
		{
			/* Something silly happens... */
			msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"),
				funny_desc[randint0(MAX_SAN_FUNNY)], m_name);

			if (one_in_(3))
			{
				msg_print(funny_comments[randint0(MAX_SAN_COMMENT)]);
				p_ptr->image = p_ptr->image + randint1(r_ptr->level);
			}

			return; /* Never mind; we can't see it clearly enough */
		}

		/* Something frightening happens... */
		msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"),
			horror_desc[randint0(MAX_SAN_HORROR)], m_name);

		r_ptr->r_flags2 |= RF2_ELDRITCH_HORROR;

		/* Demon characters are unaffected */
		if (prace_is_(RACE_IMP) || prace_is_(RACE_DEMON) || (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON)) return;
		if (p_ptr->wizard) return;

		/* Undead characters are 50% likely to be unaffected */
		if (prace_is_(RACE_SKELETON) || prace_is_(RACE_ZOMBIE)
			|| prace_is_(RACE_VAMPIRE) || prace_is_(RACE_SPECTRE) ||
			(mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_UNDEAD))
		{
			if (saving_throw(25 + p_ptr->lev)) return;
		}
	}
	else if (!necro)
	{
		monster_race *r_ptr;
		GAME_TEXT m_name[MAX_NLEN];
		concptr desc;

		get_mon_num_prep(get_nightmare, NULL);

		r_ptr = &r_info[get_mon_num(MAX_DEPTH)];
		power = r_ptr->level + 10;
		desc = r_name + r_ptr->name;

		get_mon_num_prep(NULL, NULL);

#ifndef JP
		if (!(r_ptr->flags1 & RF1_UNIQUE))
			sprintf(m_name, "%s %s", (is_a_vowel(desc[0]) ? "an" : "a"), desc);
		else
#endif
			sprintf(m_name, "%s", desc);

		if (!(r_ptr->flags1 & RF1_UNIQUE))
		{
			if (r_ptr->flags1 & RF1_FRIENDS) power /= 2;
		}
		else power *= 2;

		if (saving_throw(p_ptr->skill_sav * 100 / power))
		{
			msg_format(_("夢の中で%sに追いかけられた。", "%^s chases you through your dreams."), m_name);
			/* Safe */
			return;
		}

		if (p_ptr->image)
		{
			/* Something silly happens... */
			msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"),
				funny_desc[randint0(MAX_SAN_FUNNY)], m_name);

			if (one_in_(3))
			{
				msg_print(funny_comments[randint0(MAX_SAN_COMMENT)]);
				p_ptr->image = p_ptr->image + randint1(r_ptr->level);
			}

			/* Never mind; we can't see it clearly enough */
			return;
		}

		/* Something frightening happens... */
		msg_format(_("%s%sの顔を見てしまった！", "You behold the %s visage of %s!"),
			horror_desc[randint0(MAX_SAN_HORROR)], desc);

		r_ptr->r_flags2 |= RF2_ELDRITCH_HORROR;

		if (!p_ptr->mimic_form)
		{
			switch (p_ptr->prace)
			{
				/* Demons may make a saving throw */
			case RACE_IMP:
			case RACE_DEMON:
				if (saving_throw(20 + p_ptr->lev)) return;
				break;
				/* Undead may make a saving throw */
			case RACE_SKELETON:
			case RACE_ZOMBIE:
			case RACE_SPECTRE:
			case RACE_VAMPIRE:
				if (saving_throw(10 + p_ptr->lev)) return;
				break;
			}
		}
		else
		{
			/* Demons may make a saving throw */
			if (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON)
			{
				if (saving_throw(20 + p_ptr->lev)) return;
			}
			/* Undead may make a saving throw */
			else if (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_UNDEAD)
			{
				if (saving_throw(10 + p_ptr->lev)) return;
			}
		}
	}
	else
	{
		msg_print(_("ネクロノミコンを読んで正気を失った！", "Your sanity is shaken by reading the Necronomicon!"));
	}

	if (saving_throw(p_ptr->skill_sav - power))
	{
		return;
	}

	do {
		(void)do_dec_stat(A_INT);
	} while (randint0(100) > p_ptr->skill_sav && one_in_(2));

	do {
		(void)do_dec_stat(A_WIS);
	} while (randint0(100) > p_ptr->skill_sav && one_in_(2));

	switch (randint1(21))
	{
	case 1:
		if (!(p_ptr->muta3 & MUT3_MORONIC) && one_in_(5))
		{
			if ((p_ptr->stat_use[A_INT] < 4) && (p_ptr->stat_use[A_WIS] < 4))
			{
				msg_print(_("あなたは完璧な馬鹿になったような気がした。しかしそれは元々だった。", "You current_world_ptr->game_turn into an utter moron!"));
			}
			else
			{
				msg_print(_("あなたは完璧な馬鹿になった！", "You current_world_ptr->game_turn into an utter moron!"));
			}

			if (p_ptr->muta3 & MUT3_HYPER_INT)
			{
				msg_print(_("あなたの脳は生体コンピュータではなくなった。", "Your brain is no longer a living computer."));
				p_ptr->muta3 &= ~(MUT3_HYPER_INT);
			}
			p_ptr->muta3 |= MUT3_MORONIC;
		}
		break;
	case 2:
	case 3:
	case 4:
		if (!(p_ptr->muta2 & MUT2_COWARDICE) && !p_ptr->resist_fear)
		{
			msg_print(_("あなたはパラノイアになった！", "You become paranoid!"));

			/* Duh, the following should never happen, but anyway... */
			if (p_ptr->muta3 & MUT3_FEARLESS)
			{
				msg_print(_("あなたはもう恐れ知らずではなくなった。", "You are no longer fearless."));
				p_ptr->muta3 &= ~(MUT3_FEARLESS);
			}

			p_ptr->muta2 |= MUT2_COWARDICE;
		}
		break;
	case 5:
	case 6:
	case 7:
		if (!(p_ptr->muta2 & MUT2_HALLU) && !p_ptr->resist_chaos)
		{
			msg_print(_("幻覚をひき起こす精神錯乱に陥った！", "You are afflicted by a hallucinatory insanity!"));
			p_ptr->muta2 |= MUT2_HALLU;
		}
		break;
	case 8:
	case 9:
	case 10:
		if (!(p_ptr->muta2 & MUT2_BERS_RAGE))
		{
			msg_print(_("激烈な感情の発作におそわれるようになった！", "You become subject to fits of berserk rage!"));
			p_ptr->muta2 |= MUT2_BERS_RAGE;
		}
		break;
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		/* Brain smash */
		if (!p_ptr->resist_conf)
		{
			(void)set_confused(p_ptr->confused + randint0(4) + 4);
		}
		if (!p_ptr->free_act)
		{
			(void)set_paralyzed(p_ptr->paralyzed + randint0(4) + 4);
		}
		if (!p_ptr->resist_chaos)
		{
			(void)set_image(p_ptr->image + randint0(250) + 150);
		}
		break;
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
		/* Amnesia */
		if (lose_all_info())
			msg_print(_("あまりの恐怖に全てのことを忘れてしまった！", "You forget everything in your utmost terror!"));
		break;
	}

	p_ptr->update |= PU_BONUS;
	handle_stuff();
}


/*!
 * @brief プレイヤーの経験値について整合性のためのチェックと調整を行う /
 * Advance experience levels and print experience
 * @return なし
 */
void check_experience(void)
{
	bool level_reward = FALSE;
	bool level_mutation = FALSE;
	bool level_inc_stat = FALSE;
	bool android = (p_ptr->prace == RACE_ANDROID ? TRUE : FALSE);
	PLAYER_LEVEL old_lev = p_ptr->lev;

	/* Hack -- lower limit */
	if (p_ptr->exp < 0) p_ptr->exp = 0;
	if (p_ptr->max_exp < 0) p_ptr->max_exp = 0;
	if (p_ptr->max_max_exp < 0) p_ptr->max_max_exp = 0;

	/* Hack -- upper limit */
	if (p_ptr->exp > PY_MAX_EXP) p_ptr->exp = PY_MAX_EXP;
	if (p_ptr->max_exp > PY_MAX_EXP) p_ptr->max_exp = PY_MAX_EXP;
	if (p_ptr->max_max_exp > PY_MAX_EXP) p_ptr->max_max_exp = PY_MAX_EXP;

	/* Hack -- maintain "max" experience */
	if (p_ptr->exp > p_ptr->max_exp) p_ptr->max_exp = p_ptr->exp;

	/* Hack -- maintain "max max" experience */
	if (p_ptr->max_exp > p_ptr->max_max_exp) p_ptr->max_max_exp = p_ptr->max_exp;

	/* Redraw experience */
	p_ptr->redraw |= (PR_EXP);
	handle_stuff();


	/* Lose levels while possible */
	while ((p_ptr->lev > 1) &&
		(p_ptr->exp < ((android ? player_exp_a : player_exp)[p_ptr->lev - 2] * p_ptr->expfact / 100L)))
	{
		/* Lose a level */
		p_ptr->lev--;
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
		p_ptr->redraw |= (PR_LEV | PR_TITLE);
		p_ptr->window |= (PW_PLAYER);
		handle_stuff();
	}


	/* Gain levels while possible */
	while ((p_ptr->lev < PY_MAX_LEVEL) &&
		(p_ptr->exp >= ((android ? player_exp_a : player_exp)[p_ptr->lev - 1] * p_ptr->expfact / 100L)))
	{
		/* Gain a level */
		p_ptr->lev++;

		/* Save the highest level */
		if (p_ptr->lev > p_ptr->max_plv)
		{
			p_ptr->max_plv = p_ptr->lev;

			if ((p_ptr->pclass == CLASS_CHAOS_WARRIOR) ||
				(p_ptr->muta2 & MUT2_CHAOS_GIFT))
			{
				level_reward = TRUE;
			}
			if (p_ptr->prace == RACE_BEASTMAN)
			{
				if (one_in_(5)) level_mutation = TRUE;
			}
			level_inc_stat = TRUE;

			do_cmd_write_nikki(NIKKI_LEVELUP, p_ptr->lev, NULL);
		}

		sound(SOUND_LEVEL);

		msg_format(_("レベル %d にようこそ。", "Welcome to level %d."), p_ptr->lev);

		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
		p_ptr->redraw |= (PR_LEV | PR_TITLE | PR_EXP);
		p_ptr->window |= (PW_PLAYER | PW_SPELL | PW_INVEN);

		/* HPとMPの上昇量を表示 */
		level_up = 1;
		handle_stuff();

		level_up = 0;

		if (level_inc_stat)
		{
			if (!(p_ptr->max_plv % 10))
			{
				int choice;
				screen_save();
				while (1)
				{
					int n;
					char tmp[32];

					cnv_stat(p_ptr->stat_max[0], tmp);
					prt(format(_("        a) 腕力 (現在値 %s)", "        a) Str (cur %s)"), tmp), 2, 14);
					cnv_stat(p_ptr->stat_max[1], tmp);
					prt(format(_("        b) 知能 (現在値 %s)", "        a) Int (cur %s)"), tmp), 3, 14);
					cnv_stat(p_ptr->stat_max[2], tmp);
					prt(format(_("        c) 賢さ (現在値 %s)", "        a) Wis (cur %s)"), tmp), 4, 14);
					cnv_stat(p_ptr->stat_max[3], tmp);
					prt(format(_("        d) 器用 (現在値 %s)", "        a) Dex (cur %s)"), tmp), 5, 14);
					cnv_stat(p_ptr->stat_max[4], tmp);
					prt(format(_("        e) 耐久 (現在値 %s)", "        a) Con (cur %s)"), tmp), 6, 14);
					cnv_stat(p_ptr->stat_max[5], tmp);
					prt(format(_("        f) 魅力 (現在値 %s)", "        a) Chr (cur %s)"), tmp), 7, 14);

					prt("", 8, 14);
					prt(_("        どの能力値を上げますか？", "        Which stat do you want to raise?"), 1, 14);

					while (1)
					{
						choice = inkey();
						if ((choice >= 'a') && (choice <= 'f')) break;
					}
					for (n = 0; n < A_MAX; n++)
						if (n != choice - 'a')
							prt("", n + 2, 14);
					if (get_check(_("よろしいですか？", "Are you sure? "))) break;
				}
				do_inc_stat(choice - 'a');
				screen_load();
			}
			else if (!(p_ptr->max_plv % 2))
				do_inc_stat(randint0(6));
		}

		if (level_mutation)
		{
			msg_print(_("あなたは変わった気がする...", "You feel different..."));
			(void)gain_mutation(p_ptr, 0);
			level_mutation = FALSE;
		}

		/*
		 * 報酬でレベルが上ると再帰的に check_experience() が
		 * 呼ばれるので順番を最後にする。
		 */
		if (level_reward)
		{
			gain_level_reward(0);
			level_reward = FALSE;
		}

		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
		p_ptr->redraw |= (PR_LEV | PR_TITLE);
		p_ptr->window |= (PW_PLAYER | PW_SPELL);
		handle_stuff();
	}

	/* Load an autopick preference file */
	if (old_lev != p_ptr->lev) autopick_load_pref(FALSE);
}

/*!
 * @brief 現在の修正後能力値を3～17及び18/xxx形式に変換する / Converts stat num into a six-char (right justified) string
 * @param val 能力値
 * @param out_val 出力先文字列ポインタ
 * @return なし
 */
void cnv_stat(int val, char *out_val)
{
	/* Above 18 */
	if (val > 18)
	{
		int bonus = (val - 18);

		if (bonus >= 220)
		{
			sprintf(out_val, "18/%3s", "***");
		}
		else if (bonus >= 100)
		{
			sprintf(out_val, "18/%03d", bonus);
		}
		else
		{
			sprintf(out_val, " 18/%02d", bonus);
		}
	}

	/* From 3 to 18 */
	else
	{
		sprintf(out_val, "    %2d", val);
	}
}

/*!
 * @brief 能力値現在値から3～17及び18/xxx様式に基づく加減算を行う。
 * Modify a stat value by a "modifier", return new value
 * @param value 現在値
 * @param amount 加減算値
 * @return 加減算後の値
 * @details
 * <pre>
 * Stats go up: 3,4,...,17,18,18/10,18/20,...,18/220
 * Or even: 18/13, 18/23, 18/33, ..., 18/220
 * Stats go down: 18/220, 18/210,..., 18/10, 18, 17, ..., 3
 * Or even: 18/13, 18/03, 18, 17, ..., 3
 * </pre>
 */
s16b modify_stat_value(int value, int amount)
{
	int    i;

	/* Reward */
	if (amount > 0)
	{
		/* Apply each point */
		for (i = 0; i < amount; i++)
		{
			/* One point at a time */
			if (value < 18) value++;

			/* Ten "points" at a time */
			else value += 10;
		}
	}

	/* Penalty */
	else if (amount < 0)
	{
		/* Apply each point */
		for (i = 0; i < (0 - amount); i++)
		{
			/* Ten points at a time */
			if (value >= 18 + 10) value -= 10;

			/* Hack -- prevent weirdness */
			else if (value > 18) value = 18;

			/* One point at a time */
			else if (value > 3) value--;
		}
	}

	/* Return new value */
	return (s16b)(value);
}


/*!
 * @brief スコアを計算する /
 * Hack -- Calculates the total number of points earned		-JWT-
 * @return なし
 * @details
 */
long calc_score(void)
{
	int i, mult = 100;
	DEPTH max_dl = 0;
	u32b point, point_h, point_l;
	int arena_win = MIN(p_ptr->arena_number, MAX_ARENA_MONS);

	if (!preserve_mode) mult += 10;
	if (!autoroller) mult += 10;
	if (!smart_learn) mult -= 20;
	if (smart_cheat) mult += 30;
	if (ironman_shops) mult += 50;
	if (ironman_small_levels) mult += 10;
	if (ironman_empty_levels) mult += 20;
	if (!powerup_home) mult += 50;
	if (ironman_rooms) mult += 100;
	if (ironman_nightmare) mult += 100;

	if (mult < 5) mult = 5;

	for (i = 0; i < max_d_idx; i++)
		if (max_dlv[i] > max_dl)
			max_dl = max_dlv[i];

	point_l = (p_ptr->max_max_exp + (100 * max_dl));
	point_h = point_l / 0x10000L;
	point_l = point_l % 0x10000L;
	point_h *= mult;
	point_l *= mult;
	point_h += point_l / 0x10000L;
	point_l %= 0x10000L;

	point_l += ((point_h % 100) << 16);
	point_h /= 100;
	point_l /= 100;

	point = (point_h << 16) + (point_l);
	if (p_ptr->arena_number >= 0)
		point += (arena_win * arena_win * (arena_win > 29 ? 1000 : 100));

	if (ironman_downward) point *= 2;
	if (p_ptr->pclass == CLASS_BERSERKER)
	{
		if (p_ptr->prace == RACE_SPECTRE)
			point = point / 5;
	}

	if ((p_ptr->pseikaku == SEIKAKU_MUNCHKIN) && point)
	{
		point = 1;
		if (p_ptr->total_winner) point = 2;
	}
	if (easy_band) point = (0 - point);

	return point;
}

