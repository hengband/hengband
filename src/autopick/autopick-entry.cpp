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
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include <optional>
#include <sstream>
#include <string>

#ifdef JP
static char kanji_colon[] = "：";
#endif

/*!
 * @brief A function to create new entry
 */
bool autopick_new_entry(autopick_type *entry, concptr str, bool allow_default)
{
    if (str[0] && str[1] == ':') {
        switch (str[0]) {
        case '?':
        case '%':
        case 'A':
        case 'P':
        case 'C':
            return false;
        }
    }

    entry->flags[0] = entry->flags[1] = 0L;
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

        if (isupper(c)) {
            c = (char)tolower(c);
        }

        buf[i] = c;
    }

    buf[i] = '\0';
    if (!allow_default && *buf == 0) {
        return false;
    }
    if (*buf == 0 && insc) {
        return false;
    }

    concptr prev_ptr, ptr;
    ptr = prev_ptr = buf;
    concptr old_ptr = nullptr;
    while (old_ptr != ptr) {
        old_ptr = ptr;
        if (MATCH_KEY(KEY_ALL)) {
            entry->add(FLG_ALL);
        }
        if (MATCH_KEY(KEY_COLLECTING)) {
            entry->add(FLG_COLLECTING);
        }
        if (MATCH_KEY(KEY_UNAWARE)) {
            entry->add(FLG_UNAWARE);
        }
        if (MATCH_KEY(KEY_UNIDENTIFIED)) {
            entry->add(FLG_UNIDENTIFIED);
        }
        if (MATCH_KEY(KEY_IDENTIFIED)) {
            entry->add(FLG_IDENTIFIED);
        }
        if (MATCH_KEY(KEY_STAR_IDENTIFIED)) {
            entry->add(FLG_STAR_IDENTIFIED);
        }
        if (MATCH_KEY(KEY_BOOSTED)) {
            entry->add(FLG_BOOSTED);
        }

        /*** Weapons whose dd*ds is more than nn ***/
        if (MATCH_KEY2(KEY_MORE_THAN)) {
            int k = 0;
            entry->dice = 0;

            while (' ' == *ptr) {
                ptr++;
            }

            while ('0' <= *ptr && *ptr <= '9') {
                entry->dice = 10 * entry->dice + (*ptr - '0');
                ptr++;
                k++;
            }

            if (k > 0 && k <= 2) {
                (void)MATCH_KEY(KEY_DICE);
                entry->add(FLG_MORE_DICE);
            } else {
                ptr = prev_ptr;
            }
        }

        /*** Items whose magical bonus is more than n ***/
        if (MATCH_KEY2(KEY_MORE_BONUS)) {
            int k = 0;
            entry->bonus = 0;

            while (' ' == *ptr) {
                ptr++;
            }

            while ('0' <= *ptr && *ptr <= '9') {
                entry->bonus = 10 * entry->bonus + (*ptr - '0');
                ptr++;
                k++;
            }

            if (k > 0 && k <= 2) {
#ifdef JP
                (void)MATCH_KEY(KEY_MORE_BONUS2);
#else
                if (' ' == *ptr) {
                    ptr++;
                }
#endif
                entry->add(FLG_MORE_BONUS);
            } else {
                ptr = prev_ptr;
            }
        }

        if (MATCH_KEY(KEY_WORTHLESS)) {
            entry->add(FLG_WORTHLESS);
        }
        if (MATCH_KEY(KEY_EGO)) {
            entry->add(FLG_EGO);
        }
        if (MATCH_KEY(KEY_GOOD)) {
            entry->add(FLG_GOOD);
        }
        if (MATCH_KEY(KEY_NAMELESS)) {
            entry->add(FLG_NAMELESS);
        }
        if (MATCH_KEY(KEY_AVERAGE)) {
            entry->add(FLG_AVERAGE);
        }
        if (MATCH_KEY(KEY_RARE)) {
            entry->add(FLG_RARE);
        }
        if (MATCH_KEY(KEY_COMMON)) {
            entry->add(FLG_COMMON);
        }
        if (MATCH_KEY(KEY_WANTED)) {
            entry->add(FLG_WANTED);
        }
        if (MATCH_KEY(KEY_UNIQUE)) {
            entry->add(FLG_UNIQUE);
        }
        if (MATCH_KEY(KEY_HUMAN)) {
            entry->add(FLG_HUMAN);
        }
        if (MATCH_KEY(KEY_UNREADABLE)) {
            entry->add(FLG_UNREADABLE);
        }
        if (MATCH_KEY(KEY_REALM1)) {
            entry->add(FLG_REALM1);
        }
        if (MATCH_KEY(KEY_REALM2)) {
            entry->add(FLG_REALM2);
        }
        if (MATCH_KEY(KEY_FIRST)) {
            entry->add(FLG_FIRST);
        }
        if (MATCH_KEY(KEY_SECOND)) {
            entry->add(FLG_SECOND);
        }
        if (MATCH_KEY(KEY_THIRD)) {
            entry->add(FLG_THIRD);
        }
        if (MATCH_KEY(KEY_FOURTH)) {
            entry->add(FLG_FOURTH);
        }
    }

    std::optional<int> previous_flag = std::nullopt;
    if (MATCH_KEY2(KEY_ARTIFACT)) {
        entry->add(FLG_ARTIFACT);
        previous_flag = FLG_ARTIFACT;
    }

    if (MATCH_KEY2(KEY_ITEMS)) {
        entry->add(FLG_ITEMS);
        previous_flag = FLG_ITEMS;
    } else if (MATCH_KEY2(KEY_WEAPONS)) {
        entry->add(FLG_WEAPONS);
        previous_flag = FLG_WEAPONS;
    } else if (MATCH_KEY2(KEY_FAVORITE_WEAPONS)) {
        entry->add(FLG_FAVORITE_WEAPONS);
        previous_flag = FLG_FAVORITE_WEAPONS;
    } else if (MATCH_KEY2(KEY_ARMORS)) {
        entry->add(FLG_ARMORS);
        previous_flag = FLG_ARMORS;
    } else if (MATCH_KEY2(KEY_MISSILES)) {
        entry->add(FLG_MISSILES);
        previous_flag = FLG_MISSILES;
    } else if (MATCH_KEY2(KEY_DEVICES)) {
        entry->add(FLG_DEVICES);
        previous_flag = FLG_DEVICES;
    } else if (MATCH_KEY2(KEY_LIGHTS)) {
        entry->add(FLG_LIGHTS);
        previous_flag = FLG_LIGHTS;
    } else if (MATCH_KEY2(KEY_JUNKS)) {
        entry->add(FLG_JUNKS);
        previous_flag = FLG_JUNKS;
    } else if (MATCH_KEY2(KEY_CORPSES)) {
        entry->add(FLG_CORPSES);
        previous_flag = FLG_CORPSES;
    } else if (MATCH_KEY2(KEY_SPELLBOOKS)) {
        entry->add(FLG_SPELLBOOKS);
        previous_flag = FLG_SPELLBOOKS;
    } else if (MATCH_KEY2(KEY_HAFTED)) {
        entry->add(FLG_HAFTED);
        previous_flag = FLG_HAFTED;
    } else if (MATCH_KEY2(KEY_SHIELDS)) {
        entry->add(FLG_SHIELDS);
        previous_flag = FLG_SHIELDS;
    } else if (MATCH_KEY2(KEY_BOWS)) {
        entry->add(FLG_BOWS);
        previous_flag = FLG_BOWS;
    } else if (MATCH_KEY2(KEY_RINGS)) {
        entry->add(FLG_RINGS);
        previous_flag = FLG_RINGS;
    } else if (MATCH_KEY2(KEY_AMULETS)) {
        entry->add(FLG_AMULETS);
        previous_flag = FLG_AMULETS;
    } else if (MATCH_KEY2(KEY_SUITS)) {
        entry->add(FLG_SUITS);
        previous_flag = FLG_SUITS;
    } else if (MATCH_KEY2(KEY_CLOAKS)) {
        entry->add(FLG_CLOAKS);
        previous_flag = FLG_CLOAKS;
    } else if (MATCH_KEY2(KEY_HELMS)) {
        entry->add(FLG_HELMS);
        previous_flag = FLG_HELMS;
    } else if (MATCH_KEY2(KEY_GLOVES)) {
        entry->add(FLG_GLOVES);
        previous_flag = FLG_GLOVES;
    } else if (MATCH_KEY2(KEY_BOOTS)) {
        entry->add(FLG_BOOTS);
        previous_flag = FLG_BOOTS;
    }

    if (*ptr == ':') {
        ptr++;
    }
#ifdef JP
    else if (ptr[0] == kanji_colon[0] && ptr[1] == kanji_colon[1]) {
        ptr += 2;
    }
#endif
    else if (*ptr == '\0') {
        if (!previous_flag.has_value()) {
            entry->add(FLG_ITEMS);
            previous_flag = FLG_ITEMS;
        }
    } else {
        if (previous_flag.has_value()) {
            entry->remove(previous_flag.value());
            ptr = prev_ptr;
        }
    }

    entry->name = ptr;
    entry->action = act;
    entry->insc = insc != nullptr ? insc : "";

    return true;
}

/*!
 * @brief Get auto-picker entry from o_ptr.
 */
void autopick_entry_from_object(PlayerType *player_ptr, autopick_type *entry, ItemEntity *o_ptr)
{
    /* Assume that object name is to be added */
    bool name = true;
    entry->name.clear();
    entry->insc = o_ptr->inscription.value_or("");
    entry->action = DO_AUTOPICK | DO_DISPLAY;
    entry->flags[0] = entry->flags[1] = 0L;
    entry->dice = 0;

    // エゴ銘が邪魔かもしれないので、デフォルトで「^」は付けない.
    // We can always use the ^ mark in English.
    bool is_hat_added = _(false, true);
    if (!o_ptr->is_aware()) {
        entry->add(FLG_UNAWARE);
        is_hat_added = true;
    } else if (!o_ptr->is_known()) {
        if (!(o_ptr->ident & IDENT_SENSE)) {
            entry->add(FLG_UNIDENTIFIED);
            is_hat_added = true;
        } else {
            switch (o_ptr->feeling) {
            case FEEL_AVERAGE:
            case FEEL_GOOD:
                entry->add(FLG_NAMELESS);
                is_hat_added = true;
                break;

            case FEEL_BROKEN:
            case FEEL_CURSED:
                entry->add(FLG_NAMELESS);
                entry->add(FLG_WORTHLESS);
                is_hat_added = true;
                break;

            case FEEL_TERRIBLE:
            case FEEL_WORTHLESS:
                entry->add(FLG_WORTHLESS);
                break;

            case FEEL_EXCELLENT:
                entry->add(FLG_EGO);
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
                auto &ego = o_ptr->get_ego();
#ifdef JP
                /* エゴ銘には「^」マークが使える */
                entry->name = "^";
                entry->name.append(ego.name);
#else
                /* We omit the basename and cannot use the ^ mark */
                entry->name = ego.name;
#endif
                name = false;
                if (!o_ptr->is_rare()) {
                    entry->add(FLG_COMMON);
                }
            }

            entry->add(FLG_EGO);
        } else if (o_ptr->is_fixed_or_random_artifact()) {
            entry->add(FLG_ARTIFACT);
        } else {
            if (o_ptr->is_equipment()) {
                entry->add(FLG_NAMELESS);
            }

            is_hat_added = true;
        }
    }

    if (o_ptr->is_melee_weapon()) {
        const auto &baseitem = o_ptr->get_baseitem();
        if ((o_ptr->dd != baseitem.dd) || (o_ptr->ds != baseitem.ds)) {
            entry->add(FLG_BOOSTED);
        }
    }

    if (object_is_bounty(player_ptr, o_ptr)) {
        entry->remove(FLG_WORTHLESS);
        entry->add(FLG_WANTED);
    }

    const auto r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
    const auto &bi_key = o_ptr->bi_key;
    const auto tval = bi_key.tval();
    if ((tval == ItemKindType::CORPSE || tval == ItemKindType::STATUE) && monraces_info[r_idx].kind_flags.has(MonsterKindType::UNIQUE)) {
        entry->add(FLG_UNIQUE);
    }

    if (tval == ItemKindType::CORPSE && angband_strchr("pht", monraces_info[r_idx].d_char)) {
        entry->add(FLG_HUMAN);
    }

    if (o_ptr->is_spell_book() && !check_book_realm(player_ptr, bi_key)) {
        entry->add(FLG_UNREADABLE);
        if (tval != ItemKindType::ARCANE_BOOK) {
            name = false;
        }
    }

    PlayerClass pc(player_ptr);
    auto realm_except_class = pc.equals(PlayerClassType::SORCERER) || pc.equals(PlayerClassType::RED_MAGE);

    if ((get_realm1_book(player_ptr) == tval) && !realm_except_class) {
        entry->add(FLG_REALM1);
        name = false;
    }

    if ((get_realm2_book(player_ptr) == tval) && !realm_except_class) {
        entry->add(FLG_REALM2);
        name = false;
    }

    const auto sval = bi_key.sval();
    if (o_ptr->is_spell_book() && (sval == 0)) {
        entry->add(FLG_FIRST);
    }
    if (o_ptr->is_spell_book() && (sval == 1)) {
        entry->add(FLG_SECOND);
    }
    if (o_ptr->is_spell_book() && (sval == 2)) {
        entry->add(FLG_THIRD);
    }
    if (o_ptr->is_spell_book() && (sval == 3)) {
        entry->add(FLG_FOURTH);
    }

    if (o_ptr->is_ammo()) {
        entry->add(FLG_MISSILES);
    } else if (tval == ItemKindType::SCROLL || o_ptr->is_wand_staff() || o_ptr->is_wand_rod()) {
        entry->add(FLG_DEVICES);
    } else if (tval == ItemKindType::LITE) {
        entry->add(FLG_LIGHTS);
    } else if (o_ptr->is_junk()) {
        entry->add(FLG_JUNKS);
    } else if (tval == ItemKindType::CORPSE) {
        entry->add(FLG_CORPSES);
    } else if (o_ptr->is_spell_book()) {
        entry->add(FLG_SPELLBOOKS);
    } else if (o_ptr->is_melee_weapon()) {
        entry->add(FLG_WEAPONS);
    } else if (tval == ItemKindType::SHIELD) {
        entry->add(FLG_SHIELDS);
    } else if (tval == ItemKindType::BOW) {
        entry->add(FLG_BOWS);
    } else if (tval == ItemKindType::RING) {
        entry->add(FLG_RINGS);
    } else if (tval == ItemKindType::AMULET) {
        entry->add(FLG_AMULETS);
    } else if (o_ptr->is_armour()) {
        entry->add(FLG_SUITS);
    } else if (tval == ItemKindType::CLOAK) {
        entry->add(FLG_CLOAKS);
    } else if (tval == ItemKindType::HELM) {
        entry->add(FLG_HELMS);
    } else if (tval == ItemKindType::GLOVES) {
        entry->add(FLG_GLOVES);
    } else if (tval == ItemKindType::BOOTS) {
        entry->add(FLG_BOOTS);
    }

    if (!name) {
        str_tolower(entry->name.data());
        return;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL | OD_NAME_ONLY));

    /*
     * If necessary, add a '^' which indicates the
     * beginning of line.
     */
    entry->name = std::string(is_hat_added ? "^" : "").append(item_name);
    str_tolower(entry->name.data());
}

std::string shape_autopick_key(const std::string &key)
{
#ifdef JP
    return key;
#else
    std::stringstream ss;
    ss << key << ' ';
    return ss.str();
#endif
}

/*!
 * @brief Reconstruct preference line from entry
 */
concptr autopick_line_from_entry(const autopick_type &entry)
{
    std::stringstream ss;
    if (!(entry.action & DO_DISPLAY)) {
        ss << '(';
    }

    if (entry.action & DO_QUERY_AUTOPICK) {
        ss << ';';
    }

    if (entry.action & DO_AUTODESTROY) {
        ss << '!';
    }

    if (entry.action & DONT_AUTOPICK) {
        ss << '~';
    }

    if (entry.has(FLG_ALL)) {
        ss << shape_autopick_key(KEY_ALL);
    }
    if (entry.has(FLG_COLLECTING)) {
        ss << shape_autopick_key(KEY_COLLECTING);
    }
    if (entry.has(FLG_UNAWARE)) {
        ss << shape_autopick_key(KEY_UNAWARE);
    }
    if (entry.has(FLG_UNIDENTIFIED)) {
        ss << shape_autopick_key(KEY_UNIDENTIFIED);
    }
    if (entry.has(FLG_IDENTIFIED)) {
        ss << shape_autopick_key(KEY_IDENTIFIED);
    }
    if (entry.has(FLG_STAR_IDENTIFIED)) {
        ss << shape_autopick_key(KEY_STAR_IDENTIFIED);
    }
    if (entry.has(FLG_BOOSTED)) {
        ss << shape_autopick_key(KEY_BOOSTED);
    }

    if (entry.has(FLG_MORE_DICE)) {
        ss << shape_autopick_key(KEY_MORE_THAN);
        ss << entry.dice;
        ss << shape_autopick_key(KEY_DICE);
    }

    if (entry.has(FLG_MORE_BONUS)) {
        ss << shape_autopick_key(KEY_MORE_BONUS);
        ss << entry.bonus;
        ss << shape_autopick_key(KEY_MORE_BONUS2);
    }

    if (entry.has(FLG_UNREADABLE)) {
        ss << shape_autopick_key(KEY_UNREADABLE);
    }

    if (entry.has(FLG_REALM1)) {
        ss << shape_autopick_key(KEY_REALM1);
    }

    if (entry.has(FLG_REALM2)) {
        ss << shape_autopick_key(KEY_REALM2);
    }

    if (entry.has(FLG_FIRST)) {
        ss << shape_autopick_key(KEY_FIRST);
    }

    if (entry.has(FLG_SECOND)) {
        ss << shape_autopick_key(KEY_SECOND);
    }

    if (entry.has(FLG_THIRD)) {
        ss << shape_autopick_key(KEY_THIRD);
    }

    if (entry.has(FLG_FOURTH)) {
        ss << shape_autopick_key(KEY_FOURTH);
    }

    if (entry.has(FLG_WANTED)) {
        ss << shape_autopick_key(KEY_WANTED);
    }

    if (entry.has(FLG_UNIQUE)) {
        ss << shape_autopick_key(KEY_UNIQUE);
    }

    if (entry.has(FLG_HUMAN)) {
        ss << shape_autopick_key(KEY_HUMAN);
    }

    if (entry.has(FLG_WORTHLESS)) {
        ss << shape_autopick_key(KEY_WORTHLESS);
    }

    if (entry.has(FLG_GOOD)) {
        ss << shape_autopick_key(KEY_GOOD);
    }

    if (entry.has(FLG_NAMELESS)) {
        ss << shape_autopick_key(KEY_NAMELESS);
    }

    if (entry.has(FLG_AVERAGE)) {
        ss << shape_autopick_key(KEY_AVERAGE);
    }

    if (entry.has(FLG_RARE)) {
        ss << shape_autopick_key(KEY_RARE);
    }

    if (entry.has(FLG_COMMON)) {
        ss << shape_autopick_key(KEY_COMMON);
    }

    if (entry.has(FLG_EGO)) {
        ss << shape_autopick_key(KEY_EGO);
    }

    if (entry.has(FLG_ARTIFACT)) {
        ss << shape_autopick_key(KEY_ARTIFACT);
    }

    auto should_separate = true;
    if (entry.has(FLG_ITEMS)) {
        ss << KEY_ITEMS;
    } else if (entry.has(FLG_WEAPONS)) {
        ss << KEY_WEAPONS;
    } else if (entry.has(FLG_FAVORITE_WEAPONS)) {
        ss << KEY_FAVORITE_WEAPONS;
    } else if (entry.has(FLG_ARMORS)) {
        ss << KEY_ARMORS;
    } else if (entry.has(FLG_MISSILES)) {
        ss << KEY_MISSILES;
    } else if (entry.has(FLG_DEVICES)) {
        ss << KEY_DEVICES;
    } else if (entry.has(FLG_LIGHTS)) {
        ss << KEY_LIGHTS;
    } else if (entry.has(FLG_JUNKS)) {
        ss << KEY_JUNKS;
    } else if (entry.has(FLG_CORPSES)) {
        ss << KEY_CORPSES;
    } else if (entry.has(FLG_SPELLBOOKS)) {
        ss << KEY_SPELLBOOKS;
    } else if (entry.has(FLG_HAFTED)) {
        ss << KEY_HAFTED;
    } else if (entry.has(FLG_SHIELDS)) {
        ss << KEY_SHIELDS;
    } else if (entry.has(FLG_BOWS)) {
        ss << KEY_BOWS;
    } else if (entry.has(FLG_RINGS)) {
        ss << KEY_RINGS;
    } else if (entry.has(FLG_AMULETS)) {
        ss << KEY_AMULETS;
    } else if (entry.has(FLG_SUITS)) {
        ss << KEY_SUITS;
    } else if (entry.has(FLG_CLOAKS)) {
        ss << KEY_CLOAKS;
    } else if (entry.has(FLG_HELMS)) {
        ss << KEY_HELMS;
    } else if (entry.has(FLG_GLOVES)) {
        ss << KEY_GLOVES;
    } else if (entry.has(FLG_BOOTS)) {
        ss << KEY_BOOTS;
    } else if (!entry.has(FLG_ARTIFACT)) {
        should_separate = false;
    }

    if (!entry.name.empty()) {
        if (should_separate) {
            ss << ":";
        }

        ss << entry.name;
    }

    if (entry.insc.empty()) {
        auto str = ss.str();
        return string_make(str.data());
    }

    ss << '#' << entry.insc;
    auto str = ss.str();
    return string_make(str.data());
}

/*!
 * @brief Choose an item and get auto-picker entry from it.
 */
bool entry_from_choosed_object(PlayerType *player_ptr, autopick_type *entry)
{
    constexpr auto q = _("どのアイテムを登録しますか? ", "Enter which item? ");
    constexpr auto s = _("アイテムを持っていない。", "You have nothing to enter.");
    auto *o_ptr = choose_object(player_ptr, nullptr, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP);
    if (!o_ptr) {
        return false;
    }

    autopick_entry_from_object(player_ptr, entry, o_ptr);
    return true;
}
