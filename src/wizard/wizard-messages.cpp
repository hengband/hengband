#include "wizard/wizard-messages.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "io/write-diary.h"
#include "view/display-messages.h"
#include <array>
#include <sstream>
#include <string>

void msg_print_wizard(PlayerType *player_ptr, int cheat_type, std::string_view msg)
{
    constexpr auto max_type = 4;
    if ((cheat_type < 0) || (cheat_type >= max_type)) {
        throw std::logic_error("Invalid cheat type is specified!");
    }

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

    static const std::array<std::string, max_type> cheat_mes = { { "ITEM:", "MONS:", "DUNG:", "MISC:" } };
    std::stringstream ss;
    ss << "WIZ-" << cheat_mes[cheat_type] << msg;
    const auto mes = ss.str();
    msg_print(mes);
    if (cheat_diary_output) {
        exe_write_diary(player_ptr, DiaryKind::WIZARD_LOG, 0, mes);
    }
}

/*
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format_wizard(PlayerType *player_ptr, int cheat_type, const char *fmt, ...)
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
