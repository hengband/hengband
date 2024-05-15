#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io/read-pref-file.h"
#include "monster-race/monster-race.h"
#include "system/baseitem-info.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"

/*!
 * @brief オブジェクト、地形の表示シンボルなど初期化する / Reset the "visual" lists
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void reset_visuals(PlayerType *player_ptr)
{
    for (auto &terrain : TerrainList::get_instance()) {
        for (int j = 0; j < F_LIT_MAX; j++) {
            terrain.cc_configs[j] = terrain.cc_defs[j];
        }
    }

    BaseitemList::get_instance().reset_all_visuals();
    for (auto &[monrace_id, monrace] : monraces_info) {
        monrace.x_attr = monrace.d_attr;
        monrace.x_char = monrace.d_char;
    }

    const auto pref_file = use_graphics ? "graf.prf" : "font.prf";
    process_pref_file(player_ptr, pref_file);
    std::stringstream ss;
    ss << (use_graphics ? "graf-" : "font-") << player_ptr->base_name << ".prf";
    process_pref_file(player_ptr, ss.str());
}
