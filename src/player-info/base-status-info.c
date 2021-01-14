#include "player-info/base-status-info.h"
#include "inventory/inventory-slot-types.h"
#include "player-info/self-info-util.h"
#include "object/object-flags.h"
#include "object-enchant/tr-types.h"
#include "util/bit-flags-calculator.h"

void set_equipment_influence(player_type *creature_ptr, self_info_type *si_ptr)
{
    for (int k = INVEN_RARM; k < INVEN_TOTAL; k++) {
        u32b tflgs[TR_FLAG_SIZE];
        object_type *o_ptr = &creature_ptr->inventory_list[k];
        if (o_ptr->k_idx == 0)
            continue;

        object_flags(creature_ptr, o_ptr, tflgs);
        for (int j = 0; j < TR_FLAG_SIZE; j++)
            si_ptr->flags[j] |= tflgs[j];
    }

    if (has_flag(si_ptr->flags, TR_STR))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̘r�͂͑����ɂ���ĉe�����󂯂Ă���B", "Your strength is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_INT))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̒m�\�͑����ɂ���ĉe�����󂯂Ă���B", "Your intelligence is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_WIS))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̌����͑����ɂ���ĉe�����󂯂Ă���B", "Your wisdom is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_DEX))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̊�p���͑����ɂ���ĉe�����󂯂Ă���B", "Your dexterity is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_CON))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̑ϋv�͂͑����ɂ���ĉe�����󂯂Ă���B", "Your constitution is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_CHR))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̖��͂͑����ɂ���ĉe�����󂯂Ă���B", "Your charisma is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_STEALTH))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̉B���s���\�͂͑����ɂ���ĉe�����󂯂Ă���B", "Your stealth is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_SEARCH))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̒T���\�͂͑����ɂ���ĉe�����󂯂Ă���B", "Your searching ability is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_INFRA))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̐ԊO�����͂͑����ɂ���ĉe�����󂯂Ă���B", "Your infravision is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_TUNNEL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̍̌@�\�͂͑����ɂ���ĉe�����󂯂Ă���B", "Your digging ability is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_SPEED))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̃X�s�[�h�͑����ɂ���ĉe�����󂯂Ă���B", "Your speed is affected by your equipment.");

    if (has_flag(si_ptr->flags, TR_BLOWS))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̍U�����x�͑����ɂ���ĉe�����󂯂Ă���B", "Your attack speed is affected by your equipment.");
}

void set_status_sustain_info(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->sustain_str) {
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̘r�͈͂ێ�����Ă���B", "Your strength is sustained.");
    }
    if (creature_ptr->sustain_int) {
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̒m�\�͈ێ�����Ă���B", "Your intelligence is sustained.");
    }
    if (creature_ptr->sustain_wis) {
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̌����͈ێ�����Ă���B", "Your wisdom is sustained.");
    }
    if (creature_ptr->sustain_con) {
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̑ϋv�͈͂ێ�����Ă���B", "Your constitution is sustained.");
    }
    if (creature_ptr->sustain_dex) {
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̊�p���͈ێ�����Ă���B", "Your dexterity is sustained.");
    }
    if (creature_ptr->sustain_chr) {
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̖��͈͂ێ�����Ă���B", "Your charisma is sustained.");
    }
}
