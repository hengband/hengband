#include "action/action-limited.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 魔法系コマンドが制限されているかを返す。
 * @return 魔法系コマンドを使用可能ならFALSE、不可能ならば理由をメッセージ表示してTRUEを返す。
 */
bool cmd_limit_cast(player_type *creature_ptr)
{
    if (creature_ptr->current_floor_ptr->dun_level && (d_info[creature_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC)) {
        msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
        msg_print(NULL);
        return TRUE;
    }

    if (creature_ptr->anti_magic) {
        msg_print(_("反魔法バリアが魔法を邪魔した！", "An anti-magic shell disrupts your magic!"));
        return TRUE;
    }

    if (is_shero(creature_ptr) && (creature_ptr->pclass != CLASS_BERSERKER)) {
        msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
        return TRUE;
    }

    return FALSE;
}

bool cmd_limit_confused(player_type *creature_ptr)
{
    if (creature_ptr->confused) {
        msg_print(_("混乱していてできない！", "You are too confused!"));
        return TRUE;
    }

    return FALSE;
}

bool cmd_limit_image(player_type *creature_ptr)
{
    if (creature_ptr->image) {
        msg_print(_("幻覚が見えて集中できない！", "Your hallucinations prevent you from concentrating!"));
        return TRUE;
    }

    return FALSE;
}

bool cmd_limit_stun(player_type *creature_ptr)
{
    if (creature_ptr->stun) {
        msg_print(_("頭が朦朧としていて集中できない！", "You are too stunned!"));
        return TRUE;
    }

    return FALSE;
}

bool cmd_limit_arena(player_type *creature_ptr)
{
    if (creature_ptr->current_floor_ptr->inside_arena) {
        msg_print(_("アリーナが魔法を吸収した！", "The arena absorbs all attempted magic!"));
        msg_print(NULL);
        return TRUE;
    }

    return FALSE;
}

bool cmd_limit_blind(player_type *creature_ptr)
{
    if (creature_ptr->blind) {
        msg_print(_("目が見えない。", "You can't see anything."));
        return TRUE;
    }

    if (no_lite(creature_ptr)) {
        msg_print(_("明かりがないので見えない。", "You have no light."));
        return TRUE;
    }

    return FALSE;
}

bool cmd_limit_time_walk(player_type *creature_ptr)
{
    if (creature_ptr->timewalk) {
        if (flush_failure)
            flush();

        msg_print(_("止まった時の中ではうまく働かないようだ。", "It shows no reaction."));
        sound(SOUND_FAIL);
        return TRUE;
    }

    return FALSE;
}
