#include "wizard/wizard-messages.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "io/write-diary.h"
#include "view/display-messages.h"
#include <string>

void msg_print_wizard(PlayerType *player_ptr, int cheat_type, concptr msg)
{
    if (!cheat_room && cheat_type == CHEAT_DUNGEON) {
        return;
    }
    if (!cheat_peek && cheat_type == CHEAT_OBJECT) {
        return;
    }
    if (!cheat_hear && cheat_type == CHEAT_MONSTER) {
        return;
    }
    if (!cheat_xtra && cheat_type == CHEAT_MISC) {
        return;
    }

    concptr cheat_mes[] = { "ITEM:", "MONS:", "DUNG:", "MISC:" };
    std::string buf = "WIZ-";
    buf.append(cheat_mes[cheat_type]).append(msg);
    msg_print(buf);

    if (cheat_diary_output) {
        exe_write_diary(player_ptr, DIARY_WIZARD_LOG, 0, buf.data());
    }
}

/*
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format_wizard(PlayerType *player_ptr, int cheat_type, concptr fmt, ...)
{
    if (!cheat_room && cheat_type == CHEAT_DUNGEON) {
        return;
    }
    if (!cheat_peek && cheat_type == CHEAT_OBJECT) {
        return;
    }
    if (!cheat_hear && cheat_type == CHEAT_MONSTER) {
        return;
    }
    if (!cheat_xtra && cheat_type == CHEAT_MISC) {
        return;
    }

    va_list vp;
    char buf[1024];
    va_start(vp, fmt);
    (void)vstrnfmt(buf, 1024, fmt, vp);
    va_end(vp);
    msg_print_wizard(player_ptr, cheat_type, buf);
}
