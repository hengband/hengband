/*!
 * @brief 個々のアイテム種別について、未鑑定名/鑑定後の正式な名前を取得する処理
 * @date 2020/07/07
 * @author Hourier
 */

#include "flavor/tval-description-switcher.h"
#include "flavor/flavor-util.h"
#include "locale/english.h"
#include "monster-race/monster-race.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#ifdef JP
#else
#include "monster-race/race-flags1.h"
#include "player-info/class-info.h"
#endif

static void describe_monster_ball(flavor_type *flavor_ptr)
{
    monster_race *r_ptr = &r_info[flavor_ptr->o_ptr->pval];
    if (!flavor_ptr->known)
        return;

    if (!flavor_ptr->o_ptr->pval) {
        flavor_ptr->modstr = _(" (空)", " (empty)");
        return;
    }

#ifdef JP
    sprintf(flavor_ptr->tmp_val2, " (%s)", r_ptr->name.c_str());
    flavor_ptr->modstr = flavor_ptr->tmp_val2;
#else
    flavor_ptr->t = format("%s", r_ptr->name.c_str());
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
    flavor_ptr->modstr = r_ptr->name.c_str();
#else
    flavor_ptr->t = format("%s", r_ptr->name.c_str());
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
    flavor_ptr->modstr = r_ptr->name.c_str();
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
    if (flavor_ptr->aware && (flavor_ptr->o_ptr->is_fixed_artifact() || flavor_ptr->k_ptr->gen_flags.has(TRG::INSTA_ART)))
        return;

    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%のアミュレット", "& Amulet~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#アミュレット", "& # Amulet~ of %");
    else
        flavor_ptr->basenm = _("#アミュレット", "& # Amulet~");
}

static void describe_ring(flavor_type *flavor_ptr)
{
    if (flavor_ptr->aware && (flavor_ptr->o_ptr->is_fixed_artifact() || flavor_ptr->k_ptr->gen_flags.has(TRG::INSTA_ART)))
        return;

    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の指輪", "& Ring~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#指輪", "& # Ring~ of %");
    else
        flavor_ptr->basenm = _("#指輪", "& # Ring~");

    if (!flavor_ptr->k_ptr->to_h && !flavor_ptr->k_ptr->to_d && (flavor_ptr->o_ptr->to_h || flavor_ptr->o_ptr->to_d))
        flavor_ptr->show_weapon = true;
}

static void describe_staff(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の杖", "& Staff~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#杖", "& # Staff~ of %");
    else
        flavor_ptr->basenm = _("#杖", "& # Staff~");
}

static void describe_wand(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の魔法棒", "& Wand~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#魔法棒", "& # Wand~ of %");
    else
        flavor_ptr->basenm = _("#魔法棒", "& # Wand~");
}

static void describe_rod(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%のロッド", "& Rod~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#ロッド", "& # Rod~ of %");
    else
        flavor_ptr->basenm = _("#ロッド", "& # Rod~");
}

static void describe_scroll(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の巻物", "& Scroll~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("「#」と書かれた%の巻物", "& Scroll~ titled \"#\" of %");
    else
        flavor_ptr->basenm = _("「#」と書かれた巻物", "& Scroll~ titled \"#\"");
}

static void describe_potion(flavor_type *flavor_ptr)
{
    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%の薬", "& Potion~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#薬", "& # Potion~ of %");
    else
        flavor_ptr->basenm = _("#薬", "& # Potion~");
}

static void describe_food(flavor_type *flavor_ptr)
{
    if (flavor_ptr->k_ptr->flavor_name.empty())
        return;

    flavor_ptr->modstr = flavor_ptr->flavor_k_ptr->flavor_name.c_str();
    if (!flavor_ptr->flavor)
        flavor_ptr->basenm = _("%のキノコ", "& Mushroom~ of %");
    else if (flavor_ptr->aware)
        flavor_ptr->basenm = _("%の#キノコ", "& # Mushroom~ of %");
    else
        flavor_ptr->basenm = _("#キノコ", "& # Mushroom~");
}

static void describe_book_life(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "生命の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Life Magic %";
    else
        flavor_ptr->basenm = "& Life Spellbook~ %";
#endif
}

static void describe_book_sorcery(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "仙術の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Sorcery %";
    else
        flavor_ptr->basenm = "& Sorcery Spellbook~ %";
#endif
}

static void describe_book_nature(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "自然の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Nature Magic %";
    else
        flavor_ptr->basenm = "& Nature Spellbook~ %";
#endif
}

static void describe_book_chaos(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "カオスの魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Chaos Magic %";
    else
        flavor_ptr->basenm = "& Chaos Spellbook~ %";
#endif
}

static void describe_book_death(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "暗黒の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Death Magic %";
    else
        flavor_ptr->basenm = "& Death Spellbook~ %";
#endif
}

static void describe_book_trump(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "トランプの魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Trump Magic %";
    else
        flavor_ptr->basenm = "& Trump Spellbook~ %";
#endif
}

static void describe_book_arcane(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "秘術の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Arcane Magic %";
    else
        flavor_ptr->basenm = "& Arcane Spellbook~ %";
#endif
}

static void describe_book_craft(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "匠の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Craft Magic %";
    else
        flavor_ptr->basenm = "& Craft Spellbook~ %";
#endif
}

static void describe_book_demon(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "悪魔の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Daemon Magic %";
    else
        flavor_ptr->basenm = "& Daemon Spellbook~ %";
#endif
}

static void describe_book_crusade(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "破邪の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Crusade Magic %";
    else
        flavor_ptr->basenm = "& Crusade Spellbook~ %";
#endif
}

static void describe_book_hex(flavor_type *flavor_ptr)
{
#ifdef JP
    flavor_ptr->basenm = "呪術の魔法書%";
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK)
        flavor_ptr->basenm = "& Book~ of Hex Magic %";
    else
        flavor_ptr->basenm = "& Hex Spellbook~ %";
#endif
}

void switch_tval_description(flavor_type *flavor_ptr)
{
    switch (flavor_ptr->o_ptr->tval) {
    case ItemKindType::NONE:
        flavor_ptr->basenm = _("(なし)", "(Nothing)");
        break;
    case ItemKindType::SKELETON:
    case ItemKindType::BOTTLE:
    case ItemKindType::JUNK:
    case ItemKindType::SPIKE:
    case ItemKindType::FLASK:
    case ItemKindType::CHEST:
    case ItemKindType::WHISTLE:
        break;
    case ItemKindType::CAPTURE:
        describe_monster_ball(flavor_ptr);
        break;
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
        describe_statue(flavor_ptr);
        break;
    case ItemKindType::CORPSE:
        describe_corpse(flavor_ptr);
        break;
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::BOW:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
        flavor_ptr->show_weapon = true;
        break;
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CLOAK:
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
    case ItemKindType::SHIELD:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        flavor_ptr->show_armour = true;
        break;
    case ItemKindType::LITE:
        break;
    case ItemKindType::AMULET:
        describe_amulet(flavor_ptr);
        break;
    case ItemKindType::RING:
        describe_ring(flavor_ptr);
        break;
    case ItemKindType::CARD:
        break;
    case ItemKindType::STAFF:
        describe_staff(flavor_ptr);
        break;
    case ItemKindType::WAND:
        describe_wand(flavor_ptr);
        break;
    case ItemKindType::ROD:
        describe_rod(flavor_ptr);
        break;
    case ItemKindType::SCROLL:
        describe_scroll(flavor_ptr);
        break;
    case ItemKindType::POTION:
        describe_potion(flavor_ptr);
        break;
    case ItemKindType::FOOD:
        describe_food(flavor_ptr);
        break;
    case ItemKindType::PARCHMENT:
        flavor_ptr->basenm = _("羊皮紙 - %", "& Parchment~ - %");
        break;
    case ItemKindType::LIFE_BOOK:
        describe_book_life(flavor_ptr);
        break;
    case ItemKindType::SORCERY_BOOK:
        describe_book_sorcery(flavor_ptr);
        break;
    case ItemKindType::NATURE_BOOK:
        describe_book_nature(flavor_ptr);
        break;
    case ItemKindType::CHAOS_BOOK:
        describe_book_chaos(flavor_ptr);
        break;
    case ItemKindType::DEATH_BOOK:
        describe_book_death(flavor_ptr);
        break;
    case ItemKindType::TRUMP_BOOK:
        describe_book_trump(flavor_ptr);
        break;
    case ItemKindType::ARCANE_BOOK:
        describe_book_arcane(flavor_ptr);
        break;
    case ItemKindType::CRAFT_BOOK:
        describe_book_craft(flavor_ptr);
        break;
    case ItemKindType::DEMON_BOOK:
        describe_book_demon(flavor_ptr);
        break;
    case ItemKindType::CRUSADE_BOOK:
        describe_book_crusade(flavor_ptr);
        break;
    case ItemKindType::MUSIC_BOOK:
        flavor_ptr->basenm = _("歌集%", "& Song Book~ %");
        break;
    case ItemKindType::HISSATSU_BOOK:
        flavor_ptr->basenm = _("& 武芸の書%", "Book~ of Kendo %");
        break;
    case ItemKindType::HEX_BOOK:
        describe_book_hex(flavor_ptr);
        break;
    case ItemKindType::GOLD:
        strcpy(flavor_ptr->buf, flavor_ptr->basenm);
        return;
    default:
        strcpy(flavor_ptr->buf, _("(なし)", "(nothing)"));
        return;
    }
}
