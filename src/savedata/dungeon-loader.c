#include "savedata/dungeon-loader.h"
#include "floor/floor-save.h"
#include "floor/floor.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/floor-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-5-0.h"
#include "savedata/save.h"
#include "world/world.h"

/*!
 * @brief 保存されたフロアを読み込む / Read the dungeon
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 * @details
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
errr rd_dungeon(player_type *player_ptr)
{
    init_saved_floors(player_ptr, FALSE);
    errr err = 0;
    if (h_older_than(1, 5, 0, 0)) {
        err = rd_dungeon_old(player_ptr);
        if (player_ptr->dungeon_idx) {
            player_ptr->floor_id = get_new_floor_id(player_ptr);
            get_sf_ptr(player_ptr->floor_id)->dun_level = player_ptr->current_floor_ptr->dun_level;
        }

        return err;
    }

    rd_s16b(&max_floor_id);
    byte tmp8u;
    rd_byte(&tmp8u);
    player_ptr->dungeon_idx = (DUNGEON_IDX)tmp8u;
    byte num;
    rd_byte(&num);
    if (num == 0) {
        err = rd_saved_floor(player_ptr, NULL);
    } else {
        for (int i = 0; i < num; i++) {
            saved_floor_type *sf_ptr = &saved_floors[i];

            rd_s16b(&sf_ptr->floor_id);
            rd_byte(&tmp8u);
            sf_ptr->savefile_id = (s16b)tmp8u;

            s16b tmp16s;
            rd_s16b(&tmp16s);
            sf_ptr->dun_level = (DEPTH)tmp16s;

            rd_s32b(&sf_ptr->last_visit);
            rd_u32b(&sf_ptr->visit_mark);
            rd_s16b(&sf_ptr->upper_floor_id);
            rd_s16b(&sf_ptr->lower_floor_id);
        }

        for (int i = 0; i < num; i++) {
            saved_floor_type *sf_ptr = &saved_floors[i];
            if (!sf_ptr->floor_id)
                continue;
            rd_byte(&tmp8u);
            if (tmp8u)
                continue;

            err = rd_saved_floor(player_ptr, sf_ptr);
            if (err)
                break;

            if (!save_floor(player_ptr, sf_ptr, SLF_SECOND))
                err = 182;

            if (err)
                break;
        }

        if (err == 0) {
            if (!load_floor(player_ptr, get_sf_ptr(player_ptr->floor_id), SLF_SECOND))
                err = 183;
        }
    }

    switch (err) {
    case 151:
        load_note(_("アイテムの配列が大きすぎる！", "Too many object entries!"));
        break;

    case 152:
        load_note(_("アイテム配置エラー", "Object allocation error"));
        break;

    case 161:
        load_note(_("モンスターの配列が大きすぎる！", "Too many monster entries!"));
        break;

    case 162:
        load_note(_("モンスター配置エラー", "Monster allocation error"));
        break;

    case 171:
        load_note(_("保存されたフロアのダンジョンデータが壊れています！", "Dungeon data of saved floors are broken!"));
        break;

    case 182:
        load_note(_("テンポラリ・ファイルを作成できません！", "Failed to make temporary files!"));
        break;

    case 183:
        load_note(_("Error 183", "Error 183"));
        break;
    }

    current_world_ptr->character_dungeon = TRUE;
    return err;
}
