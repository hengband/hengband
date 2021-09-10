/*!
 * @file action-limited.cpp
 * @brief プレイヤーの行動制約判定定義
 */

#include "action/action-limited.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player/player-status.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 魔法系コマンドが制限されているかを返す。
 * @return 魔法系コマンドを使用可能ならFALSE、不可能ならば理由をメッセージ表示してTRUEを返す。
 */
bool cmd_limit_cast(player_type *player_ptr)
{
    if (is_in_dungeon(player_ptr) && (d_info[player_ptr->dungeon_idx].flags.has(DF::NO_MAGIC))) {
        msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
        msg_print(nullptr);
        return true;
    }

    if (player_ptr->anti_magic) {
        msg_print(_("反魔法バリアが魔法を邪魔した！", "An anti-magic shell disrupts your magic!"));
        return true;
    }

    if (is_shero(player_ptr) && (player_ptr->pclass != CLASS_BERSERKER)) {
        msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
        return true;
    }

    return false;
}

bool cmd_limit_confused(player_type *player_ptr)
{
    if (player_ptr->confused) {
        msg_print(_("混乱していてできない！", "You are too confused!"));
        return true;
    }

    return false;
}

bool cmd_limit_image(player_type *player_ptr)
{
    if (player_ptr->image) {
        msg_print(_("幻覚が見えて集中できない！", "Your hallucinations prevent you from concentrating!"));
        return true;
    }

    return false;
}

bool cmd_limit_stun(player_type *player_ptr)
{
    if (player_ptr->stun) {
        msg_print(_("頭が朦朧としていて集中できない！", "You are too stunned!"));
        return true;
    }

    return false;
}

bool cmd_limit_arena(player_type *player_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena) {
        msg_print(_("アリーナが魔法を吸収した！", "The arena absorbs all attempted magic!"));
        msg_print(nullptr);
        return true;
    }

    return false;
}

bool cmd_limit_blind(player_type *player_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("目が見えない。", "You can't see anything."));
        return true;
    }

    if (no_lite(player_ptr)) {
        msg_print(_("明かりがないので見えない。", "You have no light."));
        return true;
    }

    return false;
}

bool cmd_limit_time_walk(player_type *player_ptr)
{
    if (player_ptr->timewalk) {
        if (flush_failure)
            flush();

        msg_print(_("止まった時の中ではうまく働かないようだ。", "It shows no reaction."));
        sound(SOUND_FAIL);
        return true;
    }

    return false;
}
