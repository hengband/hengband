#include "lore/lore-util.h"
#include "game-option/birth-options.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-race.h"
#include "system/monster-race-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

const who_word_definition Who::words = {
    { WHO_WORD_TYPE::WHO,
        {
            { false, { { MSEX_NONE, _("それ", "it") }, { MSEX_MALE, _("彼", "he") }, { MSEX_FEMALE, _("彼女", "she") } } },
            { true, { { MSEX_NONE, _("それら", "they") }, { MSEX_MALE, _("彼ら", "they") }, { MSEX_FEMALE, _("彼女ら", "they") } } },
        } },
    { WHO_WORD_TYPE::WHOSE,
        {
            { false, { { MSEX_NONE, _("それの", "its") }, { MSEX_MALE, _("彼の", "his") }, { MSEX_FEMALE, _("彼女の", "her") } } },
            { true, { { MSEX_NONE, _("それらの", "their") }, { MSEX_MALE, _("彼らの", "their") }, { MSEX_FEMALE, _("彼女らの", "their") } } },
        } },
    { WHO_WORD_TYPE::WHOM,
        {
            { false, { { MSEX_NONE, _("それ", "it") }, { MSEX_MALE, _("彼", "him") }, { MSEX_FEMALE, _("彼女", "her") } } },
            { true, { { MSEX_NONE, _("それら", "them") }, { MSEX_MALE, _("彼ら", "them") }, { MSEX_FEMALE, _("彼女ら", "them") } } },
        } },
};

/*
 * Prepare hook for c_roff(). It will be changed for spoiler generation in wizard1.c.
 */
hook_c_roff_pf hook_c_roff = c_roff;

lore_type *initialize_lore_type(lore_type *lore_ptr, MonsterRaceId r_idx, monster_lore_mode mode)
{
#ifdef JP
#else
    lore_ptr->sin = false;
#endif
    lore_ptr->r_idx = r_idx;
    lore_ptr->nightmare = ironman_nightmare && (mode != MONSTER_LORE_DEBUG);
    lore_ptr->r_ptr = &r_info[r_idx];
    lore_ptr->speed = lore_ptr->nightmare ? lore_ptr->r_ptr->speed + 5 : lore_ptr->r_ptr->speed;
    lore_ptr->drop_gold = lore_ptr->r_ptr->r_drop_gold;
    lore_ptr->drop_item = lore_ptr->r_ptr->r_drop_item;
    lore_ptr->flags1 = (lore_ptr->r_ptr->flags1 & lore_ptr->r_ptr->r_flags1);
    lore_ptr->flags2 = (lore_ptr->r_ptr->flags2 & lore_ptr->r_ptr->r_flags2);
    lore_ptr->flags3 = (lore_ptr->r_ptr->flags3 & lore_ptr->r_ptr->r_flags3);
    lore_ptr->ability_flags = (lore_ptr->r_ptr->ability_flags & lore_ptr->r_ptr->r_ability_flags);
    lore_ptr->aura_flags = (lore_ptr->r_ptr->aura_flags & lore_ptr->r_ptr->r_aura_flags);
    lore_ptr->behavior_flags = (lore_ptr->r_ptr->behavior_flags & lore_ptr->r_ptr->r_behavior_flags);
    lore_ptr->drop_flags = (lore_ptr->r_ptr->drop_flags & lore_ptr->r_ptr->r_drop_flags);
    lore_ptr->flags7 = (lore_ptr->r_ptr->flags7 & lore_ptr->r_ptr->flags7);
    lore_ptr->resistance_flags = (lore_ptr->r_ptr->resistance_flags & lore_ptr->r_ptr->r_resistance_flags);
    lore_ptr->feature_flags = (lore_ptr->r_ptr->feature_flags & lore_ptr->r_ptr->r_feature_flags);
    lore_ptr->reinforce = false;
    lore_ptr->know_everything = false;
    lore_ptr->mode = mode;
    lore_ptr->old = false;
    lore_ptr->count = 0;
    return lore_ptr;
}

/*!
 * @brief モンスターの思い出メッセージをあらかじめ指定された関数ポインタに基づき出力する
 * @param str 出力文字列
 */
void hooked_roff(concptr str)
{
    hook_c_roff(TERM_WHITE, str);
}
