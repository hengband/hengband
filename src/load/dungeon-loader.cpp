#include "load/dungeon-loader.h"
#include "dungeon/quest.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "load/angband-version-comparer.h"
#include "load/dummy-loader.h"
#include "load/floor-loader.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "save/floor-writer.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "world/world.h"

/*!
 * @brief 保存されたフロアを読み込む / Read the dungeon
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード
 * @details
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
static errr rd_dungeon(player_type *player_ptr)
{
    init_saved_floors(player_ptr, false);
    errr err = 0;
    if (h_older_than(1, 5, 0, 0)) {
        err = rd_dungeon_old(player_ptr);
        if (player_ptr->dungeon_idx) {
            player_ptr->floor_id = get_new_floor_id(player_ptr);
            get_sf_ptr(player_ptr->floor_id)->dun_level = player_ptr->current_floor_ptr->dun_level;
        }

        return err;
    }

    max_floor_id = rd_s16b();
    player_ptr->dungeon_idx = rd_byte(); // @todo セーブデータの方を16ビットにするかdungeon_idxの定義を8ビットにした方が良い.
    auto num = rd_byte();
    if (num == 0) {
        err = rd_saved_floor(player_ptr, nullptr);
    } else {
        for (int i = 0; i < num; i++) {
            saved_floor_type *sf_ptr = &saved_floors[i];

            sf_ptr->floor_id = rd_s16b();
            sf_ptr->savefile_id = rd_byte();

            sf_ptr->dun_level = rd_s16b();

            sf_ptr->last_visit = rd_s32b();
            sf_ptr->visit_mark = rd_u32b();
            sf_ptr->upper_floor_id = rd_s16b();
            sf_ptr->lower_floor_id = rd_s16b();
        }

        for (int i = 0; i < num; i++) {
            saved_floor_type *sf_ptr = &saved_floors[i];
            if (!sf_ptr->floor_id)
                continue;
            if (rd_byte() != 0)
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

        // latest_visit_mark の復元。
        // 全ての保存フロアについての visit_mark の最大値 + 1 とする。
        for (int i = 0; i < num; ++i) {
            const uint32_t next_visit_mark = saved_floors[i].visit_mark + 1;
            if (next_visit_mark > latest_visit_mark)
                latest_visit_mark = next_visit_mark;
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

    w_ptr->character_dungeon = true;
    return err;
}

errr restore_dungeon(player_type *player_ptr)
{
    if (player_ptr->is_dead) {
        for (int i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++)
            r_info[quest[i].r_idx].flags1 &= ~RF1_QUESTOR;

        return 0;
    }

    load_note(_("ダンジョン復元中...", "Restoring Dungeon..."));
    if (rd_dungeon(player_ptr)) {
        load_note(_("ダンジョンデータ読み込み失敗", "Error reading dungeon data"));
        return 34;
    }

    rd_ghost();
    auto tmp32s = rd_s32b();
    strip_bytes(tmp32s);
    return 0;
}
