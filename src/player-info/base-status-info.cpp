#include "player-info/base-status-info.h"
#include "inventory/inventory-slot-types.h"
#include "player-info/self-info-util.h"
#include "player/player-status-flags.h"
#include "object/object-flags.h"
#include "object-enchant/tr-types.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

void set_equipment_influence(player_type *creature_ptr, self_info_type *self_ptr)
{
    for (int k = INVEN_MAIN_HAND; k < INVEN_TOTAL; k++) {
        TrFlags tflgs;
        object_type *o_ptr = &creature_ptr->inventory_list[k];
        if (o_ptr->k_idx == 0)
            continue;

        object_flags(o_ptr, tflgs);
        for (int j = 0; j < TR_FLAG_SIZE; j++)
            self_ptr->flags[j] |= tflgs[j];
    }

    if (has_flag(self_ptr->flags, TR_STR))
        self_ptr->info[self_ptr->line++] = _("あなたの腕力は装備によって影響を受けている。", "Your strength is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_INT))
        self_ptr->info[self_ptr->line++] = _("あなたの知能は装備によって影響を受けている。", "Your intelligence is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_WIS))
        self_ptr->info[self_ptr->line++] = _("あなたの賢さは装備によって影響を受けている。", "Your wisdom is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_DEX))
        self_ptr->info[self_ptr->line++] = _("あなたの器用さは装備によって影響を受けている。", "Your dexterity is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_CON))
        self_ptr->info[self_ptr->line++] = _("あなたの耐久力は装備によって影響を受けている。", "Your constitution is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_CHR))
        self_ptr->info[self_ptr->line++] = _("あなたの魅力は装備によって影響を受けている。", "Your charisma is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_STEALTH))
        self_ptr->info[self_ptr->line++] = _("あなたの隠密行動能力は装備によって影響を受けている。", "Your stealth is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_SEARCH))
        self_ptr->info[self_ptr->line++] = _("あなたの探索能力は装備によって影響を受けている。", "Your searching ability is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_INFRA))
        self_ptr->info[self_ptr->line++] = _("あなたの赤外線視力は装備によって影響を受けている。", "Your infravision is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_TUNNEL))
        self_ptr->info[self_ptr->line++] = _("あなたの採掘能力は装備によって影響を受けている。", "Your digging ability is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_SPEED))
        self_ptr->info[self_ptr->line++] = _("あなたのスピードは装備によって影響を受けている。", "Your speed is affected by your equipment.");

    if (has_flag(self_ptr->flags, TR_BLOWS))
        self_ptr->info[self_ptr->line++] = _("あなたの攻撃速度は装備によって影響を受けている。", "Your attack speed is affected by your equipment.");
}

void set_status_sustain_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (has_sustain_str(creature_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたの腕力は維持されている。", "Your strength is sustained.");
    }
    if (has_sustain_int(creature_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたの知能は維持されている。", "Your intelligence is sustained.");
    }
    if (has_sustain_wis(creature_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたの賢さは維持されている。", "Your wisdom is sustained.");
    }
    if (has_sustain_con(creature_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたの耐久力は維持されている。", "Your constitution is sustained.");
    }
    if (has_sustain_dex(creature_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたの器用さは維持されている。", "Your dexterity is sustained.");
    }
    if (has_sustain_chr(creature_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたの魅力は維持されている。", "Your charisma is sustained.");
    }
}
