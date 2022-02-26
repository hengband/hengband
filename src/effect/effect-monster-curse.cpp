#include "effect/effect-monster-curse.h"
#include "effect/effect-monster-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "view/display-messages.h"

process_result effect_monster_curse_1(effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    if (!em_ptr->who) {
        msg_format(_("%sを指差して呪いをかけた。", "You point at %s and curse."), em_ptr->m_name);
    }
    if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
    }

    return PROCESS_CONTINUE;
}

process_result effect_monster_curse_2(effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    if (!em_ptr->who) {
        msg_format(_("%sを指差して恐ろしげに呪いをかけた。", "You point at %s and curse horribly."), em_ptr->m_name);
    }

    if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
    }

    return PROCESS_CONTINUE;
}

process_result effect_monster_curse_3(effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    if (!em_ptr->who) {
        msg_format(_("%sを指差し、恐ろしげに呪文を唱えた！", "You point at %s, incanting terribly!"), em_ptr->m_name);
    }

    if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
    }

    return PROCESS_CONTINUE;
}

process_result effect_monster_curse_4(effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    if (!em_ptr->who) {
        msg_format(_("%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。",
                       "You point at %s, screaming the word, 'DIE!'."),
            em_ptr->m_name);
    }

    if ((randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35)) && ((em_ptr->who <= 0) || (em_ptr->m_caster_ptr->r_idx != MON_KENSHIROU))) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
    }

    return PROCESS_CONTINUE;
}
