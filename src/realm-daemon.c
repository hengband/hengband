﻿
#include "angband.h"
#include "cmd-spell.h"
#include "spells-summon.h"
#include "spells-status.h"

/*!
* @brief 悪魔領域魔法の各処理を行う
* @param spell 魔法ID
* @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
* @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はNULL文字列を返す。
*/
concptr do_daemon_spell(SPELL_IDX spell, BIT_FLAGS mode)
{
	bool name = (mode == SPELL_NAME) ? TRUE : FALSE;
	bool desc = (mode == SPELL_DESC) ? TRUE : FALSE;
	bool info = (mode == SPELL_INFO) ? TRUE : FALSE;
	bool cast = (mode == SPELL_CAST) ? TRUE : FALSE;

	DIRECTION dir;
	PLAYER_LEVEL plev = p_ptr->lev;

	switch (spell)
	{
	case 0:
		if (name) return _("マジック・ミサイル", "Magic Missile");
		if (desc) return _("弱い魔法の矢を放つ。", "Fires a weak bolt of magic.");

		{
			DICE_NUMBER dice = 3 + (plev - 1) / 5;
			DICE_SID sides = 4;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_bolt_or_beam(beam_chance() - 10, GF_MISSILE, dir, damroll(dice, sides));
			}
		}
		break;

	case 1:
		if (name) return _("無生命感知", "Detect Unlife");
		if (desc) return _("近くの生命のないモンスターを感知する。", "Detects all nonliving monsters in your vicinity.");

		{
			POSITION rad = DETECT_RAD_DEFAULT;

			if (info) return info_radius(rad);

			if (cast)
			{
				detect_monsters_nonliving(rad);
			}
		}
		break;

	case 2:
		if (name) return _("邪なる祝福", "Evil Bless");
		if (desc) return _("一定時間、命中率とACにボーナスを得る。", "Gives bonus to hit and AC for a few turns.");

		{
			int base = 12;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_blessed(randint1(base) + base, FALSE);
			}
		}
		break;

	case 3:
		if (name) return _("耐火炎", "Resist Fire");
		if (desc) return _("一定時間、炎への耐性を得る。装備による耐性に累積する。",
			"Gives resistance to fire, cold and electricity for a while. These resistances can be added to which from equipment for more powerful resistances.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_fire(randint1(base) + base, FALSE);
			}
		}
		break;

	case 4:
		if (name) return _("恐慌", "Horrify");
		if (desc) return _("モンスター1体を恐怖させ、朦朧させる。抵抗されると無効。", "Attempts to scare and stun a monster.");

		{
			PLAYER_LEVEL power = plev;

			if (info) return info_power(power);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fear_monster(dir, power);
				stun_monster(dir, power);
			}
		}
		break;

	case 5:
		if (name) return _("地獄の矢", "Nether Bolt");
		if (desc) return _("地獄のボルトもしくはビームを放つ。", "Fires a bolt or beam of nether.");

		{
			DICE_NUMBER dice = 6 + (plev - 5) / 4;
			DICE_SID sides = 8;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_bolt_or_beam(beam_chance(), GF_NETHER, dir, damroll(dice, sides));
			}
		}
		break;

	case 6:
		if (name) return _("古代の死霊召喚", "Summon Manes");
		if (desc) return _("古代の死霊を召喚する。", "Summons a manes.");

		{
			if (cast)
			{
				if (!summon_specific(-1, p_ptr->y, p_ptr->x, (plev * 3) / 2, SUMMON_MANES, (PM_ALLOW_GROUP | PM_FORCE_PET), '\0'))
				{
					msg_print(_("古代の死霊は現れなかった。", "No Manes arrive."));
				}
			}
		}
		break;

	case 7:
		if (name) return _("地獄の焔", "Hellish Flame");
		if (desc) return _("邪悪な力を持つボールを放つ。善良なモンスターには大きなダメージを与える。",
			"Fires a ball of evil power. Hurts good monsters greatly.");

		{
			DICE_NUMBER dice = 3;
			DICE_SID sides = 6;
			POSITION rad = (plev < 30) ? 2 : 3;
			int base;

			if (IS_WIZARD_CLASS())
				base = plev + plev / 2;
			else
				base = plev + plev / 4;


			if (info) return info_damage(dice, sides, base);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball(GF_HELL_FIRE, dir, damroll(dice, sides) + base, rad);
			}
		}
		break;

	case 8:
		if (name) return _("デーモン支配", "Dominate Demon");
		if (desc) return _("悪魔1体を魅了する。抵抗されると無効", "Attempts to charm a demon.");

		{
			int power = plev;

			if (info) return info_power(power);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				control_one_demon(dir, plev);
			}
		}
		break;

	case 9:
		if (name) return _("ビジョン", "Vision");
		if (desc) return _("周辺の地形を感知する。", "Maps nearby area.");

		{
			POSITION rad = DETECT_RAD_MAP;

			if (info) return info_radius(rad);

			if (cast)
			{
				map_area(rad);
			}
		}
		break;

	case 10:
		if (name) return _("耐地獄", "Resist Nether");
		if (desc) return _("一定時間、地獄への耐性を得る。", "Gives resistance to nether for a while.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_tim_res_nether(randint1(base) + base, FALSE);
			}
		}
		break;

	case 11:
		if (name) return _("プラズマ・ボルト", "Plasma bolt");
		if (desc) return _("プラズマのボルトもしくはビームを放つ。", "Fires a bolt or beam of plasma.");

		{
			DICE_NUMBER dice = 11 + (plev - 5) / 4;
			DICE_SID sides = 8;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_bolt_or_beam(beam_chance(), GF_PLASMA, dir, damroll(dice, sides));
			}
		}
		break;

	case 12:
		if (name) return _("ファイア・ボール", "Fire Ball");
		if (desc) return _("炎の球を放つ。", "Fires a ball of fire.");

		{
			HIT_POINT dam = plev + 55;
			POSITION rad = 2;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball(GF_FIRE, dir, dam, rad);
			}
		}
		break;

	case 13:
		if (name) return _("炎の刃", "Fire Branding");
		if (desc) return _("武器に炎の属性をつける。", "Makes current weapon fire branded.");

		{
			if (cast)
			{
				brand_weapon(1);
			}
		}
		break;

	case 14:
		if (name) return _("地獄球", "Nether Ball");
		if (desc) return _("大きな地獄の球を放つ。", "Fires a huge ball of nether.");

		{
			HIT_POINT dam = plev * 3 / 2 + 100;
			POSITION rad = plev / 20 + 2;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball(GF_NETHER, dir, dam, rad);
			}
		}
		break;

	case 15:
		if (name) return _("デーモン召喚", "Summon Demon");
		if (desc) return _("悪魔1体を召喚する。", "Summons a demon.");

		{
			if (cast)
			{
				cast_summon_demon(plev * 2 / 3 + randint1(plev / 2));
			}
		}
		break;

	case 16:
		if (name) return _("悪魔の目", "Devilish Eye");
		if (desc) return _("一定時間、テレパシー能力を得る。", "Gives telepathy for a while.");

		{
			int base = 30;
			DICE_SID sides = 25;

			if (info) return info_duration(base, sides);

			if (cast)
			{
				set_tim_esp(randint1(sides) + base, FALSE);
			}
		}
		break;

	case 17:
		if (name) return _("悪魔のクローク", "Devil Cloak");
		if (desc) return _("恐怖を取り除き、一定時間、炎と冷気の耐性、炎のオーラを得る。耐性は装備による耐性に累積する。",
			"Removes fear. Gives resistance to fire and cold, and aura of fire. These resistances can be added to which from equipment for more powerful resistances.");

		{
			TIME_EFFECT base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				TIME_EFFECT dur = randint1(base) + base;

				set_oppose_fire(dur, FALSE);
				set_oppose_cold(dur, FALSE);
				set_tim_sh_fire(dur, FALSE);
				set_afraid(0);
				break;
			}
		}
		break;

	case 18:
		if (name) return _("溶岩流", "The Flow of Lava");
		if (desc) return _("自分を中心とした炎の球を作り出し、床を溶岩に変える。",
			"Generates a ball of fire centered on you which transforms floors to magma.");

		{
			HIT_POINT dam = (55 + plev) * 2;
			POSITION rad = 3;

			if (info) return info_damage(0, 0, dam / 2);

			if (cast)
			{
				fire_ball(GF_FIRE, 0, dam, rad);
				fire_ball_hide(GF_LAVA_FLOW, 0, 2 + randint1(2), rad);
			}
		}
		break;

	case 19:
		if (name) return _("プラズマ球", "Plasma Ball");
		if (desc) return _("プラズマの球を放つ。", "Fires a ball of plasma.");

		{
			HIT_POINT dam = plev * 3 / 2 + 80;
			POSITION rad = 2 + plev / 40;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball(GF_PLASMA, dir, dam, rad);
			}
		}
		break;

	case 20:
		if (name) return _("悪魔変化", "Polymorph Demon");
		if (desc) return _("一定時間、悪魔に変化する。変化している間は本来の種族の能力を失い、代わりに悪魔としての能力を得る。",
			"Mimic a demon for a while. Loses abilities of original race and gets abilities as a demon.");

		{
			int base = 10 + plev / 2;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_mimic(base + randint1(base), MIMIC_DEMON, FALSE);
			}
		}
		break;

	case 21:
		if (name) return _("地獄の波動", "Nather Wave");
		if (desc) return _("視界内の全てのモンスターにダメージを与える。善良なモンスターに特に大きなダメージを与える。",
			"Damages all monsters in sight. Hurts good monsters greatly.");

		{
			int sides1 = plev * 2;
			int sides2 = plev * 2;

			if (info) return format("%sd%d+d%d", KWD_DAM, sides1, sides2);

			if (cast)
			{
				dispel_monsters(randint1(sides1));
				dispel_good(randint1(sides2));
			}
		}
		break;

	case 22:
		if (name) return _("サキュバスの接吻", "Kiss of Succubus");
		if (desc) return _("因果混乱の球を放つ。", "Fires a ball of nexus.");

		{
			HIT_POINT dam = 100 + plev * 2;
			POSITION rad = 4;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_ball(GF_NEXUS, dir, dam, rad);
			}
		}
		break;

	case 23:
		if (name) return _("破滅の手", "Doom Hand");
		if (desc) return _("破滅の手を放つ。食らったモンスターはそのときのHPの半分前後のダメージを受ける。", "Attempts to make a monster's HP almost half.");

		{
			if (cast)
			{
				if (!get_aim_dir(&dir))
					return NULL;
				else
					msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));

				fire_ball_hide(GF_HAND_DOOM, dir, plev * 2, 0);
			}
		}
		break;

	case 24:
		if (name) return _("士気高揚", "Raise the Morale");
		if (desc) return _("一定時間、ヒーロー気分になる。", "Removes fear, and gives bonus to hit and 10 more HP for a while.");

		{
			int base = 25;
			if (info) return info_duration(base, base);
			if (cast)heroism(base);
		}
		break;

	case 25:
		if (name) return _("不滅の肉体", "Immortal Body");
		if (desc) return _("一定時間、時間逆転への耐性を得る。", "Gives resistance to time for a while.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_tim_res_time(randint1(base) + base, FALSE);
			}
		}
		break;

	case 26:
		if (name) return _("狂気の円環", "Insanity Circle");
		if (desc) return _("自分を中心としたカオスの球、混乱の球を発生させ、近くのモンスターを魅了する。",
			"Generate balls of chaos, confusion and charm centered on you.");

		{
			HIT_POINT dam = 50 + plev;
			int power = 20 + plev;
			POSITION rad = 3 + plev / 20;

			if (info) return format("%s%d+%d", KWD_DAM, dam / 2, dam / 2);

			if (cast)
			{
				fire_ball(GF_CHAOS, 0, dam, rad);
				fire_ball(GF_CONFUSION, 0, dam, rad);
				fire_ball(GF_CHARM, 0, power, rad);
			}
		}
		break;

	case 27:
		if (name) return _("ペット爆破", "Explode Pets");
		if (desc) return _("全てのペットを強制的に爆破させる。", "Makes all pets explode.");

		{
			if (cast)
			{
				discharge_minion();
			}
		}
		break;

	case 28:
		if (name) return _("グレーターデーモン召喚", "Summon Greater Demon");
		if (desc) return _("上級デーモンを召喚する。召喚するには人間('p','h','t'で表されるモンスター)の死体を捧げなければならない。",
			"Summons greater demon. It need to sacrifice a corpse of human ('p','h' or 't').");

		{
			if (cast)
			{
				if (!cast_summon_greater_demon()) return NULL;
			}
		}
		break;

	case 29:
		if (name) return _("地獄嵐", "Nether Storm");
		if (desc) return _("超巨大な地獄の球を放つ。", "Generate a huge ball of nether.");

		{
			HIT_POINT dam = plev * 15;
			POSITION rad = plev / 5;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball(GF_NETHER, dir, dam, rad);
			}
		}
		break;

	case 30:
		if (name) return _("血の呪い", "Bloody Curse");
		if (desc) return _("自分がダメージを受けることによって対象に呪いをかけ、ダメージを与え様々な効果を引き起こす。",
			"Puts blood curse which damages and causes various effects on a monster. You also take damage.");

		{
			HIT_POINT dam = 600;
			POSITION rad = 0;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball_hide(GF_BLOOD_CURSE, dir, dam, rad);
				take_hit(DAMAGE_USELIFE, 20 + randint1(30), _("血の呪い", "Blood curse"), -1);
			}
		}
		break;

	case 31:
		if (name) return _("魔王変化", "Polymorph Demonlord");
		if (desc) return _("悪魔の王に変化する。変化している間は本来の種族の能力を失い、代わりに悪魔の王としての能力を得、壁を破壊しながら歩く。",
			"Mimic a demon lord for a while. Loses abilities of original race and gets great abilities as a demon lord. Even hard walls can't stop your walking.");

		{
			int base = 15;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_mimic(base + randint1(base), MIMIC_DEMON_LORD, FALSE);
			}
		}
		break;
	}

	return "";
}

