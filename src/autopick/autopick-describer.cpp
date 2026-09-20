/*!
 * @brief 自動拾いの記述
 * @date 2020/04/25
 * @author Hourier
 * @todo 1つ300行近い関数なので後ほど要分割
 */

#include "autopick/autopick-describer.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "util/string-processor.h"
#include <sstream>
#include <string>
#include <vector>

struct autopick_describer {
    std::string name;
    EnumClassFlagGroup<AutopickMethod> act;
    std::string inscription;
    bool top;
    std::string body_str;
};

#ifdef JP
static std::string describe_autopick_jp(const autopick_type &entry, autopick_describer &describer)
{
    std::vector<std::string> before_strings;
    before_strings.reserve(100);
    if (entry.has(FLG_COLLECTING)) {
        before_strings.emplace_back("収集中で既に持っているスロットにまとめられる");
    }

    if (entry.has(FLG_UNAWARE)) {
        before_strings.emplace_back("未鑑定でその効果も判明していない");
    }

    if (entry.has(FLG_UNIDENTIFIED)) {
        before_strings.emplace_back("未鑑定の");
    }

    if (entry.has(FLG_IDENTIFIED)) {
        before_strings.emplace_back("鑑定済みの");
    }

    if (entry.has(FLG_STAR_IDENTIFIED)) {
        before_strings.emplace_back("完全に鑑定済みの");
    }

    if (entry.has(FLG_BOOSTED)) {
        before_strings.emplace_back("ダメージダイスが通常より大きい");
        describer.body_str = "武器";
    }

    if (entry.has(FLG_MORE_DICE)) {
        describer.body_str = "武器";
        before_strings.emplace_back("ダメージダイスの最大値が");
        before_strings.emplace_back(std::to_string(entry.dice));
        before_strings.emplace_back("以上の");
    }

    if (entry.has(FLG_MORE_BONUS)) {
        before_strings.emplace_back("修正値が(+");
        before_strings.emplace_back(std::to_string(entry.bonus));
        before_strings.emplace_back(")以上の");
    }

    if (entry.has(FLG_WORTHLESS)) {
        before_strings.emplace_back("店で無価値と判定される");
    }

    if (entry.has(FLG_ARTIFACT)) {
        before_strings.emplace_back("アーティファクトの");
        describer.body_str = "装備";
    }

    if (entry.has(FLG_EGO)) {
        before_strings.emplace_back("エゴアイテムの");
        describer.body_str = "装備";
    }

    if (entry.has(FLG_GOOD)) {
        before_strings.emplace_back("上質の");
        describer.body_str = "装備";
    }

    if (entry.has(FLG_NAMELESS)) {
        before_strings.emplace_back("エゴでもアーティファクトでもない");
        describer.body_str = "装備";
    }

    if (entry.has(FLG_AVERAGE)) {
        before_strings.emplace_back("並の");
        describer.body_str = "装備";
    }

    if (entry.has(FLG_RARE)) {
        before_strings.emplace_back("ドラゴン装備やカオス・ブレード等を含む珍しい");
        describer.body_str = "装備";
    }

    if (entry.has(FLG_COMMON)) {
        before_strings.emplace_back("ありふれた(ドラゴン装備やカオス・ブレード等の珍しい物ではない)");
        describer.body_str = "装備";
    }

    if (entry.has(FLG_WANTED)) {
        before_strings.emplace_back("ハンター事務所で賞金首とされている");
        describer.body_str = "死体や骨";
    }

    if (entry.has(FLG_HUMAN)) {
        before_strings.emplace_back("悪魔魔法で使うための人間やヒューマノイドの");
        describer.body_str = "死体や骨";
    }

    if (entry.has(FLG_UNIQUE)) {
        before_strings.emplace_back("ユニークモンスターの");
        describer.body_str = "死体や骨";
    }

    if (entry.has(FLG_UNREADABLE)) {
        before_strings.emplace_back("あなたが読めない領域の");
        describer.body_str = "魔法書";
    }

    if (entry.has(FLG_REALM1)) {
        before_strings.emplace_back("第一領域の");
        describer.body_str = "魔法書";
    }

    if (entry.has(FLG_REALM2)) {
        before_strings.emplace_back("第二領域の");
        describer.body_str = "魔法書";
    }

    if (entry.has(FLG_FIRST)) {
        before_strings.emplace_back("全4冊の内の1冊目の");
        describer.body_str = "魔法書";
    }

    if (entry.has(FLG_SECOND)) {
        before_strings.emplace_back("全4冊の内の2冊目の");
        describer.body_str = "魔法書";
    }

    if (entry.has(FLG_THIRD)) {
        before_strings.emplace_back("全4冊の内の3冊目の");
        describer.body_str = "魔法書";
    }

    if (entry.has(FLG_FOURTH)) {
        before_strings.emplace_back("全4冊の内の4冊目の");
        describer.body_str = "魔法書";
    }

    if (entry.has(FLG_ITEMS)) {
        ;
    } /* Nothing to do */
    else if (entry.has(FLG_WEAPONS)) {
        describer.body_str = "武器";
    } else if (entry.has(FLG_FAVORITE_WEAPONS)) {
        describer.body_str = "得意武器";
    } else if (entry.has(FLG_ARMORS)) {
        describer.body_str = "防具";
    } else if (entry.has(FLG_MISSILES)) {
        describer.body_str = "弾や矢やクロスボウの矢";
    } else if (entry.has(FLG_DEVICES)) {
        describer.body_str = "巻物や魔法棒や杖やロッド";
    } else if (entry.has(FLG_LIGHTS)) {
        describer.body_str = "光源用のアイテム";
    } else if (entry.has(FLG_JUNKS)) {
        describer.body_str = "折れた棒等のガラクタ";
    } else if (entry.has(FLG_CORPSES)) {
        describer.body_str = "死体や骨";
    } else if (entry.has(FLG_SPELLBOOKS)) {
        describer.body_str = "魔法書";
    } else if (entry.has(FLG_HAFTED)) {
        describer.body_str = "鈍器";
    } else if (entry.has(FLG_SHIELDS)) {
        describer.body_str = "盾";
    } else if (entry.has(FLG_BOWS)) {
        describer.body_str = "スリングや弓やクロスボウ";
    } else if (entry.has(FLG_RINGS)) {
        describer.body_str = "指輪";
    } else if (entry.has(FLG_AMULETS)) {
        describer.body_str = "アミュレット";
    } else if (entry.has(FLG_SUITS)) {
        describer.body_str = "鎧";
    } else if (entry.has(FLG_CLOAKS)) {
        describer.body_str = "クローク";
    } else if (entry.has(FLG_HELMS)) {
        describer.body_str = "ヘルメットや冠";
    } else if (entry.has(FLG_GLOVES)) {
        describer.body_str = "籠手";
    } else if (entry.has(FLG_BOOTS)) {
        describer.body_str = "ブーツ";
    }

    std::stringstream ss;
    if (before_strings.empty()) {
        ss << "全ての";
    } else {
        for (const auto &before_string : before_strings) {
            ss << before_string;
        }
    }

    ss << describer.body_str;

    if (!describer.name.empty()) {
        if (describer.name.starts_with('^')) {
            describer.name = describer.name.substr(1);
            describer.top = true;
        }

        ss << "で、名前が「";
        ss << describer.name;
        if (describer.top) {
            ss << "」で始まるもの";
        } else {
            ss << "」を含むもの";
        }
    }

    if (!describer.inscription.empty()) {
        ss << "に「" << describer.inscription << "」";

        if (str_find(describer.inscription, "%%all")) {
            ss << "(%%allは全能力を表す英字の記号で置換)";
        } else if (str_find(describer.inscription, "%all")) {
            ss << "(%allは全能力を表す記号で置換)";
        } else if (str_find(describer.inscription, "%%")) {
            ss << "(%%は追加能力を表す英字の記号で置換)";
        } else if (str_find(describer.inscription, "%")) {
            ss << "(%は追加能力を表す記号で置換)";
        }

        ss << "と刻んで";
    } else {
        ss << "を";
    }

    if (describer.act.has(AutopickMethod::NOT_AUTOPICK)) {
        ss << "放置する。";
    } else if (describer.act.has(AutopickMethod::AUTODESTROY)) {
        ss << "破壊する。";
    } else if (describer.act.has(AutopickMethod::QUERY_AUTOPICK)) {
        ss << "確認の後に拾う。";
    } else {
        ss << "拾う。";
    }

    if (describer.act.has(AutopickMethod::DISPLAY)) {
        if (describer.act.has(AutopickMethod::NOT_AUTOPICK)) {
            ss << "全体マップ('M')で'N'を押したときに表示する。";
        } else if (describer.act.has(AutopickMethod::AUTODESTROY)) {
            ss << "全体マップ('M')で'K'を押したときに表示する。";
        } else {
            ss << "全体マップ('M')で'M'を押したときに表示する。";
        }
    } else {
        ss << "全体マップには表示しない。";
    }

    return ss.str();
}
#else

std::string describe_autopick_en(const autopick_type &entry, autopick_describer &describer)
{
    std::vector<std::string> before_strings;
    before_strings.reserve(20);
    std::vector<std::string> after_strings;
    after_strings.reserve(20);
    std::vector<std::string> which_strings;
    which_strings.reserve(20);
    std::vector<std::string> whose_strings;
    whose_strings.reserve(20);
    std::vector<std::string> whose_arg_strings;
    whose_arg_strings.reserve(20);
    if (entry.has(FLG_COLLECTING)) {
        which_strings.emplace_back("can be absorbed into an existing inventory list slot");
    }

    if (entry.has(FLG_UNAWARE)) {
        before_strings.emplace_back("unidentified");
        whose_strings.emplace_back("basic abilities are not known");
        whose_arg_strings.emplace_back("");
    }

    if (entry.has(FLG_UNIDENTIFIED)) {
        before_strings.emplace_back("unidentified");
    }

    if (entry.has(FLG_IDENTIFIED)) {
        before_strings.emplace_back("identified");
    }

    if (entry.has(FLG_STAR_IDENTIFIED)) {
        before_strings.emplace_back("fully identified");
    }

    if (entry.has(FLG_RARE)) {
        before_strings.emplace_back("very rare");
        after_strings.emplace_back("such as Dragon armor, Blades of Chaos, etc.");
    }

    if (entry.has(FLG_COMMON)) {
        before_strings.emplace_back("relatively common");
        after_strings.emplace_back("compared to very rare Dragon armor, Blades of Chaos, etc.");
    }

    if (entry.has(FLG_WORTHLESS)) {
        before_strings.emplace_back("worthless");
        which_strings.emplace_back("can not be sold at stores");
    }

    if (entry.has(FLG_ARTIFACT)) {
        before_strings.emplace_back("artifact");
    }

    if (entry.has(FLG_EGO)) {
        before_strings.emplace_back("ego");
    }

    if (entry.has(FLG_GOOD)) {
        which_strings.emplace_back("are of good quality");
    }

    if (entry.has(FLG_NAMELESS)) {
        which_strings.emplace_back("are neither ego items nor artifacts");
    }

    if (entry.has(FLG_AVERAGE)) {
        which_strings.emplace_back("are of average quality");
    }

    if (entry.has(FLG_BOOSTED)) {
        describer.body_str = "weapons";
        whose_strings.emplace_back("damage dice is bigger than normal");
        whose_arg_strings.emplace_back("");
    }

    if (entry.has(FLG_MORE_DICE)) {
        describer.body_str = "weapons";
        whose_strings.emplace_back("maximum damage from dice is bigger than ");
        whose_arg_strings.emplace_back(std::to_string(entry.dice));
    }

    if (entry.has(FLG_MORE_BONUS)) {
        whose_strings.emplace_back("magical bonuses are bigger than (+");
        whose_arg_strings.emplace_back(std::to_string(entry.bonus) + ")");
    }

    if (entry.has(FLG_WANTED)) {
        describer.body_str = "corpses or skeletons";
        which_strings.emplace_back("are wanted at the Hunter's Office");
    }

    if (entry.has(FLG_HUMAN)) {
        before_strings.emplace_back("humanoid");
        describer.body_str = "corpses or skeletons";
        which_strings.emplace_back("can be used for Daemon magic");
    }

    if (entry.has(FLG_UNIQUE)) {
        before_strings.emplace_back("unique monsters'");
        describer.body_str = "corpses or skeletons";
    }

    if (entry.has(FLG_UNREADABLE)) {
        describer.body_str = "spellbooks";
        after_strings.emplace_back("of different realms from yours");
    }

    if (entry.has(FLG_REALM1)) {
        describer.body_str = "spellbooks";
        after_strings.emplace_back("of your first realm");
    }

    if (entry.has(FLG_REALM2)) {
        describer.body_str = "spellbooks";
        after_strings.emplace_back("of your second realm");
    }

    if (entry.has(FLG_FIRST)) {
        before_strings.emplace_back("first one of four");
        describer.body_str = "spellbooks";
    }

    if (entry.has(FLG_SECOND)) {
        before_strings.emplace_back("second one of four");
        describer.body_str = "spellbooks";
    }

    if (entry.has(FLG_THIRD)) {
        before_strings.emplace_back("third one of four");
        describer.body_str = "spellbooks";
    }

    if (entry.has(FLG_FOURTH)) {
        before_strings.emplace_back("fourth one of four");
        describer.body_str = "spellbooks";
    }

    if (entry.has(FLG_ITEMS)) {
        ;
    } /* Nothing to do */
    else if (entry.has(FLG_WEAPONS)) {
        describer.body_str = "weapons";
    } else if (entry.has(FLG_FAVORITE_WEAPONS)) {
        describer.body_str = "favorite weapons";
    } else if (entry.has(FLG_ARMORS)) {
        describer.body_str = "pieces of armor";
    } else if (entry.has(FLG_MISSILES)) {
        describer.body_str = "shots, arrows or crossbow bolts";
    } else if (entry.has(FLG_DEVICES)) {
        describer.body_str = "scrolls, wands, staffs or rods";
    } else if (entry.has(FLG_LIGHTS)) {
        describer.body_str = "light sources";
    } else if (entry.has(FLG_JUNKS)) {
        describer.body_str = "pieces of junk such as broken sticks";
    } else if (entry.has(FLG_CORPSES)) {
        describer.body_str = "corpses or skeletons";
    } else if (entry.has(FLG_SPELLBOOKS)) {
        describer.body_str = "spellbooks";
    } else if (entry.has(FLG_HAFTED)) {
        describer.body_str = "hafted weapons";
    } else if (entry.has(FLG_SHIELDS)) {
        describer.body_str = "shields";
    } else if (entry.has(FLG_BOWS)) {
        describer.body_str = "slings, bows or crossbows";
    } else if (entry.has(FLG_RINGS)) {
        describer.body_str = "rings";
    } else if (entry.has(FLG_AMULETS)) {
        describer.body_str = "amulets";
    } else if (entry.has(FLG_SUITS)) {
        describer.body_str = "pieces of body armor";
    } else if (entry.has(FLG_CLOAKS)) {
        describer.body_str = "cloaks";
    } else if (entry.has(FLG_HELMS)) {
        describer.body_str = "helms or crowns";
    } else if (entry.has(FLG_GLOVES)) {
        describer.body_str = "gloves";
    } else if (entry.has(FLG_BOOTS)) {
        describer.body_str = "boots";
    }

    if (!describer.name.empty()) {
        if (describer.name.starts_with('^')) {
            describer.name = describer.name.substr(1);
            describer.top = true;
            whose_strings.emplace_back("names begin with \"");
            whose_arg_strings.emplace_back("");
        } else {
            which_strings.emplace_back("have \"");
        }
    }

    std::stringstream ss;
    if (describer.act.has(AutopickMethod::NOT_AUTOPICK)) {
        ss << "Leave on floor ";
    } else if (describer.act.has(AutopickMethod::AUTODESTROY)) {
        ss << "Destroy ";
    } else if (describer.act.has(AutopickMethod::QUERY_AUTOPICK)) {
        ss << "Ask to pick up ";
    } else {
        ss << "Pickup ";
    }

    if (!describer.inscription.empty()) {
        const auto inscriptions = str_separate(describer.inscription, 80);
        ss << "and inscribe \"" << inscriptions[0] << "\"";

        if (str_find(describer.inscription, "%all")) {
            ss << ", replacing %all with code string representing all abilities,";
        } else if (str_find(describer.inscription, "%")) {
            ss << ", replacing % with code string representing extra random abilities,";
        }

        ss << " on ";
    }

    if (before_strings.empty()) {
        ss << "all ";
    } else {
        for (const auto &before_string : before_strings) {
            ss << before_string;
            ss << " ";
        }
    }

    ss << describer.body_str;
    for (const auto &after_string : after_strings) {
        ss << " ";
        ss << after_string;
    }

    for (size_t i = 0; i < whose_strings.size(); i++) {
        if (i == 0) {
            ss << " whose ";
        } else {
            ss << ", and ";
        }

        ss << whose_strings[i];
        ss << whose_arg_strings[i];
    }

    if (!describer.name.empty() && describer.top) {
        ss << describer.name;
        ss << "\"";
    }

    if (!whose_strings.empty() && !which_strings.empty()) {
        ss << ", and ";
    }

    for (size_t i = 0; i < which_strings.size(); i++) {
        if (i == 0) {
            ss << " which ";
        } else {
            ss << ", and ";
        }

        ss << which_strings[i];
    }

    if (!describer.name.empty() && !describer.top) {
        const auto inscriptions = str_separate(describer.name, 80);
        ss << inscriptions[0];
        ss << "\" as part of their names";
    }

    ss << ".";

    if (describer.act.has(AutopickMethod::DISPLAY)) {
        if (describer.act.has(AutopickMethod::NOT_AUTOPICK)) {
            ss << "  Display these items when you press the N key in the full 'M'ap.";
        } else if (describer.act.has(AutopickMethod::AUTODESTROY)) {
            ss << "  Display these items when you press the K key in the full 'M'ap.";
        } else {
            ss << "  Display these items when you press the M key in the full 'M'ap.";
        }
    } else {
        ss << " Not displayed in the full map.";
    }

    return ss.str();
}
#endif

/*!
 * @brief Describe which kind of object is Auto-picked/destroyed
 */
std::string describe_autopick(const autopick_type &entry)
{
    autopick_describer describer{};
    describer.name = entry.name;
    describer.act = entry.action;
    describer.inscription = entry.insc;
    describer.top = false;
    describer.body_str = _("アイテム", "items");
#ifdef JP
    return describe_autopick_jp(entry, describer);
#else
    return describe_autopick_en(entry, describer);
#endif
}
