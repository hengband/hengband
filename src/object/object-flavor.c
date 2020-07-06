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

#include "object/object-flavor.h"
#include "cmd-item/cmd-smith.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/object-flavor-types.h"
#include "game-option/text-display-options.h"
#include "grid/trap.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "locale/japanese.h"
#include "locale/vowel-checker.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/artifact.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-quest.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/player-class.h"
#include "player/player-status.h"
#include "shoot.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "world/world.h"

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
    case TV_DAEMON_BOOK:
    case TV_CRUSADE_BOOK:
    case TV_MUSIC_BOOK:
    case TV_HISSATSU_BOOK:
    case TV_HEX_BOOK:
        return TRUE;
    case TV_FLASK:
    case TV_JUNK:
    case TV_BOTTLE:
    case TV_SKELETON:
    case TV_SPIKE:
    case TV_WHISTLE:
        return TRUE;
    case TV_FOOD:
    case TV_POTION:
    case TV_SCROLL:
    case TV_ROD:
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief 各種語彙からランダムな名前を作成する / Create a name from random parts.
 * @param out_string 作成した名を保管する参照ポインタ
 * @return なし
 * @details 日本語の場合 aname_j.txt 英語の場合確率に応じて
 * syllables 配列と elvish.txt を組み合わせる。\n
 */
void get_table_name_aux(char *out_string)
{
#ifdef JP
    char Syllable[80];
    get_rnd_line("aname_j.txt", 1, Syllable);
    strcpy(out_string, Syllable);
    get_rnd_line("aname_j.txt", 2, Syllable);
    strcat(out_string, Syllable);
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
        char Syllable[80];
        testcounter = randint1(2) + 1;
        while (testcounter--) {
            (void)get_rnd_line("elvish.txt", 0, Syllable);
            strcat(out_string, Syllable);
        }
    }

    out_string[0] = toupper(out_string[1]);
    out_string[16] = '\0';
#endif
}

/*!
 * @brief ランダムな名前をアーティファクト銘として整形する。 / Create a name from random parts with quotes.
 * @param out_string 作成した名を保管する参照ポインタ
 * @return なし
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
 * @return なし
 * @details sname.txtが語幹の辞書となっている。
 */
void get_table_sindarin_aux(char *out_string)
{
    char Syllable[80];
#ifdef JP
    char tmp[80];
#endif

    get_rnd_line("sname.txt", 1, Syllable);
    strcpy(_(tmp, out_string), Syllable);
    get_rnd_line("sname.txt", 2, Syllable);
#ifdef JP
    strcat(tmp, Syllable);
    sindarin_to_kana(out_string, tmp);
#else
    strcat(out_string, Syllable);
#endif
}

/*!
 * @brief シンダリン銘をアーティファクト用に整形する。 / Make random Sindarin name with quotes
 * @param out_string 作成した名を保管する参照ポインタ
 * @return なし
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
 * @return なし
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

        if (have_flag(k_ptr->flags, TR_FIXED_FLAVOR))
            continue;

        k_idx_list[k_idx_list_num] = i;
        k_idx_list_num++;
    }

    for (KIND_OBJECT_IDX i = 0; i < k_idx_list_num; i++) {
        object_kind *k1_ptr = &k_info[k_idx_list[i]];
        object_kind *k2_ptr = &k_info[k_idx_list[randint0(k_idx_list_num)]];
        s16b tmp = k1_ptr->flavor;
        k1_ptr->flavor = k2_ptr->flavor;
        k2_ptr->flavor = tmp;
    }

    C_KILL(k_idx_list, max_k_idx, s16b);
}

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン / Prepare the "variable" part of the "k_info" array.
 * @return なし
 * @details
 * Prepare the "variable" part of the "k_info" array.\n
 *\n
 * The "color"/"metal"/"type" of an item is its "flavor".\n
 * For the most part, flavors are assigned randomly each game.\n
 *\n
 * Initialize descriptions for the "colored" objects, including:\n
 * Rings, Amulets, Staffs, Wands, Rods, Food, Potions, Scrolls.\n
 *\n
 * The first 4 entries for potions are fixed (Water, Apple Juice,\n
 * Slime Mold Juice, Unused Potion).\n
 *\n
 * Scroll titles are always between 6 and 14 letters long.  This is\n
 * ensured because every title is composed of whole words, where every\n
 * word is from 1 to 8 letters long (one or two syllables of 1 to 4\n
 * letters each), and that no scroll is finished until it attempts to\n
 * grow beyond 15 letters.  The first time this can happen is when the\n
 * current title has 6 letters and the new word has 8 letters, which\n
 * would result in a 6 letter scroll title.\n
 *\n
 * Duplicate titles are avoided by requiring that no two scrolls share\n
 * the same first four letters (not the most efficient method, and not\n
 * the least efficient method, but it will always work).\n
 *\n
 * Hack -- make sure everything stays the same for each saved game\n
 * This is accomplished by the use of a saved "random seed", as in\n
 * "town_gen()".  Since no other functions are called while the special\n
 * seed is in effect, so this function is pretty "safe".\n
 *\n
 * Note that the "hacked seed" may provide an RNG with alternating parity!\n
 */
void flavor_init(void)
{
    u32b state_backup[4];
    Rand_state_backup(state_backup);
    Rand_state_set(current_world_ptr->seed_flavor);
    for (KIND_OBJECT_IDX i = 0; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        if (!k_ptr->flavor_name)
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
        if (!k_ptr->name)
            continue;

        if (!k_ptr->flavor)
            k_ptr->aware = TRUE;

        k_ptr->easy_know = object_easy_know(i);
    }
}

/*!
 * @brief 対象文字配列に一文字だけをコピーする。
 * @param t 保管先文字列ポインタ
 * @param c 保管したい1文字
 * @return なし
 * @details
 * Print a char "c" into a string "t", as if by sprintf(t, "%c", c),\n
 * and return a pointer to the terminator (t + 1).\n
 */
static char *object_desc_chr(char *t, char c)
{
    *t++ = c;
    *t = '\0';
    return t;
}

/*!
 * @brief 対象文字配列に文字列をコピーする。
 * @param t 保管先文字列ポインタ
 * @param s コピーしたい文字列ポインタ
 * @return 保管先の末尾アドレス
 * @details
 * Print a string "s" into a string "t", as if by strcpy(t, s),
 * and return a pointer to the terminator.
 */
static char *object_desc_str(char *t, concptr s)
{
    while (*s)
        *t++ = *s++;

    *t = '\0';
    return t;
}

/*!
 * @brief 対象文字配列に符号なし整数値をコピーする。
 * @param t 保管先文字列ポインタ
 * @param n コピーしたい数値
 * @return なし
 * @details
 * Print an unsigned number "n" into a string "t", as if by
 * sprintf(t, "%u", n), and return a pointer to the terminator.
 */
static char *object_desc_num(char *t, uint n)
{
    /* loop */
    uint p;
    for (p = 1; n >= p * 10; p = p * 10)
        ;

    while (p >= 1) {
        *t++ = '0' + n / p;
        n = n % p;
        p = p / 10;
    }

    *t = '\0';
    return t;
}

#ifdef JP
/*!
 * @brief 日本語の個数表示ルーチン
 * @param t 保管先文字列ポインタ
 * @param o_ptr 記述したいオブジェクトの構造体参照ポインタ
 * @return なし
 * @details
 * cmd1.c で流用するために object_desc_japanese から移動した。
 */
char *object_desc_kosuu(char *t, object_type *o_ptr)
{
    t = object_desc_num(t, o_ptr->number);
    switch (o_ptr->tval) {
    case TV_BOLT:
    case TV_ARROW:
    case TV_POLEARM:
    case TV_STAFF:
    case TV_WAND:
    case TV_ROD:
    case TV_DIGGING: {
        t = object_desc_str(t, "本");
        break;
    }
    case TV_SCROLL: {
        t = object_desc_str(t, "巻");
        break;
    }
    case TV_POTION: {
        t = object_desc_str(t, "服");
        break;
    }
    case TV_LIFE_BOOK:
    case TV_SORCERY_BOOK:
    case TV_NATURE_BOOK:
    case TV_CHAOS_BOOK:
    case TV_DEATH_BOOK:
    case TV_TRUMP_BOOK:
    case TV_ARCANE_BOOK:
    case TV_CRAFT_BOOK:
    case TV_DAEMON_BOOK:
    case TV_CRUSADE_BOOK:
    case TV_MUSIC_BOOK:
    case TV_HISSATSU_BOOK:
    case TV_HEX_BOOK: {
        t = object_desc_str(t, "冊");
        break;
    }
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_CLOAK: {
        t = object_desc_str(t, "着");
        break;
    }
    case TV_SWORD:
    case TV_HAFTED:
    case TV_BOW: {
        t = object_desc_str(t, "振");
        break;
    }
    case TV_BOOTS: {
        t = object_desc_str(t, "足");
        break;
    }
    case TV_CARD: {
        t = object_desc_str(t, "枚");
        break;
    }
    case TV_FOOD: {
        if (o_ptr->sval == SV_FOOD_JERKY) {
            t = object_desc_str(t, "切れ");
            break;
        }
    }
        /* Fall through */
    default: {
        if (o_ptr->number < 10) {
            t = object_desc_str(t, "つ");
        } else {
            t = object_desc_str(t, "個");
        }
        break;
    }
    }
    return t;
}
#endif

/*!
 * @brief 対象文字配列に符号あり整数値をコピーする。
 * @param t 保管先文字列ポインタ
 * @param v コピーしたい数値
 * @return なし
 * @details
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *object_desc_int(char *t, int v)
{
    uint p, n;
    if (v < 0) {
        n = 0 - v;
        *t++ = '-';
    } else {
        n = v;
        *t++ = '+';
    }

    /* loop */
    for (p = 1; n >= p * 10; p = p * 10)
        ;

    while (p >= 1) {
        *t++ = '0' + n / p;
        n = n % p;
        p = p / 10;
    }

    *t = '\0';
    return t;
}

/*!
 * @brief オブジェクトフラグを追加する
 * @param str フレーバー表現 (アンチテレポの"[" 等)
 * @param ptr 特性短縮表記を格納する文字列ポインタ
 * @return なし
 */
static void add_inscription(char *ptr, concptr str) { ptr = object_desc_str(ptr, str); }

/*!
 * @brief get_inscriptionのサブセットとしてオブジェクトの特性フラグを返す / Helper function for get_inscription()
 * @param fi_ptr 参照する特性表示記号テーブル
 * @param flgs 対応するオブジェクトのフラグ文字列
 * @param kanji TRUEならば漢字記述/FALSEならば英語記述
 * @param ptr フラグ群を保管する文字列参照ポインタ
 * @return フラグ群を保管する文字列参照ポインタ(ptrと同じ)
 * @details
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *inscribe_flags_aux(flag_insc_table *fi_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE], bool kanji, char *ptr)
{
#ifdef JP
#else
    (void)kanji;
#endif

    while (fi_ptr->english) {
        if (have_flag(flgs, fi_ptr->flag) && (fi_ptr->except_flag == -1 || !have_flag(flgs, fi_ptr->except_flag)))
            add_inscription(ptr, _(kanji ? fi_ptr->japanese : fi_ptr->english, fi_ptr->english));

        fi_ptr++;
    }

    return ptr;
}

/*!
 * @brief オブジェクトの特性表示記号テーブル1つに従いオブジェクトの特性フラグ配列に1つでも該当の特性があるかを返す / Special variation of have_flag for
 * auto-inscription
 * @param fi_ptr 参照する特性表示記号テーブル
 * @param flgs 対応するオブジェクトのフラグ文字列
 * @return 1つでも該当の特性があったらTRUEを返す。
 */
static bool have_flag_of(flag_insc_table *fi_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
    while (fi_ptr->english) {
        if (have_flag(flgs, fi_ptr->flag) && (fi_ptr->except_flag == -1 || !have_flag(flgs, fi_ptr->except_flag)))
            return TRUE;

        fi_ptr++;
    }

    return FALSE;
}

/*!
 * @brief オブジェクト名の特性短縮表記をまとめて提示する。
 * @param ptr 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @param kanji TRUEならば漢字表記 / FALSEなら英語表記
 * @param all TRUEならばベースアイテム上で明らかなフラグは省略する
 * @return ptrと同じアドレス
 */
static char *get_ability_abbreviation(player_type *player_ptr, char *ptr, object_type *o_ptr, bool kanji, bool all)
{
    char *prev_ptr = ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(player_ptr, o_ptr, flgs);
    if (!all) {
        object_kind *k_ptr = &k_info[o_ptr->k_idx];
        for (int j = 0; j < TR_FLAG_SIZE; j++)
            flgs[j] &= ~k_ptr->flags[j];

        if (object_is_fixed_artifact(o_ptr)) {
            artifact_type *a_ptr = &a_info[o_ptr->name1];
            for (int j = 0; j < TR_FLAG_SIZE; j++)
                flgs[j] &= ~a_ptr->flags[j];
        }

        if (object_is_ego(o_ptr)) {
            ego_item_type *e_ptr = &e_info[o_ptr->name2];
            for (int j = 0; j < TR_FLAG_SIZE; j++)
                flgs[j] &= ~e_ptr->flags[j];
        }
    }

    if (has_dark_flag(flgs)) {
        if (have_flag(flgs, TR_LITE_1))
            remove_flag(flgs, TR_LITE_1);

        if (have_flag(flgs, TR_LITE_2))
            remove_flag(flgs, TR_LITE_2);

        if (have_flag(flgs, TR_LITE_3))
            remove_flag(flgs, TR_LITE_3);
    } else if (has_lite_flag(flgs)) {
        add_flag(flgs, TR_LITE_1);
        if (have_flag(flgs, TR_LITE_2))
            remove_flag(flgs, TR_LITE_2);

        if (have_flag(flgs, TR_LITE_3))
            remove_flag(flgs, TR_LITE_3);
    }

    if (have_flag_of(flag_insc_plus, flgs) && kanji)
        add_inscription(ptr, "+");

    ptr = inscribe_flags_aux(flag_insc_plus, flgs, kanji, ptr);

    if (have_flag_of(flag_insc_immune, flgs)) {
        if (!kanji && ptr != prev_ptr) {
            add_inscription(ptr, ";");
            prev_ptr = ptr;
        }

        add_inscription(ptr, "*");
    }

    ptr = inscribe_flags_aux(flag_insc_immune, flgs, kanji, ptr);

    if (have_flag_of(flag_insc_resistance, flgs)) {
        if (kanji)
            add_inscription(ptr, "r");
        else if (ptr != prev_ptr) {
            add_inscription(ptr, ";");
            prev_ptr = ptr;
        }
    }

    ptr = inscribe_flags_aux(flag_insc_resistance, flgs, kanji, ptr);

    if (have_flag_of(flag_insc_misc, flgs) && (ptr != prev_ptr)) {
        add_inscription(ptr, ";");
        prev_ptr = ptr;
    }

    ptr = inscribe_flags_aux(flag_insc_misc, flgs, kanji, ptr);

    if (have_flag_of(flag_insc_aura, flgs))
        add_inscription(ptr, "[");

    ptr = inscribe_flags_aux(flag_insc_aura, flgs, kanji, ptr);

    if (have_flag_of(flag_insc_brand, flgs))
        add_inscription(ptr, "|");

    ptr = inscribe_flags_aux(flag_insc_brand, flgs, kanji, ptr);

    if (have_flag_of(flag_insc_kill, flgs))
        add_inscription(ptr, "/X");

    ptr = inscribe_flags_aux(flag_insc_kill, flgs, kanji, ptr);

    if (have_flag_of(flag_insc_slay, flgs))
        add_inscription(ptr, "/");

    ptr = inscribe_flags_aux(flag_insc_slay, flgs, kanji, ptr);

    if (kanji) {
        if (have_flag_of(flag_insc_esp1, flgs) || have_flag_of(flag_insc_esp2, flgs))
            add_inscription(ptr, "~");

        ptr = inscribe_flags_aux(flag_insc_esp1, flgs, kanji, ptr);
        ptr = inscribe_flags_aux(flag_insc_esp2, flgs, kanji, ptr);
    } else {
        if (have_flag_of(flag_insc_esp1, flgs))
            add_inscription(ptr, "~");

        ptr = inscribe_flags_aux(flag_insc_esp1, flgs, kanji, ptr);

        if (have_flag_of(flag_insc_esp2, flgs))
            add_inscription(ptr, "~");

        ptr = inscribe_flags_aux(flag_insc_esp2, flgs, kanji, ptr);
    }

    if (have_flag_of(flag_insc_sust, flgs))
        add_inscription(ptr, "(");

    ptr = inscribe_flags_aux(flag_insc_sust, flgs, kanji, ptr);
    *ptr = '\0';
    return ptr;
}

/*!
 * @brief オブジェクト名の特性短縮表記＋刻み内容を提示する。 / Get object inscription with auto inscription of object flags.
 * @param buff 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return なし
 */
static void get_inscription(player_type *player_ptr, char *buff, object_type *o_ptr)
{
    concptr insc = quark_str(o_ptr->inscription);
    char *ptr = buff;
    if (!object_is_fully_known(o_ptr)) {
        while (*insc) {
            if (*insc == '#')
                break;
#ifdef JP
            if (iskanji(*insc))
                *buff++ = *insc++;
#endif
            *buff++ = *insc++;
        }

        *buff = '\0';
        return;
    }

    *buff = '\0';
    for (; *insc; insc++) {
        if (*insc == '#')
            break;
        else if ('%' == *insc) {
            bool kanji = FALSE;
            bool all;
            concptr start = ptr;
            if (ptr >= buff + MAX_NLEN)
                continue;

#ifdef JP
            if ('%' == insc[1]) {
                insc++;
                kanji = FALSE;
            } else
                kanji = TRUE;
#endif

            if ('a' == insc[1] && 'l' == insc[2] && 'l' == insc[3]) {
                all = TRUE;
                insc += 3;
            } else
                all = FALSE;

            ptr = get_ability_abbreviation(player_ptr, ptr, o_ptr, kanji, all);
            if (ptr == start)
                add_inscription(ptr, " ");
        } else
            *ptr++ = *insc;
    }

    *ptr = '\0';
}

/*!
 * @brief オブジェクトの各表記を返すメイン関数 / Creates a description of the item "o_ptr", and stores it in "out_val".
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param buf 表記を返すための文字列参照ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @param mode 表記に関するオプション指定
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す
 */
void object_desc(player_type *player_ptr, char *buf, object_type *o_ptr, BIT_FLAGS mode)
{
    concptr kindname = k_name + k_info[o_ptr->k_idx].name;
    concptr basenm = kindname;
    concptr modstr = "";
    int power;
    int fire_rate;
    bool aware = FALSE;
    bool known = FALSE;
    bool flavor = TRUE;
    bool show_weapon = FALSE;
    bool show_armour = FALSE;
    concptr s, s0;
    char *t;
    char p1 = '(', p2 = ')';
    char b1 = '[', b2 = ']';
    char c1 = '{', c2 = '}';
    char tmp_val[MAX_NLEN + 160];
    char tmp_val2[MAX_NLEN + 10];
    char fake_insc_buf[30];
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_type *bow_ptr;
    object_kind *k_ptr = &k_info[o_ptr->k_idx];
    object_kind *flavor_k_ptr = &k_info[k_ptr->flavor];

    object_flags(player_ptr, o_ptr, flgs);
    if (object_is_aware(o_ptr))
        aware = TRUE;

    if (object_is_known(o_ptr))
        known = TRUE;

    if (aware && ((mode & OD_NO_FLAVOR) || plain_descriptions))
        flavor = FALSE;

    if ((mode & OD_STORE) || (o_ptr->ident & IDENT_STORE)) {
        flavor = FALSE;
        aware = TRUE;
        known = TRUE;
    }

    if (mode & OD_FORCE_FLAVOR) {
        aware = FALSE;
        flavor = TRUE;
        known = FALSE;
        flavor_k_ptr = k_ptr;
    }

    switch (o_ptr->tval) {
    case TV_SKELETON:
    case TV_BOTTLE:
    case TV_JUNK:
    case TV_SPIKE:
    case TV_FLASK:
    case TV_CHEST:
    case TV_WHISTLE:
        break;
    case TV_CAPTURE: {
        monster_race *r_ptr = &r_info[o_ptr->pval];
        if (known) {
            if (!o_ptr->pval) {
                modstr = _(" (空)", " (empty)");
            } else {
#ifdef JP
                sprintf(tmp_val2, " (%s)", r_name + r_ptr->name);
                modstr = tmp_val2;
#else
                t = r_name + r_ptr->name;

                if (!(r_ptr->flags1 & RF1_UNIQUE)) {
                    sprintf(tmp_val2, " (%s%s)", (is_a_vowel(*t) ? "an " : "a "), t);

                    modstr = tmp_val2;
                } else {
                    sprintf(tmp_val2, "(%s)", t);

                    modstr = t;
                }
#endif
            }
        }

        break;
    }
    case TV_FIGURINE:
    case TV_STATUE: {
        monster_race *r_ptr = &r_info[o_ptr->pval];
#ifdef JP
        modstr = r_name + r_ptr->name;
#else
        t = r_name + r_ptr->name;

        if (!(r_ptr->flags1 & RF1_UNIQUE)) {
            sprintf(tmp_val2, "%s%s", (is_a_vowel(*t) ? "an " : "a "), t);
            modstr = tmp_val2;
        } else
            modstr = t;
#endif

        break;
    }
    case TV_CORPSE: {
        monster_race *r_ptr = &r_info[o_ptr->pval];
        modstr = r_name + r_ptr->name;
#ifdef JP
        basenm = "#%";
#else
        if (r_ptr->flags1 & RF1_UNIQUE)
            basenm = "& % of #";
        else
            basenm = "& # %";
#endif

        break;
    }
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
    case TV_BOW:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_DIGGING: {
        show_weapon = TRUE;
        break;
    }
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR: {
        show_armour = TRUE;
        break;
    }
    case TV_LITE:
        break;
    case TV_AMULET: {
        if (aware) {
            if (object_is_fixed_artifact(o_ptr))
                break;
            if (k_ptr->gen_flags & TRG_INSTA_ART)
                break;
        }

        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%のアミュレット", "& Amulet~ of %");
        else if (aware)
            basenm = _("%の#アミュレット", "& # Amulet~ of %");
        else
            basenm = _("#アミュレット", "& # Amulet~");

        break;
    }
    case TV_RING: {
        if (aware) {
            if (object_is_fixed_artifact(o_ptr))
                break;
            if (k_ptr->gen_flags & TRG_INSTA_ART)
                break;
        }

        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%の指輪", "& Ring~ of %");
        else if (aware)
            basenm = _("%の#指輪", "& # Ring~ of %");
        else
            basenm = _("#指輪", "& # Ring~");

        if (!k_ptr->to_h && !k_ptr->to_d && (o_ptr->to_h || o_ptr->to_d))
            show_weapon = TRUE;

        break;
    }
    case TV_CARD:
        break;
    case TV_STAFF: {
        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%の杖", "& Staff~ of %");
        else if (aware)
            basenm = _("%の#杖", "& # Staff~ of %");
        else
            basenm = _("#杖", "& # Staff~");

        break;
    }
    case TV_WAND: {
        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%の魔法棒", "& Wand~ of %");
        else if (aware)
            basenm = _("%の#魔法棒", "& # Wand~ of %");
        else
            basenm = _("#魔法棒", "& # Wand~");

        break;
    }
    case TV_ROD: {
        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%のロッド", "& Rod~ of %");
        else if (aware)
            basenm = _("%の#ロッド", "& # Rod~ of %");
        else
            basenm = _("#ロッド", "& # Rod~");

        break;
    }
    case TV_SCROLL: {
        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%の巻物", "& Scroll~ of %");
        else if (aware)
            basenm = _("「#」と書かれた%の巻物", "& Scroll~ titled \"#\" of %");
        else
            basenm = _("「#」と書かれた巻物", "& Scroll~ titled \"#\"");

        break;
    }
    case TV_POTION: {
        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%の薬", "& Potion~ of %");
        else if (aware)
            basenm = _("%の#薬", "& # Potion~ of %");
        else
            basenm = _("#薬", "& # Potion~");

        break;
    }
    case TV_FOOD: {
        if (!k_ptr->flavor_name)
            break;

        modstr = k_name + flavor_k_ptr->flavor_name;
        if (!flavor)
            basenm = _("%のキノコ", "& Mushroom~ of %");
        else if (aware)
            basenm = _("%の#キノコ", "& # Mushroom~ of %");
        else
            basenm = _("#キノコ", "& # Mushroom~");

        break;
    }
    case TV_PARCHMENT: {
        basenm = _("羊皮紙 - %", "& Parchment~ - %");
        break;
    }
    case TV_LIFE_BOOK: {
#ifdef JP
        basenm = "生命の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Life Magic %";
        else
            basenm = "& Life Spellbook~ %";
#endif

        break;
    }
    case TV_SORCERY_BOOK: {
#ifdef JP
        basenm = "仙術の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Sorcery %";
        else
            basenm = "& Sorcery Spellbook~ %";
#endif

        break;
    }
    case TV_NATURE_BOOK: {
#ifdef JP
        basenm = "自然の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Nature Magic %";
        else
            basenm = "& Nature Spellbook~ %";
#endif

        break;
    }
    case TV_CHAOS_BOOK: {
#ifdef JP
        basenm = "カオスの魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Chaos Magic %";
        else
            basenm = "& Chaos Spellbook~ %";
#endif

        break;
    }
    case TV_DEATH_BOOK: {
#ifdef JP
        basenm = "暗黒の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Death Magic %";
        else
            basenm = "& Death Spellbook~ %";
#endif

        break;
    }
    case TV_TRUMP_BOOK: {
#ifdef JP
        basenm = "トランプの魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Trump Magic %";
        else
            basenm = "& Trump Spellbook~ %";
#endif

        break;
    }
    case TV_ARCANE_BOOK: {
#ifdef JP
        basenm = "秘術の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Arcane Magic %";
        else
            basenm = "& Arcane Spellbook~ %";
#endif

        break;
    }
    case TV_CRAFT_BOOK: {
#ifdef JP
        basenm = "匠の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Craft Magic %";
        else
            basenm = "& Craft Spellbook~ %";
#endif

        break;
    }
    case TV_DAEMON_BOOK: {
#ifdef JP
        basenm = "悪魔の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Daemon Magic %";
        else
            basenm = "& Daemon Spellbook~ %";
#endif

        break;
    }
    case TV_CRUSADE_BOOK: {
#ifdef JP
        basenm = "破邪の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Crusade Magic %";
        else
            basenm = "& Crusade Spellbook~ %";
#endif

        break;
    }
    case TV_MUSIC_BOOK: {
        basenm = _("歌集%", "& Song Book~ %");
        break;
    }
    case TV_HISSATSU_BOOK: {
        basenm = _("& 武芸の書%", "Book~ of Kendo %");
        break;
    }
    case TV_HEX_BOOK: {
#ifdef JP
        basenm = "呪術の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            basenm = "& Book~ of Hex Magic %";
        else
            basenm = "& Hex Spellbook~ %";
#endif

        break;
    }
    case TV_GOLD: {
        strcpy(buf, basenm);
        return;
    }
    default: {
        strcpy(buf, _("(なし)", "(nothing)"));
        return;
    }
    }

    if (aware && have_flag(flgs, TR_FULL_NAME)) {
        if (known && o_ptr->name1)
            basenm = a_name + a_info[o_ptr->name1].name;
        else
            basenm = kindname;
    }

    t = tmp_val;
#ifdef JP
    if (basenm[0] == '&')
        s = basenm + 2;
    else
        s = basenm;

    /* No prefix */
    if (mode & OD_OMIT_PREFIX) {
        /* Nothing */
    } else if (o_ptr->number > 1) {
        t = object_desc_kosuu(t, o_ptr);
        t = object_desc_str(t, "の ");
    }

    // 英語の場合アーティファクトは The が付くので分かるが、日本語では分からないのでマークをつける.
    if (known) {
        if (object_is_fixed_artifact(o_ptr))
            t = object_desc_str(t, "★");
        else if (o_ptr->art_name)
            t = object_desc_str(t, "☆");
    }
#else

    if (basenm[0] == '&') {
        s = basenm + 2;
        if (mode & OD_OMIT_PREFIX) {
            /* Nothing */
        } else if (o_ptr->number <= 0)
            t = object_desc_str(t, "no more ");
        else if (o_ptr->number > 1) {
            t = object_desc_num(t, o_ptr->number);
            t = object_desc_chr(t, ' ');
        } else if ((known && object_is_artifact(o_ptr)) || ((o_ptr->tval == TV_CORPSE) && (r_info[o_ptr->pval].flags1 & RF1_UNIQUE)))
            t = object_desc_str(t, "The ");
        else {
            bool vowel;
            switch (*s) {
            case '#':
                vowel = is_a_vowel(modstr[0]);
                break;
            case '%':
                vowel = is_a_vowel(*kindname);
                break;
            default:
                vowel = is_a_vowel(*s);
                break;
            }

            if (vowel)
                t = object_desc_str(t, "an ");
            else
                t = object_desc_str(t, "a ");
        }
    } else {
        s = basenm;
        if (mode & OD_OMIT_PREFIX) {
            /* Nothing */
        } else if (o_ptr->number <= 0)
            t = object_desc_str(t, "no more ");
        else if (o_ptr->number > 1) {
            t = object_desc_num(t, o_ptr->number);
            t = object_desc_chr(t, ' ');
        } else if (known && object_is_artifact(o_ptr))
            t = object_desc_str(t, "The ");
    }
#endif

#ifdef JP
    if (object_is_smith(player_ptr, o_ptr))
        t = object_desc_str(t, format("鍛冶師%sの", player_ptr->name));

    /* 伝説のアイテム、名のあるアイテムの名前を付加する */
    if (known) {
        /* ランダム・アーティファクト */
        if (o_ptr->art_name) {
            concptr temp = quark_str(o_ptr->art_name);

            /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
            /* 英語版のセーブファイルから来た 'of XXX' は,「XXXの」と表示する */
            if (strncmp(temp, "of ", 3) == 0) {
                t = object_desc_str(t, &temp[3]);
                t = object_desc_str(t, "の");
            } else if ((strncmp(temp, "『", 2) != 0) && (strncmp(temp, "《", 2) != 0) && (temp[0] != '\''))
                t = object_desc_str(t, temp);
        }
        /* 伝説のアイテム */
        else if (o_ptr->name1 && !have_flag(flgs, TR_FULL_NAME)) {
            artifact_type *a_ptr = &a_info[o_ptr->name1];
            /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
            if (strncmp(a_name + a_ptr->name, "『", 2) != 0) {
                t = object_desc_str(t, a_name + a_ptr->name);
            }
        }
        /* 名のあるアイテム */
        else if (object_is_ego(o_ptr)) {
            ego_item_type *e_ptr = &e_info[o_ptr->name2];
            t = object_desc_str(t, e_name + e_ptr->name);
        }
    }
#endif

    for (s0 = NULL; *s || s0;) {
        if (!*s) {
            s = s0 + 1;
            s0 = NULL;
        } else if ((*s == '#') && !s0) {
            s0 = s;
            s = modstr;
            modstr = "";
        } else if ((*s == '%') && !s0) {
            s0 = s;
            s = kindname;
            kindname = "";
        }

#ifdef JP
#else
        else if (*s == '~') {
            if (!(mode & OD_NO_PLURAL) && (o_ptr->number != 1)) {
                char k = t[-1];
                if ((k == 's') || (k == 'h'))
                    *t++ = 'e';

                *t++ = 's';
            }

            s++;
        }
#endif
        else
            *t++ = *s++;
    }

    *t = '\0';

#ifdef JP
    /* '『'から始まる伝説のアイテムの名前は最後に付加する */
    if (known) {
        // ランダムアーティファクトの名前はセーブファイルに記録されるので、英語版の名前もそれらしく変換する.
        if (o_ptr->art_name) {
            char temp[256];
            int itemp;
            strcpy(temp, quark_str(o_ptr->art_name));
            if (strncmp(temp, "『", 2) == 0 || strncmp(temp, "《", 2) == 0)
                t = object_desc_str(t, temp);
            else if (temp[0] == '\'') {
                itemp = strlen(temp);
                temp[itemp - 1] = 0;
                t = object_desc_str(t, "『");
                t = object_desc_str(t, &temp[1]);
                t = object_desc_str(t, "』");
            }
        } else if (object_is_fixed_artifact(o_ptr)) {
            artifact_type *a_ptr = &a_info[o_ptr->name1];
            if (strncmp(a_name + a_ptr->name, "『", 2) == 0)
                t = object_desc_str(t, a_name + a_ptr->name);
        } else if (o_ptr->inscription) {
            concptr str = quark_str(o_ptr->inscription);
            while (*str) {
                if (iskanji(*str)) {
                    str += 2;
                    continue;
                }

                if (*str == '#')
                    break;

                str++;
            }

            if (*str) {
                concptr str_aux = angband_strchr(quark_str(o_ptr->inscription), '#');
                t = object_desc_str(t, "『");
                t = object_desc_str(t, &str_aux[1]);
                t = object_desc_str(t, "』");
            }
        }
    }
#else
    if (object_is_smith(player_ptr, o_ptr))
        t = object_desc_str(t, format(" of %s the Smith", player_ptr->name));

    if (known && !have_flag(flgs, TR_FULL_NAME)) {
        if (o_ptr->art_name) {
            t = object_desc_chr(t, ' ');
            t = object_desc_str(t, quark_str(o_ptr->art_name));
        } else if (object_is_fixed_artifact(o_ptr)) {
            artifact_type *a_ptr = &a_info[o_ptr->name1];

            t = object_desc_chr(t, ' ');
            t = object_desc_str(t, a_name + a_ptr->name);
        } else {
            if (object_is_ego(o_ptr)) {
                ego_item_type *e_ptr = &e_info[o_ptr->name2];
                t = object_desc_chr(t, ' ');
                t = object_desc_str(t, e_name + e_ptr->name);
            }

            if (o_ptr->inscription && angband_strchr(quark_str(o_ptr->inscription), '#')) {
                concptr str = angband_strchr(quark_str(o_ptr->inscription), '#');
                t = object_desc_chr(t, ' ');
                t = object_desc_str(t, &str[1]);
            }
        }
    }
#endif

    if (mode & OD_NAME_ONLY) {
        angband_strcpy(buf, tmp_val, MAX_NLEN);
        return;
    }

    if (o_ptr->tval == TV_CHEST) {
        if (!known) {
            /* Nothing */
        } else if (!o_ptr->pval)
            t = object_desc_str(t, _("(空)", " (empty)"));
        else if (o_ptr->pval < 0)
            if (chest_traps[0 - o_ptr->pval])
                t = object_desc_str(t, _("(解除済)", " (disarmed)"));
            else
                t = object_desc_str(t, _("(非施錠)", " (unlocked)"));
        else {
            switch (chest_traps[o_ptr->pval]) {
            case 0: {
                t = object_desc_str(t, _("(施錠)", " (Locked)"));
                break;
            }
            case CHEST_LOSE_STR: {
                t = object_desc_str(t, _("(毒針)", " (Poison Needle)"));
                break;
            }
            case CHEST_LOSE_CON: {
                t = object_desc_str(t, _("(毒針)", " (Poison Needle)"));
                break;
            }
            case CHEST_POISON: {
                t = object_desc_str(t, _("(ガス・トラップ)", " (Gas Trap)"));
                break;
            }
            case CHEST_PARALYZE: {
                t = object_desc_str(t, _("(ガス・トラップ)", " (Gas Trap)"));
                break;
            }
            case CHEST_EXPLODE: {
                t = object_desc_str(t, _("(爆発装置)", " (Explosion Device)"));
                break;
            }
            case CHEST_SUMMON:
            case CHEST_BIRD_STORM:
            case CHEST_E_SUMMON:
            case CHEST_H_SUMMON: {
                t = object_desc_str(t, _("(召喚のルーン)", " (Summoning Runes)"));
                break;
            }
            case CHEST_RUNES_OF_EVIL: {
                t = object_desc_str(t, _("(邪悪なルーン)", " (Gleaming Black Runes)"));
                break;
            }
            case CHEST_ALARM: {
                t = object_desc_str(t, _("(警報装置)", " (Alarm)"));
                break;
            }
            default: {
                t = object_desc_str(t, _("(マルチ・トラップ)", " (Multiple Traps)"));
                break;
            }
            }
        }
    }

    if (have_flag(flgs, TR_SHOW_MODS))
        show_weapon = TRUE;

    if (object_is_smith(player_ptr, o_ptr) && (o_ptr->xtra3 == 1 + ESSENCE_SLAY_GLOVE))
        show_weapon = TRUE;

    if (o_ptr->to_h && o_ptr->to_d)
        show_weapon = TRUE;

    if (o_ptr->ac)
        show_armour = TRUE;

    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_DIGGING: {
        if (object_is_quest_target(player_ptr, o_ptr) && !known)
            break;

        t = object_desc_chr(t, ' ');
        t = object_desc_chr(t, p1);
        t = object_desc_num(t, o_ptr->dd);
        t = object_desc_chr(t, 'd');
        t = object_desc_num(t, o_ptr->ds);
        t = object_desc_chr(t, p2);
        break;
    }
    case TV_BOW: {
        power = bow_tmul(o_ptr->sval);
        if (have_flag(flgs, TR_XTRA_MIGHT))
            power++;

        t = object_desc_chr(t, ' ');
        t = object_desc_chr(t, p1);
        t = object_desc_chr(t, 'x');
        t = object_desc_num(t, power);
        t = object_desc_chr(t, p2);
        fire_rate = calc_num_fire(player_ptr, o_ptr);
        if (fire_rate != 0 && power > 0 && known) {
            fire_rate = bow_energy(o_ptr->sval) / fire_rate;
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, p1);
            t = object_desc_num(t, fire_rate / 100);
            t = object_desc_chr(t, '.');
            t = object_desc_num(t, fire_rate % 100);
            t = object_desc_str(t, "turn");
            t = object_desc_chr(t, p2);
        }

        break;
    }
    }

    if (known) {
        if (show_weapon) {
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, p1);
            t = object_desc_int(t, o_ptr->to_h);
            t = object_desc_chr(t, ',');
            t = object_desc_int(t, o_ptr->to_d);
            t = object_desc_chr(t, p2);
        } else if (o_ptr->to_h) {
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, p1);
            t = object_desc_int(t, o_ptr->to_h);
            t = object_desc_chr(t, p2);
        } else if (o_ptr->to_d) {
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, p1);
            t = object_desc_int(t, o_ptr->to_d);
            t = object_desc_chr(t, p2);
        }
    }

    bow_ptr = &player_ptr->inventory_list[INVEN_BOW];
    if (bow_ptr->k_idx && (o_ptr->tval == player_ptr->tval_ammo)) {
        int avgdam = o_ptr->dd * (o_ptr->ds + 1) * 10 / 2;
        int tmul = bow_tmul(bow_ptr->sval);
        ENERGY energy_fire = bow_energy(bow_ptr->sval);
        if (object_is_known(bow_ptr))
            avgdam += (bow_ptr->to_d * 10);

        if (known)
            avgdam += (o_ptr->to_d * 10);

        if (player_ptr->xtra_might)
            tmul++;

        tmul = tmul * (100 + (int)(adj_str_td[player_ptr->stat_ind[A_STR]]) - 128);
        avgdam *= tmul;
        avgdam /= (100 * 10);
        if (player_ptr->concent)
            avgdam = boost_concentration_damage(player_ptr, avgdam);

        if (avgdam < 0)
            avgdam = 0;

        t = object_desc_chr(t, ' ');
        t = object_desc_chr(t, p1);
        if (show_ammo_no_crit) {
            t = object_desc_num(t, avgdam);
            t = object_desc_str(t, show_ammo_detail ? "/shot " : "/");
        }

        avgdam = calc_expect_crit_shot(player_ptr, o_ptr->weight, o_ptr->to_h, bow_ptr->to_h, avgdam);
        t = object_desc_num(t, avgdam);
        t = show_ammo_no_crit ? object_desc_str(t, show_ammo_detail ? "/crit " : "/") : object_desc_str(t, show_ammo_detail ? "/shot " : "/");
        if (player_ptr->num_fire == 0)
            t = object_desc_chr(t, '0');
        else {
            avgdam *= (player_ptr->num_fire * 100);
            avgdam /= energy_fire;
            t = object_desc_num(t, avgdam);
            t = object_desc_str(t, show_ammo_detail ? "/turn" : "");
            if (show_ammo_crit_ratio) {
                int percent = calc_crit_ratio_shot(player_ptr, known ? o_ptr->to_h : 0, known ? bow_ptr->to_h : 0);
                t = object_desc_chr(t, '/');
                t = object_desc_num(t, percent / 100);
                t = object_desc_chr(t, '.');
                if (percent % 100 < 10)
                    t = object_desc_chr(t, '0');

                t = object_desc_num(t, percent % 100);
                t = object_desc_str(t, show_ammo_detail ? "% crit" : "%");
            }
        }

        t = object_desc_chr(t, p2);
    } else if ((player_ptr->pclass == CLASS_NINJA) && (o_ptr->tval == TV_SPIKE)) {
        int avgdam = player_ptr->mighty_throw ? (1 + 3) : 1;
        s16b energy_fire = 100 - player_ptr->lev;
        avgdam += ((player_ptr->lev + 30) * (player_ptr->lev + 30) - 900) / 55;
        t = object_desc_chr(t, ' ');
        t = object_desc_chr(t, p1);
        t = object_desc_num(t, avgdam);
        t = object_desc_chr(t, '/');
        avgdam = 100 * avgdam / energy_fire;
        t = object_desc_num(t, avgdam);
        t = object_desc_chr(t, p2);
    }

    if (known) {
        if (show_armour) {
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, b1);
            t = object_desc_num(t, o_ptr->ac);
            t = object_desc_chr(t, ',');
            t = object_desc_int(t, o_ptr->to_a);
            t = object_desc_chr(t, b2);
        } else if (o_ptr->to_a) {
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, b1);
            t = object_desc_int(t, o_ptr->to_a);
            t = object_desc_chr(t, b2);
        }
    } else if (show_armour) {
        t = object_desc_chr(t, ' ');
        t = object_desc_chr(t, b1);
        t = object_desc_num(t, o_ptr->ac);
        t = object_desc_chr(t, b2);
    }

    if (mode & OD_NAME_AND_ENCHANT) {
        angband_strcpy(buf, tmp_val, MAX_NLEN);
        return;
    }

    if (known) {
        if (((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND))) {
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, p1);
            if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1)) {
                t = object_desc_num(t, o_ptr->number);
                t = object_desc_str(t, "x ");
            }

            t = object_desc_num(t, o_ptr->pval);
            t = object_desc_str(t, _("回分", " charge"));
#ifdef JP
#else
            if (o_ptr->pval != 1)
                t = object_desc_chr(t, 's');
#endif

            t = object_desc_chr(t, p2);
        } else if (o_ptr->tval == TV_ROD) {
            if (o_ptr->timeout) {
                if (o_ptr->number > 1) {
                    if (k_ptr->pval == 0)
                        k_ptr->pval = 1;

                    power = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
                    if (power > o_ptr->number)
                        power = o_ptr->number;

                    t = object_desc_str(t, " (");
                    t = object_desc_num(t, power);
                    t = object_desc_str(t, _("本 充填中)", " charging)"));
                } else
                    t = object_desc_str(t, _("(充填中)", " (charging)"));
            }
        }

        if (have_pval_flags(flgs)) {
            t = object_desc_chr(t, ' ');
            t = object_desc_chr(t, p1);
            t = object_desc_int(t, o_ptr->pval);
            if (have_flag(flgs, TR_HIDE_TYPE)) {
                /* Nothing */
            } else if (have_flag(flgs, TR_SPEED))
                t = object_desc_str(t, _("加速", " to speed"));
            else if (have_flag(flgs, TR_BLOWS)) {
                t = object_desc_str(t, _("攻撃", " attack"));
#ifdef JP
#else
                if (ABS(o_ptr->pval) != 1)
                    t = object_desc_chr(t, 's');
#endif
            } else if (have_flag(flgs, TR_STEALTH))
                t = object_desc_str(t, _("隠密", " to stealth"));
            else if (have_flag(flgs, TR_SEARCH))
                t = object_desc_str(t, _("探索", " to searching"));
            else if (have_flag(flgs, TR_INFRA))
                t = object_desc_str(t, _("赤外線視力", " to infravision"));

            t = object_desc_chr(t, p2);
        }

        if ((o_ptr->tval == TV_LITE) && (!(object_is_fixed_artifact(o_ptr) || (o_ptr->sval == SV_LITE_FEANOR)))) {
            t = object_desc_str(t, _("(", " (with "));
            if (o_ptr->name2 == EGO_LITE_LONG)
                t = object_desc_num(t, o_ptr->xtra4 * 2);
            else
                t = object_desc_num(t, o_ptr->xtra4);

            t = object_desc_str(t, _("ターンの寿命)", " turns of light)"));
        }

        if (o_ptr->timeout && (o_ptr->tval != TV_ROD))
            t = object_desc_str(t, _("(充填中)", " (charging)"));
    }

    if (mode & OD_OMIT_INSCRIPTION) {
        angband_strcpy(buf, tmp_val, MAX_NLEN);
        return;
    }

    tmp_val2[0] = '\0';
    if ((abbrev_extra || abbrev_all) && object_is_fully_known(o_ptr)) {
        if (!o_ptr->inscription || !angband_strchr(quark_str(o_ptr->inscription), '%')) {
            bool kanji, all;
            kanji = _(TRUE, FALSE);
            all = abbrev_all;
            get_ability_abbreviation(player_ptr, tmp_val2, o_ptr, kanji, all);
        }
    }

    if (o_ptr->inscription) {
        char buff[1024];
        if (tmp_val2[0])
            strcat(tmp_val2, ", ");

        get_inscription(player_ptr, buff, o_ptr);
        angband_strcat(tmp_val2, buff, sizeof(tmp_val2));
    }

    fake_insc_buf[0] = '\0';
    if (o_ptr->feeling)
        strcpy(fake_insc_buf, game_inscriptions[o_ptr->feeling]);
    else if (object_is_cursed(o_ptr) && (known || (o_ptr->ident & IDENT_SENSE)))
        strcpy(fake_insc_buf, _("呪われている", "cursed"));
    else if (((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET) || (o_ptr->tval == TV_LITE) || (o_ptr->tval == TV_FIGURINE)) && aware && !known
        && !(o_ptr->ident & IDENT_SENSE))
        strcpy(fake_insc_buf, _("未鑑定", "unidentified"));
    else if (!known && (o_ptr->ident & IDENT_EMPTY))
        strcpy(fake_insc_buf, _("空", "empty"));
    else if (!aware && object_is_tried(o_ptr))
        strcpy(fake_insc_buf, _("未判明", "tried"));

    if (o_ptr->discount) {
        if (!tmp_val2[0] || (o_ptr->ident & IDENT_STORE)) {
            char discount_num_buf[4];
            if (fake_insc_buf[0])
                strcat(fake_insc_buf, ", ");

            (void)object_desc_num(discount_num_buf, o_ptr->discount);
            strcat(fake_insc_buf, discount_num_buf);
            strcat(fake_insc_buf, _("%引き", "% off"));
        }
    }

    if (fake_insc_buf[0] || tmp_val2[0]) {
        t = object_desc_chr(t, ' ');
        t = object_desc_chr(t, c1);
        if (fake_insc_buf[0])
            t = object_desc_str(t, fake_insc_buf);

        if (fake_insc_buf[0] && tmp_val2[0]) {
            t = object_desc_chr(t, ',');
            t = object_desc_chr(t, ' ');
        }

        if (tmp_val2[0])
            t = object_desc_str(t, tmp_val2);

        t = object_desc_chr(t, c2);
    }

    angband_strcpy(buf, tmp_val, MAX_NLEN);
}

/*!
 * @brief nameバッファ内からベースアイテム名を返す / Strip an "object name" into a buffer
 * @param buf ベースアイテム格納先の参照ポインタ
 * @param k_idx ベースアイテムID
 * @return なし
 */
void strip_name(char *buf, KIND_OBJECT_IDX k_idx)
{
    object_kind *k_ptr = &k_info[k_idx];
    concptr str = (k_name + k_ptr->name);
    while ((*str == ' ') || (*str == '&'))
        str++;

    char *t;
    for (t = buf; *str; str++) {
#ifdef JP
        if (iskanji(*str)) {
            *t++ = *str++;
            *t++ = *str;
            continue;
        }
#endif
        if (*str != '~')
            *t++ = *str;
    }

    *t = '\0';
}

bool has_lite_flag(BIT_FLAGS *flags) { return have_flag(flags, TR_LITE_1) || have_flag(flags, TR_LITE_2) || have_flag(flags, TR_LITE_3); }

bool has_dark_flag(BIT_FLAGS *flags) { return have_flag(flags, TR_LITE_M1) || have_flag(flags, TR_LITE_M2) || have_flag(flags, TR_LITE_M3); }
