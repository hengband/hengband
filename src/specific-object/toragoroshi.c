#include "specific-object/toragoroshi.h"
#include "player-attack/player-attack.h"
#include "view/display-messages.h"

bool activate_toragoroshi(player_type *user_ptr)
{
    msg_print(_("あなたは妖刀に魅入られた…", "You are enchanted by cursed blade..."));
    msg_print(_("「狂ほしく 血のごとき 月はのぼれり 秘めおきし 魔剣 いずこぞや」", "'Behold the blade arts.'"));
    massacre(user_ptr);
    return TRUE;
}
