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
#include "cmd-item/cmd-smith.h"
#include "combat/shoot.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "game-option/text-display-options.h"
#include "grid/trap.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
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
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/player-class.h"
#include "player/player-status.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "world/world.h"
#ifdef JP
#include "locale/japanese.h"
#else
#include "locale/english.h"
#endif

/*!
 * @brief 最初から簡易な名称が明らかになるベースアイテムの判定。 /  Certain items, if aware, are known instantly
 * @param i ベースアイテムID
 * @return 簡易名称を明らかにするならTRUEを返す。
 * @details
 * This function is used only by "flavor_init()"
 */
static bool object_easy_know(int i)
{
    object_kind *k_ptr = &k_info[i];
    switch (k_ptr->tval) {
    case TV_LIFE_BOOK:
    case TV_SORCERY_BOOK:
    case TV_NATURE_BOOK:
    case TV_CHAOS_BOOK:
    case TV_DEATH_BOOK:
    case TV_TRUMP_BOOK:
    case TV_ARCANE_BOOK:
    case TV_CRAFT_BOOK:
    case TV_DEMON_BOOK:
    case TV_CRUSADE_BOOK:
    case TV_MUSIC_BOOK:
    case TV_HISSATSU_BOOK:
    case TV_HEX_BOOK:
        return true;
    case TV_FLASK:
    case TV_JUNK:
    case TV_BOTTLE:
    case TV_SKELETON:
    case TV_SPIKE:
    case TV_WHISTLE:
        return true;
    case TV_FOOD:
    case TV_POTION:
    case TV_SCROLL:
    case TV_ROD:
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
        while (testcounter--)
            strcat(out_string, syllables[randint0(MAX_SYLLABLES)]);
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
static void shuffle_flavors(tval_type tval)
{
    KIND_OBJECT_IDX *k_idx_list;
    KIND_OBJECT_IDX k_idx_list_num = 0;
    C_MAKE(k_idx_list, max_k_idx, KIND_OBJECT_IDX);
    for (KIND_OBJECT_IDX i = 0; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        if (k_ptr->tval != tval)
            continue;

        if (!k_ptr->flavor)
            continue;

        if (has_flag(k_ptr->flags, TR_FIXED_FLAVOR))
            continue;

        k_idx_list[k_idx_list_num] = i;
        k_idx_list_num++;
    }

    for (KIND_OBJECT_IDX i = 0; i < k_idx_list_num; i++) {
        object_kind *k1_ptr = &k_info[k_idx_list[i]];
        object_kind *k2_ptr = &k_info[k_idx_list[randint0(k_idx_list_num)]];
        int16_t tmp = k1_ptr->flavor;
        k1_ptr->flavor = k2_ptr->flavor;
        k2_ptr->flavor = tmp;
    }

    C_KILL(k_idx_list, max_k_idx, int16_t);
}

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン / Prepare the "variable" part of the "k_info" array.
 * @param なし
 */
void flavor_init(void)
{
    uint32_t state_backup[4];
    Rand_state_backup(state_backup);
    Rand_state_set(current_world_ptr->seed_flavor);
    for (KIND_OBJECT_IDX i = 0; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        if (k_ptr->flavor_name.empty())
            continue;

        k_ptr->flavor = i;
    }

    shuffle_flavors(TV_RING);
    shuffle_flavors(TV_AMULET);
    shuffle_flavors(TV_STAFF);
    shuffle_flavors(TV_WAND);
    shuffle_flavors(TV_ROD);
    shuffle_flavors(TV_FOOD);
    shuffle_flavors(TV_POTION);
    shuffle_flavors(TV_SCROLL);
    Rand_state_restore(state_backup);
    for (KIND_OBJECT_IDX i = 1; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        if (k_ptr->name.empty())
            continue;

        if (!k_ptr->flavor)
            k_ptr->aware = true;

        k_ptr->easy_know = object_easy_know(i);
    }
}

/*!
 * @brief nameバッファ内からベースアイテム名を返す / Strip an "object name" into a buffer
 * @param buf ベースアイテム格納先の参照ポインタ
 * @param k_idx ベースアイテムID
 */
void strip_name(char *buf, KIND_OBJECT_IDX k_idx)
{
    auto k_ptr = &k_info[k_idx];
    auto tok = str_split(k_ptr->name, ' ');
    std::string name = "";
    for (auto s : tok) {
        if (s == "" || s == "~" || s == "&" || s == "#")
            continue;

        auto offset = 0;
        auto endpos = s.size();
        auto is_kanji = false;

        if (s[0] == '~' || s[0] == '#')
            offset++;
#ifdef JP
        if (s.size() > 2)
            is_kanji = iskanji(s[endpos - 2]);

#endif
        if (!is_kanji && (s[endpos - 1] == '~' || s[endpos - 1] == '#'))
            endpos--;

        name += s.substr(offset, endpos);
    }

    name += " ";
    strcpy(buf, name.c_str());
}
