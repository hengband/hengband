/*!
    @file bldg.c
    @brief 町の施設処理 / Building commands
    @date 2013/12/23
    @author
    Created by Ken Wigle for Kangband - a variant of Angband 2.8.3\n
    -KMW-\n
    \n
    Rewritten for Kangband 2.8.3i using Kamband's version of\n
    bldg.c as written by Ivan Tkatchev\n
    \n
    Changed for ZAngband by Robert Ruehlmann\n
*/

#include "angband.h"
#include "util.h"
#include "term.h"

#include "core.h"
#include "cmd-dump.h"
#include "cmd-magiceat.h"
#include "floor.h"
#include "floor-events.h"
#include "floor-save.h"
#include "autopick.h"
#include "objectkind.h"
#include "object-boost.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "monster.h"
#include "monsterrace-hook.h"
#include "melee.h"
#include "wild.h"
#include "world.h"
#include "sort.h"

#include "avatar.h"
#include "bldg.h"
#include "dungeon.h"
#include "mutation.h"
#include "quest.h"
#include "artifact.h"
#include "cmd-spell.h"
#include "rumor.h"
#include "spells.h"
#include "spells-object.h"
#include "spells-status.h"
#include "realm-hex.h"
#include "dungeon-file.h"

#include "files.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-class.h"
#include "player-personality.h"
#include "player-inventory.h"
#include "scores.h"
#include "shoot.h"
#include "view-mainwindow.h"
#include "monsterrace.h"
#include "autopick.h"


/*
 * Buildings
 */
building_type building[MAX_BLDG];

MONRACE_IDX battle_mon[4];
u32b mon_odds[4];
int battle_odds;
PRICE kakekin;
int sel_monster;

bool reinit_wilderness = FALSE;
MONSTER_IDX today_mon;

/*!
 * @brief 闘技場のモンスターID及び報酬アイテムテーブル
 */
const arena_type arena_info[MAX_ARENA_MONS + 2] =
{
	{ MON_NOBORTA,       TV_AMULET, SV_AMULET_ADORNMENT           },
	{ MON_MORI_TROLL,    TV_FOOD,   SV_FOOD_PINT_OF_WINE          },
	{ MON_IMP,           TV_POTION, SV_POTION_SPEED               },
	{ MON_LION_HEART,    0,         0                             },
	{ MON_MASTER_YEEK,   TV_POTION, SV_POTION_CURING              },
	{ MON_SABRE_TIGER,   TV_WAND,   SV_WAND_STONE_TO_MUD          },
	{ MON_LIZARD_KING,   TV_WAND,   SV_WAND_TELEPORT_AWAY         },
	{ MON_WYVERN,        TV_POTION, SV_POTION_HEALING             },
	{ MON_ARCH_VILE,     TV_POTION, SV_POTION_RESISTANCE          },
	{ MON_ELF_LORD   ,   TV_POTION, SV_POTION_ENLIGHTENMENT       },
	{ MON_GHOUL_KING,    TV_FOOD,   SV_FOOD_RESTORING             },
	{ MON_COLBRAN,       TV_RING,   SV_RING_ELEC                  },
	{ MON_BICLOPS,       TV_WAND,   SV_WAND_ACID_BALL             },
	{ MON_M_MINDCRAFTER, TV_POTION, SV_POTION_SELF_KNOWLEDGE      },
	{ MON_GROO,          TV_SCROLL, SV_SCROLL_ACQUIREMENT         },
	{ MON_RAAL,          TV_SCROLL, SV_SCROLL_STAR_DESTRUCTION    },
	{ MON_DREADMASTER,   TV_WAND,   SV_WAND_HYPODYNAMIA            },
	{ MON_ULTRA_PALADIN, TV_STAFF,  SV_STAFF_DISPEL_EVIL          },
	{ MON_BARNEY,        TV_RING,   SV_RING_RES_CHAOS             },
	{ MON_TROLL_KING,    TV_SCROLL, SV_SCROLL_MASS_GENOCIDE       },
	{ MON_BARON_HELL,    TV_POTION, SV_POTION_AUGMENTATION        },
	{ MON_F_ANGEL,       TV_SCROLL, SV_SCROLL_RUNE_OF_PROTECTION  },
	{ MON_G_C_DRAKE,     TV_WAND,   SV_WAND_DRAGON_FIRE           },
	{ MON_IRON_LICH,     TV_STAFF,  SV_STAFF_DESTRUCTION          },
	{ MON_DROLEM,        TV_POTION, SV_POTION_STAR_HEALING        },
	{ MON_G_TITAN,       TV_WAND,   SV_WAND_GENOCIDE              },
	{ MON_G_BALROG,      TV_POTION, SV_POTION_EXPERIENCE          },
	{ MON_ELDER_VAMPIRE, TV_RING,   SV_RING_SUSTAIN               },
	{ MON_NIGHTWALKER,   TV_WAND,   SV_WAND_STRIKING              },
	{ MON_S_TYRANNO,     TV_SCROLL, SV_SCROLL_STAR_ACQUIREMENT    },
	{ MON_G_MASTER_MYS,  TV_ROD,    SV_ROD_IDENTIFY               },
	{ MON_LORD_CHAOS,    TV_POTION, SV_POTION_LIFE                },
	{ MON_SHADOWLORD,    TV_POTION, SV_POTION_STAR_ENLIGHTENMENT  },
	{ MON_ULT_BEHOLDER,  TV_AMULET, SV_AMULET_REFLECTION          },
	{ MON_JABBERWOCK,    TV_ROD,    SV_ROD_HEALING                },
	{ MON_LOCKE_CLONE,   TV_WAND,   SV_WAND_DISINTEGRATE          },
	{ MON_WYRM_SPACE,    TV_ROD,    SV_ROD_RESTORATION            },
	{ MON_SHAMBLER,      TV_SCROLL, SV_SCROLL_STAR_ACQUIREMENT    },
	{ MON_BLACK_REAVER,  TV_RING,   SV_RING_LORDLY                },
	{ MON_FENGHUANG,     TV_STAFF,  SV_STAFF_THE_MAGI             },
	{ MON_WYRM_POWER,    TV_SCROLL, SV_SCROLL_ARTIFACT            },
	{ 0,                 0,         0                             }, /* Victory prizing */
	{ MON_HAGURE,        TV_SCROLL, SV_SCROLL_ARTIFACT            },
};

/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 */
static bool is_owner(building_type *bldg)
{
	if (bldg->member_class[p_ptr->pclass] == BUILDING_OWNER)
	{
		return (TRUE);
	}

	if (bldg->member_race[p_ptr->prace] == BUILDING_OWNER)
	{
		return (TRUE);
	}

	if ((is_magic(p_ptr->realm1) && (bldg->member_realm[p_ptr->realm1] == BUILDING_OWNER)) ||
		(is_magic(p_ptr->realm2) && (bldg->member_realm[p_ptr->realm2] == BUILDING_OWNER)))
	{
		return (TRUE);
	}

	return (FALSE);
}

/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 （スペルマスターの特別判定つき）
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 * @todo is_owner()との実質的な多重実装なので、リファクタリングを行うべきである。
 */
static bool is_member(building_type *bldg)
{
	if (bldg->member_class[p_ptr->pclass])
	{
		return (TRUE);
	}

	if (bldg->member_race[p_ptr->prace])
	{
		return (TRUE);
	}

	if ((is_magic(p_ptr->realm1) && bldg->member_realm[p_ptr->realm1]) ||
	    (is_magic(p_ptr->realm2) && bldg->member_realm[p_ptr->realm2]))
	{
		return (TRUE);
	}


	if (p_ptr->pclass == CLASS_SORCERER)
	{
		int i;
		bool OK = FALSE;
		for (i = 0; i < MAX_MAGIC; i++)
		{
			if (bldg->member_realm[i+1]) OK = TRUE;
		}
		return OK;
	}
	return (FALSE);
}

/*!
 * @brief コンソールに表示された施設に関する情報を消去する / Clear the building information
 * @details 消去は行毎にヌル文字列で行われる。
 * @param min_row 開始行番号
 * @param max_row 末尾行番号
 * @return なし
 */
void clear_bldg(int min_row, int max_row)
{
	int   i;

	for (i = min_row; i <= max_row; i++)
		prt("", i, 0);
}

/*!
 * @brief 所持金を表示する。
 * @return なし
 */
static void building_prt_gold(void)
{
	char tmp_str[80];
	prt(_("手持ちのお金: ", "Gold Remaining: "), 23,53);
	sprintf(tmp_str, "%9ld", (long)p_ptr->au);
	prt(tmp_str, 23, 68);
}

/*!
 * @brief 施設のサービス一覧を表示する / Display a building.
 * @param bldg 施設構造体の参照ポインタ
 * @return なし
 */
static void show_building(building_type* bldg)
{
	char buff[20];
	int i;
	byte action_color;
	char tmp_str[80];

	Term_clear();
	sprintf(tmp_str, "%s (%s) %35s", bldg->owner_name, bldg->owner_race, bldg->name);
	prt(tmp_str, 2, 1);


	for (i = 0; i < 8; i++)
	{
		if (bldg->letters[i])
		{
			if (bldg->action_restr[i] == 0)
			{
				if ((is_owner(bldg) && (bldg->member_costs[i] == 0)) ||
					(!is_owner(bldg) && (bldg->other_costs[i] == 0)))
				{
					action_color = TERM_WHITE;
					buff[0] = '\0';
				}
				else if (is_owner(bldg))
				{
					action_color = TERM_YELLOW;
					sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
				}
				else
				{
					action_color = TERM_YELLOW;
					sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->other_costs[i]);				}
			}
			else if (bldg->action_restr[i] == 1)
			{
				if (!is_member(bldg))
				{
					action_color = TERM_L_DARK;
					strcpy(buff, _("(閉店)", "(closed)"));
				}
				else if ((is_owner(bldg) && (bldg->member_costs[i] == 0)) ||
					(is_member(bldg) && (bldg->other_costs[i] == 0)))
				{
					action_color = TERM_WHITE;
					buff[0] = '\0';
				}
				else if (is_owner(bldg))
				{
					action_color = TERM_YELLOW;
					sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
				}
				else
				{
					action_color = TERM_YELLOW;
					sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->other_costs[i]);
				}
			}
			else
			{
				if (!is_owner(bldg))
				{
					action_color = TERM_L_DARK;
					strcpy(buff, _("(閉店)", "(closed)"));
				}
				else if (bldg->member_costs[i] != 0)
				{
					action_color = TERM_YELLOW;
					sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
				}
				else
				{
					action_color = TERM_WHITE;
					buff[0] = '\0';
				}
			}

			sprintf(tmp_str," %c) %s %s", bldg->letters[i], bldg->act_names[i], buff);
			c_put_str(action_color, tmp_str, 19+(i/2), 35*(i%2));
		}
	}
	prt(_(" ESC) 建物を出る", " ESC) Exit building"), 23, 0);
}

/*!
 * @brief 闘技場に入るコマンドの処理 / arena commands
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param cmd 闘技場処理のID
 * @return なし
 */
static void arena_comm(player_type *player_ptr, int cmd)
{
	monster_race    *r_ptr;
	concptr            name;
	
	switch (cmd)
	{
		case BACT_ARENA:
			if (player_ptr->arena_number == MAX_ARENA_MONS)
			{
				clear_bldg(5, 19);
				prt(_("アリーナの優勝者！", "               Arena Victor!"), 5, 0);
				prt(_("おめでとう！あなたは全ての敵を倒しました。", "Congratulations!  You have defeated all before you."), 7, 0);
				prt(_("賞金として $1,000,000 が与えられます。", "For that, receive the prize: 1,000,000 gold pieces"), 8, 0);

				prt("", 10, 0);
				prt("", 11, 0);
				player_ptr->au += 1000000L;
				msg_print(_("スペースキーで続行", "Press the space bar to continue"));
				msg_print(NULL);
				player_ptr->arena_number++;
			}
			else if (player_ptr->arena_number > MAX_ARENA_MONS)
			{
				if (player_ptr->arena_number < MAX_ARENA_MONS+2)
				{
					msg_print(_("君のために最強の挑戦者を用意しておいた。", "The strongest challenger is waiting for you."));
					msg_print(NULL);
					if (get_check(_("挑戦するかね？", "Do you fight? ")))
					{	
						msg_print(_("死ぬがよい。", "Die, maggots."));
						msg_print(NULL);
					
						player_ptr->exit_bldg = FALSE;
						reset_tim_flags(player_ptr);

						/* Save the surface floor as saved floor */
						prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

						player_ptr->current_floor_ptr->inside_arena = TRUE;
						player_ptr->leaving = TRUE;
						player_ptr->leave_bldg = TRUE;
					}
					else
					{
						msg_print(_("残念だ。", "We are disappointed."));
					}
				}
				else
				{
					msg_print(_("あなたはアリーナに入り、しばらくの間栄光にひたった。",
								"You enter the arena briefly and bask in your glory."));
					msg_print(NULL);
				}
			}
			else if (player_ptr->riding && (player_ptr->pclass != CLASS_BEASTMASTER) && (player_ptr->pclass != CLASS_CAVALRY))
			{
				msg_print(_("ペットに乗ったままではアリーナへ入れさせてもらえなかった。",
							"You don't have permission to enter with pet."));
				msg_print(NULL);
			}
			else
			{
				player_ptr->exit_bldg = FALSE;
				reset_tim_flags(player_ptr);

				/* Save the surface floor as saved floor */
				prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

				player_ptr->current_floor_ptr->inside_arena = TRUE;
				player_ptr->leaving = TRUE;
				player_ptr->leave_bldg = TRUE;
			}
			break;
		case BACT_POSTER:
			if (player_ptr->arena_number == MAX_ARENA_MONS)
				msg_print(_("あなたは勝利者だ。 アリーナでのセレモニーに参加しなさい。",
							"You are victorious. Enter the arena for the ceremony."));

			else if (player_ptr->arena_number > MAX_ARENA_MONS)
			{
				msg_print(_("あなたはすべての敵に勝利した。", "You have won against all foes."));
			}
			else
			{
				r_ptr = &r_info[arena_info[player_ptr->arena_number].r_idx];
				name = (r_name + r_ptr->name);
				msg_format(_("%s に挑戦するものはいないか？", "Do I hear any challenges against: %s"), name);

				player_ptr->monster_race_idx = arena_info[player_ptr->arena_number].r_idx;
				player_ptr->window |= (PW_MONSTER);
				handle_stuff();

			}
			break;
		case BACT_ARENA_RULES:
			screen_save();

			/* Peruse the arena help file */
			(void)show_file(TRUE, _("arena_j.txt", "arena.txt"), NULL, 0, 0);
			screen_load();

			break;
	}
}


/*!
 * @brief カジノのスロットシンボルを表示する / display fruit for dice slots
 * @param row シンボルを表示する行の上端
 * @param col シンボルを表示する行の左端
 * @param fruit 表示するシンボルID
 * @return なし
 */
static void display_fruit(int row, int col, int fruit)
{
	switch (fruit)
	{
		case 0: /* lemon */
			c_put_str(TERM_YELLOW, "   ####.", row, col);
			c_put_str(TERM_YELLOW, "  #    #", row + 1, col);
			c_put_str(TERM_YELLOW, " #     #", row + 2, col);
			c_put_str(TERM_YELLOW, "#      #", row + 3, col);
			c_put_str(TERM_YELLOW, "#      #", row + 4, col);
			c_put_str(TERM_YELLOW, "#     # ", row + 5, col);
			c_put_str(TERM_YELLOW, "#    #  ", row + 6, col);
			c_put_str(TERM_YELLOW, ".####   ", row + 7, col);
			prt(                 _(" レモン ",
						           " Lemon  "), row + 8, col);
			break;
		case 1: /* orange */
			c_put_str(TERM_ORANGE, "   ##   ", row, col);
			c_put_str(TERM_ORANGE, "  #..#  ", row + 1, col);
			c_put_str(TERM_ORANGE, " #....# ", row + 2, col);
			c_put_str(TERM_ORANGE, "#......#", row + 3, col);
			c_put_str(TERM_ORANGE, "#......#", row + 4, col);
			c_put_str(TERM_ORANGE, " #....# ", row + 5, col);
			c_put_str(TERM_ORANGE, "  #..#  ", row + 6, col);
			c_put_str(TERM_ORANGE, "   ##   ", row + 7, col);
			prt(                 _("オレンジ",
								   " Orange "), row + 8, col);
			break;
		case 2: /* sword */
			c_put_str(TERM_SLATE, _("   Λ   ",  "   /\\   ") , row, col);
			c_put_str(TERM_SLATE, _("   ||   ", "   ##   ") , row + 1, col);
			c_put_str(TERM_SLATE, _("   ||   ", "   ##   ") , row + 2, col);
			c_put_str(TERM_SLATE, _("   ||   ", "   ##   ") , row + 3, col);
			c_put_str(TERM_SLATE, _("   ||   ", "   ##   ") , row + 4, col);
			c_put_str(TERM_SLATE, _("   ||   ", "   ##   ") , row + 5, col);
			c_put_str(TERM_UMBER, _(" |=亜=| ", " ###### ") , row + 6, col);
			c_put_str(TERM_UMBER, _("   目   ", "   ##   ") , row + 7, col);
			prt(                  _("   剣   ", " Sword  ") , row + 8, col);
			break;
		case 3: /* shield */
			c_put_str(TERM_SLATE, " ###### ", row, col);
			c_put_str(TERM_SLATE, "#      #", row + 1, col);
			c_put_str(TERM_SLATE, "# ++++ #", row + 2, col);
			c_put_str(TERM_SLATE, "# +==+ #", row + 3, col);
			c_put_str(TERM_SLATE, "#  ++  #", row + 4, col);
			c_put_str(TERM_SLATE, " #    # ", row + 5, col);
			c_put_str(TERM_SLATE, "  #  #  ", row + 6, col);
			c_put_str(TERM_SLATE, "   ##   ", row + 7, col);
			prt(                _("   盾   ",
								  " Shield "), row + 8, col);
			break;
		case 4: /* plum */
			c_put_str(TERM_VIOLET, "   ##   ", row, col);
			c_put_str(TERM_VIOLET, " ###### ", row + 1, col);
			c_put_str(TERM_VIOLET, "########", row + 2, col);
			c_put_str(TERM_VIOLET, "########", row + 3, col);
			c_put_str(TERM_VIOLET, "########", row + 4, col);
			c_put_str(TERM_VIOLET, " ###### ", row + 5, col);
			c_put_str(TERM_VIOLET, "  ####  ", row + 6, col);
			c_put_str(TERM_VIOLET, "   ##   ", row + 7, col);
			prt(                 _(" プラム ",
								   "  Plum  "), row + 8, col);
			break;
		case 5: /* cherry */
			c_put_str(TERM_RED, "      ##", row, col);
			c_put_str(TERM_RED, "   ###  ", row + 1, col);
			c_put_str(TERM_RED, "  #..#  ", row + 2, col);
			c_put_str(TERM_RED, "  #..#  ", row + 3, col);
			c_put_str(TERM_RED, " ###### ", row + 4, col);
			c_put_str(TERM_RED, "#..##..#", row + 5, col);
			c_put_str(TERM_RED, "#..##..#", row + 6, col);
			c_put_str(TERM_RED, " ##  ## ", row + 7, col);
			prt(              _("チェリー",
								" Cherry "), row + 8, col);
			break;
	}
}

/*! @note
 * kpoker no (tyuto-hannpa na)pakuri desu...
 * joker ha shineru node haitte masen.
 *
 * TODO: donataka! tsukutte!
 *  - agatta yaku no kiroku (like DQ).
 *  - kakkoii card no e.
 *  - sousa-sei no koujyo.
 *  - code wo wakariyasuku.
 *  - double up.
 *  - Joker... -- done.
 *
 * 9/13/2000 --Koka
 * 9/15/2000 joker wo jissou. soreto, code wo sukosi kakikae. --Habu
 */

#define SUIT_OF(card)  ((card) / 13) /*!< トランプカードのスートを返す */
#define NUM_OF(card)   ((card) % 13) /*!< トランプカードの番号を返す */
#define IS_JOKER(card) ((card) == 52) /*!< トランプカードがジョーカーかどうかを返す */

static int cards[5]; /*!< ポーカーの現在の手札ID */

/*!
 * @brief ポーカーの山札を切る。
 * @param deck デッキの配列
 * @return なし
 */
static void reset_deck(int deck[])
{
	int i;
	for (i = 0; i < 53; i++) deck[i] = i;

	/* shuffle cards */
	for (i = 0; i < 53; i++){
		int tmp1 = randint0(53 - i) + i;
		int tmp2 = deck[i];
		deck[i] = deck[tmp1];
		deck[tmp1] = tmp2;
	}
}

/*!
 * @brief ポーカープレイ中にジョーカーを持っているかの判定を返す。
 * @return ジョーカーを持っているか。
 */
static bool have_joker(void)
{
	int i;

	for (i = 0; i < 5; i++){
	  if(IS_JOKER(cards[i])) return TRUE;
	}
	return FALSE;
}

/*!
 * @brief ポーカーの手札に該当の番号の札があるかを返す。
 * @param num 探したいカードの番号。
 * @return 該当の番号が手札にあるか。
 */
static bool find_card_num(int num)
{
	int i;
	for (i = 0; i < 5; i++)
		if (NUM_OF(cards[i]) == num && !IS_JOKER(cards[i])) return TRUE;
	return FALSE;
}

/*!
 * @brief ポーカーの手札がフラッシュ役を得ているかを帰す。
 * @return 役の判定結果
 */
static bool yaku_check_flush(void)
{
	int i, suit;
	bool joker_is_used = FALSE;

	suit = IS_JOKER(cards[0]) ? SUIT_OF(cards[1]) : SUIT_OF(cards[0]);
	for (i = 0; i < 5; i++){
		if (SUIT_OF(cards[i]) != suit){
		  if(have_joker() && !joker_is_used)
		    joker_is_used = TRUE;
		  else
		    return FALSE;
		}
	}

	return TRUE;
}

/*!
 * @brief ポーカーの手札がストレートを含んだ高位の役を得ているかを帰す。
 * @return 役の判定結果 0…ストレート、フラッシュいずれもなし/1…ストレートのみ/2…ストレートフラッシュ/3…ロイヤルストレートフラッシュ
 */
static int yaku_check_straight(void)
{
	int i, lowest = 99;
	bool joker_is_used = FALSE;
	bool straight = FALSE;

	/* get lowest */
	for (i = 0; i < 5; i++)
	{
		if (NUM_OF(cards[i]) < lowest && !IS_JOKER(cards[i]))
			lowest = NUM_OF(cards[i]);
	}
	
	/* Check Royal Straight Flush */
	if (yaku_check_flush())
	{
	  if( lowest == 0 ){
		for (i = 0; i < 4; i++)
		{
			if (!find_card_num(9 + i)){
				if( have_joker() && !joker_is_used )
				  joker_is_used = TRUE;
				else
				  break;
			}
		}
		if (i == 4) return 3; /* Wow! Royal Straight Flush!!! */
	  }
	  if(lowest == 9){
		for (i = 0; i < 3; i++)
		{
			if (!find_card_num(10 + i))
				break;
		}
		if (i == 3 && have_joker()) return 3; /* Wow! Royal Straight Flush!!! */
	  }
	}

	joker_is_used = FALSE;

	/* Straight Only Check */

	if (lowest == 0) { /* (10 - J - Q - K)[JOKER] - A */
		for (i = 0; i < 4; i++)
		{
			if (!find_card_num(9 + i)) {
				if (have_joker() && !joker_is_used)
					joker_is_used = TRUE;
				else
					break; /* None */
			}
		}
		if(i == 4) straight = TRUE;
	}

	joker_is_used = FALSE;

	for (i = 0; i < 5; i++)
	{
		if(!find_card_num(lowest + i)){
			if( have_joker() && !joker_is_used )
				joker_is_used = TRUE;
			else
				break; /* None */
		}
	}
	if(i == 5) straight = TRUE;
	
	if (straight && yaku_check_flush()) return 2; /* Straight Flush */
	else if(straight) return 1; /* Only Straight */
	else return 0;
}

/*!
 * @brief ポーカーのペア役の状態を返す。
 * @return 0:nopair 1:1 pair 2:2 pair 3:3 cards 4:full house 6:4cards
 */
static int yaku_check_pair(void)
{
	int i, i2, matching = 0;

	for (i = 0; i < 5; i++)
	{
		for (i2 = i+1; i2 < 5; i2++)
		{
			if (IS_JOKER(cards[i]) || IS_JOKER(cards[i2])) continue;
			if (NUM_OF(cards[i]) == NUM_OF(cards[i2]))
				matching++;
		}
	}

	if(have_joker()){
	  switch(matching){
	  case 0:
	    matching = 1;
	    break;
	  case 1:
	    matching = 3;
	    break;
	  case 2:
	    matching = 4;
	    break;
	  case 3:
	    matching = 6;
	    break;
	  case 6:
	    matching = 7;
	    break;
	  default:
	    /* don't reach */
	    break;
	  }
	}

	return matching;
}

#define ODDS_5A 3000 /*!< ファイブエースの役倍率 */
#define ODDS_5C 400 /*!< ファイブカードの役倍率 */
#define ODDS_RF 200 /*!< ロイヤルストレートフラッシュの役倍率 */
#define ODDS_SF 80 /*!< ストレートフラッシュの役倍率 */
#define ODDS_4C 16 /*!< フォアカードの役倍率 */
#define ODDS_FH 12 /*!< フルハウスの役倍率 */
#define ODDS_FL 8 /*!< フラッシュの役倍率 */
#define ODDS_ST 4 /*!< ストレートの役倍率 */
#define ODDS_3C 1 /*!< スリーカードの役倍率 */
#define ODDS_2P 1 /*!< ツーペアの役倍率 */

/*!
 * @brief ポーカーの役をチェックし、その結果を画面に表示しつつ結果を返す。
 * @return 役のID
 */
static int yaku_check(void)
{
	prt("                            ", 4, 3);

	switch(yaku_check_straight()){
	case 3: /* RF! */
		c_put_str(TERM_YELLOW, _("ロイヤルストレートフラッシュ", "Royal Flush"),  4,  3);
		return ODDS_RF;
	case 2: /* SF! */
		c_put_str(TERM_YELLOW, _("ストレートフラッシュ", "Straight Flush"),  4,  3);
		return ODDS_SF;
	case 1:
		c_put_str(TERM_YELLOW, _("ストレート", "Straight"),  4,  3);
		return ODDS_ST;
	default:
		/* Not straight -- fall through */
		break;
	}

	if (yaku_check_flush())
	{
		c_put_str(TERM_YELLOW, _("フラッシュ", "Flush"),  4,  3);
		return ODDS_FL;
	}

	switch (yaku_check_pair())
	{
	case 1:
		c_put_str(TERM_YELLOW, _("ワンペア", "One pair"),  4,  3);
		return 0;
	case 2:
		c_put_str(TERM_YELLOW, _("ツーペア", "Two pair"),  4,  3);
		return ODDS_2P;
	case 3:
		c_put_str(TERM_YELLOW, _("スリーカード", "Three of a kind"),  4,  3);
		return ODDS_3C;
	case 4:
		c_put_str(TERM_YELLOW, _("フルハウス", "Full house"),  4,  3);
		return ODDS_FH;
	case 6:
		c_put_str(TERM_YELLOW, _("フォーカード", "Four of a kind"),  4,  3);
		return ODDS_4C;
	case 7:
		if (!NUM_OF(cards[0]) && !NUM_OF(cards[1]))
		{
			c_put_str(TERM_YELLOW, _("ファイブエース", "Five ace"),  4,  3);
			return ODDS_5A;
		}
		else
		{
			c_put_str(TERM_YELLOW, _("ファイブカード", "Five of a kind"),  4,  3);
			return ODDS_5C;
		}
	default:
		break;
	}
	return 0;
}

/*!
 * @brief ポーカーの捨てる/残すインターフェイスの表示を更新する。
 * @param hoge カーソルの現在位置
 * @param kaeruka カードの捨てる/残すフラグ配列
 * @return なし
 */
static void display_kaeruka(int hoge, int kaeruka[])
{
	int i;
	char col = TERM_WHITE;
	for (i = 0; i < 5; i++)
	{
		if (i == hoge) col = TERM_YELLOW;
		else if(kaeruka[i]) col = TERM_WHITE;
		else col = TERM_L_BLUE;
		
		if(kaeruka[i])
			c_put_str(col, _("かえる", "Change"), 14,  5+i*16);
		else
			c_put_str(col, _("のこす", " Stay "), 14,  5+i*16);
	}
	if (hoge > 4) col = TERM_YELLOW;
	else col = TERM_WHITE;
	c_put_str(col, _("決定", "Sure"), 16,  38);

	/* Hilite current option */
	if (hoge < 5) move_cursor(14, 5+hoge*16);
	else move_cursor(16, 38);
}

/*!
 * @brief ポーカーの手札を表示する。
 * @return なし
 */
static void display_cards(void)
{
	int i, j;
	char suitcolor[4] = {TERM_YELLOW, TERM_L_RED, TERM_L_BLUE, TERM_L_GREEN};
#ifdef JP
	concptr suit[4] = {"★", "●", "¶", "†"};
	concptr card_grph[13][7] = {{"Ａ   %s     ",
				  "     変     ",
				  "     愚     ",
				  "     蛮     ",
				  "     怒     ",
				  "     %s     ",
				  "          Ａ"},
				 {"２          ",
				  "     %s     ",
				  "            ",
				  "            ",
				  "            ",
				  "     %s     ",
				  "          ２"},
				 {"３          ",
				  "     %s     ",
				  "            ",
				  "     %s     ",
				  "            ",
				  "     %s     ",
				  "          ３"},
				 {"４          ",
				  "   %s  %s   ",
				  "            ",
				  "            ",
				  "            ",
				  "   %s  %s   ",
				  "          ４"},
				 {"５          ",
				  "   %s  %s   ",
				  "            ",
				  "     %s     ",
				  "            ",
				  "   %s  %s   ",
				  "          ５"},
				 {"６          ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "          ６"},
				 {"７          ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "          ７"},
				 {"８          ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "          ８"},
				 {"９ %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s ９"},
				 {"10 %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s 10"},
				 {"Ｊ   Λ     ",
				  "%s   ||     ",
				  "     ||     ",
				  "     ||     ",
				  "     ||     ",
				  "   |=亜=| %s",
				  "     目   Ｊ"},
				 {"Ｑ ######   ",
				  "%s#      #  ",
				  "  # ++++ #  ",
				  "  # +==+ #  ",
				  "   # ++ #   ",
				  "    #  #  %s",
				  "     ##   Ｑ"},
				 {"Ｋ          ",
				  "%s ｀⌒´   ",
				  "  γγγλ  ",
				  "  ο ο ι  ",
				  "   υ    ∂ ",
				  "    σ ノ %s",
				  "          Ｋ"}};
	concptr joker_grph[7] = {    "            ",
				  "     Ｊ     ",
				  "     Ｏ     ",
				  "     Ｋ     ",
				  "     Ｅ     ",
				  "     Ｒ     ",
				  "            "};

#else

	concptr suit[4] = {"[]", "qp", "<>", "db"};
	concptr card_grph[13][7] = {{"A    %s     ",
				  "     He     ",
				  "     ng     ",
				  "     ba     ",
				  "     nd     ",
				  "     %s     ",
				  "           A"},
				 {"2           ",
				  "     %s     ",
				  "            ",
				  "            ",
				  "            ",
				  "     %s     ",
				  "           2"},
				 {"3           ",
				  "     %s     ",
				  "            ",
				  "     %s     ",
				  "            ",
				  "     %s     ",
				  "           3"},
				 {"4           ",
				  "   %s  %s   ",
				  "            ",
				  "            ",
				  "            ",
				  "   %s  %s   ",
				  "           4"},
				 {"5           ",
				  "   %s  %s   ",
				  "            ",
				  "     %s     ",
				  "            ",
				  "   %s  %s   ",
				  "           5"},
				 {"6           ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "           6"},
				 {"7           ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "           7"},
				 {"8           ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "           8"},
				 {"9  %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s  9"},
				 {"10 %s  %s   ",
				  "     %s     ",
				  "   %s  %s   ",
				  "            ",
				  "   %s  %s   ",
				  "     %s     ",
				  "   %s  %s 10"},
				 {"J    /\\     ",
				  "%s   ||     ",
				  "     ||     ",
				  "     ||     ",
				  "     ||     ",
				  "   |=HH=| %s",
				  "     ][    J"},
				 {"Q  ######   ",
				  "%s#      #  ",
				  "  # ++++ #  ",
				  "  # +==+ #  ",
				  "   # ++ #   ",
				  "    #  #  %s",
				  "     ##    Q"},
				 {"K           ",
				  "%s _'~~`_   ",
				  "   jjjjj$&  ",
				  "   q q uu   ",
				  "   c    &   ",
				  "    v__/  %s",
				  "           K"}};
	concptr joker_grph[7] = {    "            ",
				  "     J      ",
				  "     O      ",
				  "     K      ",
				  "     E      ",
				  "     R      ",
				  "            "};
#endif

	for (i = 0; i < 5; i++)
	{
		prt(_("┏━━━━━━┓", " +------------+ "),  5,  i*16);
	}

	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 7; j++)
		{
			prt(_("┃", " |"),  j+6,  i*16);
			if(IS_JOKER(cards[i]))
				c_put_str(TERM_VIOLET, joker_grph[j],  j+6,  2+i*16);
			else
				c_put_str(suitcolor[SUIT_OF(cards[i])], format(card_grph[NUM_OF(cards[i])][j], suit[SUIT_OF(cards[i])], suit[SUIT_OF(cards[i])]),  j+6,  2+i*16);
			prt(_("┃", "| "),  j+6,  i*16+14);
		}
	}
	for (i = 0; i < 5; i++)
	{
		prt(_("┗━━━━━━┛", " +------------+ "), 13,  i*16);
	}
}

/*!
 * @brief ポーカーの１プレイルーチン。
 * @return １プレイの役の結果
 */
static int do_poker(void)
{
	int i, k = 2;
	char cmd;
	int deck[53]; /* yamafuda : 0...52 */
	int deck_ptr = 0;
	int kaeruka[5]; /* 0:kaenai 1:kaeru */

	bool done = FALSE;
	bool kettei = TRUE;
	bool kakikae = TRUE;

	reset_deck(deck);

	for (i = 0; i < 5; i++)
	{
		cards[i] = deck[deck_ptr++];
		kaeruka[i] = 0; /* default:nokosu */
	}

	/* suteruno wo kimeru */
	prt(_("残すカードを決めて下さい(方向で移動, スペースで選択)。", "Stay witch? "), 0, 0);

	display_cards();
	yaku_check();

	while (!done)
	{
		if (kakikae) display_kaeruka(k+kettei*5, kaeruka);
		kakikae = FALSE;
		cmd = inkey();
		switch (cmd)
		{
		case '6': case 'l': case 'L': case KTRL('F'):
			if (!kettei) k = (k+1)%5;
			else {k = 0;kettei = FALSE;}
			kakikae = TRUE;
			break;
		case '4': case 'h': case 'H': case KTRL('B'):
			if (!kettei) k = (k+4)%5;
			else {k = 4;kettei = FALSE;}
			kakikae = TRUE;
			break;
		case '2': case 'j': case 'J': case KTRL('N'):
			if (!kettei) {kettei = TRUE;kakikae = TRUE;}
			break;
		case '8': case 'k': case 'K': case KTRL('P'):
			if (kettei) {kettei = FALSE;kakikae = TRUE;}
			break;
		case ' ': case '\r':
			if (kettei) done = TRUE;
			else {kaeruka[k] = !kaeruka[k];kakikae = TRUE;}
			break;
		default:
			break;
		}
	}
	
	prt("",0,0);

	for (i = 0; i < 5; i++)
		if (kaeruka[i] == 1) cards[i] = deck[deck_ptr++]; /* soshite toru */

	display_cards();
	
	return yaku_check();
}
#undef SUIT_OF
#undef NUM_OF
#undef IS_JOKER
/* end of poker codes --Koka */

/*!
 * @brief カジノ１プレイごとのメインルーチン / gamble_comm
 * @param cmd プレイするゲームID
 * @return なし
 */
static bool gamble_comm(int cmd)
{
	int i;
	int roll1, roll2, roll3, choice, odds, win;
	s32b wager;
	s32b maxbet;
	s32b oldgold;

	char out_val[160], tmp_str[80], again;
	concptr p;

	screen_save();

	if (cmd == BACT_GAMBLE_RULES)
	{
		/* Peruse the gambling help file */
		(void)show_file(TRUE, _("jgambling.txt", "gambling.txt"), NULL, 0, 0);
	}
	else
	{
		/* No money */
		if (p_ptr->au < 1)
		{
			msg_print(_("おい！おまえ一文なしじゃないか！こっから出ていけ！", 
						"Hey! You don't have gold - get out of here!"));
			msg_print(NULL);
			screen_load();
			return FALSE;
		}

		clear_bldg(5, 23);

		maxbet = p_ptr->lev * 200;

		/* We can't bet more than we have */
		maxbet = MIN(maxbet, p_ptr->au);

		/* Get the wager */
		strcpy(out_val, "");
		sprintf(tmp_str,_("賭け金 (1-%ld)？", "Your wager (1-%ld) ? "), (long int)maxbet);


		/*
		 * Use get_string() because we may need more than
		 * the s16b value returned by get_quantity().
		 */
		if (get_string(tmp_str, out_val, 32))
		{
			/* Strip spaces */
			for (p = out_val; *p == ' '; p++);

			/* Get the wager */
			wager = atol(p);

			if (wager > p_ptr->au)
			{
				msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));
				msg_print(NULL);
				screen_load();
				return (FALSE);
			}
			else if (wager > maxbet)
			{
				msg_format(_("%ldゴールドだけ受けよう。残りは取っときな。",
							 "I'll take %ld gold of that. Keep the rest."), (long int)maxbet);
				wager = maxbet;
			}
			else if (wager < 1)
			{
				msg_print(_("ＯＫ、１ゴールドからはじめよう。", "Ok, we'll start with 1 gold."));
				wager = 1;
			}
			msg_print(NULL);
			win = FALSE;
			odds = 0;
			oldgold = p_ptr->au;

			sprintf(tmp_str, _("ゲーム前の所持金: %9ld", "Gold before game: %9ld"), (long int)oldgold);
			prt(tmp_str, 20, 2);
			sprintf(tmp_str, _("現在の掛け金:     %9ld", "Current Wager:    %9ld"), (long int)wager);
			prt(tmp_str, 21, 2);

			do
			{
				p_ptr->au -= wager;
				switch (cmd)
				{
				 case BACT_IN_BETWEEN: /* Game of In-Between */
					c_put_str(TERM_GREEN, _("イン・ビトイーン", "In Between"),5,2);

					odds = 4;
					win = FALSE;
					roll1 = randint1(10);
					roll2 = randint1(10);
					choice = randint1(10);
					sprintf(tmp_str, _("黒ダイス: %d        黒ダイス: %d", "Black die: %d       Black Die: %d"), roll1, roll2);

					prt(tmp_str, 8, 3);
					sprintf(tmp_str, _("赤ダイス: %d", "Red die: %d"), choice);

					prt(tmp_str, 11, 14);
					if (((choice > roll1) && (choice < roll2)) ||
						((choice < roll1) && (choice > roll2)))
						win = TRUE;
					break;
				case BACT_CRAPS:  /* Game of Craps */
					c_put_str(TERM_GREEN, _("クラップス", "Craps"), 5, 2);

					win = 3;
					odds = 2;
					roll1 = randint1(6);
					roll2 = randint1(6);
					roll3 = roll1 +  roll2;
					choice = roll3;
					sprintf(tmp_str, _("１振りめ: %d %d      Total: %d", 
									   "First roll: %d %d    Total: %d"), roll1, roll2, roll3);
					prt(tmp_str, 7, 5);
					if ((roll3 == 7) || (roll3 == 11))
						win = TRUE;
					else if ((roll3 == 2) || (roll3 == 3) || (roll3 == 12))
						win = FALSE;
					else
						do
						{
							msg_print(_("なにかキーを押すともう一回振ります。", "Hit any key to roll again"));

							msg_print(NULL);
							roll1 = randint1(6);
							roll2 = randint1(6);
							roll3 = roll1 +  roll2;
							sprintf(tmp_str, _("出目: %d %d          合計:      %d", 
										   "Roll result: %d %d   Total:     %d"), roll1, roll2, roll3);
							prt(tmp_str, 8, 5);
							if (roll3 == choice)
								win = TRUE;
							else if (roll3 == 7)
								win = FALSE;
						} while ((win != TRUE) && (win != FALSE));
					break;

				case BACT_SPIN_WHEEL:  /* Spin the Wheel Game */
					win = FALSE;
					odds = 9;
					c_put_str(TERM_GREEN, _("ルーレット", "Wheel"), 5, 2);

					prt("0  1  2  3  4  5  6  7  8  9", 7, 5);
					prt("--------------------------------", 8, 3);
					strcpy(out_val, "");
					get_string(_("何番？ (0-9): ", "Pick a number (0-9): "), out_val, 32);

					for (p = out_val; iswspace(*p); p++);
					choice = atol(p);
					if (choice < 0)
					{
						msg_print(_("0番にしとくぜ。", "I'll put you down for 0."));
						choice = 0;
					}
					else if (choice > 9)
					{
						msg_print(_("ＯＫ、9番にしとくぜ。", "Ok, I'll put you down for 9."));
						choice = 9;
					}
					msg_print(NULL);
					roll1 = randint0(10);
					sprintf(tmp_str, _("ルーレットは回り、止まった。勝者は %d番だ。", 
									   "The wheel spins to a stop and the winner is %d"), roll1);
					prt(tmp_str, 13, 3);
					prt("", 9, 0);
					prt("*", 9, (3 * roll1 + 5));
					if (roll1 == choice)
						win = TRUE;
					break;

				case BACT_DICE_SLOTS: /* The Dice Slots */
					c_put_str(TERM_GREEN,  _("ダイス・スロット", "Dice Slots"), 5, 2);
					c_put_str(TERM_YELLOW, _("レモン   レモン            2", ""), 6, 37);
					c_put_str(TERM_YELLOW, _("レモン   レモン   レモン   5", ""), 7, 37);
					c_put_str(TERM_ORANGE, _("オレンジ オレンジ オレンジ 10", ""), 8, 37);
					c_put_str(TERM_UMBER, _("剣       剣       剣       20", ""), 9, 37);
					c_put_str(TERM_SLATE, _("盾       盾       盾       50", ""), 10, 37);
					c_put_str(TERM_VIOLET, _("プラム   プラム   プラム   200", ""), 11, 37);
					c_put_str(TERM_RED, _("チェリー チェリー チェリー 1000", ""), 12, 37);
					
					win = FALSE;
					roll1 = randint1(21);
					for (i=6;i>0;i--)
					{
						if ((roll1-i) < 1)
						{
							roll1 = 7-i;
							break;
						}
						roll1 -= i;
					}
					roll2 = randint1(21);
					for (i=6;i>0;i--)
					{
						if ((roll2-i) < 1)
						{
							roll2 = 7-i;
							break;
						}
						roll2 -= i;
					}
					choice = randint1(21);
					for (i=6;i>0;i--)
					{
						if ((choice-i) < 1)
						{
							choice = 7-i;
							break;
						}
						choice -= i;
					}
					put_str("/--------------------------\\", 7, 2);
					prt("\\--------------------------/", 17, 2);
					display_fruit(8,  3, roll1 - 1);
					display_fruit(8, 12, roll2 - 1);
					display_fruit(8, 21, choice - 1);
					if ((roll1 == roll2) && (roll2 == choice))
					{
						win = TRUE;
						switch(roll1)
						{
						case 1:
							odds = 5;break;
						case 2:
							odds = 10;break;
						case 3:
							odds = 20;break;
						case 4:
							odds = 50;break;
						case 5:
							odds = 200;break;
						case 6:
							odds = 1000;break;
						}
					}
					else if ((roll1 == 1) && (roll2 == 1))
					{
						win = TRUE;
						odds = 2;
					}
					break;
				case BACT_POKER:
					win = FALSE;
					odds = do_poker();
					if (odds) win = TRUE;
					break;
				}

				if (win)
				{
					prt(_("あなたの勝ち", "YOU WON"), 16, 37);

					p_ptr->au += odds * wager;
					sprintf(tmp_str, _("倍率: %d", "Payoff: %d"), odds);

					prt(tmp_str, 17, 37);
				}
				else
				{
					prt(_("あなたの負け", "You Lost"), 16, 37);
					prt("", 17, 37);
				}
				sprintf(tmp_str, _("現在の所持金:     %9ld", "Current Gold:     %9ld"), (long int)p_ptr->au);

				prt(tmp_str, 22, 2);
				prt(_("もう一度(Y/N)？", "Again(Y/N)?"), 18, 37);

				move_cursor(18, 52);
				again = inkey();
				prt("", 16, 37);
				prt("", 17, 37);
				prt("", 18, 37);
				if (wager > p_ptr->au)
				{
					msg_print(_("おい！金が足りないじゃないか！ここから出て行け！", 
								"Hey! You don't have the gold - get out of here!"));
					msg_print(NULL);

					/* Get out here */
					break;
				}
			} while ((again == 'y') || (again == 'Y'));

			prt("", 18, 37);
			if (p_ptr->au >= oldgold)
			{
				msg_print(_("「今回は儲けたな！でも次はこっちが勝ってやるからな、絶対に！」",
							"You came out a winner! We'll win next time, I'm sure."));
				chg_virtue(p_ptr, V_CHANCE, 3);
			}
			else
			{
				msg_print(_("「金をスッてしまったな、わはは！うちに帰った方がいいぜ。」", "You lost gold! Haha, better head home."));
				chg_virtue(p_ptr, V_CHANCE, -3);
			}
		}
		msg_print(NULL);
	}
	screen_load();
	return (TRUE);
}

/*!
 * @brief モンスター闘技場に参加するモンスターを更新する。
 * @return なし
 */
void update_gambling_monsters(void)
{
	int total, i;
	int max_dl = 0;
	int mon_level;
	int power[4];
	bool tekitou;
	bool old_inside_battle = p_ptr->phase_out;

	for (i = 0; i < current_world_ptr->max_d_idx; i++)
		if (max_dl < max_dlv[i]) max_dl = max_dlv[i];

	mon_level = randint1(MIN(max_dl, 122)) + 5;
	if (randint0(100) < 60)
	{
		i = randint1(MIN(max_dl, 122)) + 5;
		mon_level = MAX(i, mon_level);
	}
	if (randint0(100) < 30)
	{
		i = randint1(MIN(max_dl, 122)) + 5;
		mon_level = MAX(i, mon_level);
	}

	while (1)
	{
		total = 0;
		tekitou = FALSE;
		for (i = 0; i < 4; i++)
		{
			MONRACE_IDX r_idx;
			int j;
			while (1)
			{
				get_mon_num_prep(monster_can_entry_arena, NULL);
				p_ptr->phase_out = TRUE;
				r_idx = get_mon_num(mon_level);
				p_ptr->phase_out = old_inside_battle;
				if (!r_idx) continue;

				if ((r_info[r_idx].flags1 & RF1_UNIQUE) || (r_info[r_idx].flags7 & RF7_UNIQUE2))
				{
					if ((r_info[r_idx].level + 10) > mon_level) continue;
				}

				for (j = 0; j < i; j++)
					if (r_idx == battle_mon[j]) break;
				if (j < i) continue;

				break;
			}
			battle_mon[i] = r_idx;
			if (r_info[r_idx].level < 45) tekitou = TRUE;
		}

		for (i = 0; i < 4; i++)
		{
			monster_race *r_ptr = &r_info[battle_mon[i]];
			int num_taisei = count_bits(r_ptr->flagsr & (RFR_IM_ACID | RFR_IM_ELEC | RFR_IM_FIRE | RFR_IM_COLD | RFR_IM_POIS));

			if (r_ptr->flags1 & RF1_FORCE_MAXHP)
				power[i] = r_ptr->hdice * r_ptr->hside * 2;
			else
				power[i] = r_ptr->hdice * (r_ptr->hside + 1);
			power[i] = power[i] * (100 + r_ptr->level) / 100;
			if (r_ptr->speed > 110)
				power[i] = power[i] * (r_ptr->speed * 2 - 110) / 100;
			if (r_ptr->speed < 110)
				power[i] = power[i] * (r_ptr->speed - 20) / 100;
			if (num_taisei > 2)
				power[i] = power[i] * (num_taisei * 2 + 5) / 10;
			else if (r_ptr->a_ability_flags2 & RF6_INVULNER)
				power[i] = power[i] * 4 / 3;
			else if (r_ptr->a_ability_flags2 & RF6_HEAL)
				power[i] = power[i] * 4 / 3;
			else if (r_ptr->a_ability_flags1 & RF5_DRAIN_MANA)
				power[i] = power[i] * 11 / 10;
			if (r_ptr->flags1 & RF1_RAND_25)
				power[i] = power[i] * 9 / 10;
			if (r_ptr->flags1 & RF1_RAND_50)
				power[i] = power[i] * 9 / 10;
			if (r_ptr->flagsr & RFR_RES_ALL) power[i] *= 100000;
			if (r_ptr->arena_ratio) power[i] = power[i] * r_ptr->arena_ratio / 100;
			total += power[i];
		}
		for (i = 0; i < 4; i++)
		{
			if (power[i] <= 0) break;
			power[i] = total * 60 / power[i];
			if (tekitou && ((power[i] < 160) || power[i] > 1500)) break;
			if ((power[i] < 160) && randint0(20)) break;
			if (power[i] < 101) power[i] = 100 + randint1(5);
			mon_odds[i] = power[i];
		}
		if (i == 4) break;
	}
}

/*!
 * @brief モンスター闘技場のメインルーチン
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 賭けを開始したか否か
 */
static bool kakutoujou(player_type *player_ptr)
{
	PRICE maxbet;
	PRICE wager;
	char out_val[160], tmp_str[80];
	concptr p;

	if ((current_world_ptr->game_turn - current_world_ptr->arena_start_turn) > TURNS_PER_TICK * 250)
	{
		update_gambling_monsters();
		current_world_ptr->arena_start_turn = current_world_ptr->game_turn;
	}

	screen_save();

	/* No money */
	if (player_ptr->au <= 1)
	{
		msg_print(_("おい！おまえ一文なしじゃないか！こっから出ていけ！", "Hey! You don't have gold - get out of here!"));
		msg_print(NULL);
		screen_load();
		return FALSE;
	}
	else
	{
		int i;

		clear_bldg(4, 10);

		prt(_("モンスター                                                     倍率",
			  "Monsters                                                       Odds"), 4, 4);
		for (i=0;i<4;i++)
		{
			char buf[80];
			monster_race *r_ptr = &r_info[battle_mon[i]];

			sprintf(buf, _("%d) %-58s  %4ld.%02ld倍", "%d) %-58s  %4ld.%02ld"), i+1, 
						 _(format("%s%s",r_name + r_ptr->name, (r_ptr->flags1 & RF1_UNIQUE) ? "もどき" : "      "),
						   format("%s%s", (r_ptr->flags1 & RF1_UNIQUE) ? "Fake " : "", r_name + r_ptr->name)),
						(long int)mon_odds[i]/100, (long int)mon_odds[i]%100);
			prt(buf, 5+i, 1);
		}
		prt(_("どれに賭けますか:", "Which monster: "), 0, 0);
		while(1)
		{
			i = inkey();

			if (i == ESCAPE)
			{
				screen_load();
				return FALSE;
			}
			if (i >= '1' && i <= '4')
			{
				sel_monster = i-'1';
				battle_odds = mon_odds[sel_monster];
				break;
			}
			else bell();
		}

		clear_bldg(4, 4);
		for (i = 0; i < 4; i++)
			if (i != sel_monster) clear_bldg(i + 5, i + 5);

		maxbet = player_ptr->lev * 200;

		/* We can't bet more than we have */
		maxbet = MIN(maxbet, player_ptr->au);

		/* Get the wager */
		strcpy(out_val, "");
		sprintf(tmp_str,_("賭け金 (1-%ld)？", "Your wager (1-%ld) ? "), (long int)maxbet);
		/*
		 * Use get_string() because we may need more than
		 * the s16b value returned by get_quantity().
		 */
		if (get_string(tmp_str, out_val, 32))
		{
			/* Strip spaces */
			for (p = out_val; *p == ' '; p++);

			/* Get the wager */
			wager = atol(p);

			if (wager > player_ptr->au)
			{
				msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));

				msg_print(NULL);
				screen_load();
				return (FALSE);
			}
			else if (wager > maxbet)
			{
				msg_format(_("%ldゴールドだけ受けよう。残りは取っときな。", "I'll take %ld gold of that. Keep the rest."), (long int)maxbet);

				wager = maxbet;
			}
			else if (wager < 1)
			{
				msg_print(_("ＯＫ、１ゴールドでいこう。", "Ok, we'll start with 1 gold."));
				wager = 1;
			}
			msg_print(NULL);
			battle_odds = MAX(wager+1, wager * battle_odds / 100);
			kakekin = wager;
			player_ptr->au -= wager;
			reset_tim_flags(player_ptr);

			/* Save the surface floor as saved floor */
			prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

			player_ptr->phase_out = TRUE;
			player_ptr->leaving = TRUE;
			player_ptr->leave_bldg = TRUE;

			screen_load();

			return (TRUE);
		}
	}
	screen_load();

	return (FALSE);
}

/*!
 * @brief 本日の賞金首情報を表示する。
 * @return なし
 */
static void today_target(void)
{
	char buf[160];
	monster_race *r_ptr = &r_info[today_mon];

	clear_bldg(4,18);
	c_put_str(TERM_YELLOW, _("本日の賞金首", "Wanted monster that changes from day to day"), 5, 10);
	sprintf(buf,_("ターゲット： %s", "target: %s"),r_name + r_ptr->name);
	c_put_str(TERM_YELLOW, buf, 6, 10);
	sprintf(buf,_("死体 ---- $%d", "corpse   ---- $%d"), (int)r_ptr->level * 50 + 100);
	prt(buf, 8, 10);
	sprintf(buf,_("骨   ---- $%d", "skeleton ---- $%d"), (int)r_ptr->level * 30 + 60);
	prt(buf, 9, 10);
	p_ptr->today_mon = today_mon;
}

/*!
 * @brief ツチノコの賞金首情報を表示する。
 * @return なし
 */
static void tsuchinoko(void)
{
	clear_bldg(4,18);
	c_put_str(TERM_YELLOW, _("一獲千金の大チャンス！！！", "Big chance to quick money!!!"), 5, 10);
	c_put_str(TERM_YELLOW, _("ターゲット：幻の珍獣「ツチノコ」", "target: the rarest animal 'Tsuchinoko'"), 6, 10);
	c_put_str(TERM_WHITE, _("生け捕り ---- $1,000,000", "catch alive ---- $1,000,000"), 8, 10);
	c_put_str(TERM_WHITE, _("死体     ----   $200,000", "corpse      ----   $200,000"), 9, 10);
	c_put_str(TERM_WHITE, _("骨       ----   $100,000", "bones       ----   $100,000"), 10, 10);
}

/*!
 * @brief 通常の賞金首情報を表示する。
 * @return なし
 */
static void show_bounty(void)
{
	int i;
	TERM_LEN y = 0;

	clear_bldg(4,18);
	prt(_("死体を持ち帰れば報酬を差し上げます。", "Offer a prize when you bring a wanted monster's corpse"),4 ,10);
	c_put_str(TERM_YELLOW, _("現在の賞金首", "Wanted monsters"), 6, 10);

	for (i = 0; i < MAX_BOUNTY; i++)
	{
		byte color;
		concptr done_mark;
		monster_race *r_ptr = &r_info[(current_world_ptr->bounty_r_idx[i] > 10000 ? current_world_ptr->bounty_r_idx[i] - 10000 : current_world_ptr->bounty_r_idx[i])];

		if (current_world_ptr->bounty_r_idx[i] > 10000)
		{
			color = TERM_RED;
			done_mark = _("(済)", "(done)");
		}
		else
		{
			color = TERM_WHITE;
			done_mark = "";
		}

		c_prt(color, format("%s %s", r_name + r_ptr->name, done_mark), y+7, 10);

		y = (y+1) % 10;
		if (!y && (i < MAX_BOUNTY -1))
		{
			prt(_("何かキーを押してください", "Hit any key."), 0, 0);
			(void)inkey();
			prt("", 0, 0);
			clear_bldg(7,18);
		}
	}
}



/*!
 * 賞金首の報酬テーブル / List of prize object
 */
static struct {
	OBJECT_TYPE_VALUE tval; /*!< ベースアイテムのメイン種別ID */
	OBJECT_SUBTYPE_VALUE sval; /*!< ベースアイテムのサブ種別ID */
} prize_list[MAX_BOUNTY] = 
{
	{TV_POTION, SV_POTION_CURING},
	{TV_POTION, SV_POTION_SPEED},
	{TV_POTION, SV_POTION_SPEED},
	{TV_POTION, SV_POTION_RESISTANCE},
	{TV_POTION, SV_POTION_ENLIGHTENMENT},

	{TV_POTION, SV_POTION_HEALING},
	{TV_POTION, SV_POTION_RESTORE_MANA},
	{TV_SCROLL, SV_SCROLL_STAR_DESTRUCTION},
	{TV_POTION, SV_POTION_STAR_ENLIGHTENMENT},
	{TV_SCROLL, SV_SCROLL_SUMMON_PET},

	{TV_SCROLL, SV_SCROLL_GENOCIDE},
	{TV_POTION, SV_POTION_STAR_HEALING},
	{TV_POTION, SV_POTION_STAR_HEALING},
	{TV_POTION, SV_POTION_NEW_LIFE},
	{TV_SCROLL, SV_SCROLL_MASS_GENOCIDE},

	{TV_POTION, SV_POTION_LIFE},
	{TV_POTION, SV_POTION_LIFE},
	{TV_POTION, SV_POTION_AUGMENTATION},
	{TV_POTION, SV_POTION_INVULNERABILITY},
	{TV_SCROLL, SV_SCROLL_ARTIFACT},
};

/*!
 * @brief 賞金首の引き換え処理 / Get prize
 * @return 各種賞金首のいずれかでも換金が行われたか否か。
 */
static bool kankin(void)
{
	INVENTORY_IDX i;
	int j;
	bool change = FALSE;
	GAME_TEXT o_name[MAX_NLEN];
	object_type *o_ptr;

	/* Loop for inventory and right/left arm */
	for (i = 0; i <= INVEN_LARM; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		/* Living Tsuchinoko worthes $1000000 */
		if ((o_ptr->tval == TV_CAPTURE) && (o_ptr->pval == MON_TSUCHINOKO))
		{
			char buf[MAX_NLEN+20];
			object_desc(o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "),o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(1000000L * o_ptr->number));
				p_ptr->au += 1000000L * o_ptr->number;
				p_ptr->redraw |= (PR_GOLD);
				vary_item(i, -o_ptr->number);
			}
			change = TRUE;
		}
	}

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		/* Corpse of Tsuchinoko worthes $200000 */
		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_CORPSE) && (o_ptr->pval == MON_TSUCHINOKO))
		{
			char buf[MAX_NLEN+20];
			object_desc(o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "),o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(200000L * o_ptr->number));
				p_ptr->au += 200000L * o_ptr->number;
				p_ptr->redraw |= (PR_GOLD);
				vary_item(i, -o_ptr->number);
			}
			change = TRUE;
		}
	}

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		/* Bones of Tsuchinoko worthes $100000 */
		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON) && (o_ptr->pval == MON_TSUCHINOKO))
		{
			char buf[MAX_NLEN+20];
			object_desc(o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "),o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(100000L * o_ptr->number));
				p_ptr->au += 100000L * o_ptr->number;
				p_ptr->redraw |= (PR_GOLD);
				vary_item(i, -o_ptr->number);
			}
			change = TRUE;
		}
	}

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_CORPSE) && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name)))
		{
			char buf[MAX_NLEN+20];
			object_desc(o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "),o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[today_mon].level * 50 + 100) * o_ptr->number));
				p_ptr->au += (r_info[today_mon].level * 50 + 100) * o_ptr->number;
				p_ptr->redraw |= (PR_GOLD);
				vary_item(i, -o_ptr->number);
			}
			change = TRUE;
		}
	}

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON) && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name)))
		{
			char buf[MAX_NLEN+20];
			object_desc(o_name, o_ptr, 0);
			sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "),o_name);
			if (get_check(buf))
			{
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[today_mon].level * 30 + 60) * o_ptr->number));
				p_ptr->au += (r_info[today_mon].level * 30 + 60) * o_ptr->number;
				p_ptr->redraw |= (PR_GOLD);
				vary_item(i, -o_ptr->number);
			}
			change = TRUE;
		}
	}

	for (j = 0; j < MAX_BOUNTY; j++)
	{
		/* Need reverse order --- Positions will be changed in the loop */
		for (i = INVEN_PACK-1; i >= 0; i--)
		{
			o_ptr = &p_ptr->inventory_list[i];
			if ((o_ptr->tval == TV_CORPSE) && (o_ptr->pval == current_world_ptr->bounty_r_idx[j]))
			{
				char buf[MAX_NLEN+20];
				int num, k;
				INVENTORY_IDX item_new;
				object_type forge;

				object_desc(o_name, o_ptr, 0);
				sprintf(buf, _("%sを渡しますか？", "Hand %s over? "),o_name);
				if (!get_check(buf)) continue;

#if 0 /* Obsoleted */
				msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (r_info[current_world_ptr->bounty_r_idx[j]].level + 1) * 300 * o_ptr->number);
				p_ptr->au += (r_info[current_world_ptr->bounty_r_idx[j]].level+1) * 300 * o_ptr->number;
				p_ptr->redraw |= (PR_GOLD);
				vary_item(i, -o_ptr->number);
				chg_virtue(p_ptr, V_JUSTICE, 5);
				current_world_ptr->bounty_r_idx[j] += 10000;

				change = TRUE;
#endif /* Obsoleted */

				/* Hand it first */
				vary_item(i, -o_ptr->number);

				chg_virtue(p_ptr, V_JUSTICE, 5);
				current_world_ptr->bounty_r_idx[j] += 10000;

				/* Count number of unique corpses already handed */
				for (num = 0, k = 0; k < MAX_BOUNTY; k++)
				{
					if (current_world_ptr->bounty_r_idx[k] >= 10000) num++;
				}
				msg_format(_("これで合計 %d ポイント獲得しました。" ,"You earned %d point%s total."), num, (num > 1 ? "s" : ""));

				/* Prepare to make a prize */
				object_prep(&forge, lookup_kind(prize_list[num-1].tval, prize_list[num-1].sval));
				apply_magic(&forge, p_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART);

				object_aware(&forge);
				object_known(&forge);

				/*
				 * Hand it --- Assume there is an empty slot.
				 * Since a corpse is handed at first,
				 * there is at least one empty slot.
				 */
				item_new = inven_carry(p_ptr, &forge);

				object_desc(o_name, &forge, 0);
				msg_format(_("%s(%c)を貰った。", "You get %s (%c). "), o_name, index_to_label(item_new));

				/* Auto-inscription */
				autopick_alter_item(item_new, FALSE);
				handle_stuff();

				change = TRUE;
			}
		}
	}

	if (!change)
	{
		msg_print(_("賞金を得られそうなものは持っていなかった。", "You have nothing."));
		msg_print(NULL);
		return FALSE;
	}
	return TRUE;
}

/*!
 * @brief 宿屋の利用サブルーチン
 * @details inn commands\n
 * Note that resting for the night was a perfect way to avoid player\n
 * ghosts in the town *if* you could only make it to the inn in time (-:\n
 * Now that the ghosts are temporarily disabled in 2.8.X, this function\n
 * will not be that useful.  I will keep it in the hopes the player\n
 * ghost code does become a reality again. Does help to avoid filthy urchins.\n
 * Resting at night is also a quick way to restock stores -KMW-\n
 * @param cmd 宿屋の利用施設ID
 * @return 施設の利用が実際に行われたか否か。
 */
static bool inn_comm(player_type *customer_ptr, int cmd)
{
	switch (cmd)
	{
		case BACT_FOOD: /* Buy food & drink */
			if (customer_ptr->food >= PY_FOOD_FULL)
			{
				msg_print(_("今は満腹だ。", "You are full now."));
				return FALSE;
			}
			msg_print(_("バーテンはいくらかの食べ物とビールをくれた。", "The barkeep gives you some gruel and a beer."));
			(void)set_food(customer_ptr, PY_FOOD_MAX - 1);
			break;

		case BACT_REST: /* Rest for the night */
			if ((customer_ptr->poisoned) || (customer_ptr->cut))
			{
				msg_print(_("あなたに必要なのは部屋ではなく、治療者です。", "You need a healer, not a room."));
				msg_print(NULL);
				msg_print(_("すみません、でもうちで誰かに死なれちゃ困りますんで。", "Sorry, but don't want anyone dying in here."));
			}
			else
			{
				s32b oldturn = current_world_ptr->game_turn;
				int prev_day, prev_hour, prev_min;

				extract_day_hour_min(&prev_day, &prev_hour, &prev_min);
				if ((prev_hour >= 6) && (prev_hour <= 17)) 
					exe_write_diary(customer_ptr, NIKKI_BUNSHOU, 0, _("宿屋に泊まった。", "stay over daytime at the inn."));
				else
					exe_write_diary(customer_ptr, NIKKI_BUNSHOU, 0, _("宿屋に泊まった。", "stay over night at the inn."));
				
				current_world_ptr->game_turn = (current_world_ptr->game_turn / (TURNS_PER_TICK * TOWN_DAWN / 2) + 1) * (TURNS_PER_TICK * TOWN_DAWN / 2);
				if (current_world_ptr->dungeon_turn < current_world_ptr->dungeon_turn_limit)
				{
					current_world_ptr->dungeon_turn += MIN((current_world_ptr->game_turn - oldturn), TURNS_PER_TICK * 250) * INN_DUNGEON_TURN_ADJ;
					if (current_world_ptr->dungeon_turn > current_world_ptr->dungeon_turn_limit) current_world_ptr->dungeon_turn = current_world_ptr->dungeon_turn_limit;
				}

				prevent_turn_overflow();

				if ((prev_hour >= 18) && (prev_hour <= 23)) exe_write_diary(customer_ptr, NIKKI_HIGAWARI, 0, NULL);
				customer_ptr->chp = customer_ptr->mhp;

				if (ironman_nightmare)
				{
					msg_print(_("眠りに就くと恐ろしい光景が心をよぎった。", "Horrible visions flit through your mind as you sleep."));

					/* Have some nightmares */
					while(1)
					{
						sanity_blast(customer_ptr, NULL, FALSE);
						if (!one_in_(3)) break;
					}

					msg_print(_("あなたは絶叫して目を覚ました。", "You awake screaming."));
					exe_write_diary(customer_ptr, NIKKI_BUNSHOU, 0, _("悪夢にうなされてよく眠れなかった。", "be troubled by a nightmare."));
				}
				else
				{
					set_blind(customer_ptr, 0);
					set_confused(customer_ptr, 0);
					customer_ptr->stun = 0;
					customer_ptr->chp = customer_ptr->mhp;
					customer_ptr->csp = customer_ptr->msp;
					if (customer_ptr->pclass == CLASS_MAGIC_EATER)
					{
						int i;
						for (i = 0; i < 72; i++)
						{
							customer_ptr->magic_num1[i] = customer_ptr->magic_num2[i] * EATER_CHARGE;
						}
						for (; i < 108; i++)
						{
							customer_ptr->magic_num1[i] = 0;
						}
					}

					if ((prev_hour >= 6) && (prev_hour <= 17))
					{
						msg_print(_("あなたはリフレッシュして目覚め、夕方を迎えた。", "You awake refreshed for the evening."));
						exe_write_diary(customer_ptr, NIKKI_BUNSHOU, 0, _("夕方を迎えた。", "awake refreshed."));
					}
					else
					{
						msg_print(_("あなたはリフレッシュして目覚め、新たな日を迎えた。", "You awake refreshed for the new day."));
						exe_write_diary(customer_ptr, NIKKI_BUNSHOU, 0, _("すがすがしい朝を迎えた。", "awake refreshed."));
					}
				}
			}
			break;

		case BACT_RUMORS: /* Listen for rumors */
			{
				display_rumor(TRUE);
				break;
			}
	}

	return (TRUE);
}


/*!
 * @brief クエスト情報を表示しつつ処理する。/ Display quest information
 * @param questnum クエストのID
 * @param do_init クエストの開始処理(TRUE)、結果処理か(FALSE)
 * @return なし
 */
static void get_questinfo(IDX questnum, bool do_init)
{
	int i;
	QUEST_IDX old_quest;
	GAME_TEXT tmp_str[80];

	/* Clear the text */
	for (i = 0; i < 10; i++)
	{
		quest_text[i][0] = '\0';
	}

	quest_text_line = 0;

	/* Set the quest number temporary */
	old_quest = p_ptr->current_floor_ptr->inside_quest;
	p_ptr->current_floor_ptr->inside_quest = questnum;

	/* Get the quest text */
	init_flags = INIT_SHOW_TEXT;
	if (do_init) init_flags |= INIT_ASSIGN;

	process_dungeon_file("q_info.txt", 0, 0, 0, 0);

	/* Reset the old quest number */
	p_ptr->current_floor_ptr->inside_quest = old_quest;

	/* Print the quest info */
	sprintf(tmp_str, _("クエスト情報 (危険度: %d 階相当)", "Quest Information (Danger level: %d)"), (int)quest[questnum].level);

	prt(tmp_str, 5, 0);

	prt(quest[questnum].name, 7, 0);

	for (i = 0; i < 10; i++)
	{
		c_put_str(TERM_YELLOW, quest_text[i], i + 8, 0);
	}
}

/*!
 * @brief クエスト処理のメインルーチン / Request a quest from the Lord.
 * @return なし
 */
static void castle_quest(void)
{
	QUEST_IDX q_index = 0;
	monster_race *r_ptr;
	quest_type *q_ptr;
	concptr name;


	clear_bldg(4, 18);

	/* Current quest of the building */
	q_index = p_ptr->current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].special;

	/* Is there a quest available at the building? */
	if (!q_index)
	{
		put_str(_("今のところクエストはありません。", "I don't have a quest for you at the moment."), 8, 0);
		return;
	}

	q_ptr = &quest[q_index];

	/* Quest is completed */
	if (q_ptr->status == QUEST_STATUS_COMPLETED)
	{
		/* Rewarded quest */
		q_ptr->status = QUEST_STATUS_REWARDED;

		get_questinfo(q_index, FALSE);

		reinit_wilderness = TRUE;
	}
	/* Failed quest */
	else if (q_ptr->status == QUEST_STATUS_FAILED)
	{
		get_questinfo(q_index, FALSE);

		/* Mark quest as done (but failed) */
		q_ptr->status = QUEST_STATUS_FAILED_DONE;

		reinit_wilderness = TRUE;
	}
	/* Quest is still unfinished */
	else if (q_ptr->status == QUEST_STATUS_TAKEN)
	{
		put_str(_("あなたは現在のクエストを終了させていません！", "You have not completed your current quest yet!"), 8, 0);
		put_str(_("CTRL-Qを使えばクエストの状態がチェックできます。", "Use CTRL-Q to check the status of your quest."), 9, 0);
		put_str(_("クエストを終わらせたら戻って来て下さい。", "Return when you have completed your quest."), 12, 0);
	}
	/* No quest yet */
	else if (q_ptr->status == QUEST_STATUS_UNTAKEN)
	{
		q_ptr->status = QUEST_STATUS_TAKEN;

		reinit_wilderness = TRUE;

		/* Assign a new quest */
		if (q_ptr->type == QUEST_TYPE_KILL_ANY_LEVEL)
		{
			if (q_ptr->r_idx == 0)
			{
				/* Random monster at least 5 - 10 levels out of deep */
				q_ptr->r_idx = get_mon_num(q_ptr->level + 4 + randint1(6));
			}

			r_ptr = &r_info[q_ptr->r_idx];

			while ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->rarity != 1))
			{
				q_ptr->r_idx = get_mon_num(q_ptr->level) + 4 + randint1(6);
				r_ptr = &r_info[q_ptr->r_idx];
			}

			if (q_ptr->max_num == 0)
			{
				/* Random monster number */
				if (randint1(10) > 7)
					q_ptr->max_num = 1;
				else
					q_ptr->max_num = randint1(3) + 1;
			}

			q_ptr->cur_num = 0;
			name = (r_name + r_ptr->name);
			msg_format(_("クエスト: %sを %d体倒す", "Your quest: kill %d %s"), name,q_ptr->max_num);
		}
		else
		{
			get_questinfo(q_index, TRUE);
		}
	}
}


/*!
 * @brief 町に関するヘルプを表示する / Display town history
 * @return なし
 */
static void town_history(void)
{
	screen_save();

	/* Peruse the building help file */
	(void)show_file(TRUE, _("jbldg.txt", "bldg.txt"), NULL, 0, 0);
	screen_load();
}

/*!
 * @brief 攻撃時スレイによるダメージ期待値修正計算 / critical happens at i / 10000
 * @param dam 基本ダメージ
 * @param mult スレイ倍率（掛け算部分）
 * @param div スレイ倍率（割り算部分）
 * @param force 理力特別計算フラグ
 * @return ダメージ期待値
 */
static HIT_POINT calc_slaydam(HIT_POINT dam, int mult, int div, bool force)
{
	int tmp;
	if(force)
	{
		tmp = dam * 60;
		tmp *= mult * 3;
		tmp /= div * 2;
		tmp += dam * 60 * 2;
		tmp /= 60;
	}
	else
	{
		tmp = dam * 60;
		tmp *= mult; 
		tmp /= div;
		tmp /= 60;
	}
	return tmp;
}

/*!
 * @brief 攻撃時の期待値計算（スレイ→重量クリティカル→切れ味効果）
 * @param dam 基本ダメージ
 * @param mult スレイ倍率（掛け算部分）
 * @param div スレイ倍率（割り算部分）
 * @param force 理力特別計算フラグ
 * @param weight 重量
 * @param plus 武器ダメージ修正
 * @param meichuu 命中値
 * @param dokubari 毒針処理か否か
 * @param vorpal_mult 切れ味倍率（掛け算部分）
 * @param vorpal_div 切れ味倍率（割り算部分）
 * @return ダメージ期待値
 */
static u32b calc_expect_dice(u32b dam, int mult, int div, bool force, WEIGHT weight, int plus, s16b meichuu, bool dokubari, int vorpal_mult, int vorpal_div)
{
	dam = calc_slaydam(dam, mult, div, force);
	dam = calc_expect_crit(weight, plus, dam, meichuu, dokubari);
	dam = calc_slaydam(dam, vorpal_mult, vorpal_div, FALSE);
	return dam;
}


/*!
 * @brief 武器の各条件毎のダメージ期待値を表示する。
 * @param r 表示行
 * @param c 表示列
 * @param mindice ダイス部分最小値
 * @param maxdice ダイス部分最大値
 * @param blows 攻撃回数
 * @param dam_bonus ダメージ修正値
 * @param attr 条件内容
 * @param color 条件内容の表示色
 * @details
 * Display the damage figure of an object\n
 * (used by compare_weapon_aux)\n
 * \n
 * Only accurate for the current weapon, because it includes\n
 * the current +dam of the player.\n
 * @return なし
 */
static void show_weapon_dmg(int r, int c, int mindice, int maxdice, int blows, int dam_bonus, concptr attr, byte color)
{
	GAME_TEXT tmp_str[80];
	int mindam, maxdam;
	
	mindam = blows * (mindice + dam_bonus);
	maxdam = blows * (maxdice + dam_bonus);

	/* Print the intro text */
	c_put_str(color, attr, r, c);

	/* Calculate the min and max damage figures */
	sprintf(tmp_str, _("１ターン: %d-%d ダメージ", "Attack: %d-%d damage"), mindam, maxdam);
	
	/* Print the damage */
	put_str(tmp_str, r, c + 8);
}


/*!
 * @brief 武器一つ毎のダメージ情報を表示する。
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @param col 表示する行の上端
 * @param r 表示する列の左端
 * @details
 * Show the damage figures for the various monster types\n
 * \n
 * Only accurate for the current weapon, because it includes\n
 * the current number of blows for the player.\n
 * @return なし
 */
static void compare_weapon_aux(player_type *owner_ptr, object_type *o_ptr, int col, int r)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	int blow = owner_ptr->num_blow[0];
	bool force = FALSE;
	bool dokubari = FALSE;
	
	/* Effective dices */
	int eff_dd = o_ptr->dd + owner_ptr->to_dd[0];
	int eff_ds = o_ptr->ds + owner_ptr->to_ds[0];
	
	int mindice = eff_dd;
	int maxdice = eff_ds * eff_dd;
	int mindam = 0;
	int maxdam = 0;
	int vorpal_mult = 1;
	int vorpal_div = 1;
	int dmg_bonus = o_ptr->to_d + owner_ptr->to_d[0];
	

	/* Get the flags of the weapon */
	object_flags(o_ptr, flgs);
	
	if((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI)) dokubari = TRUE;
	
	
	/* Show Critical Damage*/
	mindam = calc_expect_crit(o_ptr->weight, o_ptr->to_h, mindice, owner_ptr->to_h[0], dokubari);
	maxdam = calc_expect_crit(o_ptr->weight, o_ptr->to_h, maxdice, owner_ptr->to_h[0], dokubari);
	
	show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("会心:", "Critical:"), TERM_L_RED);

	
	/* Vorpal Hit*/
	if ((have_flag(flgs, TR_VORPAL) || hex_spelling(HEX_RUNESWORD)))
	{
		if((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD))
		{
			vorpal_mult = 5;
			vorpal_div = 3;
		}
		else
		{
			vorpal_mult = 11;
			vorpal_div = 9;
		}
		
		mindam = calc_expect_dice(mindice, 1, 1, FALSE, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 1, 1, FALSE, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);		
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("切れ味:", "Vorpal:") , TERM_L_RED);
	}	
	
	if ((owner_ptr->pclass != CLASS_SAMURAI) && have_flag(flgs, TR_FORCE_WEAPON) && (owner_ptr->csp > (o_ptr->dd * o_ptr->ds / 5)))
	{
		force = TRUE;
		
		mindam = calc_expect_dice(mindice, 1, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 1, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("理力:", "Force  :"), TERM_L_BLUE);
	}
		
	/* Print the relevant lines */
	if (have_flag(flgs, TR_KILL_ANIMAL))
	{
		mindam = calc_expect_dice(mindice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);		
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("動物:", "Animals:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_ANIMAL)) 
	{
		mindam = calc_expect_dice(mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("動物:", "Animals:"), TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_EVIL))
	{	
		mindam = calc_expect_dice(mindice, 7, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 7, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("邪悪:", "Evil:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_EVIL))
	{	
		mindam = calc_expect_dice(mindice, 2, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 2, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);		
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("邪悪:", "Evil:"), TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_HUMAN))
	{	
		mindam = calc_expect_dice(mindice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 4, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("人間:", "Human:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_HUMAN))
	{	
		mindam = calc_expect_dice(mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("人間:", "Human:"), TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_UNDEAD))
	{
		mindam = calc_expect_dice(mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("不死:", "Undead:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_UNDEAD)) 
	{
		mindam = calc_expect_dice(mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("不死:", "Undead:"), TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_DEMON))
	{	
		mindam = calc_expect_dice(mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("悪魔:", "Demons:") , TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_DEMON))
	{	
		mindam = calc_expect_dice(mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("悪魔:", "Demons:") , TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_ORC))
	{
		mindam = calc_expect_dice(mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("オーク:", "Orcs:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_ORC))
	{
		mindam = calc_expect_dice(mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("オーク:", "Orcs:"), TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_TROLL))
	{
		mindam = calc_expect_dice(mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("トロル:", "Trolls:") , TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_TROLL))
	{
		mindam = calc_expect_dice(mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,   _("トロル:", "Trolls:") , TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_GIANT))
	{
		mindam = calc_expect_dice(mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("巨人:", "Giants:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_GIANT))
	{
		mindam = calc_expect_dice(mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("巨人:", "Giants:"), TERM_YELLOW);
	}
	if (have_flag(flgs, TR_KILL_DRAGON))
	{
		mindam = calc_expect_dice(mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("竜:", "Dragons:"), TERM_YELLOW);
	}
	else if (have_flag(flgs, TR_SLAY_DRAGON))
	{		
		mindam = calc_expect_dice(mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,   _("竜:", "Dragons:"), TERM_YELLOW);
	}
	if (have_flag(flgs, TR_BRAND_ACID))
	{
		mindam = calc_expect_dice(mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("酸属性:", "Acid:"), TERM_RED);
	}
	if (have_flag(flgs, TR_BRAND_ELEC))
	{
		mindam = calc_expect_dice(mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("電属性:", "Elec:"), TERM_RED);
	}
	if (have_flag(flgs, TR_BRAND_FIRE))
	{
		mindam = calc_expect_dice(mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("炎属性:", "Fire:"), TERM_RED);
	}
	if (have_flag(flgs, TR_BRAND_COLD))
	{
		mindam = calc_expect_dice(mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus,  _("冷属性:", "Cold:"), TERM_RED);
	}
	if (have_flag(flgs, TR_BRAND_POIS))
	{
		mindam = calc_expect_dice(mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		maxdam = calc_expect_dice(maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, owner_ptr->to_h[0], dokubari, vorpal_mult, vorpal_div);
		show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("毒属性:", "Poison:"), TERM_RED);
	}
}

/*!
 * @brief 武器匠における武器一つ毎の完全情報を表示する。
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @param row 表示する列の左端
 * @param col 表示する行の上端
 * @details
 * Displays all info about a weapon
 *
 * Only accurate for the current weapon, because it includes
 * various info about the player's +to_dam and number of blows.
 * @return なし
 */
static void list_weapon(object_type *o_ptr, TERM_LEN row, TERM_LEN col)
{
	GAME_TEXT o_name[MAX_NLEN];
	GAME_TEXT tmp_str[80];

	DICE_NUMBER eff_dd = o_ptr->dd + p_ptr->to_dd[0];
	DICE_SID eff_ds = o_ptr->ds + p_ptr->to_ds[0];
	HIT_RELIABILITY reli = p_ptr->skill_thn + (p_ptr->to_h[0] + o_ptr->to_h) * BTH_PLUS_ADJ;

	/* Print the weapon name */
	object_desc(o_name, o_ptr, OD_NAME_ONLY);
	c_put_str(TERM_YELLOW, o_name, row, col);

	/* Print the player's number of blows */
	sprintf(tmp_str, _("攻撃回数: %d", "Number of Blows: %d"), p_ptr->num_blow[0]);
	put_str(tmp_str, row+1, col);

	/* Print to_hit and to_dam of the weapon */
	sprintf(tmp_str, _("命中率:  0  50 100 150 200 (敵のAC)", "To Hit:  0  50 100 150 200 (AC)"));
	put_str(tmp_str, row+2, col);

	/* Print the weapons base damage dice */
	sprintf(tmp_str, "        %2d  %2d  %2d  %2d  %2d (%%)",
		(int)hit_chance(reli, 0),
		(int)hit_chance(reli, 50),
		(int)hit_chance(reli, 100),
		(int)hit_chance(reli, 150),
		(int)hit_chance(reli, 200));
	put_str(tmp_str, row+3, col);
	c_put_str(TERM_YELLOW, _("可能なダメージ:", "Possible Damage:"), row+5, col);

	/* Damage for one blow (if it hits) */
	sprintf(tmp_str, _("攻撃一回につき %d-%d", "One Strike: %d-%d damage"),
	    (int)(eff_dd + o_ptr->to_d + p_ptr->to_d[0]),
		(int)(eff_ds * eff_dd + o_ptr->to_d + p_ptr->to_d[0]));
	put_str(tmp_str, row+6, col+1);

	/* Damage for the complete attack (if all blows hit) */
	sprintf(tmp_str, _("１ターンにつき %d-%d", "One Attack: %d-%d damage"),
		(int)(p_ptr->num_blow[0] * (eff_dd + o_ptr->to_d + p_ptr->to_d[0])),
			(int)(p_ptr->num_blow[0] * (eff_ds * eff_dd + o_ptr->to_d + p_ptr->to_d[0])));
	put_str(tmp_str, row+7, col+1);
}

/*!
 * @brief 武器匠鑑定１回分（オブジェクト２種）の処理。/ Compare weapons
 * @details 
 * Copies the weapons to compare into the weapon-slot and\n
 * compares the values for both weapons.\n
 * 武器１つだけで比較をしないなら費用は半額になる。
 * @param bcost 基本鑑定費用
 * @return 最終的にかかった費用
 */
static PRICE compare_weapons(player_type *customer_ptr, PRICE bcost)
{
	int i, n;
	OBJECT_IDX item, item2;
	object_type *o_ptr[2];
	object_type orig_weapon;
	object_type *i_ptr;
	concptr q, s;
	TERM_LEN row = 2;
	TERM_LEN wid = 38, mgn = 2;
	bool old_character_xtra = current_world_ptr->character_xtra;
	char ch;
	PRICE total = 0;
	PRICE cost = 0; /* First time no price */

	screen_save();
	clear_bldg(0, 22);

	/* Store copy of original wielded weapon */
	i_ptr = &customer_ptr->inventory_list[INVEN_RARM];
	object_copy(&orig_weapon, i_ptr);

	item_tester_hook = item_tester_hook_orthodox_melee_weapons;

	/* Get the first weapon */
	q = _("第一の武器は？", "What is your first weapon? ");
	s = _("比べるものがありません。", "You have nothing to compare.");

	o_ptr[0] = choose_object(customer_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr[0])
	{
		screen_load();
		return (0);
	}

	n = 1;
	total = bcost;

	while (TRUE)
	{
		clear_bldg(0, 22);

		/* Only compare melee weapons */
		item_tester_hook = item_tester_hook_orthodox_melee_weapons;

		/* Hack -- prevent "icky" message */
		current_world_ptr->character_xtra = TRUE;

		/* Diaplay selected weapon's infomation */
		for (i = 0; i < n; i++)
		{
			int col = (wid * i + mgn);

			/* Copy i-th weapon into the weapon slot (if it's not already there) */
			if (o_ptr[i] != i_ptr) object_copy(i_ptr, o_ptr[i]);

			customer_ptr->update |= PU_BONUS;
			handle_stuff();

			/* List the new values */
			list_weapon(o_ptr[i], row, col);
			compare_weapon_aux(customer_ptr, o_ptr[i], col, row + 8);

			/* Copy back the original weapon into the weapon slot */
			object_copy(i_ptr, &orig_weapon);
		}

		/* Reset the values for the old weapon */
		customer_ptr->update |= PU_BONUS;
		handle_stuff();

		current_world_ptr->character_xtra = old_character_xtra;

#ifdef JP
		put_str(format("[ 比較対象: 's'で変更 ($%d) ]", cost), 1, (wid + mgn));
		put_str("(一番高いダメージが適用されます。複数の倍打効果は足し算されません。)", row + 4, 0);
		prt("現在の技量から判断すると、あなたの武器は以下のような威力を発揮します:", 0, 0);
#else
		put_str(format("[ 's' Select secondary weapon($%d) ]", cost), row + 1, (wid * i + mgn));
		put_str("(Only highest damage applies per monster. Special damage not cumulative.)", row + 4, 0);
		prt("Based on your current abilities, here is what your weapons will do", 0, 0);
#endif

		flush();
		ch = inkey();

		if (ch == 's')
		{
			if (total + cost > customer_ptr->au)
			{
				msg_print(_("お金が足りません！", "You don't have enough money!"));
				msg_print(NULL);
				continue;
			}

			q = _("第二の武器は？", "What is your second weapon? ");
			s = _("比べるものがありません。", "You have nothing to compare.");

			/* Get the second weapon */
			o_ptr[1] = choose_object(customer_ptr, &item2, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT), 0);
			if (!o_ptr[1]) continue;

			total += cost;
			cost = bcost / 2;
			n = 2;
		}
		else
		{
			break;
		}
	}
	screen_load();

	return (total);
}


/*!
 * @brief ACから回避率、ダメージ減少率を計算し表示する。 / Evaluate AC
 * @details 
 * Calculate and display the dodge-rate and the protection-rate
 * based on AC
 * @param iAC プレイヤーのAC。
 * @return 常にTRUEを返す。
 */
static bool eval_ac(ARMOUR_CLASS iAC)
{
#ifdef JP
	const char memo[] =
		"ダメージ軽減率とは、敵の攻撃が当たった時そのダメージを\n"
		"何パーセント軽減するかを示します。\n"
		"ダメージ軽減は通常の直接攻撃(種類が「攻撃する」と「粉砕する」の物)\n"
		"に対してのみ効果があります。\n \n"
		"敵のレベルとは、その敵が通常何階に現れるかを示します。\n \n"
		"回避率は敵の直接攻撃を何パーセントの確率で避けるかを示し、\n"
		"敵のレベルとあなたのACによって決定されます。\n \n"
		"ダメージ期待値とは、敵の１００ポイントの通常攻撃に対し、\n"
		"回避率とダメージ軽減率を考慮したダメージの期待値を示します。\n";
#else
	const char memo[] =
		"'Protection Rate' means how much damage is reduced by your armor.\n"
		"Note that the Protection rate is effective only against normal "
		"'attack' and 'shatter' type melee attacks, "
		"and has no effect against any other types such as 'poison'.\n \n"
		"'Dodge Rate' indicates the success rate on dodging the "
		"monster's melee attacks.  "
		"It is depend on the level of the monster and your AC.\n \n"
		"'Average Damage' indicates the expected amount of damage "
		"when you are attacked by normal melee attacks with power=100.";
#endif

	int protection;
	TERM_LEN col, row = 2;
	DEPTH lvl;
	char buf[80*20], *t;

	/* AC lower than zero has no effect */
	if (iAC < 0) iAC = 0;

	/* ダメージ軽減率を計算 */
	protection = 100 * MIN(iAC, 150) / 250;

	screen_save();
	clear_bldg(0, 22);

	put_str(format(_("あなたの現在のAC: %3d", "Your current AC : %3d"), iAC), row++, 0);
	put_str(format(_("ダメージ軽減率  : %3d%%", "Protection rate : %3d%%"), protection), row++, 0);
	row++;

	put_str(_("敵のレベル      :", "Level of Monster:"), row + 0, 0);
	put_str(_("回避率          :", "Dodge Rate      :"), row + 1, 0);
	put_str(_("ダメージ期待値  :", "Average Damage  :"), row + 2, 0);
    
	for (col = 17 + 1, lvl = 0; lvl <= 100; lvl += 10, col += 5)
	{
		int quality = 60 + lvl * 3; /* attack quality with power 60 */
		int dodge;   /* 回避率(%) */
		int average; /* ダメージ期待値 */

		put_str(format("%3d", lvl), row + 0, col);

		/* 回避率を計算 */
		dodge = 5 + (MIN(100, 100 * (iAC * 3 / 4) / quality) * 9 + 5) / 10;
		put_str(format("%3d%%", dodge), row + 1, col);

		/* 100点の攻撃に対してのダメージ期待値を計算 */
		average = (100 - dodge) * (100 - protection) / 100;
		put_str(format("%3d", average), row + 2, col);
	}

	/* Display note */
	roff_to_buf(memo, 70, buf, sizeof(buf));
	for (t = buf; t[0]; t += strlen(t) + 1)
		put_str(t, (row++) + 4, 4);

	prt(_("現在のあなたの装備からすると、あなたの防御力はこれくらいです:", "Defense abilities from your current Armor Class are evaluated below."), 0, 0);
  
	flush();
	(void)inkey();
	screen_load();

	return (TRUE);
}



/*!
 * @brief 修復材料のオブジェクトから修復対象に特性を移植する。
 * @param to_ptr 修復対象オブジェクトの構造体の参照ポインタ。
 * @param from_ptr 修復材料オブジェクトの構造体の参照ポインタ。
 * @return 修復対象になるならTRUEを返す。
 */
static void give_one_ability_of_object(object_type *to_ptr, object_type *from_ptr)
{
	int i, n = 0;
	int cand[TR_FLAG_MAX];
	BIT_FLAGS to_flgs[TR_FLAG_SIZE];
	BIT_FLAGS from_flgs[TR_FLAG_SIZE];

	object_flags(to_ptr, to_flgs);
	object_flags(from_ptr, from_flgs);

	for (i = 0; i < TR_FLAG_MAX; i++)
	{
		switch (i)
		{
		case TR_IGNORE_ACID:
		case TR_IGNORE_ELEC:
		case TR_IGNORE_FIRE:
		case TR_IGNORE_COLD:
		case TR_ACTIVATE:
		case TR_RIDING:
		case TR_THROW:
		case TR_SHOW_MODS:
		case TR_HIDE_TYPE:
		case TR_ES_ATTACK:
		case TR_ES_AC:
		case TR_FULL_NAME:
		case TR_FIXED_FLAVOR:
			break;
		default:
			if (have_flag(from_flgs, i) && !have_flag(to_flgs, i))
			{
				if (!(is_pval_flag(i) && (from_ptr->pval < 1))) cand[n++] = i;
			}
		}
	}

	if (n > 0)
	{
		int bmax;
		int tr_idx = cand[randint0(n)];
		add_flag(to_ptr->art_flags, tr_idx);
		if (is_pval_flag(tr_idx)) to_ptr->pval = MAX(to_ptr->pval, 1);
		bmax = MIN(3, MAX(1, 40 / (to_ptr->dd * to_ptr->ds)));
		if (tr_idx == TR_BLOWS) to_ptr->pval = MIN(to_ptr->pval, bmax);
		if (tr_idx == TR_SPEED) to_ptr->pval = MIN(to_ptr->pval, 4);
	}

	return;
}

/*!
 * @brief アイテム修復処理のメインルーチン / Repair broken weapon
 * @param bcost 基本修復費用
 * @return 実際にかかった費用
 */
static PRICE repair_broken_weapon_aux(PRICE bcost)
{
	PRICE cost;
	OBJECT_IDX item, mater;
	object_type *o_ptr, *mo_ptr; /* broken weapon and material weapon */
	object_kind *k_ptr;
	int i, dd_bonus, ds_bonus;
	KIND_OBJECT_IDX k_idx;
	char basenm[MAX_NLEN];
	concptr q, s;
	int row = 7;
	clear_bldg(0, 22);

	prt(_("修復には材料となるもう1つの武器が必要です。", "Hand one material weapon to repair a broken weapon."), row, 2);
	prt(_("材料に使用した武器はなくなります！", "The material weapon will disappear after repairing!!"), row+1, 2);

	q = _("どの折れた武器を修復しますか？", "Repair which broken weapon? ");
	s = _("修復できる折れた武器がありません。", "You have no broken weapon to repair.");

	/* Only forge broken weapons */
	item_tester_hook = item_tester_hook_broken_weapon;

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_INVEN | USE_EQUIP), 0);
	if (!o_ptr) return (0);

	/* It is worthless */
	if (!object_is_ego(o_ptr) && !object_is_artifact(o_ptr))
	{
		msg_format(_("それは直してもしょうがないぜ。", "It is worthless to repair."));
		return (0);
	}

	/* They are too many */
	if (o_ptr->number > 1)
	{
		msg_format(_("一度に複数を修復することはできません！", "They are too many to repair at once!"));
		return (0);
	}

	/* Display item name */
	object_desc(basenm, o_ptr, OD_NAME_ONLY);
	prt(format(_("修復する武器　： %s", "Repairing: %s"), basenm), row+3, 2);

	q = _("材料となる武器は？", "Which weapon for material? ");
	s = _("材料となる武器がありません。", "You have no material to repair.");

	/* Only forge broken weapons */
	item_tester_hook = item_tester_hook_orthodox_melee_weapons;

	mo_ptr = choose_object(p_ptr, &mater, q, s, (USE_INVEN | USE_EQUIP), 0);
	if (!mo_ptr) return (0);
	if (mater == item)
	{
		msg_print(_("クラインの壷じゃない！", "This is not a klein bottle!"));
		return (0);
	}

	/* Display item name */
	object_desc(basenm, mo_ptr, OD_NAME_ONLY);
	prt(format(_("材料とする武器： %s", "Material : %s"), basenm), row+4, 2);

	/* Get the value of one of the items (except curses) */
	cost = bcost + object_value_real(o_ptr) * 2;

	if (!get_check(format(_("＄%dかかりますがよろしいですか？ ", "Costs %d gold, okay? "), cost))) return (0);

	/* Check if the player has enough money */
	if (p_ptr->au < cost)
	{
		object_desc(basenm, o_ptr, OD_NAME_ONLY);
		msg_format(_("%sを修復するだけのゴールドがありません！", "You do not have the gold to repair %s!"), basenm);
		msg_print(NULL);
		return (0);
	}

	p_ptr->total_weight -= o_ptr->weight;

	if (o_ptr->sval == SV_BROKEN_DAGGER)
	{
		KIND_OBJECT_IDX j;
		int n = 1;

		/* Suppress compiler warning */
		k_idx = 0;

		for (j = 1; j < max_k_idx; j++)
		{
			object_kind *k_aux_ptr = &k_info[j];

			if (k_aux_ptr->tval != TV_SWORD) continue;
			if ((k_aux_ptr->sval == SV_BROKEN_DAGGER) ||
				(k_aux_ptr->sval == SV_BROKEN_SWORD) ||
				(k_aux_ptr->sval == SV_DOKUBARI)) continue;
			if (k_aux_ptr->weight > 99) continue;

			if (one_in_(n)) 
			{
				k_idx = j;
				n++;
			}
		}
	}
	else /* TV_BROKEN_SWORD */
	{
		/* Repair to a sword or sometimes material's type weapon */
		OBJECT_TYPE_VALUE tval = (one_in_(5) ? mo_ptr->tval : TV_SWORD);

		while(1)
		{
			object_kind *ck_ptr;

			k_idx = lookup_kind(tval, SV_ANY);
			ck_ptr = &k_info[k_idx];

			if (tval == TV_SWORD)
			{
				if ((ck_ptr->sval == SV_BROKEN_DAGGER) ||
					(ck_ptr->sval == SV_BROKEN_SWORD) ||
					(ck_ptr->sval == SV_DIAMOND_EDGE) ||
					(ck_ptr->sval == SV_DOKUBARI)) continue;
			}
			if (tval == TV_POLEARM)
			{
				if ((ck_ptr->sval == SV_DEATH_SCYTHE) ||
					(ck_ptr->sval == SV_TSURIZAO)) continue;
			}
			if (tval == TV_HAFTED)
			{
				if ((ck_ptr->sval == SV_GROND) ||
					(ck_ptr->sval == SV_WIZSTAFF) ||
					(ck_ptr->sval == SV_NAMAKE_HAMMER)) continue;
			}

			break;
		}
	}

	/* Calculate dice bonuses */
	dd_bonus = o_ptr->dd - k_info[o_ptr->k_idx].dd;
	ds_bonus = o_ptr->ds - k_info[o_ptr->k_idx].ds;
	dd_bonus += mo_ptr->dd - k_info[mo_ptr->k_idx].dd;
	ds_bonus += mo_ptr->ds - k_info[mo_ptr->k_idx].ds;

	/* Change base object */
	k_ptr = &k_info[k_idx];
	o_ptr->k_idx = k_idx;
	o_ptr->weight = k_ptr->weight;
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	/* Copy base object's ability */
	for (i = 0; i < TR_FLAG_SIZE; i++) o_ptr->art_flags[i] |= k_ptr->flags[i];
	if (k_ptr->pval) o_ptr->pval = MAX(o_ptr->pval, randint1(k_ptr->pval));
	if (have_flag(k_ptr->flags, TR_ACTIVATE)) o_ptr->xtra2 = (byte_hack)k_ptr->act_idx;

	/* Dice up */
	if (dd_bonus > 0)
	{
		o_ptr->dd++;
		for (i = 1; i < dd_bonus; i++)
		{
			if (one_in_(o_ptr->dd + i)) o_ptr->dd++;
		}
	}
	if (ds_bonus > 0)
	{
		o_ptr->ds++;
		for (i = 1; i < ds_bonus; i++)
		{
			if (one_in_(o_ptr->ds + i)) o_ptr->ds++;
		}
	}

	if (have_flag(k_ptr->flags, TR_BLOWS))
	{
		int bmax = MIN(3, MAX(1, 40 / (o_ptr->dd * o_ptr->ds)));
		o_ptr->pval = MIN(o_ptr->pval, bmax);
	}

	/* Add one random ability from material weapon */
	give_one_ability_of_object(o_ptr, mo_ptr);

	/* Add to-dam, to-hit and to-ac from material weapon */
	o_ptr->to_d += MAX(0, (mo_ptr->to_d / 3));
	o_ptr->to_h += MAX(0, (mo_ptr->to_h / 3));
	o_ptr->to_a += MAX(0, (mo_ptr->to_a));

	if ((o_ptr->name1 == ART_NARSIL) ||
		(object_is_random_artifact(o_ptr) && one_in_(1)) ||
		(object_is_ego(o_ptr) && one_in_(7)))
	{
		/* Forge it */
		if (object_is_ego(o_ptr))
		{
			add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
			add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
		}

		/* Add one random ability from material weapon */
		give_one_ability_of_object(o_ptr, mo_ptr);

		/* Add one random activation */
		if (!activation_index(o_ptr)) one_activation(o_ptr);

		/* Narsil */
		if (o_ptr->name1 == ART_NARSIL)
		{
			one_high_resistance(o_ptr);
			one_ability(o_ptr);
		}

		msg_print(_("これはかなりの業物だったようだ。", "This blade seems to be exceptionally."));
	}

	object_desc(basenm, o_ptr, OD_NAME_ONLY);
#ifdef JP
	msg_format("＄%dで%sに修復しました。", cost, basenm);
#else
	msg_format("Repaired into %s for %d gold.", basenm, cost);
#endif
	msg_print(NULL);

	/* Remove BROKEN flag */
	o_ptr->ident &= ~(IDENT_BROKEN);

	/* Add repaired flag */
	o_ptr->discount = 99;

	p_ptr->total_weight += o_ptr->weight;
	calc_android_exp(p_ptr);

	/* Decrease material object */
	inven_item_increase(mater, -1);
	inven_item_optimize(mater);

	/* Copyback */
	p_ptr->update |= PU_BONUS;
	handle_stuff();

	/* Something happened */
	return (cost);
}

/*!
 * @brief アイテム修復処理の過渡ルーチン / Repair broken weapon
 * @param bcost 基本鑑定費用
 * @return 実際にかかった費用
 */
static int repair_broken_weapon(PRICE bcost)
{
	PRICE cost;
	screen_save();
	cost = repair_broken_weapon_aux(bcost);
	screen_load();
	return cost;
}


/*!
 * @brief アイテムの強化を行う。 / Enchant item
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param cost 1回毎の費用
 * @param to_hit 命中をアップさせる量
 * @param to_dam ダメージをアップさせる量
 * @param to_ac ＡＣをアップさせる量
 * @return 実際に行ったらTRUE
 */
static bool enchant_item(player_type *player_ptr, PRICE cost, HIT_PROB to_hit, HIT_POINT to_dam, ARMOUR_CLASS to_ac)
{
	int i;
	OBJECT_IDX item;
	bool okay = FALSE;
	object_type *o_ptr;
	concptr q, s;
	int maxenchant = (player_ptr->lev / 5);
	char tmp_str[MAX_NLEN];

	clear_bldg(4, 18);
	prt(format(_("現在のあなたの技量だと、+%d まで改良できます。", "  Based on your skill, we can improve up to +%d."), maxenchant), 5, 0);
	prt(format(_(" 改良の料金は一個につき＄%d です。", "  The price for the service is %d gold per item."), cost), 7, 0);

	q = _("どのアイテムを改良しますか？", "Improve which item? ");
	s = _("改良できるものがありません。", "You have nothing to improve.");

	o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_EQUIP | IGNORE_BOTHHAND_SLOT), item_tester_tval);
	if (!o_ptr) return (FALSE);

	/* Check if the player has enough money */
	if (player_ptr->au < (cost * o_ptr->number))
	{
		object_desc(tmp_str, o_ptr, OD_NAME_ONLY);
		msg_format(_("%sを改良するだけのゴールドがありません！", "You do not have the gold to improve %s!"), tmp_str);
		return (FALSE);
	}

	/* Enchant to hit */
	for (i = 0; i < to_hit; i++)
	{
		if (o_ptr->to_h < maxenchant)
		{
			if (enchant(player_ptr, o_ptr, 1, (ENCH_TOHIT | ENCH_FORCE)))
			{
				okay = TRUE;
				break;
			}
		}
	}

	/* Enchant to damage */
	for (i = 0; i < to_dam; i++)
	{
		if (o_ptr->to_d < maxenchant)
		{
			if (enchant(player_ptr, o_ptr, 1, (ENCH_TODAM | ENCH_FORCE)))
			{
				okay = TRUE;
				break;
			}
		}
	}

	/* Enchant to AC */
	for (i = 0; i < to_ac; i++)
	{
		if (o_ptr->to_a < maxenchant)
		{
			if (enchant(player_ptr, o_ptr, 1, (ENCH_TOAC | ENCH_FORCE)))
			{
				okay = TRUE;
				break;
			}
		}
	}

	/* Failure */
	if (!okay)
	{
		if (flush_failure) flush();
		msg_print(_("改良に失敗した。", "The improvement failed."));
		return (FALSE);
	}
	else
	{
		object_desc(tmp_str, o_ptr, OD_NAME_AND_ENCHANT);
#ifdef JP
		msg_format("＄%dで%sに改良しました。", cost * o_ptr->number, tmp_str);
#else
		msg_format("Improved into %s for %d gold.", tmp_str, cost * o_ptr->number);
#endif

		/* Charge the money */
		player_ptr->au -= (cost * o_ptr->number);

		if (item >= INVEN_RARM) calc_android_exp(player_ptr);

		/* Something happened */
		return (TRUE);
	}
}


/*!
 * @brief 魔道具の使用回数を回復させる施設のメインルーチン / Recharge rods, wands and staves
 * @details
 * The player can select the number of charges to add\n
 * (up to a limit), and the recharge never fails.\n
 *\n
 * The cost for rods depends on the level of the rod. The prices\n
 * for recharging wands and staves are dependent on the cost of\n
 * the base-item.\n
 * @return なし
 */
static void building_recharge(void)
{
	OBJECT_IDX  item;
	DEPTH       lev;
	object_type *o_ptr;
	object_kind *k_ptr;
	concptr        q, s;
	PRICE       price;
	PARAMETER_VALUE charges;
	int         max_charges;
	char        tmp_str[MAX_NLEN];

	msg_flag = FALSE;

	/* Display some info */
	clear_bldg(4, 18);
	prt(_("  再充填の費用はアイテムの種類によります。", "  The prices of recharge depend on the type."), 6, 0);


	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	q = _("どのアイテムに魔力を充填しますか? ", "Recharge which item? ");
	s = _("魔力を充填すべきアイテムがない。", "You have nothing to recharge.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return;

	k_ptr = &k_info[o_ptr->k_idx];

	/*
	 * We don't want to give the player free info about
	 * the level of the item or the number of charges.
	 */
	/* The item must be "known" */
	if (!object_is_known(o_ptr))
	{
		msg_format(_("充填する前に鑑定されている必要があります！", "The item must be identified first!"));
		msg_print(NULL);

		if ((p_ptr->au >= 50) &&
			get_check(_("＄50で鑑定しますか？ ", "Identify for 50 gold? ")))

		{
			p_ptr->au -= 50;
			identify_item(p_ptr, o_ptr);
			object_desc(tmp_str, o_ptr, 0);
			msg_format(_("%s です。", "You have: %s."), tmp_str);

			/* Auto-inscription */
			autopick_alter_item(item, FALSE);

			/* Update the gold display */
			building_prt_gold();
		}
		else
		{
			return;
		}
	}

	/* Extract the object "level" */
	lev = k_info[o_ptr->k_idx].level;

	/* Price for a rod */
	if (o_ptr->tval == TV_ROD)
	{
		if (o_ptr->timeout > 0)
		{
			/* Fully recharge */
			price = (lev * 50 * o_ptr->timeout) / k_ptr->pval;
		}
		else
		{
			/* No recharge necessary */
			price = 0;
			msg_format(_("それは再充填する必要はありません。", "That doesn't need to be recharged."));
			return;
		}
	}
	else if (o_ptr->tval == TV_STAFF)
	{
		/* Price per charge ( = double the price paid by shopkeepers for the charge) */
		price = (k_info[o_ptr->k_idx].cost / 10) * o_ptr->number;

		/* Pay at least 10 gold per charge */
		price = MAX(10, price);
	}
	else
	{
		/* Price per charge ( = double the price paid by shopkeepers for the charge) */
		price = (k_info[o_ptr->k_idx].cost / 10);

		/* Pay at least 10 gold per charge */
		price = MAX(10, price);
	}

	/* Limit the number of charges for wands and staffs */
	if (o_ptr->tval == TV_WAND
		&& (o_ptr->pval / o_ptr->number >= k_ptr->pval))
	{
		if (o_ptr->number > 1)
		{
			msg_print(_("この魔法棒はもう充分に充填されています。", "These wands are already fully charged."));
		}
		else
		{
			msg_print(_("この魔法棒はもう充分に充填されています。", "This wand is already fully charged."));
		}
		return;
	}
	else if (o_ptr->tval == TV_STAFF && o_ptr->pval >= k_ptr->pval)
	{
		if (o_ptr->number > 1)
		{
			msg_print(_("この杖はもう充分に充填されています。", "These staffs are already fully charged."));
		}
		else
		{
			msg_print(_("この杖はもう充分に充填されています。", "This staff is already fully charged."));
		}
		return;
	}

	/* Check if the player has enough money */
	if (p_ptr->au < price)
	{
		object_desc(tmp_str, o_ptr, OD_NAME_ONLY);
#ifdef JP
		msg_format("%sを再充填するには＄%d 必要です！", tmp_str,price );
#else
		msg_format("You need %d gold to recharge %s!", price, tmp_str);
#endif

		return;
	}

	if (o_ptr->tval == TV_ROD)
	{
#ifdef JP
		if (get_check(format("そのロッドを＄%d で再充填しますか？", price)))
#else
		if (get_check(format("Recharge the %s for %d gold? ",
			((o_ptr->number > 1) ? "rods" : "rod"), price)))
#endif

		{
			/* Recharge fully */
			o_ptr->timeout = 0;
		}
		else
		{
			return;
		}
	}
	else
	{
		if (o_ptr->tval == TV_STAFF)
			max_charges = k_ptr->pval - o_ptr->pval;
		else
			max_charges = o_ptr->number * k_ptr->pval - o_ptr->pval;

		/* Get the quantity for staves and wands */
		charges = (PARAMETER_VALUE)get_quantity(format(_("一回分＄%d で何回分充填しますか？", "Add how many charges for %d gold? "), price), 
					MIN(p_ptr->au / price, max_charges));

		/* Do nothing */
		if (charges < 1) return;

		/* Get the new price */
		price *= charges;

		/* Recharge */
		o_ptr->pval += charges;

		/* We no longer think the item is empty */
		o_ptr->ident &= ~(IDENT_EMPTY);
	}

	/* Give feedback */
	object_desc(tmp_str, o_ptr, 0);
#ifdef JP
	msg_format("%sを＄%d で再充填しました。", tmp_str, price);
#else
	msg_format("%^s %s recharged for %d gold.", tmp_str, ((o_ptr->number > 1) ? "were" : "was"), price);
#endif
	p_ptr->update |= (PU_COMBINE | PU_REORDER);

	p_ptr->window |= (PW_INVEN);

	/* Pay the price */
	p_ptr->au -= price;

	/* Finished */
	return;
}


/*!
 * @brief 魔道具の使用回数を回復させる施設の一括処理向けサブルーチン / Recharge rods, wands and staves
 * @details
 * The player can select the number of charges to add\n
 * (up to a limit), and the recharge never fails.\n
 *\n
 * The cost for rods depends on the level of the rod. The prices\n
 * for recharging wands and staves are dependent on the cost of\n
 * the base-item.\n
 * @return なし
 */
static void building_recharge_all(void)
{
	INVENTORY_IDX i;
	DEPTH lev;
	object_type *o_ptr;
	object_kind *k_ptr;
	PRICE price = 0;
	PRICE total_cost = 0;


	/* Display some info */
	msg_flag = FALSE;
	clear_bldg(4, 18);
	prt(_("  再充填の費用はアイテムの種類によります。", "  The prices of recharge depend on the type."), 6, 0);

	/* Calculate cost */
	for ( i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
				
		/* skip non magic device */
		if (o_ptr->tval < TV_STAFF || o_ptr->tval > TV_ROD) continue;

		/* need identified */
		if (!object_is_known(o_ptr)) total_cost += 50;

		/* Extract the object "level" */
		lev = k_info[o_ptr->k_idx].level;

		k_ptr = &k_info[o_ptr->k_idx];

		switch (o_ptr->tval)
		{
		case TV_ROD:
			price = (lev * 50 * o_ptr->timeout) / k_ptr->pval;
			break;

		case TV_STAFF:
			/* Price per charge ( = double the price paid by shopkeepers for the charge) */
			price = (k_info[o_ptr->k_idx].cost / 10) * o_ptr->number;

			/* Pay at least 10 gold per charge */
			price = MAX(10, price);

			/* Fully charge */
			price = (k_ptr->pval - o_ptr->pval) * price;
			break;

		case TV_WAND:
			/* Price per charge ( = double the price paid by shopkeepers for the charge) */
			price = (k_info[o_ptr->k_idx].cost / 10);

			/* Pay at least 10 gold per charge */
			price = MAX(10, price);

			/* Fully charge */
			price = (o_ptr->number * k_ptr->pval - o_ptr->pval) * price;
			break;
		}

		/* if price <= 0 then item have enough charge */
		if (price > 0) total_cost += price;
	}

	if (!total_cost)
	{
		msg_print(_("充填する必要はありません。", "No need to recharge."));
		msg_print(NULL);
		return;
	}

	/* Check if the player has enough money */
	if (p_ptr->au < total_cost)
	{
		msg_format(_("すべてのアイテムを再充填するには＄%d 必要です！", "You need %d gold to recharge all items!"), total_cost );
		msg_print(NULL);
		return;
	}
	if (!get_check(format(_("すべてのアイテムを ＄%d で再充填しますか？", "Recharge all items for %d gold? "),  total_cost))) return;
	
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		k_ptr = &k_info[o_ptr->k_idx];

		/* skip non magic device */
		if (o_ptr->tval < TV_STAFF || o_ptr->tval > TV_ROD) continue;

		if (!object_is_known(o_ptr))
		{
			identify_item(p_ptr, o_ptr);

			/* Auto-inscription */
			autopick_alter_item(i, FALSE);
		}

		/* Recharge */
		switch (o_ptr->tval)
		{
		case TV_ROD:
			o_ptr->timeout = 0;
			break;
		case TV_STAFF:
			if (o_ptr->pval < k_ptr->pval) o_ptr->pval = k_ptr->pval;
			/* We no longer think the item is empty */
			o_ptr->ident &= ~(IDENT_EMPTY);
			break;
		case TV_WAND:
			if (o_ptr->pval < o_ptr->number * k_ptr->pval)
				o_ptr->pval = o_ptr->number * k_ptr->pval;
			/* We no longer think the item is empty */
			o_ptr->ident &= ~(IDENT_EMPTY);
			break;
		}
	}

	/* Give feedback */
	msg_format(_("＄%d で再充填しました。", "You pay %d gold."), total_cost);
	msg_print(NULL);
	p_ptr->update |= (PU_COMBINE | PU_REORDER);

	p_ptr->window |= (PW_INVEN);

	/* Pay the price */
	p_ptr->au -= total_cost;

	/* Finished */
	return;
}

/*!
 * @brief 施設でモンスターの情報を知るメインルーチン / research_mon -KMW-
 * @return 常にTRUEを返す。
 * @todo 返り値が意味不明なので直した方が良いかもしれない。
 */
static bool research_mon(void)
{
	IDX i;
	int n;
	MONRACE_IDX r_idx;
	char sym, query;
	char buf[128];
	bool notpicked;
	bool recall = FALSE;
	u16b why = 0;
	MONSTER_IDX *who;

	/* XTRA HACK WHATSEARCH */
	bool all = FALSE;
	bool uniq = FALSE;
	bool norm = FALSE;
	char temp[80] = "";

	/* XTRA HACK REMEMBER_IDX */
	static int old_sym = '\0';
	static IDX old_i = 0;

	screen_save();

	/* Get a character, or abort */
	if (!get_com(_("モンスターの文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前):",
				   "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "), &sym, FALSE)) 

	{
		screen_load();

		return (FALSE);
	}

	/* Find that character info, and describe it */
	for (i = 0; ident_info[i]; ++i)
	{
		if (sym == ident_info[i][0]) break;
	}

		/* XTRA HACK WHATSEARCH */
	if (sym == KTRL('A'))
	{
		all = TRUE;
		strcpy(buf, _("全モンスターのリスト", "Full monster list."));
	}
	else if (sym == KTRL('U'))
	{
		all = uniq = TRUE;
		strcpy(buf, _("ユニーク・モンスターのリスト", "Unique monster list."));
	}
	else if (sym == KTRL('N'))
	{
		all = norm = TRUE;
		strcpy(buf, _("ユニーク外モンスターのリスト", "Non-unique monster list."));
	}
	else if (sym == KTRL('M'))
	{
		all = TRUE;
		if (!get_string(_("名前(英語の場合小文字で可)", "Enter name:"),temp, 70))
		{
			temp[0]=0;
			screen_load();

			return FALSE;
		}
		sprintf(buf, _("名前:%sにマッチ", "Monsters with a name \"%s\""),temp);
	}
	else if (ident_info[i])
	{
		sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
	}
	else
	{
		sprintf(buf, "%c - %s", sym, _("無効な文字", "Unknown Symbol"));
	}

	/* Display the result */
	prt(buf, 16, 10);

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, MONRACE_IDX);

	/* Collect matching monsters */
	for (n = 0, i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Empty monster */
		if (!r_ptr->name) continue;

		/* XTRA HACK WHATSEARCH */
		/* Require non-unique monsters if needed */
		if (norm && (r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* Require unique monsters if needed */
		if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE))) continue;

		/* 名前検索 */
		if (temp[0])
		{
			int xx;
			char temp2[80];

			for (xx = 0; temp[xx] && xx < 80; xx++)
			{
#ifdef JP
				if (iskanji(temp[xx]))
				{
					xx++;
					continue;
				}
#endif
				if (isupper(temp[xx])) temp[xx] = (char)tolower(temp[xx]);
			}
  
#ifdef JP
			strcpy(temp2, r_name + r_ptr->E_name);
#else
			strcpy(temp2, r_name + r_ptr->name);
#endif
			for (xx = 0; temp2[xx] && xx < 80; xx++)
				if (isupper(temp2[xx])) temp2[xx] = (char)tolower(temp2[xx]);

#ifdef JP
			if (my_strstr(temp2, temp) || my_strstr(r_name + r_ptr->name, temp))
#else
			if (my_strstr(temp2, temp))
#endif
				who[n++] = i;
		}
		else if (all || (r_ptr->d_char == sym)) who[n++] = i;
	}

	/* Nothing to recall */
	if (!n)
	{
		/* Free the "who" array */
		C_KILL(who, max_r_idx, MONRACE_IDX);
		screen_load();

		return (FALSE);
	}

	/* Sort by level */
	why = 2;
	query = 'y';

	/* Sort if needed */
	if (why)
	{
		ang_sort(who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
	}


	/* Start at the end */
	/* XTRA HACK REMEMBER_IDX */
	if (old_sym == sym && old_i < n) i = old_i;
	else i = n - 1;

	notpicked = TRUE;

	/* Scan the monster memory */
	while (notpicked)
	{
		r_idx = who[i];

		/* Hack -- Begin the prompt */
		roff_top(r_idx);

		/* Hack -- Complete the prompt */
		Term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ' 'で続行, ESC]", " [(r)ecall, ESC, space to continue]"));

		/* Interact */
		while (1)
		{
			if (recall)
			{
				/*** Recall on screen ***/

				/* Get maximal info about this monster */
				lore_do_probe(r_idx);

				/* Save this monster ID */
				monster_race_track(r_idx);
				handle_stuff();

				/* know every thing mode */
				screen_roff(r_idx, 0x01);
				notpicked = FALSE;

				/* XTRA HACK REMEMBER_IDX */
				old_sym = sym;
				old_i = i;
			}

			query = inkey();

			/* Normal commands */
			if (query != 'r') break;

			recall = !recall;
		}

		/* Stop scanning */
		if (query == ESCAPE) break;

		/* Move to "prev" monster */
		if (query == '-')
		{
			if (++i == n)
			{
				i = 0;
				if (!expand_list) break;
			}
		}

		/* Move to "next" monster */
		else
		{
			if (i-- == 0)
			{
				i = n - 1;
				if (!expand_list) break;
			}
		}
	}


	/* Re-display the identity */
	/* prt(buf, 5, 5);*/

	/* Free the "who" array */
	C_KILL(who, max_r_idx, MONRACE_IDX);
	screen_load();

	return (!notpicked);
}


/*!
 * @brief 施設の処理実行メインルーチン / Execute a building command
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @param i 実行したい施設のサービステーブルの添字
 * @return なし
 */
static void bldg_process_command(player_type *player_ptr, building_type *bldg, int i)
{
	BACT_IDX bact = bldg->actions[i];
	PRICE bcost;
	bool paid = FALSE;

	msg_flag = FALSE;
	msg_erase();

	if (is_owner(bldg))
		bcost = bldg->member_costs[i];
	else
		bcost = bldg->other_costs[i];

	/* action restrictions */
	if (((bldg->action_restr[i] == 1) && !is_member(bldg)) ||
	    ((bldg->action_restr[i] == 2) && !is_owner(bldg)))
	{
		msg_print(_("それを選択する権利はありません！", "You have no right to choose that!"));
		return;
	}

	/* check gold (HACK - Recharge uses variable costs) */
	if ((bact != BACT_RECHARGE) &&
	    (((bldg->member_costs[i] > player_ptr->au) && is_owner(bldg)) ||
	     ((bldg->other_costs[i] > player_ptr->au) && !is_owner(bldg))))
	{
		msg_print(_("お金が足りません！", "You do not have the gold!"));
		return;
	}

	switch (bact)
	{
	case BACT_NOTHING:
		/* Do nothing */
		break;
	case BACT_RESEARCH_ITEM:
		paid = identify_fully(player_ptr, FALSE);
		break;
	case BACT_TOWN_HISTORY:
		town_history();
		break;
	case BACT_RACE_LEGENDS:
		race_legends(p_ptr);
		break;
	case BACT_QUEST:
		castle_quest();
		break;
	case BACT_KING_LEGENDS:
	case BACT_ARENA_LEGENDS:
	case BACT_LEGENDS:
		show_highclass(player_ptr);
		break;
	case BACT_POSTER:
	case BACT_ARENA_RULES:
	case BACT_ARENA:
		arena_comm(player_ptr, bact);
		break;
	case BACT_IN_BETWEEN:
	case BACT_CRAPS:
	case BACT_SPIN_WHEEL:
	case BACT_DICE_SLOTS:
	case BACT_GAMBLE_RULES:
	case BACT_POKER:
		gamble_comm(bact);
		break;
	case BACT_REST:
	case BACT_RUMORS:
	case BACT_FOOD:
		paid = inn_comm(player_ptr, bact);
		break;
	case BACT_RESEARCH_MONSTER:
		paid = research_mon();
		break;
	case BACT_COMPARE_WEAPONS:
		paid = TRUE;
		bcost = compare_weapons(player_ptr, bcost);
		break;
	case BACT_ENCHANT_WEAPON:
		item_tester_hook = object_allow_enchant_melee_weapon;
		enchant_item(player_ptr, bcost, 1, 1, 0);
		break;
	case BACT_ENCHANT_ARMOR:
		item_tester_hook = object_is_armour;
		enchant_item(player_ptr, bcost, 0, 0, 1);
		break;
	case BACT_RECHARGE:
		building_recharge();
		break;
	case BACT_RECHARGE_ALL:
		building_recharge_all();
		break;
	case BACT_IDENTS: /* needs work */
		if (!get_check(_("持ち物を全て鑑定してよろしいですか？", "Do you pay for identify all your possession? "))) break;
		identify_pack(player_ptr);
		msg_print(_(" 持ち物全てが鑑定されました。", "Your possessions have been identified."));
		paid = TRUE;
		break;
	case BACT_IDENT_ONE: /* needs work */
		paid = ident_spell(player_ptr, FALSE);
		break;
	case BACT_LEARN:
		do_cmd_study(player_ptr);
		break;
	case BACT_HEALING: /* needs work */
		paid = cure_critical_wounds(player_ptr, 200);
		break;
	case BACT_RESTORE: /* needs work */
		paid = restore_all_status(player_ptr);
		break;
	case BACT_ENCHANT_ARROWS:
		item_tester_hook = item_tester_hook_ammo;
		enchant_item(player_ptr, bcost, 1, 1, 0);
		break;
	case BACT_ENCHANT_BOW:
		item_tester_tval = TV_BOW;
		enchant_item(player_ptr, bcost, 1, 1, 0);
		break;

	case BACT_RECALL:
		if (recall_player(player_ptr, 1)) paid = TRUE;
		break;

	case BACT_TELEPORT_LEVEL:
		clear_bldg(4, 20);
		paid = free_level_recall(player_ptr);
		break;

	case BACT_LOSE_MUTATION:
		if (player_ptr->muta1 || player_ptr->muta2 || (player_ptr->muta3 & ~MUT3_GOOD_LUCK) ||
			(player_ptr->pseikaku != SEIKAKU_LUCKY && (player_ptr->muta3 & MUT3_GOOD_LUCK)))
		{
			while(!lose_mutation(player_ptr, 0));
			paid = TRUE;
		}
		else
		{
			msg_print(_("治すべき突然変異が無い。", "You have no mutations."));
			msg_print(NULL);
		}
		break;

	case BACT_BATTLE:
		kakutoujou(player_ptr);
		break;

	case BACT_TSUCHINOKO:
		tsuchinoko();
		break;

	case BACT_BOUNTY:
		show_bounty();
		break;

	case BACT_TARGET:
		today_target();
		break;

	case BACT_KANKIN:
		kankin();
		break;

	case BACT_HEIKOUKA:
		msg_print(_("平衡化の儀式を行なった。", "You received an equalization ritual."));
		set_virtue(player_ptr, V_COMPASSION, 0);
		set_virtue(player_ptr, V_HONOUR, 0);
		set_virtue(player_ptr, V_JUSTICE, 0);
		set_virtue(player_ptr, V_SACRIFICE, 0);
		set_virtue(player_ptr, V_KNOWLEDGE, 0);
		set_virtue(player_ptr, V_FAITH, 0);
		set_virtue(player_ptr, V_ENLIGHTEN, 0);
		set_virtue(player_ptr, V_ENCHANT, 0);
		set_virtue(player_ptr, V_CHANCE, 0);
		set_virtue(player_ptr, V_NATURE, 0);
		set_virtue(player_ptr, V_HARMONY, 0);
		set_virtue(player_ptr, V_VITALITY, 0);
		set_virtue(player_ptr, V_UNLIFE, 0);
		set_virtue(player_ptr, V_PATIENCE, 0);
		set_virtue(player_ptr, V_TEMPERANCE, 0);
		set_virtue(player_ptr, V_DILIGENCE, 0);
		set_virtue(player_ptr, V_VALOUR, 0);
		set_virtue(player_ptr, V_INDIVIDUALISM, 0);
		get_virtues(player_ptr);
		paid = TRUE;
		break;

	case BACT_TELE_TOWN:
		paid = tele_town(player_ptr);
		break;

	case BACT_EVAL_AC:
		paid = eval_ac(player_ptr->dis_ac + player_ptr->dis_to_a);
		break;

	case BACT_BROKEN_WEAPON:
		paid = TRUE;
		bcost = repair_broken_weapon(bcost);
		break;
	}

	if (paid) player_ptr->au -= bcost;
}

/*!
 * @brief 施設入り口にプレイヤーが乗った際の処理 / Do building commands
 * @param プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_bldg(player_type *player_ptr)
{
	int             i, which;
	char            command;
	bool            validcmd;
	building_type   *bldg;

	if(player_ptr->wild_mode) return;

	take_turn(player_ptr, 100);

	if (!cave_have_flag_bold(player_ptr->current_floor_ptr, player_ptr->y, player_ptr->x, FF_BLDG))
	{
		msg_print(_("ここには建物はない。", "You see no building here."));
		return;
	}

	which = f_info[player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].feat].subtype;

	bldg = &building[which];

	/* Don't re-init the wilderness */
	reinit_wilderness = FALSE;

	if ((which == 2) && (player_ptr->arena_number < 0))
	{
		msg_print(_("「敗者に用はない。」", "'There's no place here for a LOSER like you!'"));
		return;
	}
	else if ((which == 2) && player_ptr->current_floor_ptr->inside_arena)
	{
		if (!player_ptr->exit_bldg && player_ptr->current_floor_ptr->m_cnt > 0)
		{
			prt(_("ゲートは閉まっている。モンスターがあなたを待っている！", "The gates are closed.  The monster awaits!"), 0, 0);
		}
		else
		{
			/* Don't save the arena as saved floor */
			prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_NO_RETURN);

			player_ptr->current_floor_ptr->inside_arena = FALSE;
			player_ptr->leaving = TRUE;

			/* Re-enter the arena */
			command_new = SPECIAL_KEY_BUILDING;

			/* No energy needed to re-enter the arena */
			free_turn(player_ptr);
		}

		return;
	}
	else if (player_ptr->phase_out)
	{
		/* Don't save the arena as saved floor */
		prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_NO_RETURN);

		player_ptr->leaving = TRUE;
		player_ptr->phase_out = FALSE;

		/* Re-enter the monster arena */
		command_new = SPECIAL_KEY_BUILDING;

		/* No energy needed to re-enter the arena */
		free_turn(player_ptr);

		return;
	}
	else
	{
		player_ptr->oldpy = player_ptr->y;
		player_ptr->oldpx = player_ptr->x;
	}

	forget_lite(player_ptr->current_floor_ptr);
	forget_view(player_ptr->current_floor_ptr);

	/* Hack -- Increase "icky" depth */
	current_world_ptr->character_icky++;

	command_arg = 0;
	command_rep = 0;
	command_new = 0;

	show_building(bldg);
	player_ptr->leave_bldg = FALSE;

	play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);

	while (!player_ptr->leave_bldg)
	{
		validcmd = FALSE;
		prt("", 1, 0);

		building_prt_gold();

		command = inkey();

		if (command == ESCAPE)
		{
			player_ptr->leave_bldg = TRUE;
			player_ptr->current_floor_ptr->inside_arena = FALSE;
			player_ptr->phase_out = FALSE;
			break;
		}

		for (i = 0; i < 8; i++)
		{
			if (bldg->letters[i])
			{
				if (bldg->letters[i] == command)
				{
					validcmd = TRUE;
					break;
				}
			}
		}

		if(validcmd) bldg_process_command(player_ptr, bldg, i);

		handle_stuff();
	}

	select_floor_music(player_ptr);

	msg_flag = FALSE;
	msg_erase();

	/* Reinit wilderness to activate quests ... */
	if (reinit_wilderness) player_ptr->leaving = TRUE;

	/* Hack -- Decrease "icky" depth */
	current_world_ptr->character_icky--;

	Term_clear();

	player_ptr->update |= (PU_VIEW | PU_MONSTERS | PU_BONUS | PU_LITE | PU_MON_LITE);
	player_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_EQUIPPY | PR_MAP);
	player_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}


/*!
 * @brief 今日の賞金首を確定する / Determine today's bounty monster
 * @return なし
 * @note conv_old is used if loaded 0.0.3 or older save file
 */
void determine_today_mon(bool conv_old)
{
	int max_dl = 3, i;
	bool old_inside_battle = p_ptr->phase_out;
	monster_race *r_ptr;

	if (!conv_old)
	{
		for (i = 0; i < current_world_ptr->max_d_idx; i++)
		{
			if (max_dlv[i] < d_info[i].mindepth) continue;
			if (max_dl < max_dlv[i]) max_dl = max_dlv[i];
		}
	}
	else max_dl = MAX(max_dlv[DUNGEON_ANGBAND], 3);

	p_ptr->phase_out = TRUE;
	get_mon_num_prep(NULL, NULL);

	while (1)
	{
		today_mon = get_mon_num(max_dl);
		r_ptr = &r_info[today_mon];

		if (r_ptr->flags1 & RF1_UNIQUE) continue;
		if (r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)) continue;
		if (r_ptr->flags2 & RF2_MULTIPLY) continue;
		if ((r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) != (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) continue;
		if (r_ptr->level < MIN(max_dl / 2, 40)) continue;
		if (r_ptr->rarity > 10) continue;
		break;
	}

	p_ptr->today_mon = 0;
	p_ptr->phase_out = old_inside_battle;
}



/*!
 * @brief 賞金首となるユニークを確定する / Determine bounty uniques
 * @return なし
 */
void determine_bounty_uniques(void)
{
	int i, j;
	MONRACE_IDX tmp;
	monster_race *r_ptr;

	get_mon_num_prep(NULL, NULL);
	for (i = 0; i < MAX_BOUNTY; i++)
	{
		while (1)
		{
			current_world_ptr->bounty_r_idx[i] = get_mon_num(MAX_DEPTH - 1);
			r_ptr = &r_info[current_world_ptr->bounty_r_idx[i]];

			if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;

			if (!(r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON))) continue;

			if (r_ptr->rarity > 100) continue;

			if (no_questor_or_bounty_uniques(current_world_ptr->bounty_r_idx[i])) continue;

			for (j = 0; j < i; j++)
				if (current_world_ptr->bounty_r_idx[i] == current_world_ptr->bounty_r_idx[j]) break;

			if (j == i) break;
		}
	}

	/* Sort them */
	for (i = 0; i < MAX_BOUNTY - 1; i++)
	{
		for (j = i; j < MAX_BOUNTY; j++)
		{
			if (r_info[current_world_ptr->bounty_r_idx[i]].level > r_info[current_world_ptr->bounty_r_idx[j]].level)
			{
				tmp = current_world_ptr->bounty_r_idx[i];
				current_world_ptr->bounty_r_idx[i] = current_world_ptr->bounty_r_idx[j];
				current_world_ptr->bounty_r_idx[j] = tmp;
			}
		}
	}
}
