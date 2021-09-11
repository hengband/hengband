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
    for (int i = 0; i < max_f_idx; i++) {
        feature_type *f_ptr = &f_info[i];
        for (int j = 0; j < F_LIT_MAX; j++) {
            f_ptr->x_attr[j] = f_ptr->d_attr[j];
            f_ptr->x_char[j] = f_ptr->d_char[j];
        }
    }

    for (int i = 0; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        k_ptr->x_attr = k_ptr->d_attr;
        k_ptr->x_char = k_ptr->d_char;
    }

    for (int i = 0; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        r_ptr->x_attr = r_ptr->d_attr;
        r_ptr->x_char = r_ptr->d_char;
    }

    concptr pref_file = use_graphics ? "graf.prf" : "font.prf";
    concptr base_name = use_graphics ? "graf-%s.prf" : "font-%s.prf";
    char buf[1024];
    process_pref_file(player_ptr, pref_file);
    sprintf(buf, base_name, player_ptr->base_name);
    process_pref_file(player_ptr, buf);
}
