/*!
 *  @brief オブジェクトの記述処理 / Mbject flavor code
 *  @date 2014/01/03
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 */

#include "flavor/object-flavor.h"
#include "combat/shoot.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "game-option/text-display-options.h"
#include "grid/trap.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "locale/english.h"
#include "locale/japanese.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-quest.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-info/class-info.h"
#include "player/player-status.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/baseitem-info.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <functional>
#include <sstream>
#include <utility>

/*!
 * @brief 各種語彙からランダムな名前を作成する
 * @return std::string 作成した名前
 * @details 日本語の場合 aname_j.txt 英語の場合確率に応じて
 * syllables 配列と elvish.txt を組み合わせる
 */
std::string get_table_name_aux()
{
    std::stringstream ss;
#ifdef JP
    ss << get_random_line("aname_j.txt", 1).value();
    ss << get_random_line("aname_j.txt", 2).value();
    return ss.str();
#else
    static const std::vector<std::string_view> syllables = {
        "a", "ab", "ag", "aks", "ala", "an", "ankh", "app", "arg", "arze", "ash", "aus", "ban", "bar", "bat", "bek",
        "bie", "bin", "bit", "bjor", "blu", "bot", "bu", "byt", "comp", "con", "cos", "cre", "dalf", "dan", "den", "der", "doe", "dok", "eep", "el", "eng",
        "er", "ere", "erk", "esh", "evs", "fa", "fid", "flit", "for", "fri", "fu", "gan", "gar", "glen", "gop", "gre", "ha", "he", "hyd", "i", "ing", "ion",
        "ip", "ish", "it", "ite", "iv", "jo", "kho", "kli", "klis", "la", "lech", "man", "mar", "me", "mi", "mic", "mik", "mon", "mung", "mur", "nag", "nej",
        "nelg", "nep", "ner", "nes", "nis", "nih", "nin", "o", "od", "ood", "org", "orn", "ox", "oxy", "pay", "pet", "ple", "plu", "po", "pot", "prok", "re",
        "rea", "rhov", "ri", "ro", "rog", "rok", "rol", "sa", "san", "sat", "see", "sef", "seh", "shu", "ski", "sna", "sne", "snik", "sno", "so", "sol", "sri",
        "sta", "sun", "ta", "tab", "tem", "ther", "ti", "tox", "trol", "tue", "turs", "u", "ulk", "um", "un", "uni", "ur", "val", "viv", "vly", "vom", "wah",
        "wed", "werg", "wex", "whon", "wun", "x", "yerg", "yp", "zun", "tri", "blaa", "jah", "bul", "on", "foo", "ju", "xuxu"
    };

    int testcounter = randint1(3) + 1;
    if (randint1(3) == 2) {
        while (testcounter--) {
            ss << rand_choice(syllables);
        }
    } else {
        testcounter = randint1(2) + 1;
        while (testcounter--) {
            ss << get_random_line("elvish.txt", 0).value();
        }
    }

    auto name = ss.str();
    name[0] = toupper(name[0]);
    return name;
#endif
}

/*!
 * @brief ランダムな名前をアーティファクト銘として整形する。 / Create a name from random parts with quotes.
 * @return std::string 作成した名前
 * @details get_table_name_aux()ほぼ完全に実装を依存している。
 */
std::string get_table_name()
{
    return std::string(_("『", "'")).append(get_table_name_aux()).append(_("』", "'"));
}

/*!
 * @brief ランダムなシンダリン銘を作成する / Make random Sindarin name
 * @return std::string 作成した名前
 * @details sname.txtが語幹の辞書となっている。
 */
std::string get_table_sindarin_aux()
{
    std::stringstream ss;
    ss << get_random_line("sname.txt", 1).value();
    ss << get_random_line("sname.txt", 2).value();
    auto name = ss.str();
    return _(sindarin_to_kana(name), name);
}

/*!
 * @brief シンダリン銘をアーティファクト用に整形する。 / Make random Sindarin name with quotes
 * @param out_string 作成した名を保管する参照ポインタ
 * @return std::string 作成した名前
 * @details get_table_sindarin_aux()ほぼ完全に実装を依存している。
 */
std::string get_table_sindarin()
{
    return std::string(_("『", "'")).append(get_table_sindarin_aux()).append(_("』", "'"));
}

/*!
 * @brief nameバッファ内からベースアイテム名を返す / Strip an "object name" into a buffer
 * @param buf ベースアイテム格納先の参照ポインタ
 * @param bi_id ベースアイテムID
 */
std::string strip_name(short bi_id)
{
    const auto &baseitem = baseitems_info[bi_id];
    auto tok = str_split(baseitem.name, ' ');
    std::stringstream name;
    for (const auto &s : tok) {
        if (s == "" || s == "~" || s == "&" || s == "#") {
            continue;
        }

        auto offset = 0;
        auto endpos = s.size();
        auto is_kanji = false;

        if (s[0] == '~' || s[0] == '#') {
            offset++;
        }
#ifdef JP
        if (s.size() > 2) {
            is_kanji = iskanji(s[endpos - 2]);
        }

#endif
        if (!is_kanji && (s[endpos - 1] == '~' || s[endpos - 1] == '#')) {
            endpos--;
        }

        name << s.substr(offset, endpos);
    }

    name << " ";
    return name.str();
}
