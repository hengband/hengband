/*!
 * @brief 武器/防具/アクセサリアイテムにおける、耐性やスレイ等の表記
 * @date 2020/07/06
 * @author Hourier
 */

#include "flavor/flavor-describer.h"
#include "cmd-item/cmd-smith.h"
#include "combat/shoot.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "flavor/tval-description-switcher.h"
#include "game-option/text-display-options.h"
#include "grid/trap.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "monster-race/monster-race.h"
#include "object-enchant/artifact.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-quest.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-lite-types.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#ifdef JP
#else
#include "locale/english.h"
#include "monster-race/race-flags1.h"
#include "player/player-class.h"
#endif

static void check_object_known_aware(player_type *player_ptr, flavor_type *flavor_ptr)
{
    object_flags(player_ptr, flavor_ptr->o_ptr, flavor_ptr->flags);
    if (object_is_aware(flavor_ptr->o_ptr))
        flavor_ptr->aware = TRUE;

    if (object_is_known(flavor_ptr->o_ptr))
        flavor_ptr->known = TRUE;

    if (flavor_ptr->aware && ((flavor_ptr->mode & OD_NO_FLAVOR) || plain_descriptions))
        flavor_ptr->flavor = FALSE;

    if ((flavor_ptr->mode & OD_STORE) || (flavor_ptr->o_ptr->ident & IDENT_STORE)) {
        flavor_ptr->flavor = FALSE;
        flavor_ptr->aware = TRUE;
        flavor_ptr->known = TRUE;
    }

    if (flavor_ptr->mode & OD_FORCE_FLAVOR) {
        flavor_ptr->aware = FALSE;
        flavor_ptr->flavor = TRUE;
        flavor_ptr->known = FALSE;
        flavor_ptr->flavor_k_ptr = flavor_ptr->k_ptr;
    }
}

static void set_base_name(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->aware || !have_flag(flavor_ptr->flags, TR_FULL_NAME))
        return;

    flavor_ptr->basenm = (flavor_ptr->known && (flavor_ptr->o_ptr->name1 != 0)) ? a_name + a_info[flavor_ptr->o_ptr->name1].name : flavor_ptr->kindname;
}

#ifdef JP
static void describe_prefix_ja(flavor_type *flavor_ptr)
{
    flavor_ptr->s = flavor_ptr->basenm[0] == '&' ? flavor_ptr->basenm : flavor_ptr->basenm + 2;
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
 * @return なし
 * @details 英語の場合アーティファクトは The が付くので分かるが、日本語では分からないのでマークをつける.
 */
static void describe_artifact_prefix_ja(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->known)
        return;

    if (object_is_fixed_artifact(flavor_ptr->o_ptr))
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "★");
    else if (flavor_ptr->o_ptr->art_name)
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "☆");
}

/*!
 * @brief アーティファクトの説明表記
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @return なし
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
    
    if (flavor_ptr->o_ptr->name1 && !have_flag(flavor_ptr->flags, TR_FULL_NAME)) {
        artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];
        /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
        if (strncmp(a_name + a_ptr->name, "『", 2) != 0)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, a_name + a_ptr->name);

        return;
    }

    if (object_is_ego(flavor_ptr->o_ptr)) {
        ego_item_type *e_ptr = &e_info[flavor_ptr->o_ptr->name2];
        flavor_ptr->t = object_desc_str(flavor_ptr->t, e_name + e_ptr->name);
    }
}

/*!
 * @brief ランダムあーファクトの表記
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @return ランダムアーティファクトならTRUE、違うならFALSE
 * @details ランダムアーティファクトの名前はセーブファイルに記録されるので、英語版の名前もそれらしく変換する.
 */
static bool describe_random_artifact_body_ja(flavor_type *flavor_ptr)
{
    if (flavor_ptr->o_ptr->art_name == 0)
        return FALSE;

    char temp[256];
    int itemp;
    strcpy(temp, quark_str(flavor_ptr->o_ptr->art_name));
    if (strncmp(temp, "『", 2) == 0 || strncmp(temp, "《", 2) == 0) {
        flavor_ptr->t = object_desc_str(flavor_ptr->t, temp);
        return TRUE;
    }

    if (temp[0] != '\'')
        return TRUE;

    itemp = strlen(temp);
    temp[itemp - 1] = 0;
    flavor_ptr->t = object_desc_str(flavor_ptr->t, "『");
    flavor_ptr->t = object_desc_str(flavor_ptr->t, &temp[1]);
    flavor_ptr->t = object_desc_str(flavor_ptr->t, "』");
    return TRUE;
}

/*!
 * @brief アーティファクトのアイテム名を表記する
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @return なし
 * @details '『'から始まる伝説のアイテムの名前は最後に付加する
 */
static void describe_artifact_body_ja(flavor_type *flavor_ptr)
{
    if (!flavor_ptr->known)
        return;

    if (describe_random_artifact_body_ja(flavor_ptr))
        return;

    if (object_is_fixed_artifact(flavor_ptr->o_ptr)) {
        artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];
        if (strncmp(a_name + a_ptr->name, "『", 2) == 0)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, a_name + a_ptr->name);

        return;
    }
    
    if (flavor_ptr->o_ptr->inscription) {
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
        return TRUE;
    }

    if (flavor_ptr->o_ptr->number == 1)
        return FALSE;

    flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->number);
    flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
    return TRUE;
}

static void describe_artifact_en(flavor_type *flavor_ptr)
{
    flavor_ptr->s = flavor_ptr->basenm + 2;
    if (flavor_ptr->mode & OD_OMIT_PREFIX)
        return;

    if (describe_prefix_en(flavor_ptr))
        return;

    if ((flavor_ptr->known && object_is_artifact(flavor_ptr->o_ptr))
        || ((flavor_ptr->o_ptr->tval == TV_CORPSE) && (r_info[flavor_ptr->o_ptr->pval].flags1 & RF1_UNIQUE))) {
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

    if (flavor_ptr->known && object_is_artifact(flavor_ptr->o_ptr))
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "The ");
}
#endif

/*!
 * @brief 銘を表記する
 * @param flavor_ptr アイテム表記への参照ポインタ
 * @return なし
 * @details ランダムアーティファクト、固定アーティファクト、エゴの順に評価する
 */
static void describe_inscription(flavor_type *flavor_ptr)
{
    for (flavor_ptr->s0 = NULL; *flavor_ptr->s || flavor_ptr->s0;) {
        if (!*flavor_ptr->s) {
            flavor_ptr->s = flavor_ptr->s0 + 1;
            flavor_ptr->s0 = NULL;
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

/*!
 * @brief オブジェクトの各表記を返すメイン関数 / Creates a description of the item "o_ptr", and stores it in "out_val".
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param buf 表記を返すための文字列参照ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @param mode 表記に関するオプション指定
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す
 */
void describe_flavor(player_type *player_ptr, char *buf, object_type *o_ptr, BIT_FLAGS mode)
{
    flavor_type tmp_flavor;
    flavor_type *flavor_ptr = initialize_flavor_type(&tmp_flavor, buf, o_ptr, mode);
    check_object_known_aware(player_ptr, flavor_ptr);
    switch_tval_description(flavor_ptr);
    set_base_name(flavor_ptr);
    flavor_ptr->t = flavor_ptr->tmp_val;
#ifdef JP
    describe_prefix_ja(flavor_ptr);
    describe_artifact_prefix_ja(flavor_ptr);
#else

    if (flavor_ptr->basenm[0] == '&')
        describe_artifact_en(flavor_ptr);
    else
        describe_basename_en(flavor_ptr);
#endif

#ifdef JP
    if (object_is_smith(player_ptr, flavor_ptr->o_ptr))
        flavor_ptr->t = object_desc_str(flavor_ptr->t, format("鍛冶師%flavor_ptr->sの", player_ptr->name));

    describe_artifact_ja(flavor_ptr);
#endif

    describe_inscription(flavor_ptr);
    *flavor_ptr->t = '\0';

#ifdef JP
    describe_artifact_body_ja(flavor_ptr);
#else
    if (object_is_smith(player_ptr, flavor_ptr->o_ptr))
        flavor_ptr->t = object_desc_str(flavor_ptr->t, format(" of %s the Smith", player_ptr->name));

    if (flavor_ptr->known && !have_flag(flavor_ptr->flags, TR_FULL_NAME)) {
        if (flavor_ptr->o_ptr->art_name) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_str(flavor_ptr->t, quark_str(flavor_ptr->o_ptr->art_name));
        } else if (object_is_fixed_artifact(flavor_ptr->o_ptr)) {
            artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];

            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_str(flavor_ptr->t, a_name + a_ptr->name);
        } else {
            if (object_is_ego(flavor_ptr->o_ptr)) {
                ego_item_type *e_ptr = &e_info[flavor_ptr->o_ptr->name2];
                flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
                flavor_ptr->t = object_desc_str(flavor_ptr->t, e_name + e_ptr->name);
            }

            if (flavor_ptr->o_ptr->inscription && angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '#')) {
                concptr str = angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '#');
                flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
                flavor_ptr->t = object_desc_str(flavor_ptr->t, &str[1]);
            }
        }
    }
#endif

    if (flavor_ptr->mode & OD_NAME_ONLY) {
        angband_strcpy(flavor_ptr->buf, flavor_ptr->tmp_val, MAX_NLEN);
        return;
    }

    if (flavor_ptr->o_ptr->tval == TV_CHEST) {
        if (!flavor_ptr->known) {
            /* Nothing */
        } else if (!flavor_ptr->o_ptr->pval)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(空)", " (empty)"));
        else if (flavor_ptr->o_ptr->pval < 0)
            if (chest_traps[0 - flavor_ptr->o_ptr->pval])
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(解除済)", " (disarmed)"));
            else
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(非施錠)", " (unlocked)"));
        else {
            switch (chest_traps[flavor_ptr->o_ptr->pval]) {
            case 0: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(施錠)", " (Locked)"));
                break;
            }
            case CHEST_LOSE_STR: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(毒針)", " (Poison Needle)"));
                break;
            }
            case CHEST_LOSE_CON: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(毒針)", " (Poison Needle)"));
                break;
            }
            case CHEST_POISON: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(ガス・トラップ)", " (Gas Trap)"));
                break;
            }
            case CHEST_PARALYZE: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(ガス・トラップ)", " (Gas Trap)"));
                break;
            }
            case CHEST_EXPLODE: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(爆発装置)", " (Explosion Device)"));
                break;
            }
            case CHEST_SUMMON:
            case CHEST_BIRD_STORM:
            case CHEST_E_SUMMON:
            case CHEST_H_SUMMON: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(召喚のルーン)", " (Summoning Runes)"));
                break;
            }
            case CHEST_RUNES_OF_EVIL: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(邪悪なルーン)", " (Gleaming Black Runes)"));
                break;
            }
            case CHEST_ALARM: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(警報装置)", " (Alarm)"));
                break;
            }
            default: {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(マルチ・トラップ)", " (Multiple Traps)"));
                break;
            }
            }
        }
    }

    if (have_flag(flavor_ptr->flags, TR_SHOW_MODS))
        flavor_ptr->show_weapon = TRUE;

    if (object_is_smith(player_ptr, flavor_ptr->o_ptr) && (flavor_ptr->o_ptr->xtra3 == 1 + ESSENCE_SLAY_GLOVE))
        flavor_ptr->show_weapon = TRUE;

    if (flavor_ptr->o_ptr->to_h && flavor_ptr->o_ptr->to_d)
        flavor_ptr->show_weapon = TRUE;

    if (flavor_ptr->o_ptr->ac)
        flavor_ptr->show_armour = TRUE;

    switch (flavor_ptr->o_ptr->tval) {
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_DIGGING: {
        if (object_is_quest_target(player_ptr, flavor_ptr->o_ptr) && !flavor_ptr->known)
            break;

        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
        flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->dd);
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, 'd');
        flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->ds);
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        break;
    }
    case TV_BOW: {
        flavor_ptr->power = bow_tmul(flavor_ptr->o_ptr->sval);
        if (have_flag(flavor_ptr->flags, TR_XTRA_MIGHT))
            flavor_ptr->power++;

        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, 'x');
        flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->power);
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        flavor_ptr->fire_rate = calc_num_fire(player_ptr, flavor_ptr->o_ptr);
        if (flavor_ptr->fire_rate != 0 && flavor_ptr->power > 0 && flavor_ptr->known) {
            flavor_ptr->fire_rate = bow_energy(flavor_ptr->o_ptr->sval) / flavor_ptr->fire_rate;
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
            flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->fire_rate / 100);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, '.');
            flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->fire_rate % 100);
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "turn");
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        }

        break;
    }
    }

    if (flavor_ptr->known) {
        if (flavor_ptr->show_weapon) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
            flavor_ptr->t = object_desc_int(flavor_ptr->t, flavor_ptr->o_ptr->to_h);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ',');
            flavor_ptr->t = object_desc_int(flavor_ptr->t, flavor_ptr->o_ptr->to_d);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        } else if (flavor_ptr->o_ptr->to_h) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
            flavor_ptr->t = object_desc_int(flavor_ptr->t, flavor_ptr->o_ptr->to_h);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        } else if (flavor_ptr->o_ptr->to_d) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
            flavor_ptr->t = object_desc_int(flavor_ptr->t, flavor_ptr->o_ptr->to_d);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        }
    }

    flavor_ptr->bow_ptr = &player_ptr->inventory_list[INVEN_BOW];
    if (flavor_ptr->bow_ptr->k_idx && (flavor_ptr->o_ptr->tval == player_ptr->tval_ammo)) {
        int avgdam = flavor_ptr->o_ptr->dd * (flavor_ptr->o_ptr->ds + 1) * 10 / 2;
        int tmul = bow_tmul(flavor_ptr->bow_ptr->sval);
        ENERGY energy_fire = bow_energy(flavor_ptr->bow_ptr->sval);
        if (object_is_known(flavor_ptr->bow_ptr))
            avgdam += (flavor_ptr->bow_ptr->to_d * 10);

        if (flavor_ptr->known)
            avgdam += (flavor_ptr->o_ptr->to_d * 10);

        if (player_ptr->xtra_might)
            tmul++;

        tmul = tmul * (100 + (int)(adj_str_td[player_ptr->stat_ind[A_STR]]) - 128);
        avgdam *= tmul;
        avgdam /= (100 * 10);
        if (player_ptr->concent)
            avgdam = boost_concentration_damage(player_ptr, avgdam);

        if (avgdam < 0)
            avgdam = 0;

        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
        if (show_ammo_no_crit) {
            flavor_ptr->t = object_desc_num(flavor_ptr->t, avgdam);
            flavor_ptr->t = object_desc_str(flavor_ptr->t, show_ammo_detail ? "/shot " : "/");
        }

        avgdam = calc_expect_crit_shot(player_ptr, flavor_ptr->o_ptr->weight, flavor_ptr->o_ptr->to_h, flavor_ptr->bow_ptr->to_h, avgdam);
        flavor_ptr->t = object_desc_num(flavor_ptr->t, avgdam);
        flavor_ptr->t = show_ammo_no_crit ? object_desc_str(flavor_ptr->t, show_ammo_detail ? "/crit " : "/")
                                          : object_desc_str(flavor_ptr->t, show_ammo_detail ? "/shot " : "/");
        if (player_ptr->num_fire == 0)
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, '0');
        else {
            avgdam *= (player_ptr->num_fire * 100);
            avgdam /= energy_fire;
            flavor_ptr->t = object_desc_num(flavor_ptr->t, avgdam);
            flavor_ptr->t = object_desc_str(flavor_ptr->t, show_ammo_detail ? "/turn" : "");
            if (show_ammo_crit_ratio) {
                int percent
                    = calc_crit_ratio_shot(player_ptr, flavor_ptr->known ? flavor_ptr->o_ptr->to_h : 0, flavor_ptr->known ? flavor_ptr->bow_ptr->to_h : 0);
                flavor_ptr->t = object_desc_chr(flavor_ptr->t, '/');
                flavor_ptr->t = object_desc_num(flavor_ptr->t, percent / 100);
                flavor_ptr->t = object_desc_chr(flavor_ptr->t, '.');
                if (percent % 100 < 10)
                    flavor_ptr->t = object_desc_chr(flavor_ptr->t, '0');

                flavor_ptr->t = object_desc_num(flavor_ptr->t, percent % 100);
                flavor_ptr->t = object_desc_str(flavor_ptr->t, show_ammo_detail ? "% crit" : "%");
            }
        }

        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
    } else if ((player_ptr->pclass == CLASS_NINJA) && (flavor_ptr->o_ptr->tval == TV_SPIKE)) {
        int avgdam = player_ptr->mighty_throw ? (1 + 3) : 1;
        s16b energy_fire = 100 - player_ptr->lev;
        avgdam += ((player_ptr->lev + 30) * (player_ptr->lev + 30) - 900) / 55;
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
        flavor_ptr->t = object_desc_num(flavor_ptr->t, avgdam);
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, '/');
        avgdam = 100 * avgdam / energy_fire;
        flavor_ptr->t = object_desc_num(flavor_ptr->t, avgdam);
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
    }

    if (flavor_ptr->known) {
        if (flavor_ptr->show_armour) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->b1);
            flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->ac);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ',');
            flavor_ptr->t = object_desc_int(flavor_ptr->t, flavor_ptr->o_ptr->to_a);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->b2);
        } else if (flavor_ptr->o_ptr->to_a) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->b1);
            flavor_ptr->t = object_desc_int(flavor_ptr->t, flavor_ptr->o_ptr->to_a);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->b2);
        }
    } else if (flavor_ptr->show_armour) {
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->b1);
        flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->ac);
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->b2);
    }

    if (flavor_ptr->mode & OD_NAME_AND_ENCHANT) {
        angband_strcpy(flavor_ptr->buf, flavor_ptr->tmp_val, MAX_NLEN);
        return;
    }

    if (flavor_ptr->known) {
        if (((flavor_ptr->o_ptr->tval == TV_STAFF) || (flavor_ptr->o_ptr->tval == TV_WAND))) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
            if ((flavor_ptr->o_ptr->tval == TV_STAFF) && (flavor_ptr->o_ptr->number > 1)) {
                flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->number);
                flavor_ptr->t = object_desc_str(flavor_ptr->t, "x ");
            }

            flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->pval);
            flavor_ptr->t = object_desc_str(flavor_ptr->t, _("回分", " charge"));
#ifdef JP
#else
            if (flavor_ptr->o_ptr->pval != 1)
                flavor_ptr->t = object_desc_chr(flavor_ptr->t, 's');
#endif

            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        } else if (flavor_ptr->o_ptr->tval == TV_ROD) {
            if (flavor_ptr->o_ptr->timeout) {
                if (flavor_ptr->o_ptr->number > 1) {
                    if (flavor_ptr->k_ptr->pval == 0)
                        flavor_ptr->k_ptr->pval = 1;

                    flavor_ptr->power = (flavor_ptr->o_ptr->timeout + (flavor_ptr->k_ptr->pval - 1)) / flavor_ptr->k_ptr->pval;
                    if (flavor_ptr->power > flavor_ptr->o_ptr->number)
                        flavor_ptr->power = flavor_ptr->o_ptr->number;

                    flavor_ptr->t = object_desc_str(flavor_ptr->t, " (");
                    flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->power);
                    flavor_ptr->t = object_desc_str(flavor_ptr->t, _("本 充填中)", " charging)"));
                } else
                    flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(充填中)", " (charging)"));
            }
        }

        if (have_pval_flags(flavor_ptr->flags)) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p1);
            flavor_ptr->t = object_desc_int(flavor_ptr->t, flavor_ptr->o_ptr->pval);
            if (have_flag(flavor_ptr->flags, TR_HIDE_TYPE)) {
                /* Nothing */
            } else if (have_flag(flavor_ptr->flags, TR_SPEED))
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("加速", " to speed"));
            else if (have_flag(flavor_ptr->flags, TR_BLOWS)) {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("攻撃", " attack"));
#ifdef JP
#else
                if (ABS(flavor_ptr->o_ptr->pval) != 1)
                    flavor_ptr->t = object_desc_chr(flavor_ptr->t, 's');
#endif
            } else if (have_flag(flavor_ptr->flags, TR_STEALTH))
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("隠密", " to stealth"));
            else if (have_flag(flavor_ptr->flags, TR_SEARCH))
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("探索", " to searching"));
            else if (have_flag(flavor_ptr->flags, TR_INFRA))
                flavor_ptr->t = object_desc_str(flavor_ptr->t, _("赤外線視力", " to infravision"));

            flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->p2);
        }

        if ((flavor_ptr->o_ptr->tval == TV_LITE) && (!(object_is_fixed_artifact(flavor_ptr->o_ptr) || (flavor_ptr->o_ptr->sval == SV_LITE_FEANOR)))) {
            flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(", " (with "));
            if (flavor_ptr->o_ptr->name2 == EGO_LITE_LONG)
                flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->xtra4 * 2);
            else
                flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->xtra4);

            flavor_ptr->t = object_desc_str(flavor_ptr->t, _("ターンの寿命)", " turns of light)"));
        }

        if (flavor_ptr->o_ptr->timeout && (flavor_ptr->o_ptr->tval != TV_ROD))
            flavor_ptr->t = object_desc_str(flavor_ptr->t, _("(充填中)", " (charging)"));
    }

    if (flavor_ptr->mode & OD_OMIT_INSCRIPTION) {
        angband_strcpy(flavor_ptr->buf, flavor_ptr->tmp_val, MAX_NLEN);
        return;
    }

    flavor_ptr->tmp_val2[0] = '\0';
    if ((abbrev_extra || abbrev_all) && object_is_fully_known(flavor_ptr->o_ptr)) {
        if (!flavor_ptr->o_ptr->inscription || !angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '%')) {
            bool kanji, all;
            kanji = _(TRUE, FALSE);
            all = abbrev_all;
            get_ability_abbreviation(player_ptr, flavor_ptr->tmp_val2, flavor_ptr->o_ptr, kanji, all);
        }
    }

    if (flavor_ptr->o_ptr->inscription) {
        char buff[1024];
        if (flavor_ptr->tmp_val2[0])
            strcat(flavor_ptr->tmp_val2, ", ");

        get_inscription(player_ptr, buff, flavor_ptr->o_ptr);
        angband_strcat(flavor_ptr->tmp_val2, buff, sizeof(flavor_ptr->tmp_val2));
    }

    flavor_ptr->fake_insc_buf[0] = '\0';
    if (flavor_ptr->o_ptr->feeling)
        strcpy(flavor_ptr->fake_insc_buf, game_inscriptions[flavor_ptr->o_ptr->feeling]);
    else if (object_is_cursed(flavor_ptr->o_ptr) && (flavor_ptr->known || (flavor_ptr->o_ptr->ident & IDENT_SENSE)))
        strcpy(flavor_ptr->fake_insc_buf, _("呪われている", "cursed"));
    else if (((flavor_ptr->o_ptr->tval == TV_RING) || (flavor_ptr->o_ptr->tval == TV_AMULET) || (flavor_ptr->o_ptr->tval == TV_LITE)
                 || (flavor_ptr->o_ptr->tval == TV_FIGURINE))
        && flavor_ptr->aware && !flavor_ptr->known && !(flavor_ptr->o_ptr->ident & IDENT_SENSE))
        strcpy(flavor_ptr->fake_insc_buf, _("未鑑定", "unidentified"));
    else if (!flavor_ptr->known && (flavor_ptr->o_ptr->ident & IDENT_EMPTY))
        strcpy(flavor_ptr->fake_insc_buf, _("空", "empty"));
    else if (!flavor_ptr->aware && object_is_tried(flavor_ptr->o_ptr))
        strcpy(flavor_ptr->fake_insc_buf, _("未判明", "tried"));

    if (flavor_ptr->o_ptr->discount) {
        if (!flavor_ptr->tmp_val2[0] || (flavor_ptr->o_ptr->ident & IDENT_STORE)) {
            char discount_num_buf[4];
            if (flavor_ptr->fake_insc_buf[0])
                strcat(flavor_ptr->fake_insc_buf, ", ");

            (void)object_desc_num(discount_num_buf, flavor_ptr->o_ptr->discount);
            strcat(flavor_ptr->fake_insc_buf, discount_num_buf);
            strcat(flavor_ptr->fake_insc_buf, _("%引き", "% off"));
        }
    }

    if (flavor_ptr->fake_insc_buf[0] || flavor_ptr->tmp_val2[0]) {
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->c1);
        if (flavor_ptr->fake_insc_buf[0])
            flavor_ptr->t = object_desc_str(flavor_ptr->t, flavor_ptr->fake_insc_buf);

        if (flavor_ptr->fake_insc_buf[0] && flavor_ptr->tmp_val2[0]) {
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ',');
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        }

        if (flavor_ptr->tmp_val2[0])
            flavor_ptr->t = object_desc_str(flavor_ptr->t, flavor_ptr->tmp_val2);

        flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->c2);
    }

    angband_strcpy(flavor_ptr->buf, flavor_ptr->tmp_val, MAX_NLEN);
}
