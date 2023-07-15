#include "monster-attack/insults-moans.h"

/*! モンスターの侮辱行為メッセージテーブル / Hack -- possible "insult" messages */
concptr desc_insult[MAX_INSULTS] = {
    _("があなたを侮辱した！", "insults you!"),
    _("があなたの母を侮辱した！", "insults your mother!"),
    _("があなたを軽蔑した！", "gives you the finger!"),
    _("があなたを辱めた！", "humiliates you!"),
    _("があなたを汚した！", "defiles you!"),
    _("があなたの回りで踊った！", "dances around you!"),
    _("が猥褻な身ぶりをした！", "makes obscene gestures!"),
    _("があなたをぼんやりと見た！！！", "moons you!!!"),
    _("があなたをパラサイト呼ばわりした！", "calls you a parasite!"),
    _("があなたをサイボーグ扱いした！", "calls you a cyborg!"),
};

/*! マゴットのぼやきメッセージテーブル / Hack -- possible "insult" messages */
concptr desc_moan[MAX_MOANS] = {
    _("は何かを悲しんでいるようだ。", "seems sad about something."),
    _("が彼の飼い犬を見なかったかと尋ねている。", "asks if you have seen his dogs."),
    _("が縄張りから出て行けと言っている。", "tells you to get off his land."),
    _("はキノコがどうとか呟いている。", "mumbles something about mushrooms."),
};
