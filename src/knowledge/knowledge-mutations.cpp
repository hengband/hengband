/*!
 * @brief 突然変異の一覧を出力する
 * @date 2020/04/24
 * @author Hourier
 */

#include "knowledge/knowledge-mutations.h"
#include "core/show-file.h"
#include "io-dump/dump-util.h"
#include "io/mutations-dump.h"
#include "util/angband-files.h"

/*!
 * @brief 突然変異表示コマンドの実装 / List mutations we have...
 */
void do_cmd_knowledge_mutations(player_type *creature_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    dump_mutations(creature_ptr, fff);
    angband_fclose(fff);

    show_file(creature_ptr, true, file_name, _("突然変異", "Mutations"), 0, 0);
    fd_kill(file_name);
}
