/*!
 * todo 1つ300行近い関数なので後ほど要分割
 * @brief 自動拾いの記述
 * @date 2020/04/25
 * @author Hourier
 */

#include "angband.h"
#include "autopick/autopick-util.h"
#include "autopick/autopick-describer.h"
#include "autopick/autopick-key-flag-process.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-methods-table.h"

#if JP
static void describe_autpick_jp(char *buff, autopick_type *entry)
{
	concptr str = entry->name;
	byte act = entry->action;
	concptr insc = entry->insc;
	int i;
	bool top = FALSE;
	concptr before_str[100];
	int before_n = 0;

	concptr body_str = "アイテム";
	if (IS_FLG(FLG_COLLECTING))
		before_str[before_n++] = "収集中で既に持っているスロットにまとめられる";

	if (IS_FLG(FLG_UNAWARE))
		before_str[before_n++] = "未鑑定でその効果も判明していない";

	if (IS_FLG(FLG_UNIDENTIFIED))
		before_str[before_n++] = "未鑑定の";

	if (IS_FLG(FLG_IDENTIFIED))
		before_str[before_n++] = "鑑定済みの";

	if (IS_FLG(FLG_STAR_IDENTIFIED))
		before_str[before_n++] = "完全に鑑定済みの";

	if (IS_FLG(FLG_BOOSTED))
	{
		before_str[before_n++] = "ダメージダイスが通常より大きい";
		body_str = "武器";
	}

	if (IS_FLG(FLG_MORE_DICE))
	{
		static char more_than_desc_str[] = "___";
		before_str[before_n++] = "ダメージダイスの最大値が";
		body_str = "武器";

		sprintf(more_than_desc_str, "%d", entry->dice);
		before_str[before_n++] = more_than_desc_str;
		before_str[before_n++] = "以上の";
	}

	if (IS_FLG(FLG_MORE_BONUS))
	{
		static char more_bonus_desc_str[] = "___";
		before_str[before_n++] = "修正値が(+";

		sprintf(more_bonus_desc_str, "%d", entry->bonus);
		before_str[before_n++] = more_bonus_desc_str;
		before_str[before_n++] = ")以上の";
	}

	if (IS_FLG(FLG_WORTHLESS))
		before_str[before_n++] = "店で無価値と判定される";

	if (IS_FLG(FLG_ARTIFACT))
	{
		before_str[before_n++] = "アーティファクトの";
		body_str = "装備";
	}

	if (IS_FLG(FLG_EGO))
	{
		before_str[before_n++] = "エゴアイテムの";
		body_str = "装備";
	}

	if (IS_FLG(FLG_GOOD))
	{
		before_str[before_n++] = "上質の";
		body_str = "装備";
	}

	if (IS_FLG(FLG_NAMELESS))
	{
		before_str[before_n++] = "エゴでもアーティファクトでもない";
		body_str = "装備";
	}

	if (IS_FLG(FLG_AVERAGE))
	{
		before_str[before_n++] = "並の";
		body_str = "装備";
	}

	if (IS_FLG(FLG_RARE))
	{
		before_str[before_n++] = "ドラゴン装備やカオス・ブレード等を含む珍しい";
		body_str = "装備";
	}

	if (IS_FLG(FLG_COMMON))
	{
		before_str[before_n++] = "ありふれた(ドラゴン装備やカオス・ブレード等の珍しい物ではない)";
		body_str = "装備";
	}

	if (IS_FLG(FLG_WANTED))
	{
		before_str[before_n++] = "ハンター事務所で賞金首とされている";
		body_str = "死体や骨";
	}

	if (IS_FLG(FLG_HUMAN))
	{
		before_str[before_n++] = "悪魔魔法で使うための人間やヒューマノイドの";
		body_str = "死体や骨";
	}

	if (IS_FLG(FLG_UNIQUE))
	{
		before_str[before_n++] = "ユニークモンスターの";
		body_str = "死体や骨";
	}

	if (IS_FLG(FLG_UNREADABLE))
	{
		before_str[before_n++] = "あなたが読めない領域の";
		body_str = "魔法書";
	}

	if (IS_FLG(FLG_REALM1))
	{
		before_str[before_n++] = "第一領域の";
		body_str = "魔法書";
	}

	if (IS_FLG(FLG_REALM2))
	{
		before_str[before_n++] = "第二領域の";
		body_str = "魔法書";
	}

	if (IS_FLG(FLG_FIRST))
	{
		before_str[before_n++] = "全4冊の内の1冊目の";
		body_str = "魔法書";
	}

	if (IS_FLG(FLG_SECOND))
	{
		before_str[before_n++] = "全4冊の内の2冊目の";
		body_str = "魔法書";
	}

	if (IS_FLG(FLG_THIRD))
	{
		before_str[before_n++] = "全4冊の内の3冊目の";
		body_str = "魔法書";
	}

	if (IS_FLG(FLG_FOURTH))
	{
		before_str[before_n++] = "全4冊の内の4冊目の";
		body_str = "魔法書";
	}

	if (IS_FLG(FLG_ITEMS))
		; /* Nothing to do */
	else if (IS_FLG(FLG_WEAPONS))
		body_str = "武器";
	else if (IS_FLG(FLG_FAVORITE_WEAPONS))
		body_str = "得意武器";
	else if (IS_FLG(FLG_ARMORS))
		body_str = "防具";
	else if (IS_FLG(FLG_MISSILES))
		body_str = "弾や矢やクロスボウの矢";
	else if (IS_FLG(FLG_DEVICES))
		body_str = "巻物や魔法棒や杖やロッド";
	else if (IS_FLG(FLG_LIGHTS))
		body_str = "光源用のアイテム";
	else if (IS_FLG(FLG_JUNKS))
		body_str = "折れた棒等のガラクタ";
	else if (IS_FLG(FLG_CORPSES))
		body_str = "死体や骨";
	else if (IS_FLG(FLG_SPELLBOOKS))
		body_str = "魔法書";
	else if (IS_FLG(FLG_HAFTED))
		body_str = "鈍器";
	else if (IS_FLG(FLG_SHIELDS))
		body_str = "盾";
	else if (IS_FLG(FLG_BOWS))
		body_str = "スリングや弓やクロスボウ";
	else if (IS_FLG(FLG_RINGS))
		body_str = "指輪";
	else if (IS_FLG(FLG_AMULETS))
		body_str = "アミュレット";
	else if (IS_FLG(FLG_SUITS))
		body_str = "鎧";
	else if (IS_FLG(FLG_CLOAKS))
		body_str = "クローク";
	else if (IS_FLG(FLG_HELMS))
		body_str = "ヘルメットや冠";
	else if (IS_FLG(FLG_GLOVES))
		body_str = "籠手";
	else if (IS_FLG(FLG_BOOTS))
		body_str = "ブーツ";

	*buff = '\0';
	if (!before_n)
		strcat(buff, "全ての");
	else for (i = 0; i < before_n && before_str[i]; i++)
		strcat(buff, before_str[i]);

	strcat(buff, body_str);

	if (*str)
	{
		if (*str == '^')
		{
			str++;
			top = TRUE;
		}

		strcat(buff, "で、名前が「");
		strncat(buff, str, 80);
		if (top)
			strcat(buff, "」で始まるもの");
		else
			strcat(buff, "」を含むもの");
	}

	if (insc)
	{
		strncat(buff, format("に「%s」", insc), 80);

		if (my_strstr(insc, "%%all"))
			strcat(buff, "(%%allは全能力を表す英字の記号で置換)");
		else if (my_strstr(insc, "%all"))
			strcat(buff, "(%allは全能力を表す記号で置換)");
		else if (my_strstr(insc, "%%"))
			strcat(buff, "(%%は追加能力を表す英字の記号で置換)");
		else if (my_strstr(insc, "%"))
			strcat(buff, "(%は追加能力を表す記号で置換)");

		strcat(buff, "と刻んで");
	}
	else
		strcat(buff, "を");

	if (act & DONT_AUTOPICK)
		strcat(buff, "放置する。");
	else if (act & DO_AUTODESTROY)
		strcat(buff, "破壊する。");
	else if (act & DO_QUERY_AUTOPICK)
		strcat(buff, "確認の後に拾う。");
	else
		strcat(buff, "拾う。");

	if (act & DO_DISPLAY)
	{
		if (act & DONT_AUTOPICK)
			strcat(buff, "全体マップ('M')で'N'を押したときに表示する。");
		else if (act & DO_AUTODESTROY)
			strcat(buff, "全体マップ('M')で'K'を押したときに表示する。");
		else
			strcat(buff, "全体マップ('M')で'M'を押したときに表示する。");
	}
	else
		strcat(buff, "全体マップには表示しない。");
}
#else


void describe_autopick_en(char *buff, autopick_type *entry)
{
	concptr str = entry->name;
	byte act = entry->action;
	concptr insc = entry->insc;
	bool top = FALSE;
	concptr before_str[20], after_str[20], which_str[20], whose_str[20];
	int before_n = 0, after_n = 0, which_n = 0, whose_n = 0;
	concptr body_str = "items";
	if (IS_FLG(FLG_COLLECTING))
		which_str[which_n++] = "can be absorbed into an existing inventory list slot";

	if (IS_FLG(FLG_UNAWARE))
	{
		before_str[before_n++] = "unidentified";
		whose_str[whose_n++] = "basic abilities are not known";
	}

	if (IS_FLG(FLG_UNIDENTIFIED))
		before_str[before_n++] = "unidentified";

	if (IS_FLG(FLG_IDENTIFIED))
		before_str[before_n++] = "identified";

	if (IS_FLG(FLG_STAR_IDENTIFIED))
		before_str[before_n++] = "fully identified";

	if (IS_FLG(FLG_RARE))
	{
		before_str[before_n++] = "very rare";
		body_str = "equipments";
		after_str[after_n++] = "such as Dragon armor, Blades of Chaos, etc.";
	}

	if (IS_FLG(FLG_COMMON))
	{
		before_str[before_n++] = "relatively common";
		body_str = "equipments";
		after_str[after_n++] = "compared to very rare Dragon armor, Blades of Chaos, etc.";
	}

	if (IS_FLG(FLG_WORTHLESS))
	{
		before_str[before_n++] = "worthless";
		which_str[which_n++] = "can not be sold at stores";
	}

	if (IS_FLG(FLG_ARTIFACT))
	{
		before_str[before_n++] = "artifact";
	}

	if (IS_FLG(FLG_EGO))
	{
		before_str[before_n++] = "ego";
	}

	if (IS_FLG(FLG_GOOD))
	{
		body_str = "equipment";
		which_str[which_n++] = "have good quality";
	}

	if (IS_FLG(FLG_NAMELESS))
	{
		body_str = "equipment";
		which_str[which_n++] = "is neither ego-item nor artifact";
	}

	if (IS_FLG(FLG_AVERAGE))
	{
		body_str = "equipment";
		which_str[which_n++] = "have average quality";
	}

	if (IS_FLG(FLG_BOOSTED))
	{
		body_str = "weapons";
		whose_str[whose_n++] = "damage dice is bigger than normal";
	}

	if (IS_FLG(FLG_MORE_DICE))
	{
		static char more_than_desc_str[] =
			"maximum damage from dice is bigger than __";
		body_str = "weapons";

		sprintf(more_than_desc_str + sizeof(more_than_desc_str) - 3,
			"%d", entry->dice);
		whose_str[whose_n++] = more_than_desc_str;
	}

	if (IS_FLG(FLG_MORE_BONUS))
	{
		static char more_bonus_desc_str[] =
			"magical bonus is bigger than (+__)";

		sprintf(more_bonus_desc_str + sizeof(more_bonus_desc_str) - 4,
			"%d)", entry->bonus);
		whose_str[whose_n++] = more_bonus_desc_str;
	}

	if (IS_FLG(FLG_WANTED))
	{
		body_str = "corpse or skeletons";
		which_str[which_n++] = "is wanted at the Hunter's Office";
	}

	if (IS_FLG(FLG_HUMAN))
	{
		before_str[before_n++] = "humanoid";
		body_str = "corpse or skeletons";
		which_str[which_n++] = "can be used for Daemon magic";
	}

	if (IS_FLG(FLG_UNIQUE))
	{
		before_str[before_n++] = "unique monster's";
		body_str = "corpse or skeletons";
	}

	if (IS_FLG(FLG_UNREADABLE))
	{
		body_str = "spellbooks";
		after_str[after_n++] = "of different realms from yours";
	}

	if (IS_FLG(FLG_REALM1))
	{
		body_str = "spellbooks";
		after_str[after_n++] = "of your first realm";
	}

	if (IS_FLG(FLG_REALM2))
	{
		body_str = "spellbooks";
		after_str[after_n++] = "of your second realm";
	}

	if (IS_FLG(FLG_FIRST))
	{
		before_str[before_n++] = "first one of four";
		body_str = "spellbooks";
	}

	if (IS_FLG(FLG_SECOND))
	{
		before_str[before_n++] = "second one of four";
		body_str = "spellbooks";
	}

	if (IS_FLG(FLG_THIRD))
	{
		before_str[before_n++] = "third one of four";
		body_str = "spellbooks";
	}

	if (IS_FLG(FLG_FOURTH))
	{
		before_str[before_n++] = "fourth one of four";
		body_str = "spellbooks";
	}

	if (IS_FLG(FLG_ITEMS))
		; /* Nothing to do */
	else if (IS_FLG(FLG_WEAPONS))
		body_str = "weapons";
	else if (IS_FLG(FLG_FAVORITE_WEAPONS))
		body_str = "favorite weapons";
	else if (IS_FLG(FLG_ARMORS))
		body_str = "armors";
	else if (IS_FLG(FLG_MISSILES))
		body_str = "shots, arrows or crossbow bolts";
	else if (IS_FLG(FLG_DEVICES))
		body_str = "scrolls, wands, staffs or rods";
	else if (IS_FLG(FLG_LIGHTS))
		body_str = "light sources";
	else if (IS_FLG(FLG_JUNKS))
		body_str = "junk such as broken sticks";
	else if (IS_FLG(FLG_CORPSES))
		body_str = "corpses or skeletons";
	else if (IS_FLG(FLG_SPELLBOOKS))
		body_str = "spellbooks";
	else if (IS_FLG(FLG_HAFTED))
		body_str = "hafted weapons";
	else if (IS_FLG(FLG_SHIELDS))
		body_str = "shields";
	else if (IS_FLG(FLG_BOWS))
		body_str = "slings, bows or crossbows";
	else if (IS_FLG(FLG_RINGS))
		body_str = "rings";
	else if (IS_FLG(FLG_AMULETS))
		body_str = "amulets";
	else if (IS_FLG(FLG_SUITS))
		body_str = "body armors";
	else if (IS_FLG(FLG_CLOAKS))
		body_str = "cloaks";
	else if (IS_FLG(FLG_HELMS))
		body_str = "helms or crowns";
	else if (IS_FLG(FLG_GLOVES))
		body_str = "gloves";
	else if (IS_FLG(FLG_BOOTS))
		body_str = "boots";

	if (*str)
	{
		if (*str == '^')
		{
			str++;
			top = TRUE;
			whose_str[whose_n++] = "name begins with \"";
		}
		else
			which_str[which_n++] = "have \"";
	}


	if (act & DONT_AUTOPICK)
		strcpy(buff, "Leave on floor ");
	else if (act & DO_AUTODESTROY)
		strcpy(buff, "Destroy ");
	else if (act & DO_QUERY_AUTOPICK)
		strcpy(buff, "Ask to pick up ");
	else
		strcpy(buff, "Pickup ");

	if (insc)
	{
		strncat(buff, format("and inscribe \"%s\"", insc), 80);

		if (my_strstr(insc, "%all"))
			strcat(buff, ", replacing %all with code string representing all abilities,");
		else if (my_strstr(insc, "%"))
			strcat(buff, ", replacing % with code string representing extra random abilities,");

		strcat(buff, " on ");
	}

	if (!before_n)
		strcat(buff, "all ");
	else
		for (int i = 0; i < before_n && before_str[i]; i++)
		{
			strcat(buff, before_str[i]);
			strcat(buff, " ");
		}

	strcat(buff, body_str);

	for (int i = 0; i < after_n && after_str[i]; i++)
	{
		strcat(buff, " ");
		strcat(buff, after_str[i]);
	}

	for (int i = 0; i < whose_n && whose_str[i]; i++)
	{
		if (i == 0)
			strcat(buff, " whose ");
		else
			strcat(buff, ", and ");

		strcat(buff, whose_str[i]);
	}

	if (*str && top)
	{
		strcat(buff, str);
		strcat(buff, "\"");
	}

	if (whose_n && which_n)
		strcat(buff, ", and ");

	for (int i = 0; i < which_n && which_str[i]; i++)
	{
		if (i == 0)
			strcat(buff, " which ");
		else
			strcat(buff, ", and ");

		strcat(buff, which_str[i]);
	}

	if (*str && !top)
	{
		strncat(buff, str, 80);
		strcat(buff, "\" as part of its name");
	}
	strcat(buff, ".");

	if (act & DO_DISPLAY)
	{
		if (act & DONT_AUTOPICK)
			strcat(buff, "  Display these items when you press the N key in the full 'M'ap.");
		else if (act & DO_AUTODESTROY)
			strcat(buff, "  Display these items when you press the K key in the full 'M'ap.");
		else
			strcat(buff, "  Display these items when you press the M key in the full 'M'ap.");
	}
	else
		strcat(buff, " Not displayed in the full map.");
}
#endif


/*
 * Describe which kind of object is Auto-picked/destroyed
 */
void describe_autopick(char *buff, autopick_type *entry)
{
#ifdef JP
	describe_autpick_jp(buff, entry);
#else /* JP */
	describe_autopick_en(buff, entry);
#endif /* JP */
}
