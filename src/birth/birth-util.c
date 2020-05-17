#include "system/angband.h"
#include "birth/birth-util.h"
#include "core/show-file.h"

/*!
 * @brief プレイヤー作成を中断して変愚蛮怒を終了する
 * @return なし
 */
void birth_quit(void)
{
    quit(NULL);
}

/*!
 * @brief 指定されたヘルプファイルを表示する / Show specific help file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param helpfile ファイル名
 * @return なし
 */
void show_help(player_type* creature_ptr, concptr helpfile)
{
    screen_save();
    (void)show_file(creature_ptr, TRUE, helpfile, NULL, 0, 0);
    screen_load();
}
