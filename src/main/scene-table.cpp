/*!
 * @file scene-table.cpp
 * @brief BGM選曲の基本処理部分実装
 */

#include "main/scene-table.h"
#include "main/scene-table-floor.h"
#include "main/scene-table-monster.h"
#include "system/floor-type-definition.h"
#include "term/z-term.h"

int interrupt_scene_type;
int interrupt_scene_val;

scene_type_list scene_list;

static void resize_scene_list()
{
    const int monster_def_count = get_scene_monster_count();
    const int interrupt_def_count = 1;
    const int floor_def_count = get_scene_floor_count();
    scene_list.resize(monster_def_count + interrupt_def_count + floor_def_count);
}

/*!
 * @brief 選曲の割り込み通知
 * @details 選曲テーブル外の曲（クエストクリア等）の再生を取得しておく。
 * モンスターBGMを含む選曲テーブルを構築する場合に、
 * 1.モンスターBGM
 * 2.割り込みBGM
 * 3.通常BGM
 * の順に設定する。
 * 街の施設等で、コマンド実行→視界内モンスターリスト更新(空のリスト:再生なし)→割り込みBGMに戻るようにする。
 * 選曲テーブルでは割り込みBGMは2番目だが、
 * 一時的に優先するためにBGM対象のモンスターを忘れ、モンスターBGMに制限期間を設定する。
 * @param type action-type
 * @param val action-val
 */
void interrupt_scene(int type, int val) {
    interrupt_scene_type = type;
    interrupt_scene_val = val;

    // forget BGM-target monster
    clear_scene_target_monster();
    // モンスターBGMの再生を一時的に抑制する
    set_temp_mute_scene_monster(2);
}

/*!
 * @brief 現在のフロアに合ったBGM選曲
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void refresh_scene_table(player_type *player_ptr)
{
    // clear interrupt_scene
    interrupt_scene(0, 0);

    resize_scene_list();
    refresh_scene_floor(player_ptr, scene_list, 0);
}

/*!
 * @brief 見かけたモンスターを含め、現在のフロアに合ったBGM選曲
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monster_list 視界内モンスターリスト
 */
void refresh_scene_table(player_type *player_ptr, const std::vector<MONSTER_IDX> &monster_list)
{
    resize_scene_list();
    int index = 0;

    refresh_scene_monster(player_ptr, monster_list, scene_list, index);
    index += get_scene_monster_count();

    // interrupt scene
    scene_type &item = scene_list[index];
    item.type = interrupt_scene_type;
    item.val = interrupt_scene_val;
    ++index;

    refresh_scene_floor(player_ptr, scene_list, index);
}

/*!
 * @brief BGM選曲リスト取得
 * @param type 未使用
 */
scene_type_list &get_scene_type_list(int type)
{
    (void)type;
    return scene_list;
}
