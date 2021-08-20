#include "flavor/flavor-util.h"
#include "flavor/flag-inscriptions-table.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-enchant.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-food-types.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"

flavor_type *initialize_flavor_type(flavor_type *flavor_ptr, char *buf, object_type *o_ptr, BIT_FLAGS mode)
{
    flavor_ptr->buf = buf;
    flavor_ptr->o_ptr = o_ptr;
    flavor_ptr->mode = mode;
    flavor_ptr->kindname = k_info[o_ptr->k_idx].name.c_str();
    flavor_ptr->basenm = flavor_ptr->kindname;
    flavor_ptr->modstr = "";
    flavor_ptr->aware = false;
    flavor_ptr->known = false;
    flavor_ptr->flavor = true;
    flavor_ptr->show_weapon = false;
    flavor_ptr->show_armour = false;
    flavor_ptr->p1 = '(';
    flavor_ptr->p2 = ')';
    flavor_ptr->b1 = '[';
    flavor_ptr->b2 = ']';
    flavor_ptr->c1 = '{';
    flavor_ptr->c2 = '}';
    flavor_ptr->k_ptr = &k_info[o_ptr->k_idx];
    flavor_ptr->flavor_k_ptr = &k_info[flavor_ptr->k_ptr->flavor];
    return flavor_ptr;
}

/*!
 * @brief 対象文字配列に一文字だけをコピーする。
 * @param t 保管先文字列ポインタ
 * @param c 保管したい1文字
 * @details
 * Print a char "c" into a string "t", as if by sprintf(t, "%c", c),\n
 * and return a pointer to the terminator (t + 1).\n
 */
char *object_desc_chr(char *t, char c)
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
char *object_desc_str(char *t, concptr s)
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
 * @details
 * Print an unsigned number "n" into a string "t", actually by
 * sprintf(t, "%u", n), and return a pointer to the terminator.
 */
char *object_desc_num(char *t, uint n)
{
    int ret = sprintf(t, "%u", n);
    if (ret < 0) {
        // error
        ret = 0;
        *t = '\0';
    }
    return t + ret;
}

/*!
 * @brief 対象文字配列に符号あり整数値をコピーする。
 * @param t 保管先文字列ポインタ
 * @param v コピーしたい数値
 * @details
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
char *object_desc_int(char *t, int v)
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
 * @param short_flavor フラグの短縮表現 (反魔法/アンチテレポの"["、耐性の"r"、耐混乱の"乱" 等)
 * @param ptr 特性短縮表記を格納する文字列ポインタ
 */
static void add_inscription(char **short_flavor, concptr str) { *short_flavor = object_desc_str(*short_flavor, str); }

/*!
 * @brief get_inscriptionのサブセットとしてオブジェクトの特性フラグを返す / Helper function for get_inscription()
 * @param fi_vec 参照する特性表示記号テーブル
 * @param flgs 対応するオブジェクトのフラグ文字列
 * @param kanji TRUEならば漢字記述/FALSEならば英語記述
 * @param ptr フラグ群を保管する文字列参照ポインタ
 * @return フラグ群を保管する文字列参照ポインタ(ptrと同じ)
 * @details
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *inscribe_flags_aux(std::vector<flag_insc_table> &fi_vec, const TrFlags &flgs, bool kanji, char *ptr)
{
#ifdef JP
#else
    (void)kanji;
#endif

    for (flag_insc_table &fi : fi_vec)
        if (has_flag(flgs, fi.flag) && (fi.except_flag == -1 || !has_flag(flgs, fi.except_flag)))
            add_inscription(&ptr, _(kanji ? fi.japanese : fi.english, fi.english));

    return ptr;
}

/*!
 * @brief オブジェクトの特性表示記号テーブル1つに従いオブジェクトの特性フラグ配列に1つでも該当の特性があるかを返す / Special variation of has_flag for
 * auto-inscription
 * @param fi_vec 参照する特性表示記号テーブル
 * @param flgs 対応するオブジェクトのフラグ文字列
 * @return 1つでも該当の特性があったらTRUEを返す。
 */
static bool has_flag_of(std::vector<flag_insc_table> &fi_vec, const TrFlags &flgs)
{
    for (flag_insc_table &fi : fi_vec)
        if (has_flag(flgs, fi.flag) && (fi.except_flag == -1 || !has_flag(flgs, fi.except_flag)))
            return true;

    return false;
}

/*!
 * @brief オブジェクト名の特性短縮表記をまとめて提示する。
 * @param short_flavor 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @param kanji TRUEならば漢字表記 / FALSEなら英語表記
 * @param all TRUEならばベースアイテム上で明らかなフラグは省略する
 * @return ptrと同じアドレス
 */
char *get_ability_abbreviation(player_type *player_ptr, char *short_flavor, object_type *o_ptr, bool kanji, bool all)
{
    char *prev_ptr = short_flavor;
    TrFlags flgs;
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
        if (has_flag(flgs, TR_LITE_1))
            remove_flag(flgs, TR_LITE_1);

        if (has_flag(flgs, TR_LITE_2))
            remove_flag(flgs, TR_LITE_2);

        if (has_flag(flgs, TR_LITE_3))
            remove_flag(flgs, TR_LITE_3);
    } else if (has_lite_flag(flgs)) {
        add_flag(flgs, TR_LITE_1);
        if (has_flag(flgs, TR_LITE_2))
            remove_flag(flgs, TR_LITE_2);

        if (has_flag(flgs, TR_LITE_3))
            remove_flag(flgs, TR_LITE_3);
    }

    if (has_flag_of(flag_insc_plus, flgs) && kanji)
        add_inscription(&short_flavor, "+");

    short_flavor = inscribe_flags_aux(flag_insc_plus, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_immune, flgs)) {
        if (!kanji && short_flavor != prev_ptr) {
            add_inscription(&short_flavor, ";");
            prev_ptr = short_flavor;
        }

        add_inscription(&short_flavor, "*");
    }

    short_flavor = inscribe_flags_aux(flag_insc_immune, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_vuln, flgs)) {
        if (!kanji && short_flavor != prev_ptr) {
            add_inscription(&short_flavor, ";");
            prev_ptr = short_flavor;
        }

        add_inscription(&short_flavor, "v");
    }

    short_flavor = inscribe_flags_aux(flag_insc_vuln, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_resistance, flgs)) {
        if (kanji)
            add_inscription(&short_flavor, "r");
        else if (short_flavor != prev_ptr) {
            add_inscription(&short_flavor, ";");
            prev_ptr = short_flavor;
        }
    }

    short_flavor = inscribe_flags_aux(flag_insc_resistance, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_misc, flgs) && (short_flavor != prev_ptr)) {
        add_inscription(&short_flavor, ";");
        prev_ptr = short_flavor;
    }

    short_flavor = inscribe_flags_aux(flag_insc_misc, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_aura, flgs))
        add_inscription(&short_flavor, "[");

    short_flavor = inscribe_flags_aux(flag_insc_aura, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_brand, flgs))
        add_inscription(&short_flavor, "|");

    short_flavor = inscribe_flags_aux(flag_insc_brand, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_kill, flgs))
        add_inscription(&short_flavor, "/X");

    short_flavor = inscribe_flags_aux(flag_insc_kill, flgs, kanji, short_flavor);

    if (has_flag_of(flag_insc_slay, flgs))
        add_inscription(&short_flavor, "/");

    short_flavor = inscribe_flags_aux(flag_insc_slay, flgs, kanji, short_flavor);

    if (kanji) {
        if (has_flag_of(flag_insc_esp1, flgs) || has_flag_of(flag_insc_esp2, flgs))
            add_inscription(&short_flavor, "~");

        short_flavor = inscribe_flags_aux(flag_insc_esp1, flgs, kanji, short_flavor);
        short_flavor = inscribe_flags_aux(flag_insc_esp2, flgs, kanji, short_flavor);
    } else {
        if (has_flag_of(flag_insc_esp1, flgs))
            add_inscription(&short_flavor, "~");

        short_flavor = inscribe_flags_aux(flag_insc_esp1, flgs, kanji, short_flavor);

        if (has_flag_of(flag_insc_esp2, flgs))
            add_inscription(&short_flavor, "~");

        short_flavor = inscribe_flags_aux(flag_insc_esp2, flgs, kanji, short_flavor);
    }

    if (has_flag_of(flag_insc_sust, flgs))
        add_inscription(&short_flavor, "(");

    short_flavor = inscribe_flags_aux(flag_insc_sust, flgs, kanji, short_flavor);
    *short_flavor = '\0';
    return short_flavor;
}

/*!
 * @brief オブジェクト名の特性短縮表記＋刻み内容を提示する。 / Get object inscription with auto inscription of object flags.
 * @param buff 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 */
void get_inscription(player_type *player_ptr, char *buff, object_type *o_ptr)
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
            bool kanji = false;
            bool all;
            concptr start = ptr;
            if (ptr >= buff + MAX_NLEN)
                continue;

#ifdef JP
            if ('%' == insc[1]) {
                insc++;
                kanji = false;
            } else
                kanji = true;
#endif

            if ('a' == insc[1] && 'l' == insc[2] && 'l' == insc[3]) {
                all = true;
                insc += 3;
            } else
                all = false;

            ptr = get_ability_abbreviation(player_ptr, ptr, o_ptr, kanji, all);
            if (ptr == start)
                add_inscription(&ptr, " ");
        } else
            *ptr++ = *insc;
    }

    *ptr = '\0';
}

#ifdef JP
/*!
 * @brief 日本語の個数表示ルーチン
 * @param t 保管先文字列ポインタ
 * @param o_ptr 記述したいオブジェクトの構造体参照ポインタ
 * @details
 * cmd1.c で流用するために object_desc_japanese から移動した。
 */
char *object_desc_count_japanese(char *t, object_type *o_ptr)
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
    case TV_DEMON_BOOK:
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

bool has_lite_flag(const TrFlags &flags)
{
    return has_flag(flags, TR_LITE_1) || has_flag(flags, TR_LITE_2) || has_flag(flags, TR_LITE_3);
}

bool has_dark_flag(const TrFlags &flags)
{
    return has_flag(flags, TR_LITE_M1) || has_flag(flags, TR_LITE_M2) || has_flag(flags, TR_LITE_M3);
}
