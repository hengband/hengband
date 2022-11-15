#include "floor/floor-save-util.h"

/*
 * Sign for current process used in temporary files.
 * Actually it is the start time of current process.
 */
uint32_t saved_floor_file_sign;
saved_floor_type saved_floors[MAX_SAVED_FLOORS];
FLOOR_IDX max_floor_id; /*!< Number of floor_id used from birth */
FLOOR_IDX new_floor_id; /*!<次のフロアのID / floor_id of the destination */
uint32_t latest_visit_mark; /*!<フロアを渡った回数？(確認中) / Max number of visit_mark */
MonsterEntity party_mon[MAX_PARTY_MON]; /*!< フロア移動に保存するペットモンスターの配列 */
