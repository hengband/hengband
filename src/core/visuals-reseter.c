#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "io/read-pref-file.h"
#include "monster-race/monster-race.h"
#include "object/object-kind.h"

/*!
 * @brief オブジェクト、地形の表示シンボルなど初期化する / Reset the "visual" lists
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param process_autopick_file_command 自動拾いファイル読み込みへの関数ポインタ
 * @return なし
 */
void reset_visuals(player_type *owner_ptr, void (*process_autopick_file_command)(char *))
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

    char *pref_file = use_graphics ? "graf.prf" : "font.prf";
    char *base_name = use_graphics ? "graf-%s.prf" : "font-%s.prf";
    char buf[1024];
    process_pref_file(owner_ptr, pref_file, process_autopick_file_command);
    sprintf(buf, base_name, owner_ptr->base_name);
    process_pref_file(owner_ptr, buf, process_autopick_file_command);
}
