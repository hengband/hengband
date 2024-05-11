#include "room/rooms-pit-nest.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "room/door-definition.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "util/probability-table.h"
#include "util/sort.h"
#include "wizard/wizard-messages.h"
#include <array>
#include <optional>
#include <utility>
#include <vector>

/*!
 * @brief ダンジョン毎に指定されたピット配列を基準にランダムなpit/nestタイプを決める
 * @param l_ptr 選択されたpit/nest情報を返す参照ポインタ
 * @param allow_flag_mask 生成が許されるpit/nestのビット配列
 * @return 選択されたpit/nestのID、選択失敗した場合-1を返す。
 */
int pick_vault_type(const std::vector<nest_pit_type> &np_types, uint16_t allow_flag_mask, int dun_level)
{
    ProbabilityTable<int> table;
    for (size_t i = 0; i < np_types.size(); i++) {
        const nest_pit_type *n_ptr = &np_types.at(i);

        if (n_ptr->level > dun_level) {
            continue;
        }

        if (!(allow_flag_mask & (1UL << i))) {
            continue;
        }

        table.entry_item(i, n_ptr->chance * MAX_DEPTH / (std::min(dun_level, MAX_DEPTH - 1) - n_ptr->level + 5));
    }

    return !table.empty() ? table.pick_one_at_random() : -1;
}

/*!
 * @brief デバッグ時に生成されたpit/nestの型を出力する処理
 * @param type pit/nestの型ID
 * @param nest TRUEならばnest、FALSEならばpit
 * @return デバッグ表示文字列の参照ポインタ
 * @details
 * Hack -- Get the string describing subtype of pit/nest
 * Determined in prepare function (some pit/nest only)
 */
std::string pit_subtype_string(int type, bool nest)
{
    if (nest) {
        switch (type) {
        case NEST_TYPE_CLONE:
            return std::string("(").append(monraces_info[vault_aux_race].name).append(1, ')');
        case NEST_TYPE_SYMBOL_GOOD:
        case NEST_TYPE_SYMBOL_EVIL:
            return std::string("(").append(1, vault_aux_char).append(1, ')');
        }

        return std::string();
    }

    /* Pits */
    switch (type) {
    case PIT_TYPE_SYMBOL_GOOD:
    case PIT_TYPE_SYMBOL_EVIL:
        return std::string("(").append(1, vault_aux_char).append(1, ')');
        break;
    case PIT_TYPE_DRAGON:
        if (vault_aux_dragon_mask4.has_all_of({ MonsterAbilityType::BR_ACID, MonsterAbilityType::BR_ELEC, MonsterAbilityType::BR_FIRE, MonsterAbilityType::BR_COLD, MonsterAbilityType::BR_POIS })) {
            return _("(万色)", "(multi-hued)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_ACID)) {
            return _("(酸)", "(acid)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_ELEC)) {
            return _("(稲妻)", "(lightning)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_FIRE)) {
            return _("(火炎)", "(fire)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_COLD)) {
            return _("(冷気)", "(frost)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_POIS)) {
            return _("(毒)", "(poison)");
        } else {
            return _("(未定義)", "(undefined)");
        }
        break;
    }

    return std::string();
}
