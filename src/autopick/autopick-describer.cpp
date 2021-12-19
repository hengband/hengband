/*!
 * @brief 自動拾いの記述
 * @date 2020/04/25
 * @author Hourier
 * @todo 1つ300行近い関数なので後ほど要分割
 */

#include "autopick/autopick-describer.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-key-flag-process.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "system/angband.h"
#include "util/string-processor.h"

typedef struct {
    concptr str;
    byte act;
    concptr insc;
    bool top;
    int before_n;
    concptr body_str;
} autopick_describer;

#if JP
static void describe_autpick_jp(char *buff, autopick_type *entry, autopick_describer *describer)
{
    concptr before_str[100];
    if (IS_FLG(FLG_COLLECTING))
        before_str[describer->before_n++] = "収集中で既に持っているスロットにまとめられる";

    if (IS_FLG(FLG_UNAWARE))
        before_str[describer->before_n++] = "未鑑定でその効果も判明していない";

    if (IS_FLG(FLG_UNIDENTIFIED))
        before_str[describer->before_n++] = "未鑑定の";

    if (IS_FLG(FLG_IDENTIFIED))
        before_str[describer->before_n++] = "鑑定済みの";

    if (IS_FLG(FLG_STAR_IDENTIFIED))
        before_str[describer->before_n++] = "完全に鑑定済みの";

    if (IS_FLG(FLG_BOOSTED)) {
        before_str[describer->before_n++] = "ダメージダイスが通常より大きい";
        describer->body_str = "武器";
    }

    if (IS_FLG(FLG_MORE_DICE)) {
        static char more_than_desc_str[] = "___";
        before_str[describer->before_n++] = "ダメージダイスの最大値が";
        describer->body_str = "武器";

        snprintf(more_than_desc_str, sizeof(more_than_desc_str), "%d", entry->dice);
        before_str[describer->before_n++] = more_than_desc_str;
        before_str[describer->before_n++] = "以上の";
    }

    if (IS_FLG(FLG_MORE_BONUS)) {
        static char more_bonus_desc_str[] = "___";
        before_str[describer->before_n++] = "修正値が(+";

        snprintf(more_bonus_desc_str, sizeof(more_bonus_desc_str), "%d", entry->bonus);
        before_str[describer->before_n++] = more_bonus_desc_str;
        before_str[describer->before_n++] = ")以上の";
    }

    if (IS_FLG(FLG_WORTHLESS))
        before_str[describer->before_n++] = "店で無価値と判定される";

    if (IS_FLG(FLG_ARTIFACT)) {
        before_str[describer->before_n++] = "アーティファクトの";
        describer->body_str = "装備";
    }

    if (IS_FLG(FLG_EGO)) {
        before_str[describer->before_n++] = "エゴアイテムの";
        describer->body_str = "装備";
    }

    if (IS_FLG(FLG_GOOD)) {
        before_str[describer->before_n++] = "上質の";
        describer->body_str = "装備";
    }

    if (IS_FLG(FLG_NAMELESS)) {
        before_str[describer->before_n++] = "エゴでもアーティファクトでもない";
        describer->body_str = "装備";
    }

    if (IS_FLG(FLG_AVERAGE)) {
        before_str[describer->before_n++] = "並の";
        describer->body_str = "装備";
    }

    if (IS_FLG(FLG_RARE)) {
        before_str[describer->before_n++] = "ドラゴン装備やカオス・ブレード等を含む珍しい";
        describer->body_str = "装備";
    }

    if (IS_FLG(FLG_COMMON)) {
        before_str[describer->before_n++] = "ありふれた(ドラゴン装備やカオス・ブレード等の珍しい物ではない)";
        describer->body_str = "装備";
    }

    if (IS_FLG(FLG_WANTED)) {
        before_str[describer->before_n++] = "ハンター事務所で賞金首とされている";
        describer->body_str = "死体や骨";
    }

    if (IS_FLG(FLG_HUMAN)) {
        before_str[describer->before_n++] = "悪魔魔法で使うための人間やヒューマノイドの";
        describer->body_str = "死体や骨";
    }

    if (IS_FLG(FLG_UNIQUE)) {
        before_str[describer->before_n++] = "ユニークモンスターの";
        describer->body_str = "死体や骨";
    }

    if (IS_FLG(FLG_UNREADABLE)) {
        before_str[describer->before_n++] = "あなたが読めない領域の";
        describer->body_str = "魔法書";
    }

    if (IS_FLG(FLG_REALM1)) {
        before_str[describer->before_n++] = "第一領域の";
        describer->body_str = "魔法書";
    }

    if (IS_FLG(FLG_REALM2)) {
        before_str[describer->before_n++] = "第二領域の";
        describer->body_str = "魔法書";
    }

    if (IS_FLG(FLG_FIRST)) {
        before_str[describer->before_n++] = "全4冊の内の1冊目の";
        describer->body_str = "魔法書";
    }

    if (IS_FLG(FLG_SECOND)) {
        before_str[describer->before_n++] = "全4冊の内の2冊目の";
        describer->body_str = "魔法書";
    }

    if (IS_FLG(FLG_THIRD)) {
        before_str[describer->before_n++] = "全4冊の内の3冊目の";
        describer->body_str = "魔法書";
    }

    if (IS_FLG(FLG_FOURTH)) {
        before_str[describer->before_n++] = "全4冊の内の4冊目の";
        describer->body_str = "魔法書";
    }

    if (IS_FLG(FLG_ITEMS))
        ; /* Nothing to do */
    else if (IS_FLG(FLG_WEAPONS))
        describer->body_str = "武器";
    else if (IS_FLG(FLG_FAVORITE_WEAPONS))
        describer->body_str = "得意武器";
    else if (IS_FLG(FLG_ARMORS))
        describer->body_str = "防具";
    else if (IS_FLG(FLG_MISSILES))
        describer->body_str = "弾や矢やクロスボウの矢";
    else if (IS_FLG(FLG_DEVICES))
        describer->body_str = "巻物や魔法棒や杖やロッド";
    else if (IS_FLG(FLG_LIGHTS))
        describer->body_str = "光源用のアイテム";
    else if (IS_FLG(FLG_JUNKS))
        describer->body_str = "折れた棒等のガラクタ";
    else if (IS_FLG(FLG_CORPSES))
        describer->body_str = "死体や骨";
    else if (IS_FLG(FLG_SPELLBOOKS))
        describer->body_str = "魔法書";
    else if (IS_FLG(FLG_HAFTED))
        describer->body_str = "鈍器";
    else if (IS_FLG(FLG_SHIELDS))
        describer->body_str = "盾";
    else if (IS_FLG(FLG_BOWS))
        describer->body_str = "スリングや弓やクロスボウ";
    else if (IS_FLG(FLG_RINGS))
        describer->body_str = "指輪";
    else if (IS_FLG(FLG_AMULETS))
        describer->body_str = "アミュレット";
    else if (IS_FLG(FLG_SUITS))
        describer->body_str = "鎧";
    else if (IS_FLG(FLG_CLOAKS))
        describer->body_str = "クローク";
    else if (IS_FLG(FLG_HELMS))
        describer->body_str = "ヘルメットや冠";
    else if (IS_FLG(FLG_GLOVES))
        describer->body_str = "籠手";
    else if (IS_FLG(FLG_BOOTS))
        describer->body_str = "ブーツ";

    *buff = '\0';
    if (!describer->before_n)
        strcat(buff, "全ての");
    else
        for (int i = 0; i < describer->before_n && before_str[i]; i++)
            strcat(buff, before_str[i]);

    strcat(buff, describer->body_str);

    if (*describer->str) {
        if (*describer->str == '^') {
            describer->str++;
            describer->top = true;
        }

        strcat(buff, "で、名前が「");
        angband_strcat(buff, describer->str, (MAX_NLEN - MAX_INSCRIPTION));
        if (describer->top)
            strcat(buff, "」で始まるもの");
        else
            strcat(buff, "」を含むもの");
    }

    if (describer->insc) {
        char tmp[MAX_INSCRIPTION + 1] = "";
        angband_strcat(tmp, describer->insc, MAX_INSCRIPTION);
        angband_strcat(buff, format("に「%s」", tmp), MAX_INSCRIPTION + 6);

        if (angband_strstr(describer->insc, "%%all"))
            strcat(buff, "(%%allは全能力を表す英字の記号で置換)");
        else if (angband_strstr(describer->insc, "%all"))
            strcat(buff, "(%allは全能力を表す記号で置換)");
        else if (angband_strstr(describer->insc, "%%"))
            strcat(buff, "(%%は追加能力を表す英字の記号で置換)");
        else if (angband_strstr(describer->insc, "%"))
            strcat(buff, "(%は追加能力を表す記号で置換)");

        strcat(buff, "と刻んで");
    } else
        strcat(buff, "を");

    if (describer->act & DONT_AUTOPICK)
        strcat(buff, "放置する。");
    else if (describer->act & DO_AUTODESTROY)
        strcat(buff, "破壊する。");
    else if (describer->act & DO_QUERY_AUTOPICK)
        strcat(buff, "確認の後に拾う。");
    else
        strcat(buff, "拾う。");

    if (describer->act & DO_DISPLAY) {
        if (describer->act & DONT_AUTOPICK)
            strcat(buff, "全体マップ('M')で'N'を押したときに表示する。");
        else if (describer->act & DO_AUTODESTROY)
            strcat(buff, "全体マップ('M')で'K'を押したときに表示する。");
        else
            strcat(buff, "全体マップ('M')で'M'を押したときに表示する。");
    } else
        strcat(buff, "全体マップには表示しない。");
}
#else

void describe_autopick_en(char *buff, autopick_type *entry, autopick_describer *describer)
{
    concptr before_str[20], after_str[20], which_str[20], whose_str[20];
    concptr whose_arg_str[20];
    char arg_str[2][24];
    int after_n = 0, which_n = 0, whose_n = 0, arg_n = 0;
    if (IS_FLG(FLG_COLLECTING))
        which_str[which_n++] = "can be absorbed into an existing inventory list slot";

    if (IS_FLG(FLG_UNAWARE)) {
        before_str[describer->before_n++] = "unidentified";
        whose_str[whose_n] = "basic abilities are not known";
        whose_arg_str[whose_n] = "";
        ++whose_n;
    }

    if (IS_FLG(FLG_UNIDENTIFIED))
        before_str[describer->before_n++] = "unidentified";

    if (IS_FLG(FLG_IDENTIFIED))
        before_str[describer->before_n++] = "identified";

    if (IS_FLG(FLG_STAR_IDENTIFIED))
        before_str[describer->before_n++] = "fully identified";

    if (IS_FLG(FLG_RARE)) {
        before_str[describer->before_n++] = "very rare";
        after_str[after_n++] = "such as Dragon armor, Blades of Chaos, etc.";
    }

    if (IS_FLG(FLG_COMMON)) {
        before_str[describer->before_n++] = "relatively common";
        after_str[after_n++] = "compared to very rare Dragon armor, Blades of Chaos, etc.";
    }

    if (IS_FLG(FLG_WORTHLESS)) {
        before_str[describer->before_n++] = "worthless";
        which_str[which_n++] = "can not be sold at stores";
    }

    if (IS_FLG(FLG_ARTIFACT))
        before_str[describer->before_n++] = "artifact";

    if (IS_FLG(FLG_EGO))
        before_str[describer->before_n++] = "ego";

    if (IS_FLG(FLG_GOOD)) {
        which_str[which_n++] = "are of good quality";
    }

    if (IS_FLG(FLG_NAMELESS)) {
        which_str[which_n++] = "are neither ego items nor artifacts";
    }

    if (IS_FLG(FLG_AVERAGE)) {
        which_str[which_n++] = "are of average quality";
    }

    if (IS_FLG(FLG_BOOSTED)) {
        describer->body_str = "weapons";
        whose_str[whose_n] = "damage dice is bigger than normal";
        whose_arg_str[whose_n] = "";
        ++whose_n;
    }

    if (IS_FLG(FLG_MORE_DICE)) {
        describer->body_str = "weapons";
        whose_str[whose_n] = "maximum damage from dice is bigger than ";
        if (arg_n < (int)(sizeof(arg_str) / sizeof(arg_str[0]))) {
            snprintf(arg_str[arg_n], sizeof(arg_str[arg_n]), "%d", entry->dice);
            whose_arg_str[whose_n] = arg_str[arg_n];
            ++arg_n;
        } else {
            whose_arg_str[whose_n] = "garbled";
        }
        ++whose_n;
    }

    if (IS_FLG(FLG_MORE_BONUS)) {
        whose_str[whose_n] = "magical bonuses are bigger than (+";
        if (arg_n < (int)(sizeof(arg_str) / sizeof(arg_str[0]))) {
            snprintf(arg_str[arg_n], sizeof(arg_str[arg_n]), "%d)", entry->bonus);
            whose_arg_str[whose_n] = arg_str[arg_n];
            ++arg_n;
        } else {
            whose_arg_str[whose_n] = "garbled)";
        }
        ++whose_n;
    }

    if (IS_FLG(FLG_WANTED)) {
        describer->body_str = "corpses or skeletons";
        which_str[which_n++] = "are wanted at the Hunter's Office";
    }

    if (IS_FLG(FLG_HUMAN)) {
        before_str[describer->before_n++] = "humanoid";
        describer->body_str = "corpses or skeletons";
        which_str[which_n++] = "can be used for Daemon magic";
    }

    if (IS_FLG(FLG_UNIQUE)) {
        before_str[describer->before_n++] = "unique monsters'";
        describer->body_str = "corpses or skeletons";
    }

    if (IS_FLG(FLG_UNREADABLE)) {
        describer->body_str = "spellbooks";
        after_str[after_n++] = "of different realms from yours";
    }

    if (IS_FLG(FLG_REALM1)) {
        describer->body_str = "spellbooks";
        after_str[after_n++] = "of your first realm";
    }

    if (IS_FLG(FLG_REALM2)) {
        describer->body_str = "spellbooks";
        after_str[after_n++] = "of your second realm";
    }

    if (IS_FLG(FLG_FIRST)) {
        before_str[describer->before_n++] = "first one of four";
        describer->body_str = "spellbooks";
    }

    if (IS_FLG(FLG_SECOND)) {
        before_str[describer->before_n++] = "second one of four";
        describer->body_str = "spellbooks";
    }

    if (IS_FLG(FLG_THIRD)) {
        before_str[describer->before_n++] = "third one of four";
        describer->body_str = "spellbooks";
    }

    if (IS_FLG(FLG_FOURTH)) {
        before_str[describer->before_n++] = "fourth one of four";
        describer->body_str = "spellbooks";
    }

    if (IS_FLG(FLG_ITEMS))
        ; /* Nothing to do */
    else if (IS_FLG(FLG_WEAPONS))
        describer->body_str = "weapons";
    else if (IS_FLG(FLG_FAVORITE_WEAPONS))
        describer->body_str = "favorite weapons";
    else if (IS_FLG(FLG_ARMORS))
        describer->body_str = "pieces of armor";
    else if (IS_FLG(FLG_MISSILES))
        describer->body_str = "shots, arrows or crossbow bolts";
    else if (IS_FLG(FLG_DEVICES))
        describer->body_str = "scrolls, wands, staffs or rods";
    else if (IS_FLG(FLG_LIGHTS))
        describer->body_str = "light sources";
    else if (IS_FLG(FLG_JUNKS))
        describer->body_str = "pieces of junk such as broken sticks";
    else if (IS_FLG(FLG_CORPSES))
        describer->body_str = "corpses or skeletons";
    else if (IS_FLG(FLG_SPELLBOOKS))
        describer->body_str = "spellbooks";
    else if (IS_FLG(FLG_HAFTED))
        describer->body_str = "hafted weapons";
    else if (IS_FLG(FLG_SHIELDS))
        describer->body_str = "shields";
    else if (IS_FLG(FLG_BOWS))
        describer->body_str = "slings, bows or crossbows";
    else if (IS_FLG(FLG_RINGS))
        describer->body_str = "rings";
    else if (IS_FLG(FLG_AMULETS))
        describer->body_str = "amulets";
    else if (IS_FLG(FLG_SUITS))
        describer->body_str = "pieces of body armor";
    else if (IS_FLG(FLG_CLOAKS))
        describer->body_str = "cloaks";
    else if (IS_FLG(FLG_HELMS))
        describer->body_str = "helms or crowns";
    else if (IS_FLG(FLG_GLOVES))
        describer->body_str = "gloves";
    else if (IS_FLG(FLG_BOOTS))
        describer->body_str = "boots";

    if (*describer->str) {
        if (*describer->str == '^') {
            describer->str++;
            describer->top = true;
            whose_str[whose_n] = "names begin with \"";
            whose_arg_str[whose_n] = "";
            ++whose_n;
        } else
            which_str[which_n++] = "have \"";
    }

    if (describer->act & DONT_AUTOPICK)
        strcpy(buff, "Leave on floor ");
    else if (describer->act & DO_AUTODESTROY)
        strcpy(buff, "Destroy ");
    else if (describer->act & DO_QUERY_AUTOPICK)
        strcpy(buff, "Ask to pick up ");
    else
        strcpy(buff, "Pickup ");

    if (describer->insc) {
        strncat(buff, format("and inscribe \"%s\"", describer->insc), 80);

        if (angband_strstr(describer->insc, "%all"))
            strcat(buff, ", replacing %all with code string representing all abilities,");
        else if (angband_strstr(describer->insc, "%"))
            strcat(buff, ", replacing % with code string representing extra random abilities,");

        strcat(buff, " on ");
    }

    if (!describer->before_n)
        strcat(buff, "all ");
    else
        for (int i = 0; i < describer->before_n && before_str[i]; i++) {
            strcat(buff, before_str[i]);
            strcat(buff, " ");
        }

    strcat(buff, describer->body_str);
    for (int i = 0; i < after_n && after_str[i]; i++) {
        strcat(buff, " ");
        strcat(buff, after_str[i]);
    }

    for (int i = 0; i < whose_n && whose_str[i]; i++) {
        if (i == 0)
            strcat(buff, " whose ");
        else
            strcat(buff, ", and ");

        strcat(buff, whose_str[i]);
        strcat(buff, whose_arg_str[i]);
    }

    if (*describer->str && describer->top) {
        strcat(buff, describer->str);
        strcat(buff, "\"");
    }

    if (whose_n && which_n)
        strcat(buff, ", and ");

    for (int i = 0; i < which_n && which_str[i]; i++) {
        if (i == 0)
            strcat(buff, " which ");
        else
            strcat(buff, ", and ");

        strcat(buff, which_str[i]);
    }

    if (*describer->str && !describer->top) {
        strncat(buff, describer->str, 80);
        strcat(buff, "\" as part of their names");
    }

    strcat(buff, ".");

    if (describer->act & DO_DISPLAY) {
        if (describer->act & DONT_AUTOPICK)
            strcat(buff, "  Display these items when you press the N key in the full 'M'ap.");
        else if (describer->act & DO_AUTODESTROY)
            strcat(buff, "  Display these items when you press the K key in the full 'M'ap.");
        else
            strcat(buff, "  Display these items when you press the M key in the full 'M'ap.");
    } else
        strcat(buff, " Not displayed in the full map.");
}
#endif

/*!
 * @brief Describe which kind of object is Auto-picked/destroyed
 */
void describe_autopick(char *buff, autopick_type *entry)
{
    //! @note autopick_describer::str は non-nullable、autopick_describer::insc は nullable という制約がある
    autopick_describer describer;
    describer.str = entry->name.c_str();
    describer.act = entry->action;
    describer.insc = entry->insc.empty() ? nullptr : entry->insc.c_str();
    describer.top = false;
    describer.before_n = 0;
    describer.body_str = _("アイテム", "items");
#ifdef JP
    describe_autpick_jp(buff, entry, &describer);
#else
    describe_autopick_en(buff, entry, &describer);
#endif
}
