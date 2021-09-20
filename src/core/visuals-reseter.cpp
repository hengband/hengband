#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "io/read-pref-file.h"
#include "monster-race/monster-race.h"
#include "object/object-kind.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief オブジェクト、地形の表示シンボルなど初期化する / Reset the "visual" lists
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void reset_visuals(player_type *player_ptr)
{
    for (auto &f_ref : f_info) {
        for (int j = 0; j < F_LIT_MAX; j++) {
            f_ref.x_attr[j] = f_ref.d_attr[j];
            f_ref.x_char[j] = f_ref.d_char[j];
        }
    }

    for (auto &k_ref : k_info) {
        k_ref.x_attr = k_ref.d_attr;
        k_ref.x_char = k_ref.d_char;
    }

    for (auto &r_ref : r_info) {
        r_ref.x_attr = r_ref.d_attr;
        r_ref.x_char = r_ref.d_char;
    }

    concptr pref_file = use_graphics ? "graf.prf" : "font.prf";
    concptr base_name = use_graphics ? "graf-%s.prf" : "font-%s.prf";
    char buf[1024];
    process_pref_file(player_ptr, pref_file);
    sprintf(buf, base_name, player_ptr->base_name);
    process_pref_file(player_ptr, buf);
}
