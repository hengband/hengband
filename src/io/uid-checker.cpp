#include "io/uid-checker.h"
#include "system/player-type-definition.h"

/*!
 * @brief ファイルのドロップパーミッションチェック / Check drop permissions
 */
void safe_setuid_drop(void)
{
#if defined(SET_UID) && defined(SAFE_SETUID)
#ifdef SAFE_SETUID_POSIX
    if (auto ret = setuid(getuid()); ret != 0) {
        auto msg = _("setuid(): 正しく許可が取れません！ エラーコード：%d", "setuid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }

    if (auto ret = setgid(getgid()); ret != 0) {
        auto msg = _("setgid(): 正しく許可が取れません！ エラーコード：%d", "setgid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }
#else
    if (auto ret = setreuid(geteuid(), getuid()); ret != 0) {
        auto msg = _("setreuid(): 正しく許可が取れません！ エラーコード：%d", "setreuid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }

    if (auto ret = setregid(getegid(), getgid()); ret != 0) {
        auto msg = _("setregid(): 正しく許可が取れません！ エラーコード：%d", "setregid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }
#endif
#endif
}

/*!
 * @brief ファイルのグラブパーミッションチェック / Check grab permissions
 * @param プレイヤーへの参照ポインタ
 */
void safe_setuid_grab(PlayerType *player_ptr)
{
#if defined(SET_UID) && defined(SAFE_SETUID)
#ifdef SAFE_SETUID_POSIX
    if (auto ret = setuid(player_ptr->player_euid); ret != 0) {
        auto msg = _("setuid(): 正しく許可が取れません！ エラーコード：%d", "setuid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }

    if (auto ret = setgid(player_ptr->player_egid); ret != 0) {
        auto msg = _("setgid(): 正しく許可が取れません！ エラーコード：%d", "setgid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }
#else
    (void)player_ptr;
    if (auto ret = setreuid(geteuid(), getuid()); ret != 0) {
        auto msg = _("setreuid(): 正しく許可が取れません！ エラーコード：%d", "setreuid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }

    if (auto ret = setregid(getegid(), getgid()); ret != 0) {
        auto msg = _("setregid(): 正しく許可が取れません！ エラーコード：%d", "setregid(): cannot set permissions correctly! Error code: %d");
        quit(format(msg, ret));
    }
#endif
#else
    (void)player_ptr;
#endif
}
