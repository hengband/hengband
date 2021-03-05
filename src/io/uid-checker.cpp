﻿#include "io/uid-checker.h"
#ifdef SET_UID
#ifdef SAFE_SETUID
#ifdef SAFE_SETUID_POSIX
#include "util.h"
#endif
#endif
#endif

/*!
 * @brief ファイルのドロップパーミッションチェック / Hack -- drop permissions
 */
void safe_setuid_drop(void)
{
#ifdef SET_UID
#ifdef SAFE_SETUID
#ifdef SAFE_SETUID_POSIX

    if (setuid(getuid()) != 0) {
        quit(_("setuid(): 正しく許可が取れません！", "setuid(): cannot set permissions correctly!"));
    }
    if (setgid(getgid()) != 0) {
        quit(_("setgid(): 正しく許可が取れません！", "setgid(): cannot set permissions correctly!"));
    }
#else
    if (setreuid(geteuid(), getuid()) != 0) {
        quit(_("setreuid(): 正しく許可が取れません！", "setreuid(): cannot set permissions correctly!"));
    }
    if (setregid(getegid(), getgid()) != 0) {
        quit(_("setregid(): 正しく許可が取れません！", "setregid(): cannot set permissions correctly!"));
    }
#endif
#endif
#endif
}

/*!
 * @brief ファイルのグラブパーミッションチェック / Hack -- grab permissions
 */
void safe_setuid_grab(player_type *player_ptr)
{
#ifdef SET_UID
#ifdef SAFE_SETUID
#ifdef SAFE_SETUID_POSIX

    if (setuid(player_ptr->player_egid) != 0) {
        quit(_("setuid(): 正しく許可が取れません！", "setuid(): cannot set permissions correctly!"));
    }
    if (setgid(player_ptr->player_egid) != 0) {
        quit(_("setgid(): 正しく許可が取れません！", "setgid(): cannot set permissions correctly!"));
    }
#else
    if (setreuid(geteuid(), getuid()) != 0) {
        quit(_("setreuid(): 正しく許可が取れません！", "setreuid(): cannot set permissions correctly!"));
    }
    if (setregid(getegid(), getgid()) != 0) {
        quit(_("setregid(): 正しく許可が取れません！", "setregid(): cannot set permissions correctly!"));
    }
#endif
#endif
#else
    (void)player_ptr;
#endif
}
