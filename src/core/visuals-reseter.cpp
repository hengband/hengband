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
    for (auto &f_ref : terrains_info) {
        for (int j = 0; j < F_LIT_MAX; j++) {
            f_ref.x_attr[j] = f_ref.d_attr[j];
            f_ref.x_char[j] = f_ref.d_char[j];
        }
    }

    for (auto &baseitem : baseitems_info) {
        baseitem.x_attr = baseitem.d_attr;
        baseitem.x_char = baseitem.d_char;
    }

    for (auto &[r_idx, r_ref] : monraces_info) {
        r_ref.x_attr = r_ref.d_attr;
        r_ref.x_char = r_ref.d_char;
    }

    concptr pref_file = use_graphics ? "graf.prf" : "font.prf";
    process_pref_file(player_ptr, pref_file);
    process_pref_file(player_ptr, std::string(use_graphics ? "graf-" : "font-").append(player_ptr->base_name).append(".prf").data());
}
