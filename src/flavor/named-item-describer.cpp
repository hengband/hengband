#include "flavor/named-item-describer.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "flavor/tval-description-switcher.h"
#include "game-option/text-display-options.h"
#include "locale/english.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#ifdef JP
#else
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "system/monster-race-definition.h"
#endif

static void check_object_known_aware(flavor_type *flavor_ptr)
{
    flavor_ptr->tr_flags = object_flags(flavor_ptr->o_ptr);
    if (flavor_ptr->o_ptr->is_aware())
        flavor_ptr->aware = true;

    if (flavor_ptr->o_ptr->is_known())
        flavor_ptr->known = true;

    if (flavor_ptr->aware && ((flavor_ptr->mode & OD_NO_FLAVOR) || plain_descriptions))
        flavor_ptr->flavor = false;

    if ((flavor_ptr->mode & OD_STORE) || (flavor_ptr->o_ptr->ident & IDENT_STORE)) {
        flavor_ptr->flavor = false;
        flavor_ptr->aware = true;
        flavor_ptr->known = true;
    }

    if (flavor_ptr->mode & OD_FORCE_FLAVOR) {
        flavor_ptr->aware = false;
        flavor_ptr->flavor = true;
        flavor_ptr->known = false;
        flavor_ptr->flavor_k_ptr = flavor_ptr->k_ptr;
    }
}

static void set_base_name(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->aware || flavor_ptr->tr_flags.has_not(TR_FULL_NAME))
        return;

    flavor_ptr->basenm = (flavor_ptr->known && (flavor_ptr->o_ptr->name1 != 0)) ? a_info[flavor_ptr->o_ptr->name1].name.c_str() : flavor_ptr->kindname;
}

#ifdef JP
static void describe_prefix_ja(flavor_type *flavor_ptr)
{
    flavor_ptr->s = flavor_ptr->basenm[0] == '&' ? flavor_ptr->basenm + 2 : flavor_ptr->basenm;
    if (flavor_ptr->mode & OD_OMIT_PREFIX)
        return;

    if (flavor_ptr->o_ptr->number > 1) {
        flavor_ptr->t = object_desc_count_japanese(flavor_ptr->t, flavor_ptr->o_ptr);
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "の ");
    }
}

/*!
 * @brief アーティファクトの表記処理
 * @param アイテム表記への参照ポインタ
 * @details 英語の場合アーティファクトは The が付くので分かるが、日本語では分からないのでマークをつける.
 */
static void describe_artifact_prefix_ja(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->known)
        return;

    if (flavor_ptr->o_ptr->is_fixed_artifact())
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "★");
    else if (flavor_ptr->o_ptr->art_name)
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "☆");
}

/*!
 * @brief アーティファクトの説明表記
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @details ランダムアーティファクト、固定アーティファクト、エゴの順に評価する
 */
static void describe_artifact_ja(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->known)
        return;

    if (flavor_ptr->o_ptr->art_name) {
        concptr temp = quark_str(flavor_ptr->o_ptr->art_name);

        /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
        /* 英語版のセーブファイルから来た 'of XXX' は,「XXXの」と表示する */
        if (strncmp(temp, "of ", 3) == 0) {
            flavor_ptr->t = object_desc_str(flavor_ptr->t, &temp[3]);
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "の");
        } else if ((strncmp(temp, "『", 2) != 0) && (strncmp(temp, "《", 2) != 0) && (temp[0] != '\''))
            flavor_ptr->t = object_desc_str(flavor_ptr->t, temp);

        return;
    }

    if (flavor_ptr->o_ptr->name1 && flavor_ptr->tr_flags.has_not(TR_FULL_NAME)) {
        artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];
        /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
        if (a_ptr->name.find("『", 0, 2) != 0)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, a_ptr->name.c_str());

        return;
    }

    if (flavor_ptr->o_ptr->is_ego()) {
        ego_item_type *e_ptr = &e_info[flavor_ptr->o_ptr->name2];
        flavor_ptr->t = object_desc_str(flavor_ptr->t, e_ptr->name.c_str());
    }
}

/*!
 * @brief ランダムアーティファクトの表記
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @return ランダムアーティファクトならTRUE、違うならFALSE
 * @details ランダムアーティファクトの名前はセーブファイルに記録されるので、英語版の名前もそれらしく変換する.
 */
static bool describe_random_artifact_body_ja(flavor_type *flavor_ptr)
{
    if (flavor_ptr->o_ptr->art_name == 0)
        return false;

    char temp[256];
    int itemp;
    strcpy(temp, quark_str(flavor_ptr->o_ptr->art_name));
    if (strncmp(temp, "『", 2) == 0 || strncmp(temp, "《", 2) == 0) {
        flavor_ptr->t = object_desc_str(flavor_ptr->t, temp);
        return true;
    }

    if (temp[0] != '\'')
        return true;

    itemp = strlen(temp);
    temp[itemp - 1] = 0;
    flavor_ptr->t = object_desc_str(flavor_ptr->t, "『");
    flavor_ptr->t = object_desc_str(flavor_ptr->t, &temp[1]);
    flavor_ptr->t = object_desc_str(flavor_ptr->t, "』");
    return true;
}

static void describe_ego_body_ja(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->o_ptr->inscription)
        return;

    concptr str = quark_str(flavor_ptr->o_ptr->inscription);
    while (*str) {
        if (iskanji(*str)) {
            str += 2;
            continue;
        }

        if (*str == '#')
            break;

        str++;
    }

    if (*str == '\0')
        return;

    concptr str_aux = angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '#');
    flavor_ptr->t = object_desc_str(flavor_ptr->t, "『");
    flavor_ptr->t = object_desc_str(flavor_ptr->t, &str_aux[1]);
    flavor_ptr->t = object_desc_str(flavor_ptr->t, "』");
}

/*!
 * @brief アーティファクトのアイテム名を表記する
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @details '『'から始まる伝説のアイテムの名前は最後に付加する
 */
static void describe_artifact_body_ja(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->known)
        return;

    if (describe_random_artifact_body_ja(flavor_ptr))
        return;

    if (flavor_ptr->o_ptr->is_fixed_artifact()) {
        artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];
        if (a_ptr->name.find("『", 0, 2) == 0)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, a_ptr->name.c_str());

        return;
    }

    describe_ego_body_ja(flavor_ptr);
}
#else

static void describe_vowel(flavor_type *flavor_ptr)
{
    bool vowel;
    switch (*flavor_ptr->s) {
    case '#':
        vowel = is_a_vowel(flavor_ptr->modstr[0]);
        break;
    case '%':
        vowel = is_a_vowel(*flavor_ptr->kindname);
        break;
    default:
        vowel = is_a_vowel(*flavor_ptr->s);
        break;
    }

    if (vowel)
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "an ");
    else
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "a ");
}

/*!
 * @brief 0個、1個、2個以上の時に個数を書き分ける処理 / Process to write the number when there are 0, 1, or 2 or more.
 * @param flavor_ptr アイテム表記への参照ポインタ / Reference pointer to item's flavor
 * @return 1個ならFALSE、0または2個以上ならTRUE / If the number of items is 1, then FALE is returned, and if 0 or 2 or more, then TRUE is returned
 * @details 1個なら後続処理実行 / If the number of items is 1, then the continuous process will be run.
 */
static bool describe_prefix_en(flavor_type *flavor_ptr)
{
    if (flavor_ptr->o_ptr->number <= 0) {
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "no more ");
        return true;
    }

    if (flavor_ptr->o_ptr->number == 1)
        return false;

    flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->number);
    flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
    return true;
}

static void describe_artifact_prefix_en(flavor_type *flavor_ptr)
{
    flavor_ptr->s = flavor_ptr->basenm + 2;
    if (flavor_ptr->mode & OD_OMIT_PREFIX)
        return;

    if (describe_prefix_en(flavor_ptr))
        return;

    if ((flavor_ptr->known && flavor_ptr->o_ptr->is_artifact()) || ((flavor_ptr->o_ptr->tval == ItemKindType::CORPSE) && (r_info[flavor_ptr->o_ptr->pval].flags1 & RF1_UNIQUE))) {
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "The ");
        return;
    }

    describe_vowel(flavor_ptr);
}

static void describe_basename_en(flavor_type *flavor_ptr)
{
    flavor_ptr->s = flavor_ptr->basenm;
    if (flavor_ptr->mode & OD_OMIT_PREFIX)
        return;

    if (describe_prefix_en(flavor_ptr))
        return;

    if (flavor_ptr->known && flavor_ptr->o_ptr->is_artifact())
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "The ");
}

static void describe_artifact_body_en(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->known || flavor_ptr->tr_flags.has(TR_FULL_NAME))
        return;

    if (flavor_ptr->o_ptr->art_name) {
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_str(flavor_ptr->t, quark_str(flavor_ptr->o_ptr->art_name));
        return;
    }

    if (flavor_ptr->o_ptr->is_fixed_artifact()) {
        artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_str(flavor_ptr->t, a_ptr->name.c_str());
        return;
    }

    if (flavor_ptr->o_ptr->is_ego()) {
        ego_item_type *e_ptr = &e_info[flavor_ptr->o_ptr->name2];
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_str(flavor_ptr->t, e_ptr->name.c_str());
    }

    if (flavor_ptr->o_ptr->inscription && angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '#')) {
        concptr str = angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '#');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_str(flavor_ptr->t, &str[1]);
    }
}
#endif

/*!
 * @brief 銘を表記する
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @details ランダムアーティファクト、固定アーティファクト、エゴの順に評価する
 */
static void describe_inscription(flavor_type *flavor_ptr)
{
    for (flavor_ptr->s0 = nullptr; *flavor_ptr->s || flavor_ptr->s0;) {
        if (!*flavor_ptr->s) {
            flavor_ptr->s = flavor_ptr->s0 + 1;
            flavor_ptr->s0 = nullptr;
        } else if ((*flavor_ptr->s == '#') && !flavor_ptr->s0) {
            flavor_ptr->s0 = flavor_ptr->s;
            flavor_ptr->s = flavor_ptr->modstr;
            flavor_ptr->modstr = "";
        } else if ((*flavor_ptr->s == '%') && !flavor_ptr->s0) {
            flavor_ptr->s0 = flavor_ptr->s;
            flavor_ptr->s = flavor_ptr->kindname;
            flavor_ptr->kindname = "";
        }

#ifdef JP
#else
        else if (*flavor_ptr->s == '~') {
            if (!(flavor_ptr->mode & OD_NO_PLURAL) && (flavor_ptr->o_ptr->number != 1)) {
                char k = flavor_ptr->t[-1];
                if ((k == 's') || (k == 'h'))
                    *flavor_ptr->t++ = 'e';

                *flavor_ptr->t++ = 's';
            }

            flavor_ptr->s++;
        }
#endif
        else
            *flavor_ptr->t++ = *flavor_ptr->s++;
    }
}

void describe_named_item(player_type *player_ptr, flavor_type *flavor_ptr)
{
    check_object_known_aware(flavor_ptr);
    switch_tval_description(flavor_ptr);
    set_base_name(flavor_ptr);
    flavor_ptr->t = flavor_ptr->tmp_val;
#ifdef JP
    describe_prefix_ja(flavor_ptr);
    describe_artifact_prefix_ja(flavor_ptr);
#else

    if (flavor_ptr->basenm[0] == '&')
        describe_artifact_prefix_en(flavor_ptr);
    else
        describe_basename_en(flavor_ptr);
#endif

#ifdef JP
    if (flavor_ptr->o_ptr->is_smith())
        flavor_ptr->t = object_desc_str(flavor_ptr->t, format("鍛冶師%sの", player_ptr->name));

    describe_artifact_ja(flavor_ptr);
#endif

    describe_inscription(flavor_ptr);
    *flavor_ptr->t = '\0';

#ifdef JP
    describe_artifact_body_ja(flavor_ptr);
#else
    if (flavor_ptr->o_ptr->is_smith())
        flavor_ptr->t = object_desc_str(flavor_ptr->t, format(" of %s the Smith", player_ptr->name));

    describe_artifact_body_en(flavor_ptr);
#endif
}
