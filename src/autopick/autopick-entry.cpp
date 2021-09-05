#include "autopick/autopick-entry.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-key-flag-process.h"
#include "autopick/autopick-keys-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "core/show-file.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-quest.h"
#include "object-hook/hook-weapon.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/player-realm.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/quarks.h"
#include "util/string-processor.h"

#ifdef JP
static char kanji_colon[] = "：";
#endif

/*!
 * @brief A function to create new entry
 */
bool autopick_new_entry(autopick_type *entry, concptr str, bool allow_default)
{
    if (str[0] && str[1] == ':')
        switch (str[0]) {
        case '?':
        case '%':
        case 'A':
        case 'P':
        case 'C':
            return false;
        }

    entry->flag[0] = entry->flag[1] = 0L;
    entry->dice = 0;
    entry->bonus = 0;

    byte act = DO_AUTOPICK | DO_DISPLAY;
    while (true) {
        if ((act & DO_AUTOPICK) && *str == '!') {
            act &= ~DO_AUTOPICK;
            act |= DO_AUTODESTROY;
            str++;
            continue;
        }

        if ((act & DO_AUTOPICK) && *str == '~') {
            act &= ~DO_AUTOPICK;
            act |= DONT_AUTOPICK;
            str++;
            continue;
        }

        if ((act & DO_AUTOPICK) && *str == ';') {
            act &= ~DO_AUTOPICK;
            act |= DO_QUERY_AUTOPICK;
            str++;
            continue;
        }

        if ((act & DO_DISPLAY) && *str == '(') {
            act &= ~DO_DISPLAY;
            str++;
            continue;
        }

        break;
    }

    concptr insc = nullptr;
    char buf[MAX_LINELEN];
    int i;
    for (i = 0; *str; i++) {
        char c = *str++;
#ifdef JP
        if (iskanji(c)) {
            buf[i++] = c;
            buf[i] = *str++;
            continue;
        }
#endif
        if (c == '#') {
            buf[i] = '\0';
            insc = str;
            break;
        }

        if (isupper(c))
            c = (char)tolower(c);

        buf[i] = c;
    }

    buf[i] = '\0';
    if (!allow_default && *buf == 0)
        return false;
    if (*buf == 0 && insc)
        return false;

    concptr prev_ptr, ptr;
    ptr = prev_ptr = buf;
    concptr old_ptr = nullptr;
    while (old_ptr != ptr) {
        old_ptr = ptr;
        if (MATCH_KEY(KEY_ALL))
            ADD_FLG(FLG_ALL);
        if (MATCH_KEY(KEY_COLLECTING))
            ADD_FLG(FLG_COLLECTING);
        if (MATCH_KEY(KEY_UNAWARE))
            ADD_FLG(FLG_UNAWARE);
        if (MATCH_KEY(KEY_UNIDENTIFIED))
            ADD_FLG(FLG_UNIDENTIFIED);
        if (MATCH_KEY(KEY_IDENTIFIED))
            ADD_FLG(FLG_IDENTIFIED);
        if (MATCH_KEY(KEY_STAR_IDENTIFIED))
            ADD_FLG(FLG_STAR_IDENTIFIED);
        if (MATCH_KEY(KEY_BOOSTED))
            ADD_FLG(FLG_BOOSTED);

        /*** Weapons whose dd*ds is more than nn ***/
        if (MATCH_KEY2(KEY_MORE_THAN)) {
            int k = 0;
            entry->dice = 0;

            while (' ' == *ptr)
                ptr++;

            while ('0' <= *ptr && *ptr <= '9') {
                entry->dice = 10 * entry->dice + (*ptr - '0');
                ptr++;
                k++;
            }

            if (k > 0 && k <= 2) {
                (void)MATCH_KEY(KEY_DICE);
                ADD_FLG(FLG_MORE_DICE);
            } else
                ptr = prev_ptr;
        }

        /*** Items whose magical bonus is more than n ***/
        if (MATCH_KEY2(KEY_MORE_BONUS)) {
            int k = 0;
            entry->bonus = 0;

            while (' ' == *ptr)
                ptr++;

            while ('0' <= *ptr && *ptr <= '9') {
                entry->bonus = 10 * entry->bonus + (*ptr - '0');
                ptr++;
                k++;
            }

            if (k > 0 && k <= 2) {
#ifdef JP
                (void)MATCH_KEY(KEY_MORE_BONUS2);
#else
                if (' ' == *ptr)
                    ptr++;
#endif
                ADD_FLG(FLG_MORE_BONUS);
            } else
                ptr = prev_ptr;
        }

        if (MATCH_KEY(KEY_WORTHLESS))
            ADD_FLG(FLG_WORTHLESS);
        if (MATCH_KEY(KEY_EGO))
            ADD_FLG(FLG_EGO);
        if (MATCH_KEY(KEY_GOOD))
            ADD_FLG(FLG_GOOD);
        if (MATCH_KEY(KEY_NAMELESS))
            ADD_FLG(FLG_NAMELESS);
        if (MATCH_KEY(KEY_AVERAGE))
            ADD_FLG(FLG_AVERAGE);
        if (MATCH_KEY(KEY_RARE))
            ADD_FLG(FLG_RARE);
        if (MATCH_KEY(KEY_COMMON))
            ADD_FLG(FLG_COMMON);
        if (MATCH_KEY(KEY_WANTED))
            ADD_FLG(FLG_WANTED);
        if (MATCH_KEY(KEY_UNIQUE))
            ADD_FLG(FLG_UNIQUE);
        if (MATCH_KEY(KEY_HUMAN))
            ADD_FLG(FLG_HUMAN);
        if (MATCH_KEY(KEY_UNREADABLE))
            ADD_FLG(FLG_UNREADABLE);
        if (MATCH_KEY(KEY_REALM1))
            ADD_FLG(FLG_REALM1);
        if (MATCH_KEY(KEY_REALM2))
            ADD_FLG(FLG_REALM2);
        if (MATCH_KEY(KEY_FIRST))
            ADD_FLG(FLG_FIRST);
        if (MATCH_KEY(KEY_SECOND))
            ADD_FLG(FLG_SECOND);
        if (MATCH_KEY(KEY_THIRD))
            ADD_FLG(FLG_THIRD);
        if (MATCH_KEY(KEY_FOURTH))
            ADD_FLG(FLG_FOURTH);
    }

    int prev_flg = -1;
    if (MATCH_KEY2(KEY_ARTIFACT))
        ADD_FLG_NOUN(FLG_ARTIFACT);

    if (MATCH_KEY2(KEY_ITEMS))
        ADD_FLG_NOUN(FLG_ITEMS);
    else if (MATCH_KEY2(KEY_WEAPONS))
        ADD_FLG_NOUN(FLG_WEAPONS);
    else if (MATCH_KEY2(KEY_FAVORITE_WEAPONS))
        ADD_FLG_NOUN(FLG_FAVORITE_WEAPONS);
    else if (MATCH_KEY2(KEY_ARMORS))
        ADD_FLG_NOUN(FLG_ARMORS);
    else if (MATCH_KEY2(KEY_MISSILES))
        ADD_FLG_NOUN(FLG_MISSILES);
    else if (MATCH_KEY2(KEY_DEVICES))
        ADD_FLG_NOUN(FLG_DEVICES);
    else if (MATCH_KEY2(KEY_LIGHTS))
        ADD_FLG_NOUN(FLG_LIGHTS);
    else if (MATCH_KEY2(KEY_JUNKS))
        ADD_FLG_NOUN(FLG_JUNKS);
    else if (MATCH_KEY2(KEY_CORPSES))
        ADD_FLG_NOUN(FLG_CORPSES);
    else if (MATCH_KEY2(KEY_SPELLBOOKS))
        ADD_FLG_NOUN(FLG_SPELLBOOKS);
    else if (MATCH_KEY2(KEY_HAFTED))
        ADD_FLG_NOUN(FLG_HAFTED);
    else if (MATCH_KEY2(KEY_SHIELDS))
        ADD_FLG_NOUN(FLG_SHIELDS);
    else if (MATCH_KEY2(KEY_BOWS))
        ADD_FLG_NOUN(FLG_BOWS);
    else if (MATCH_KEY2(KEY_RINGS))
        ADD_FLG_NOUN(FLG_RINGS);
    else if (MATCH_KEY2(KEY_AMULETS))
        ADD_FLG_NOUN(FLG_AMULETS);
    else if (MATCH_KEY2(KEY_SUITS))
        ADD_FLG_NOUN(FLG_SUITS);
    else if (MATCH_KEY2(KEY_CLOAKS))
        ADD_FLG_NOUN(FLG_CLOAKS);
    else if (MATCH_KEY2(KEY_HELMS))
        ADD_FLG_NOUN(FLG_HELMS);
    else if (MATCH_KEY2(KEY_GLOVES))
        ADD_FLG_NOUN(FLG_GLOVES);
    else if (MATCH_KEY2(KEY_BOOTS))
        ADD_FLG_NOUN(FLG_BOOTS);

    if (*ptr == ':')
        ptr++;
#ifdef JP
    else if (ptr[0] == kanji_colon[0] && ptr[1] == kanji_colon[1])
        ptr += 2;
#endif
    else if (*ptr == '\0') {
        if (prev_flg == -1)
            ADD_FLG_NOUN(FLG_ITEMS);
    } else {
        if (prev_flg != -1) {
            entry->flag[prev_flg / 32] &= ~(1UL << (prev_flg % 32));
            ptr = prev_ptr;
        }
    }

    entry->name = string_make(ptr);
    entry->action = act;
    entry->insc = string_make(insc);

    return true;
}

/*!
 * @brief Get auto-picker entry from o_ptr.
 */
void autopick_entry_from_object(player_type *player_ptr, autopick_type *entry, object_type *o_ptr)
{
    /* Assume that object name is to be added */
    bool name = true;
    GAME_TEXT name_str[MAX_NLEN + 32];
    name_str[0] = '\0';
    entry->insc = string_make(quark_str(o_ptr->inscription));
    entry->action = DO_AUTOPICK | DO_DISPLAY;
    entry->flag[0] = entry->flag[1] = 0L;
    entry->dice = 0;

    // エゴ銘が邪魔かもしれないので、デフォルトで「^」は付けない.
    // We can always use the ^ mark in English.
    bool is_hat_added = _(false, true);
    if (!o_ptr->is_aware()) {
        ADD_FLG(FLG_UNAWARE);
        is_hat_added = true;
    } else if (!o_ptr->is_known()) {
        if (!(o_ptr->ident & IDENT_SENSE)) {
            ADD_FLG(FLG_UNIDENTIFIED);
            is_hat_added = true;
        } else {
            switch (o_ptr->feeling) {
            case FEEL_AVERAGE:
            case FEEL_GOOD:
                ADD_FLG(FLG_NAMELESS);
                is_hat_added = true;
                break;

            case FEEL_BROKEN:
            case FEEL_CURSED:
                ADD_FLG(FLG_NAMELESS);
                ADD_FLG(FLG_WORTHLESS);
                is_hat_added = true;
                break;

            case FEEL_TERRIBLE:
            case FEEL_WORTHLESS:
                ADD_FLG(FLG_WORTHLESS);
                break;

            case FEEL_EXCELLENT:
                ADD_FLG(FLG_EGO);
                break;

            case FEEL_UNCURSED:
                break;

            default:
                break;
            }
        }
    } else {
        if (o_ptr->is_ego()) {
            if (o_ptr->is_weapon_armour_ammo()) {
                /*
                 * Base name of ego weapons and armors
                 * are almost meaningless.
                 * Register the ego type only.
                 */
                ego_item_type *e_ptr = &e_info[o_ptr->name2];
#ifdef JP
                /* エゴ銘には「^」マークが使える */
                sprintf(name_str, "^%s", e_ptr->name.c_str());
#else
                /* We ommit the basename and cannot use the ^ mark */
                strcpy(name_str, e_ptr->name.c_str());
#endif
                name = false;
                if (!o_ptr->is_rare())
                    ADD_FLG(FLG_COMMON);
            }

            ADD_FLG(FLG_EGO);
        } else if (o_ptr->is_artifact())
            ADD_FLG(FLG_ARTIFACT);
        else {
            if (o_ptr->is_equipment())
                ADD_FLG(FLG_NAMELESS);

            is_hat_added = true;
        }
    }

    if (o_ptr->is_melee_weapon()) {
        object_kind *k_ptr = &k_info[o_ptr->k_idx];

        if ((o_ptr->dd != k_ptr->dd) || (o_ptr->ds != k_ptr->ds))
            ADD_FLG(FLG_BOOSTED);
    }

    if (object_is_bounty(player_ptr, o_ptr)) {
        REM_FLG(FLG_WORTHLESS);
        ADD_FLG(FLG_WANTED);
    }

    if ((o_ptr->tval == TV_CORPSE || o_ptr->tval == TV_STATUE) && (r_info[o_ptr->pval].flags1 & RF1_UNIQUE)) {
        ADD_FLG(FLG_UNIQUE);
    }

    if (o_ptr->tval == TV_CORPSE && angband_strchr("pht", r_info[o_ptr->pval].d_char)) {
        ADD_FLG(FLG_HUMAN);
    }

    if (o_ptr->tval >= TV_LIFE_BOOK && !check_book_realm(player_ptr, o_ptr->tval, o_ptr->sval)) {
        ADD_FLG(FLG_UNREADABLE);
        if (o_ptr->tval != TV_ARCANE_BOOK)
            name = false;
    }

    bool realm_except_class = player_ptr->pclass == CLASS_SORCERER || player_ptr->pclass == CLASS_RED_MAGE;

    if (get_realm1_book(player_ptr) == o_ptr->tval && !realm_except_class) {
        ADD_FLG(FLG_REALM1);
        name = false;
    }

    if (get_realm2_book(player_ptr) == o_ptr->tval && !realm_except_class) {
        ADD_FLG(FLG_REALM2);
        name = false;
    }

    if (o_ptr->tval >= TV_LIFE_BOOK && 0 == o_ptr->sval)
        ADD_FLG(FLG_FIRST);
    if (o_ptr->tval >= TV_LIFE_BOOK && 1 == o_ptr->sval)
        ADD_FLG(FLG_SECOND);
    if (o_ptr->tval >= TV_LIFE_BOOK && 2 == o_ptr->sval)
        ADD_FLG(FLG_THIRD);
    if (o_ptr->tval >= TV_LIFE_BOOK && 3 == o_ptr->sval)
        ADD_FLG(FLG_FOURTH);

    if (o_ptr->is_ammo())
        ADD_FLG(FLG_MISSILES);
    else if (o_ptr->tval == TV_SCROLL || o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND || o_ptr->tval == TV_ROD)
        ADD_FLG(FLG_DEVICES);
    else if (o_ptr->tval == TV_LITE)
        ADD_FLG(FLG_LIGHTS);
    else if (o_ptr->tval == TV_SKELETON || o_ptr->tval == TV_BOTTLE || o_ptr->tval == TV_JUNK || o_ptr->tval == TV_STATUE)
        ADD_FLG(FLG_JUNKS);
    else if (o_ptr->tval == TV_CORPSE)
        ADD_FLG(FLG_CORPSES);
    else if (o_ptr->tval >= TV_LIFE_BOOK)
        ADD_FLG(FLG_SPELLBOOKS);
    else if (o_ptr->tval == TV_POLEARM || o_ptr->tval == TV_SWORD || o_ptr->tval == TV_DIGGING || o_ptr->tval == TV_HAFTED)
        ADD_FLG(FLG_WEAPONS);
    else if (o_ptr->tval == TV_SHIELD)
        ADD_FLG(FLG_SHIELDS);
    else if (o_ptr->tval == TV_BOW)
        ADD_FLG(FLG_BOWS);
    else if (o_ptr->tval == TV_RING)
        ADD_FLG(FLG_RINGS);
    else if (o_ptr->tval == TV_AMULET)
        ADD_FLG(FLG_AMULETS);
    else if (o_ptr->tval == TV_DRAG_ARMOR || o_ptr->tval == TV_HARD_ARMOR || o_ptr->tval == TV_SOFT_ARMOR)
        ADD_FLG(FLG_SUITS);
    else if (o_ptr->tval == TV_CLOAK)
        ADD_FLG(FLG_CLOAKS);
    else if (o_ptr->tval == TV_HELM)
        ADD_FLG(FLG_HELMS);
    else if (o_ptr->tval == TV_GLOVES)
        ADD_FLG(FLG_GLOVES);
    else if (o_ptr->tval == TV_BOOTS)
        ADD_FLG(FLG_BOOTS);

    if (!name) {
        str_tolower(name_str);
        entry->name = string_make(name_str);
        return;
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL | OD_NAME_ONLY));

    /*
     * If necessary, add a '^' which indicates the
     * beginning of line.
     */
    sprintf(name_str, "%s%s", is_hat_added ? "^" : "", o_name);
    str_tolower(name_str);
    entry->name = string_make(name_str);
}

/*!
 * @brief Reconstruct preference line from entry
 */
concptr autopick_line_from_entry(autopick_type *entry)
{
    char buf[MAX_LINELEN];
    *buf = '\0';
    if (!(entry->action & DO_DISPLAY))
        strcat(buf, "(");
    if (entry->action & DO_QUERY_AUTOPICK)
        strcat(buf, ";");
    if (entry->action & DO_AUTODESTROY)
        strcat(buf, "!");
    if (entry->action & DONT_AUTOPICK)
        strcat(buf, "~");

    char *ptr;
    ptr = buf;
    if (IS_FLG(FLG_ALL))
        ADD_KEY(KEY_ALL);
    if (IS_FLG(FLG_COLLECTING))
        ADD_KEY(KEY_COLLECTING);
    if (IS_FLG(FLG_UNAWARE))
        ADD_KEY(KEY_UNAWARE);
    if (IS_FLG(FLG_UNIDENTIFIED))
        ADD_KEY(KEY_UNIDENTIFIED);
    if (IS_FLG(FLG_IDENTIFIED))
        ADD_KEY(KEY_IDENTIFIED);
    if (IS_FLG(FLG_STAR_IDENTIFIED))
        ADD_KEY(KEY_STAR_IDENTIFIED);
    if (IS_FLG(FLG_BOOSTED))
        ADD_KEY(KEY_BOOSTED);

    if (IS_FLG(FLG_MORE_DICE)) {
        ADD_KEY(KEY_MORE_THAN);
        strcat(ptr, format("%d", entry->dice));
        ADD_KEY(KEY_DICE);
    }

    if (IS_FLG(FLG_MORE_BONUS)) {
        ADD_KEY(KEY_MORE_BONUS);
        strcat(ptr, format("%d", entry->bonus));
        ADD_KEY(KEY_MORE_BONUS2);
    }

    if (IS_FLG(FLG_UNREADABLE))
        ADD_KEY(KEY_UNREADABLE);
    if (IS_FLG(FLG_REALM1))
        ADD_KEY(KEY_REALM1);
    if (IS_FLG(FLG_REALM2))
        ADD_KEY(KEY_REALM2);
    if (IS_FLG(FLG_FIRST))
        ADD_KEY(KEY_FIRST);
    if (IS_FLG(FLG_SECOND))
        ADD_KEY(KEY_SECOND);
    if (IS_FLG(FLG_THIRD))
        ADD_KEY(KEY_THIRD);
    if (IS_FLG(FLG_FOURTH))
        ADD_KEY(KEY_FOURTH);
    if (IS_FLG(FLG_WANTED))
        ADD_KEY(KEY_WANTED);
    if (IS_FLG(FLG_UNIQUE))
        ADD_KEY(KEY_UNIQUE);
    if (IS_FLG(FLG_HUMAN))
        ADD_KEY(KEY_HUMAN);
    if (IS_FLG(FLG_WORTHLESS))
        ADD_KEY(KEY_WORTHLESS);
    if (IS_FLG(FLG_GOOD))
        ADD_KEY(KEY_GOOD);
    if (IS_FLG(FLG_NAMELESS))
        ADD_KEY(KEY_NAMELESS);
    if (IS_FLG(FLG_AVERAGE))
        ADD_KEY(KEY_AVERAGE);
    if (IS_FLG(FLG_RARE))
        ADD_KEY(KEY_RARE);
    if (IS_FLG(FLG_COMMON))
        ADD_KEY(KEY_COMMON);
    if (IS_FLG(FLG_EGO))
        ADD_KEY(KEY_EGO);

    if (IS_FLG(FLG_ARTIFACT))
        ADD_KEY(KEY_ARTIFACT);

    bool sepa_flag = true;
    if (IS_FLG(FLG_ITEMS))
        ADD_KEY2(KEY_ITEMS);
    else if (IS_FLG(FLG_WEAPONS))
        ADD_KEY2(KEY_WEAPONS);
    else if (IS_FLG(FLG_FAVORITE_WEAPONS))
        ADD_KEY2(KEY_FAVORITE_WEAPONS);
    else if (IS_FLG(FLG_ARMORS))
        ADD_KEY2(KEY_ARMORS);
    else if (IS_FLG(FLG_MISSILES))
        ADD_KEY2(KEY_MISSILES);
    else if (IS_FLG(FLG_DEVICES))
        ADD_KEY2(KEY_DEVICES);
    else if (IS_FLG(FLG_LIGHTS))
        ADD_KEY2(KEY_LIGHTS);
    else if (IS_FLG(FLG_JUNKS))
        ADD_KEY2(KEY_JUNKS);
    else if (IS_FLG(FLG_CORPSES))
        ADD_KEY2(KEY_CORPSES);
    else if (IS_FLG(FLG_SPELLBOOKS))
        ADD_KEY2(KEY_SPELLBOOKS);
    else if (IS_FLG(FLG_HAFTED))
        ADD_KEY2(KEY_HAFTED);
    else if (IS_FLG(FLG_SHIELDS))
        ADD_KEY2(KEY_SHIELDS);
    else if (IS_FLG(FLG_BOWS))
        ADD_KEY2(KEY_BOWS);
    else if (IS_FLG(FLG_RINGS))
        ADD_KEY2(KEY_RINGS);
    else if (IS_FLG(FLG_AMULETS))
        ADD_KEY2(KEY_AMULETS);
    else if (IS_FLG(FLG_SUITS))
        ADD_KEY2(KEY_SUITS);
    else if (IS_FLG(FLG_CLOAKS))
        ADD_KEY2(KEY_CLOAKS);
    else if (IS_FLG(FLG_HELMS))
        ADD_KEY2(KEY_HELMS);
    else if (IS_FLG(FLG_GLOVES))
        ADD_KEY2(KEY_GLOVES);
    else if (IS_FLG(FLG_BOOTS))
        ADD_KEY2(KEY_BOOTS);
    else if (!IS_FLG(FLG_ARTIFACT))
        sepa_flag = false;

    if (entry->name && entry->name[0]) {
        if (sepa_flag)
            strcat(buf, ":");

        int i = strlen(buf);
        int j = 0;
        while (entry->name[j] && i < MAX_LINELEN - 2 - 1) {
#ifdef JP
            if (iskanji(entry->name[j]))
                buf[i++] = entry->name[j++];
#endif
            buf[i++] = entry->name[j++];
        }
        buf[i] = '\0';
    }

    if (!entry->insc)
        return string_make(buf);

    int i, j = 0;
    strcat(buf, "#");
    i = strlen(buf);

    while (entry->insc[j] && i < MAX_LINELEN - 2) {
#ifdef JP
        if (iskanji(entry->insc[j]))
            buf[i++] = entry->insc[j++];
#endif
        buf[i++] = entry->insc[j++];
    }

    buf[i] = '\0';
    return string_make(buf);
}

/*!
 * @brief Reconstruct preference line from entry and kill entry
 */
concptr autopick_line_from_entry_kill(autopick_type *entry)
{
    concptr ptr = autopick_line_from_entry(entry);
    autopick_free_entry(entry);
    return ptr;
}

/*!
 * @brief Choose an item and get auto-picker entry from it.
 */
bool entry_from_choosed_object(player_type *player_ptr, autopick_type *entry)
{
    concptr q = _("どのアイテムを登録しますか? ", "Enter which item? ");
    concptr s = _("アイテムを持っていない。", "You have nothing to enter.");
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, nullptr, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP);
    if (!o_ptr)
        return false;

    autopick_entry_from_object(player_ptr, entry, o_ptr);
    return true;
}
