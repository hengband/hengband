#include "angband.h"
#include "core.h"
#include "util.h"

#include "cmd-spell.h"
#include "cmd-basic.h"
#include "dungeon.h"
#include "floor.h"
#include "melee.h"
#include "monsterrace-hook.h"
#include "artifact.h"
#include "monster.h"
#include "player-move.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-damage.h"
#include "feature.h"
#include "spells.h"
#include "grid.h"
#include "targeting.h"
#include "view-mainwindow.h"
#include "spells-floor.h"

/*!
* @brief 剣術の各処理を行う
* @param spell 剣術ID
* @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_CAST)
* @return SPELL_NAME / SPELL_DESC 時には文字列ポインタを返す。SPELL_CAST時はNULL文字列を返す。
*/
concptr do_hissatsu_spell(player_type *caster_ptr, SPELL_IDX spell, BIT_FLAGS mode)
{
	bool name = (mode == SPELL_NAME) ? TRUE : FALSE;
	bool desc = (mode == SPELL_DESC) ? TRUE : FALSE;
	bool cast = (mode == SPELL_CAST) ? TRUE : FALSE;

	DIRECTION dir;
	PLAYER_LEVEL plev = caster_ptr->lev;

	switch (spell)
	{
	case 0:
		if (name) return _("飛飯綱", "Tobi-Izuna");
		if (desc) return _("2マス離れたところにいるモンスターを攻撃する。", "Attacks a two squares distant monster.");

		if (cast)
		{
			project_length = 2;
			if (!get_aim_dir(&dir)) return NULL;

			project_hook(GF_ATTACK, dir, HISSATSU_2, PROJECT_STOP | PROJECT_KILL);
		}
		break;

	case 1:
		if (name) return _("五月雨斬り", "3-Way Attack");
		if (desc) return _("3方向に対して攻撃する。", "Attacks in 3 directions in one time.");

		if (cast)
		{
			DIRECTION cdir;
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			for (cdir = 0; cdir < 8; cdir++)
			{
				if (cdd[cdir] == dir) break;
			}

			if (cdir == 8) return NULL;

			y = caster_ptr->y + ddy_cdd[cdir];
			x = caster_ptr->x + ddx_cdd[cdir];
			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, 0);
			else
				msg_print(_("攻撃は空を切った。", "You attack the empty air."));

			y = caster_ptr->y + ddy_cdd[(cdir + 7) % 8];
			x = caster_ptr->x + ddx_cdd[(cdir + 7) % 8];
			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, 0);
			else
				msg_print(_("攻撃は空を切った。", "You attack the empty air."));

			y = caster_ptr->y + ddy_cdd[(cdir + 1) % 8];
			x = caster_ptr->x + ddx_cdd[(cdir + 1) % 8];
			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, 0);
			else
				msg_print(_("攻撃は空を切った。", "You attack the empty air."));
		}
		break;

	case 2:
		if (name) return _("ブーメラン", "Boomerang");
		if (desc) return _("武器を手元に戻ってくるように投げる。戻ってこないこともある。",
			"Throws current weapon. And it'll return to your hand unless failed.");

		if (cast)
		{
			if (!do_cmd_throw(caster_ptr, 1, TRUE, -1)) return NULL;
		}
		break;

	case 3:
		if (name) return _("焔霊", "Burning Strike");
		if (desc) return _("火炎耐性のないモンスターに大ダメージを与える。", "Attacks a monster with more damage unless it has resistance to fire.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_FIRE);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 4:
		if (name) return _("殺気感知", "Detect Ferocity");
		if (desc) return _("近くの思考することができるモンスターを感知する。", "Detects all monsters except mindless in your vicinity.");

		if (cast)
		{
			detect_monsters_mind(DETECT_RAD_DEFAULT);
		}
		break;

	case 5:
		if (name) return _("みね打ち", "Strike to Stun");
		if (desc) return _("相手にダメージを与えないが、朦朧とさせる。", "Attempts to stun a monster in the adjacent.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_MINEUCHI);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 6:
		if (name) return _("カウンター", "Counter");
		if (desc) return _("相手に攻撃されたときに反撃する。反撃するたびにMPを消費。",
			"Prepares to counterattack. When attack by a monster, strikes back using SP each time.");

		if (cast)
		{
			if (caster_ptr->riding)
			{
				msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
				return NULL;
			}
			msg_print(_("相手の攻撃に対して身構えた。", "You prepare to counter blow."));
			caster_ptr->counter = TRUE;
		}
		break;

	case 7:
		if (name) return _("払い抜け", "Harainuke");
		if (desc) return _("攻撃した後、反対側に抜ける。",
			"Attacks monster with your weapons normally, then move through counter side of the monster.");

		if (cast)
		{
			POSITION y, x;

			if (caster_ptr->riding)
			{
				msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
				return NULL;
			}

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;

			if (dir == 5) return NULL;
			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (!current_floor_ptr->grid_array[y][x].m_idx)
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}

			py_attack(p_ptr, y, x, 0);

			if (!player_can_enter(current_floor_ptr->grid_array[y][x].feat, 0) || is_trap(current_floor_ptr->grid_array[y][x].feat))
				break;

			y += ddy[dir];
			x += ddx[dir];

			if (player_can_enter(current_floor_ptr->grid_array[y][x].feat, 0) && !is_trap(current_floor_ptr->grid_array[y][x].feat) && !current_floor_ptr->grid_array[y][x].m_idx)
			{
				msg_print(NULL);
				(void)move_player_effect(caster_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
			}
		}
		break;

	case 8:
		if (name) return _("サーペンツタン", "Serpent's Tongue");
		if (desc) return _("毒耐性のないモンスターに大ダメージを与える。", "Attacks a monster with more damage unless it has resistance to poison.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_POISON);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 9:
		if (name) return _("斬魔剣弐の太刀", "Zammaken");
		if (desc) return _("生命のない邪悪なモンスターに大ダメージを与えるが、他のモンスターには全く効果がない。",
			"Attacks an evil unliving monster with great damage. No effect to other  monsters.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_ZANMA);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 10:
		if (name) return _("裂風剣", "Wind Blast");
		if (desc) return _("攻撃した相手を後方へ吹き飛ばす。", "Attacks an adjacent monster, and blow it away.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, 0);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
			if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
			{
				return "";
			}
			if (current_floor_ptr->grid_array[y][x].m_idx)
			{
				int i;
				POSITION ty = y, tx = x;
				POSITION oy = y, ox = x;
				MONSTER_IDX m_idx = current_floor_ptr->grid_array[y][x].m_idx;
				monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
				GAME_TEXT m_name[MAX_NLEN];

				monster_desc(m_name, m_ptr, 0);

				for (i = 0; i < 5; i++)
				{
					y += ddy[dir];
					x += ddx[dir];
					if (cave_empty_bold(y, x))
					{
						ty = y;
						tx = x;
					}
					else break;
				}
				if ((ty != oy) || (tx != ox))
				{
					msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name);
					current_floor_ptr->grid_array[oy][ox].m_idx = 0;
					current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
					m_ptr->fy = ty;
					m_ptr->fx = tx;

					update_monster(m_idx, TRUE);
					lite_spot(oy, ox);
					lite_spot(ty, tx);

					if (r_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
						caster_ptr->update |= (PU_MON_LITE);
				}
			}
		}
		break;

	case 11:
		if (name) return _("刀匠の目利き", "Judge");
		if (desc) return _("武器・防具を1つ識別する。レベル45以上で武器・防具の能力を完全に知ることができる。",
			"Identifies a weapon or armor. Or *identifies* these at level 45.");

		if (cast)
		{
			if (plev > 44)
			{
				if (!identify_fully(TRUE)) return NULL;
			}
			else
			{
				if (!ident_spell(TRUE)) return NULL;
			}
		}
		break;

	case 12:
		if (name) return _("破岩斬", "Rock Smash");
		if (desc) return _("岩を壊し、岩石系のモンスターに大ダメージを与える。", "Breaks rock. Or greatly damage a monster made by rocks.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_HAGAN);

			if (!cave_have_flag_bold(y, x, FF_HURT_ROCK)) break;

			/* Destroy the feature */
			cave_alter_feat(y, x, FF_HURT_ROCK);
			caster_ptr->update |= (PU_FLOW);
		}
		break;

	case 13:
		if (name) return _("乱れ雪月花", "Midare-Setsugekka");
		if (desc) return _("攻撃回数が増え、冷気耐性のないモンスターに大ダメージを与える。",
			"Attacks a monster with increased number of attacks and more damage unless it has resistance to cold.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_COLD);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 14:
		if (name) return _("急所突き", "Spot Aiming");
		if (desc) return _("モンスターを一撃で倒す攻撃を繰り出す。失敗すると1点しかダメージを与えられない。",
			"Attempts to kill a monster instantly. If failed cause only 1HP of damage.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_KYUSHO);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 15:
		if (name) return _("魔神斬り", "Majingiri");
		if (desc) return _("会心の一撃で攻撃する。攻撃がかわされやすい。",
			"Attempts to attack with critical hit. But this attack is easy to evade for a monster.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_MAJIN);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 16:
		if (name) return _("捨て身", "Desperate Attack");
		if (desc) return _("強力な攻撃を繰り出す。次のターンまでの間、食らうダメージが増える。",
			"Attacks with all of your power. But all damages you take will be doubled for one current_world_ptr->game_turn.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_SUTEMI);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
			caster_ptr->sutemi = TRUE;
		}
		break;

	case 17:
		if (name) return _("雷撃鷲爪斬", "Lightning Eagle");
		if (desc) return _("電撃耐性のないモンスターに非常に大きいダメージを与える。",
			"Attacks a monster with more damage unless it has resistance to electricity.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_ELEC);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 18:
		if (name) return _("入身", "Rush Attack");
		if (desc) return _("素早く相手に近寄り攻撃する。", "Steps close to a monster and attacks at a time.");

		if (cast)
		{
			if (!rush_attack(NULL)) return NULL;
		}
		break;

	case 19:
		if (name) return _("赤流渦", "Bloody Maelstrom");
		if (desc) return _("自分自身も傷を作りつつ、その傷が深いほど大きい威力で全方向の敵を攻撃できる。生きていないモンスターには効果がない。",
			"Attacks all adjacent monsters with power corresponding to your cut status. Then increases your cut status. No effect to unliving monsters.");

		if (cast)
		{
			POSITION y = 0, x = 0;

			grid_type       *g_ptr;
			monster_type    *m_ptr;

			if (caster_ptr->cut < 300)
				set_cut(caster_ptr,caster_ptr->cut + 300);
			else
				set_cut(caster_ptr,caster_ptr->cut * 2);

			for (dir = 0; dir < 8; dir++)
			{
				y = caster_ptr->y + ddy_ddd[dir];
				x = caster_ptr->x + ddx_ddd[dir];
				g_ptr = &current_floor_ptr->grid_array[y][x];
				m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];

				/* Hack -- attack monsters */
				if (g_ptr->m_idx && (m_ptr->ml || cave_have_flag_bold(y, x, FF_PROJECT)))
				{
					if (!monster_living(m_ptr->r_idx))
					{
						GAME_TEXT m_name[MAX_NLEN];

						monster_desc(m_name, m_ptr, 0);
						msg_format(_("%sには効果がない！", "%s is unharmed!"), m_name);
					}
					else py_attack(p_ptr, y, x, HISSATSU_SEKIRYUKA);
				}
			}
		}
		break;

	case 20:
		if (name) return _("激震撃", "Earthquake Blow");
		if (desc) return _("地震を起こす。", "Shakes dungeon structure, and results in random swapping of floors and walls.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_QUAKE);
			else
				earthquake(caster_ptr->y, caster_ptr->x, 10, 0);
		}
		break;

	case 21:
		if (name) return _("地走り", "Crack");
		if (desc) return _("衝撃波のビームを放つ。", "Fires a beam of shock wave.");

		if (cast)
		{
			int total_damage = 0, basedam, i;
			BIT_FLAGS flgs[TR_FLAG_SIZE];
			object_type *o_ptr;
			if (!get_aim_dir(&dir)) return NULL;
			msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
			for (i = 0; i < 2; i++)
			{
				int damage;

				if (!has_melee_weapon(caster_ptr, INVEN_RARM + i)) break;
				o_ptr = &caster_ptr->inventory_list[INVEN_RARM + i];
				basedam = (o_ptr->dd * (o_ptr->ds + 1)) * 50;
				damage = o_ptr->to_d * 100;
				object_flags(o_ptr, flgs);
				if ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD))
				{
					/* vorpal blade */
					basedam *= 5;
					basedam /= 3;
				}
				else if (have_flag(flgs, TR_VORPAL))
				{
					/* vorpal flag only */
					basedam *= 11;
					basedam /= 9;
				}
				damage += basedam;
				damage *= caster_ptr->num_blow[i];
				total_damage += damage / 200;
				if (i) total_damage = total_damage * 7 / 10;
			}
			fire_beam(GF_FORCE, dir, total_damage);
		}
		break;

	case 22:
		if (name) return _("気迫の雄叫び", "War Cry");
		if (desc) return _("視界内の全モンスターに対して轟音の攻撃を行う。さらに、近くにいるモンスターを怒らせる。",
			"Damages all monsters in sight with sound. Aggravate nearby monsters.");

		if (cast)
		{
			msg_print(_("雄叫びをあげた！", "You roar out!"));
			project_all_los(GF_SOUND, randint1(plev * 3));
			aggravate_monsters(0);
		}
		break;

	case 23:
		if (name) return _("無双三段", "Musou-Sandan");
		if (desc) return _("強力な3段攻撃を繰り出す。", "Attacks with powerful 3 strikes.");

		if (cast)
		{
			int i;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			for (i = 0; i < 3; i++)
			{
				POSITION y, x;
				POSITION ny, nx;
				MONSTER_IDX m_idx;
				grid_type *g_ptr;
				monster_type *m_ptr;

				y = caster_ptr->y + ddy[dir];
				x = caster_ptr->x + ddx[dir];
				g_ptr = &current_floor_ptr->grid_array[y][x];

				if (g_ptr->m_idx)
					py_attack(p_ptr, y, x, HISSATSU_3DAN);
				else
				{
					msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
					return NULL;
				}

				if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
				{
					return "";
				}

				/* Monster is dead? */
				if (!g_ptr->m_idx) break;

				ny = y + ddy[dir];
				nx = x + ddx[dir];
				m_idx = g_ptr->m_idx;
				m_ptr = &current_floor_ptr->m_list[m_idx];

				/* Monster cannot move back? */
				if (!monster_can_enter(ny, nx, &r_info[m_ptr->r_idx], 0))
				{
					/* -more- */
					if (i < 2) msg_print(NULL);
					continue;
				}

				g_ptr->m_idx = 0;
				current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;
				m_ptr->fy = ny;
				m_ptr->fx = nx;

				update_monster(m_idx, TRUE);

				/* Redraw the old spot */
				lite_spot(y, x);

				/* Redraw the new spot */
				lite_spot(ny, nx);

				/* Player can move forward? */
				if (player_can_enter(g_ptr->feat, 0))
				{
					if (!move_player_effect(caster_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP)) break;
				}
				else
				{
					break;
				}

				/* -more- */
				if (i < 2) msg_print(NULL);
			}
		}
		break;

	case 24:
		if (name) return _("吸血鬼の牙", "Vampire's Fang");
		if (desc) return _("攻撃した相手の体力を吸いとり、自分の体力を回復させる。生命を持たないモンスターには通じない。",
			"Attacks with vampiric strikes which absorbs HP from a monster and gives them to you. No effect to unliving monsters.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_DRAIN);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
		}
		break;

	case 25:
		if (name) return _("幻惑", "Moon Dazzling");
		if (desc) return _("視界内の起きている全モンスターに朦朧、混乱、眠りを与えようとする。", "Attempts to stun, confuse and sleep all waking monsters.");

		if (cast)
		{
			msg_print(_("武器を不規則に揺らした．．．", "You irregularly wave your weapon..."));
			project_all_los(GF_ENGETSU, plev * 4);
			project_all_los(GF_ENGETSU, plev * 4);
			project_all_los(GF_ENGETSU, plev * 4);
		}
		break;

	case 26:
		if (name) return _("百人斬り", "Hundred Slaughter");
		if (desc) return _("連続して入身でモンスターを攻撃する。攻撃するたびにMPを消費。MPがなくなるか、モンスターを倒せなかったら百人斬りは終了する。",
			"Performs a series of rush attacks. The series continues while killing each monster in a time and SP remains.");

		if (cast)
		{
			const int mana_cost_per_monster = 8;
			bool is_new = TRUE;
			bool mdeath;

			do
			{
				if (!rush_attack(&mdeath)) break;
				if (is_new)
				{
					/* Reserve needed mana point */
					caster_ptr->csp -= technic_info[REALM_HISSATSU - MIN_TECHNIC][26].smana;
					is_new = FALSE;
				}
				else
					caster_ptr->csp -= mana_cost_per_monster;

				if (!mdeath) break;
				command_dir = 0;

				caster_ptr->redraw |= PR_MANA;
				handle_stuff();
			} while (caster_ptr->csp > mana_cost_per_monster);

			if (is_new) return NULL;

			/* Restore reserved mana */
			caster_ptr->csp += technic_info[REALM_HISSATSU - MIN_TECHNIC][26].smana;
		}
		break;

	case 27:
		if (name) return _("天翔龍閃", "Dragonic Flash");
		if (desc) return _("視界内の場所を指定して、その場所と自分の間にいる全モンスターを攻撃し、その場所に移動する。",
			"Runs toward given location while attacking all monsters on the path.");

		if (cast)
		{
			POSITION y, x;

			if (!tgt_pt(&x, &y)) return NULL;

			if (!cave_player_teleportable_bold(y, x, 0L) ||
				(distance(y, x, caster_ptr->y, caster_ptr->x) > MAX_SIGHT / 2) ||
				!projectable(caster_ptr->y, caster_ptr->x, y, x))
			{
				msg_print(_("失敗！", "You cannot move to that place!"));
				break;
			}
			if (caster_ptr->anti_tele)
			{
				msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
				break;
			}
			project(0, 0, y, x, HISSATSU_ISSEN, GF_ATTACK, PROJECT_BEAM | PROJECT_KILL, -1);
			teleport_player_to(y, x, 0L);
		}
		break;

	case 28:
		if (name) return _("二重の剣撃", "Twin Slash");
		if (desc) return _("1ターンで2度攻撃を行う。", "double attacks at a time.");

		if (cast)
		{
			POSITION x, y;

			if (!get_rep_dir(&dir, FALSE)) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
			{
				py_attack(p_ptr, y, x, 0);
				if (current_floor_ptr->grid_array[y][x].m_idx)
				{
					handle_stuff();
					py_attack(p_ptr, y, x, 0);
				}
			}
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
				return NULL;
			}
		}
		break;

	case 29:
		if (name) return _("虎伏絶刀勢", "Kofuku-Zettousei");
		if (desc) return _("強力な攻撃を行い、近くの場所にも効果が及ぶ。", "Performs a powerful attack which even effect nearby monsters.");

		if (cast)
		{
			int total_damage = 0, basedam, i;
			POSITION y, x;
			BIT_FLAGS flgs[TR_FLAG_SIZE];
			object_type *o_ptr;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
			{
				msg_print(_("なぜか攻撃することができない。", "Something prevent you from attacking."));
				return "";
			}
			msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
			for (i = 0; i < 2; i++)
			{
				int damage;
				if (!has_melee_weapon(caster_ptr, INVEN_RARM + i)) break;
				o_ptr = &caster_ptr->inventory_list[INVEN_RARM + i];
				basedam = (o_ptr->dd * (o_ptr->ds + 1)) * 50;
				damage = o_ptr->to_d * 100;
				object_flags(o_ptr, flgs);
				if ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD))
				{
					/* vorpal blade */
					basedam *= 5;
					basedam /= 3;
				}
				else if (have_flag(flgs, TR_VORPAL))
				{
					/* vorpal flag only */
					basedam *= 11;
					basedam /= 9;
				}
				damage += basedam;
				damage += caster_ptr->to_d[i] * 100;
				damage *= caster_ptr->num_blow[i];
				total_damage += (damage / 100);
			}
			project(0, (cave_have_flag_bold(y, x, FF_PROJECT) ? 5 : 0), y, x, total_damage * 3 / 2, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
		}
		break;

	case 30:
		if (name) return _("慶雲鬼忍剣", "Keiun-Kininken");
		if (desc) return _("自分もダメージをくらうが、相手に非常に大きなダメージを与える。アンデッドには特に効果がある。",
			"Attacks a monster with extremely powerful damage. But you also takes some damages. Hurts a undead monster greatly.");

		if (cast)
		{
			POSITION y, x;

			if (!get_direction(&dir, FALSE, FALSE)) return NULL;
			if (dir == 5) return NULL;

			y = caster_ptr->y + ddy[dir];
			x = caster_ptr->x + ddx[dir];

			if (current_floor_ptr->grid_array[y][x].m_idx)
				py_attack(p_ptr, y, x, HISSATSU_UNDEAD);
			else
			{
				msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
				return NULL;
			}
			take_hit(caster_ptr, DAMAGE_NOESCAPE, 100 + randint1(100), _("慶雲鬼忍剣を使った衝撃", "exhaustion on using Keiun-Kininken"), -1);
		}
		break;

	case 31:
		if (name) return _("切腹", "Harakiri");
		if (desc) return _("「武士道とは、死ぬことと見つけたり。」", "'Busido is found in death'");

		if (cast)
		{
			int i;
			if (!get_check(_("本当に自殺しますか？", "Do you really want to commit suicide? "))) return NULL;
			/* Special Verification for suicide */
			prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);

			flush();
			i = inkey();
			prt("", 0, 0);
			if (i != '@') return NULL;
			if (caster_ptr->total_winner)
			{
				take_hit(caster_ptr, DAMAGE_FORCE, 9999, "Seppuku", -1);
				caster_ptr->total_winner = TRUE;
			}
			else
			{
				msg_print(_("武士道とは、死ぬことと見つけたり。", "Meaning of Bushi-do is found in the death."));
				take_hit(caster_ptr, DAMAGE_FORCE, 9999, "Seppuku", -1);
			}
		}
		break;
	}

	return "";
}
