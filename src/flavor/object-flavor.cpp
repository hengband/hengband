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
#include "system/baseitem-info-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <functional>
#include <sstream>
#include <utility>

/*!
 * @brief 最初から簡易な名称が明らかになるベースアイテムの判定。 /  Certain items, if aware, are known instantly
 * @param i ベースアイテムID
 * @return 簡易名称を明らかにするならTRUEを返す。
 * @details
 * This function is used only by "flavor_init()"
 */
static bool object_easy_know(int i)
{
    auto *k_ptr = &baseitems_info[i];
    switch (k_ptr->tval) {
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
        return true;
    case ItemKindType::FLASK:
    case ItemKindType::JUNK:
    case ItemKindType::BOTTLE:
    case ItemKindType::SKELETON:
    case ItemKindType::SPIKE:
    case ItemKindType::WHISTLE:
        return true;
    case ItemKindType::FOOD:
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
    case ItemKindType::ROD:
        return true;

    default:
        break;
    }

    return false;
}

/*!
 * @brief 各種語彙からランダムな名前を作成する / Create a name from random parts.
 * @param out_string 作成した名を保管する参照ポインタ
 * @details 日本語の場合 aname_j.txt 英語の場合確率に応じて
 * syllables 配列と elvish.txt を組み合わせる。\n
 */
void get_table_name_aux(char *out_string)
{
#ifdef JP
    char syllable[80];
    get_rnd_line("aname_j.txt", 1, syllable);
    strcpy(out_string, syllable);
    get_rnd_line("aname_j.txt", 2, syllable);
    strcat(out_string, syllable);
#else
#define MAX_SYLLABLES 164 /* Used with scrolls (see below) */

    static concptr syllables[MAX_SYLLABLES] = { "a", "ab", "ag", "aks", "ala", "an", "ankh", "app", "arg", "arze", "ash", "aus", "ban", "bar", "bat", "bek",
        "bie", "bin", "bit", "bjor", "blu", "bot", "bu", "byt", "comp", "con", "cos", "cre", "dalf", "dan", "den", "der", "doe", "dok", "eep", "el", "eng",
        "er", "ere", "erk", "esh", "evs", "fa", "fid", "flit", "for", "fri", "fu", "gan", "gar", "glen", "gop", "gre", "ha", "he", "hyd", "i", "ing", "ion",
        "ip", "ish", "it", "ite", "iv", "jo", "kho", "kli", "klis", "la", "lech", "man", "mar", "me", "mi", "mic", "mik", "mon", "mung", "mur", "nag", "nej",
        "nelg", "nep", "ner", "nes", "nis", "nih", "nin", "o", "od", "ood", "org", "orn", "ox", "oxy", "pay", "pet", "ple", "plu", "po", "pot", "prok", "re",
        "rea", "rhov", "ri", "ro", "rog", "rok", "rol", "sa", "san", "sat", "see", "sef", "seh", "shu", "ski", "sna", "sne", "snik", "sno", "so", "sol", "sri",
        "sta", "sun", "ta", "tab", "tem", "ther", "ti", "tox", "trol", "tue", "turs", "u", "ulk", "um", "un", "uni", "ur", "val", "viv", "vly", "vom", "wah",
        "wed", "werg", "wex", "whon", "wun", "x", "yerg", "yp", "zun", "tri", "blaa", "jah", "bul", "on", "foo", "ju", "xuxu" };

    int testcounter = randint1(3) + 1;
    strcpy(out_string, "");
    if (randint1(3) == 2) {
        while (testcounter--) {
            strcat(out_string, syllables[randint0(MAX_SYLLABLES)]);
        }
    } else {
        char syllable[80];
        testcounter = randint1(2) + 1;
        while (testcounter--) {
            (void)get_rnd_line("elvish.txt", 0, syllable);
            strcat(out_string, syllable);
        }
    }

    out_string[0] = toupper(out_string[1]);
    out_string[16] = '\0';
#endif
}

/*!
 * @brief ランダムな名前をアーティファクト銘として整形する。 / Create a name from random parts with quotes.
 * @param out_string 作成した名を保管する参照ポインタ
 * @details get_table_name_aux()ほぼ完全に実装を依存している。
 */
void get_table_name(char *out_string)
{
    char buff[80];
    get_table_name_aux(buff);
    sprintf(out_string, _("『%s』", "'%s'"), buff);
}

/*!
 * @brief ランダムなシンダリン銘を作成する / Make random Sindarin name
 * @param out_string 作成した名を保管する参照ポインタ
 * @details sname.txtが語幹の辞書となっている。
 */
void get_table_sindarin_aux(char *out_string)
{
    char syllable[80];
#ifdef JP
    char tmp[80];
#endif

    get_rnd_line("sname.txt", 1, syllable);
    strcpy(_(tmp, out_string), syllable);
    get_rnd_line("sname.txt", 2, syllable);
#ifdef JP
    strcat(tmp, syllable);
    sindarin_to_kana(out_string, tmp);
#else
    strcat(out_string, syllable);
#endif
}

/*!
 * @brief シンダリン銘をアーティファクト用に整形する。 / Make random Sindarin name with quotes
 * @param out_string 作成した名を保管する参照ポインタ
 * @details get_table_sindarin_aux()ほぼ完全に実装を依存している。
 */
void get_table_sindarin(char *out_string)
{
    char buff[80];
    get_table_sindarin_aux(buff);
    sprintf(out_string, _("『%s』", "'%s'"), buff);
}

/*!
 * @brief ベースアイテムの未確定名を共通tval間でシャッフルする / Shuffle flavor indices of a group of objects with given tval
 * @param tval シャッフルしたいtval
 * @details 巻物、各種魔道具などに利用される。
 */
static void shuffle_flavors(ItemKindType tval)
{
    std::vector<std::reference_wrapper<IDX>> flavor_idx_ref_list;
    for (const auto &k_ref : baseitems_info) {
        if (k_ref.tval != tval) {
            continue;
        }

        if (!k_ref.flavor) {
            continue;
        }

        if (k_ref.flags.has(TR_FIXED_FLAVOR)) {
            continue;
        }

        flavor_idx_ref_list.push_back(baseitems_info[k_ref.idx].flavor);
    }

    rand_shuffle(flavor_idx_ref_list.begin(), flavor_idx_ref_list.end());
}

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン
 * @param なし
 */
void flavor_init(void)
{
    const auto state_backup = w_ptr->rng.get_state();
    w_ptr->rng.set_state(w_ptr->seed_flavor);
    for (auto &k_ref : baseitems_info) {
        if (k_ref.flavor_name.empty()) {
            continue;
        }

        k_ref.flavor = k_ref.idx;
    }

    shuffle_flavors(ItemKindType::RING);
    shuffle_flavors(ItemKindType::AMULET);
    shuffle_flavors(ItemKindType::STAFF);
    shuffle_flavors(ItemKindType::WAND);
    shuffle_flavors(ItemKindType::ROD);
    shuffle_flavors(ItemKindType::FOOD);
    shuffle_flavors(ItemKindType::POTION);
    shuffle_flavors(ItemKindType::SCROLL);
    w_ptr->rng.set_state(state_backup);
    for (auto &k_ref : baseitems_info) {
        if (k_ref.idx == 0 || k_ref.name.empty()) {
            continue;
        }

        if (!k_ref.flavor) {
            k_ref.aware = true;
        }

        k_ref.easy_know = object_easy_know(k_ref.idx);
    }
}

/*!
 * @brief nameバッファ内からベースアイテム名を返す / Strip an "object name" into a buffer
 * @param buf ベースアイテム格納先の参照ポインタ
 * @param k_idx ベースアイテムID
 */
std::string strip_name(KIND_OBJECT_IDX k_idx)
{
    auto k_ptr = &baseitems_info[k_idx];
    auto tok = str_split(k_ptr->name, ' ');
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
