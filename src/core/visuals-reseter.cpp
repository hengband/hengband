#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io/read-pref-file.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"

/*!
 * @brief オブジェクト、地形の表示シンボルなど初期化する / Reset the "visual" lists
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void reset_visuals(PlayerType *player_ptr)
{
    for (auto &terrain : TerrainList::get_instance()) {
        for (int j = 0; j < F_LIT_MAX; j++) {
            terrain.symbol_configs[j] = terrain.symbol_definitions[j];
        }
    }

    BaseitemList::get_instance().reset_all_visuals();
    MonraceList::get_instance().reset_all_visuals();
    const auto pref_file = use_graphics ? "graf.prf" : "font.prf";
    process_pref_file(player_ptr, pref_file);
    std::stringstream ss;
    ss << (use_graphics ? "graf-" : "font-") << player_ptr->base_name << ".prf";
    process_pref_file(player_ptr, ss.str());
}
