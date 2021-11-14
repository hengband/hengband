#include "specific-object/toragoroshi.h"
#include "player-attack/player-attack.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool activate_toragoroshi(PlayerType *player_ptr)
{
    msg_print(_("あなたは妖刀に魅入られた…", "You are fascinated by the cursed blade..."));
    msg_print(_("「狂ほしく 血のごとき 月はのぼれり 秘めおきし 魔剣 いずこぞや」", "'Behold the blade arts.'"));
    massacre(player_ptr);
    return true;
}
