#include "specific-object/toragoroshi.h"
#include "player-attack/player-attack.h"
#include "view/display-messages.h"

bool activate_toragoroshi(player_type *user_ptr)
{
    msg_print(_("‚ ‚È‚½‚Í—d“‚É–£“ü‚ç‚ê‚½c", "You are enchanted by cursed blade..."));
    msg_print(_("u‹¶‚Ù‚µ‚­ ŒŒ‚Ì‚²‚Æ‚« Œ‚Í‚Ì‚Ú‚ê‚è ”é‚ß‚¨‚«‚µ –‚Œ• ‚¢‚¸‚±‚¼‚âv", "'Behold the blade arts.'"));
    massacre(user_ptr);
    return TRUE;
}
