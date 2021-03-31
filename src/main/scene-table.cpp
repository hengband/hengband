#include "main/scene-table.h"
#include "main/scene-table-floor.h"
#include "main/scene-table-monster.h"
#include "term/z-term.h"

scene_type_list scene_table_mute = { { TERM_XTRA_MUSIC_MUTE, 0 } };

scene_type_list &get_scene_type_list(scene_table_type type)
{
    switch (type) {
    case scene_table_type::SCENE_TABLE_FLOOR:
        return get_floor_scene_type_list();
    case scene_table_type::SCENE_TABLE_MONSTER:
        return get_monster_scene_type_list();
    default:
        return scene_table_mute;
    }
}
