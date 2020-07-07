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

static void describe_monster_ball(flavor_type *flavor_ptr)
{
    monster_race *r_ptr = &r_info[flavor_ptr->o_ptr->pval];
    if (flavor_ptr->known)
        return;

    if (!flavor_ptr->o_ptr->pval) {
        flavor_ptr->modstr = _(" (空)", " (empty)");
        return;
    }

#ifdef JP
    sprintf(flavor_ptr->tmp_val2, " (%s)", r_name + r_ptr->name);
    flavor_ptr->modstr = flavor_ptr->tmp_val2;
#else
    flavor_ptr->t = r_name + r_ptr->name;
    if (!(r_ptr->flags1 & RF1_UNIQUE)) {
        sprintf(flavor_ptr->tmp_val2, " (%s%s)", (is_a_vowel(*flavor_ptr->t) ? "an " : "a "), flavor_ptr->t);
        flavor_ptr->modstr = flavor_ptr->tmp_val2;
    } else {
        sprintf(flavor_ptr->tmp_val2, "(%s)", flavor_ptr->t);
        flavor_ptr->modstr = flavor_ptr->t;
    }
#endif
}

static void describe_statue(flavor_type *flavor_ptr)
{
    monster_race *r_ptr = &r_info[flavor_ptr->o_ptr->pval];
#ifdef JP
    flavor_ptr->modstr = r_name + r_ptr->name;
#else
    flavor_ptr->t = r_name + r_ptr->name;
    if (!(r_ptr->flags1 & RF1_UNIQUE)) {
        sprintf(flavor_ptr->tmp_val2, "%s%s", (is_a_vowel(*flavor_ptr->t) ? "an " : "a "), flavor_ptr->t);
        flavor_ptr->modstr = flavor_ptr->tmp_val2;
    } else
        flavor_ptr->modstr = flavor_ptr->t;
#endif
}

static void describe_corpse(flavor_type *flavor_ptr)
{
    monster_race *r_ptr = &r_info[flavor_ptr->o_ptr->pval];
    flavor_ptr->modstr = r_name + r_ptr->name;
#ifdef JP
    flavor_ptr->basenm = "#%";
#else
    if (r_ptr->flags1 & RF1_UNIQUE)
        flavor_ptr->basenm = "& % of #";
    else
        flavor_ptr->basenm = "& # %";
#endif
}

static void describe_amulet(flavor_type *flavor_ptr)
{
    if (flavor_ptr->aware && (object_is_fixed_artifact(flavor_ptr->o_ptr) || ((flavor_ptr->k_ptr->gen_flags & TRG_INSTA_ART) != 0)))
        return;

    flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%のアミュレット", "& Amulet~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#アミュレット", "& # Amulet~ of %");
    else
        flavor_ptr->basenm = _("#アミュレット", "& # Amulet~");
}

static void describe_ring(flavor_type *flavor_ptr)
{
    if (flavor_ptr->aware && (object_is_fixed_artifact(flavor_ptr->o_ptr) || (flavor_ptr->k_ptr->gen_flags & TRG_INSTA_ART) != 0))
        return;

    flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の指輪", "& Ring~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#指輪", "& # Ring~ of %");
    else
        flavor_ptr->basenm = _("#指輪", "& # Ring~");

    if (!flavor_ptr->k_ptr->to_h && !flavor_ptr->k_ptr->to_d && (flavor_ptr->o_ptr->to_h || flavor_ptr->o_ptr->to_d))
        flavor_ptr->show_weapon = TRUE;
}

static void describe_staff(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の杖", "& Staff~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#杖", "& # Staff~ of %");
    else
        flavor_ptr->basenm = _("#杖", "& # Staff~");
}

static void describe_wand(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の魔法棒", "& Wand~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#魔法棒", "& # Wand~ of %");
    else
        flavor_ptr->basenm = _("#魔法棒", "& # Wand~");
}

static void describe_rod(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%のロッド", "& Rod~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#ロッド", "& # Rod~ of %");
    else
        flavor_ptr->basenm = _("#ロッド", "& # Rod~");
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
    switch (flavor_ptr->o_ptr->tval) {
    case TV_SKELETON:
    case TV_BOTTLE:
    case TV_JUNK:
    case TV_SPIKE:
    case TV_FLASK:
    case TV_CHEST:
    case TV_WHISTLE:
        break;
    case TV_CAPTURE:
        describe_monster_ball(flavor_ptr);
        break;
    case TV_FIGURINE:
    case TV_STATUE:
        describe_statue(flavor_ptr);
        break;
    case TV_CORPSE:
        describe_corpse(flavor_ptr);
        break;
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
    case TV_BOW:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_DIGGING:
        flavor_ptr->show_weapon = TRUE;
        break;
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
        flavor_ptr->show_armour = TRUE;
        break;
    case TV_LITE:
        break;
    case TV_AMULET:
        describe_amulet(flavor_ptr);
        break;
    case TV_RING:
        describe_ring(flavor_ptr);
        break;
    case TV_CARD:
        break;
    case TV_STAFF:
        describe_staff(flavor_ptr);
        break;
    case TV_WAND:
        describe_wand(flavor_ptr);
        break;
    case TV_ROD:
        describe_rod(flavor_ptr);
        break;
    case TV_SCROLL: {
        flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
        if (!flavor_ptr->flavor)
            flavor_ptr->basenm = _("%の巻物", "& Scroll~ of %");
        else if (flavor_ptr->aware)
            flavor_ptr->basenm = _("「#」と書かれた%の巻物", "& Scroll~ titled \"#\" of %");
        else
            flavor_ptr->basenm = _("「#」と書かれた巻物", "& Scroll~ titled \"#\"");

        break;
    }
    case TV_POTION: {
        flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
        if (!flavor_ptr->flavor)
            flavor_ptr->basenm = _("%の薬", "& Potion~ of %");
        else if (flavor_ptr->aware)
            flavor_ptr->basenm = _("%の#薬", "& # Potion~ of %");
        else
            flavor_ptr->basenm = _("#薬", "& # Potion~");

        break;
    }
    case TV_FOOD: {
        if (!flavor_ptr->k_ptr->flavor_name)
            break;

        flavor_ptr->modstr = k_name + flavor_ptr->flavor_k_ptr->flavor_name;
        if (!flavor_ptr->flavor)
            flavor_ptr->basenm = _("%のキノコ", "& Mushroom~ of %");
        else if (flavor_ptr->aware)
            flavor_ptr->basenm = _("%の#キノコ", "& # Mushroom~ of %");
        else
            flavor_ptr->basenm = _("#キノコ", "& # Mushroom~");

        break;
    }
    case TV_PARCHMENT: {
        flavor_ptr->basenm = _("羊皮紙 - %", "& Parchment~ - %");
        break;
    }
    case TV_LIFE_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "生命の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Life Magic %";
        else
            flavor_ptr->basenm = "& Life Spellbook~ %";
#endif

        break;
    }
    case TV_SORCERY_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "仙術の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Sorcery %";
        else
            flavor_ptr->basenm = "& Sorcery Spellbook~ %";
#endif

        break;
    }
    case TV_NATURE_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "自然の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Nature Magic %";
        else
            flavor_ptr->basenm = "& Nature Spellbook~ %";
#endif

        break;
    }
    case TV_CHAOS_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "カオスの魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Chaos Magic %";
        else
            flavor_ptr->basenm = "& Chaos Spellbook~ %";
#endif

        break;
    }
    case TV_DEATH_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "暗黒の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Death Magic %";
        else
            flavor_ptr->basenm = "& Death Spellbook~ %";
#endif

        break;
    }
    case TV_TRUMP_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "トランプの魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Trump Magic %";
        else
            flavor_ptr->basenm = "& Trump Spellbook~ %";
#endif

        break;
    }
    case TV_ARCANE_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "秘術の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Arcane Magic %";
        else
            flavor_ptr->basenm = "& Arcane Spellbook~ %";
#endif

        break;
    }
    case TV_CRAFT_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "匠の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Craft Magic %";
        else
            flavor_ptr->basenm = "& Craft Spellbook~ %";
#endif

        break;
    }
    case TV_DAEMON_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "悪魔の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Daemon Magic %";
        else
            flavor_ptr->basenm = "& Daemon Spellbook~ %";
#endif

        break;
    }
    case TV_CRUSADE_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "破邪の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Crusade Magic %";
        else
            flavor_ptr->basenm = "& Crusade Spellbook~ %";
#endif

        break;
    }
    case TV_MUSIC_BOOK: {
        flavor_ptr->basenm = _("歌集%", "& Song Book~ %");
        break;
    }
    case TV_HISSATSU_BOOK: {
        flavor_ptr->basenm = _("& 武芸の書%", "Book~ of Kendo %");
        break;
    }
    case TV_HEX_BOOK: {
#ifdef JP
        flavor_ptr->basenm = "呪術の魔法書%";
#else
        if (mp_ptr->spell_book == TV_LIFE_BOOK)
            flavor_ptr->basenm = "& Book~ of Hex Magic %";
        else
            flavor_ptr->basenm = "& Hex Spellbook~ %";
#endif

        break;
    }
    case TV_GOLD: {
        strcpy(flavor_ptr->buf, flavor_ptr->basenm);
        return;
    }
    default: {
        strcpy(flavor_ptr->buf, _("(なし)", "(nothing)"));
        return;
    }
    }

    if (flavor_ptr->aware && have_flag(flavor_ptr->flags, TR_FULL_NAME)) {
        if (flavor_ptr->known && flavor_ptr->o_ptr->name1)
            flavor_ptr->basenm = a_name + a_info[flavor_ptr->o_ptr->name1].name;
        else
            flavor_ptr->basenm = flavor_ptr->kindname;
    }

    flavor_ptr->t = flavor_ptr->tmp_val;
#ifdef JP
    if (flavor_ptr->basenm[0] == '&')
        flavor_ptr->s = flavor_ptr->basenm + 2;
    else
        flavor_ptr->s = flavor_ptr->basenm;

    /* No prefix */
    if (flavor_ptr->mode & OD_OMIT_PREFIX) {
        /* Nothing */
    } else if (flavor_ptr->o_ptr->number > 1) {
        flavor_ptr->t = object_desc_count_japanese(flavor_ptr->t, flavor_ptr->o_ptr);
        flavor_ptr->t = object_desc_str(flavor_ptr->t, "の ");
    }

    // 英語の場合アーティファクトは The が付くので分かるが、日本語では分からないのでマークをつける.
    if (flavor_ptr->known) {
        if (object_is_fixed_artifact(flavor_ptr->o_ptr))
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "★");
        else if (flavor_ptr->o_ptr->art_name)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "☆");
    }
#else

    if (flavor_ptr->basenm[0] == '&') {
        flavor_ptr->s = flavor_ptr->basenm + 2;
        if (flavor_ptr->mode & OD_OMIT_PREFIX) {
            /* Nothing */
        } else if (flavor_ptr->o_ptr->number <= 0)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "no more ");
        else if (flavor_ptr->o_ptr->number > 1) {
            flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->number);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        } else if ((flavor_ptr->known && object_is_artifact(flavor_ptr->o_ptr)) || ((flavor_ptr->o_ptr->tval == TV_CORPSE) && (r_info[flavor_ptr->o_ptr->pval].flags1 & RF1_UNIQUE)))
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "The ");
        else {
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
    } else {
        flavor_ptr->s = flavor_ptr->basenm;
        if (flavor_ptr->mode & OD_OMIT_PREFIX) {
            /* Nothing */
        } else if (flavor_ptr->o_ptr->number <= 0)
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "no more ");
        else if (flavor_ptr->o_ptr->number > 1) {
            flavor_ptr->t = object_desc_num(flavor_ptr->t, flavor_ptr->o_ptr->number);
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
        } else if (flavor_ptr->known && object_is_artifact(flavor_ptr->o_ptr))
            flavor_ptr->t = object_desc_str(flavor_ptr->t, "The ");
    }
#endif

#ifdef JP
    if (object_is_smith(player_ptr, flavor_ptr->o_ptr))
        flavor_ptr->t = object_desc_str(flavor_ptr->t, format("鍛冶師%flavor_ptr->sの", player_ptr->name));

    /* 伝説のアイテム、名のあるアイテムの名前を付加する */
    if (flavor_ptr->known) {
        /* ランダム・アーティファクト */
        if (flavor_ptr->o_ptr->art_name) {
            concptr temp = quark_str(flavor_ptr->o_ptr->art_name);

            /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
            /* 英語版のセーブファイルから来た 'of XXX' は,「XXXの」と表示する */
            if (strncmp(temp, "of ", 3) == 0) {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, &temp[3]);
                flavor_ptr->t = object_desc_str(flavor_ptr->t, "の");
            } else if ((strncmp(temp, "『", 2) != 0) && (strncmp(temp, "《", 2) != 0) && (temp[0] != '\''))
                flavor_ptr->t = object_desc_str(flavor_ptr->t, temp);
        }
        /* 伝説のアイテム */
        else if (flavor_ptr->o_ptr->name1 && !have_flag(flavor_ptr->flags, TR_FULL_NAME)) {
            artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];
            /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
            if (strncmp(a_name + a_ptr->name, "『", 2) != 0) {
                flavor_ptr->t = object_desc_str(flavor_ptr->t, a_name + a_ptr->name);
            }
        }
        /* 名のあるアイテム */
        else if (object_is_ego(flavor_ptr->o_ptr)) {
            ego_item_type *e_ptr = &e_info[flavor_ptr->o_ptr->name2];
            flavor_ptr->t = object_desc_str(flavor_ptr->t, e_name + e_ptr->name);
        }
    }
#endif

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

    *flavor_ptr->t = '\0';

#ifdef JP
    /* '『'から始まる伝説のアイテムの名前は最後に付加する */
    if (flavor_ptr->known) {
        // ランダムアーティファクトの名前はセーブファイルに記録されるので、英語版の名前もそれらしく変換する.
        if (flavor_ptr->o_ptr->art_name) {
            char temp[256];
            int itemp;
            strcpy(temp, quark_str(flavor_ptr->o_ptr->art_name));
            if (strncmp(temp, "『", 2) == 0 || strncmp(temp, "《", 2) == 0)
                flavor_ptr->t = object_desc_str(flavor_ptr->t, temp);
            else if (temp[0] == '\'') {
                itemp = strlen(temp);
                temp[itemp - 1] = 0;
                flavor_ptr->t = object_desc_str(flavor_ptr->t, "『");
                flavor_ptr->t = object_desc_str(flavor_ptr->t, &temp[1]);
                flavor_ptr->t = object_desc_str(flavor_ptr->t, "』");
            }
        } else if (object_is_fixed_artifact(flavor_ptr->o_ptr)) {
            artifact_type *a_ptr = &a_info[flavor_ptr->o_ptr->name1];
            if (strncmp(a_name + a_ptr->name, "『", 2) == 0)
                flavor_ptr->t = object_desc_str(flavor_ptr->t, a_name + a_ptr->name);
        } else if (flavor_ptr->o_ptr->inscription) {
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

            if (*str) {
                concptr str_aux = angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '#');
                flavor_ptr->t = object_desc_str(flavor_ptr->t, "『");
                flavor_ptr->t = object_desc_str(flavor_ptr->t, &str_aux[1]);
                flavor_ptr->t = object_desc_str(flavor_ptr->t, "』");
            }
        }
    }
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
        flavor_ptr->t = show_ammo_no_crit ? object_desc_str(flavor_ptr->t, show_ammo_detail ? "/crit " : "/") : object_desc_str(flavor_ptr->t, show_ammo_detail ? "/shot " : "/");
        if (player_ptr->num_fire == 0)
            flavor_ptr->t = object_desc_chr(flavor_ptr->t, '0');
        else {
            avgdam *= (player_ptr->num_fire * 100);
            avgdam /= energy_fire;
            flavor_ptr->t = object_desc_num(flavor_ptr->t, avgdam);
            flavor_ptr->t = object_desc_str(flavor_ptr->t, show_ammo_detail ? "/turn" : "");
            if (show_ammo_crit_ratio) {
                int percent = calc_crit_ratio_shot(player_ptr, flavor_ptr->known ? flavor_ptr->o_ptr->to_h : 0, flavor_ptr->known ? flavor_ptr->bow_ptr->to_h : 0);
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
    else if (((flavor_ptr->o_ptr->tval == TV_RING) || (flavor_ptr->o_ptr->tval == TV_AMULET) || (flavor_ptr->o_ptr->tval == TV_LITE) || (flavor_ptr->o_ptr->tval == TV_FIGURINE)) && flavor_ptr->aware && !flavor_ptr->known
        && !(flavor_ptr->o_ptr->ident & IDENT_SENSE))
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
