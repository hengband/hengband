#include "io/uid-checker.h"
#include "system/angband.h"
#if defined(SET_UID) && defined(SAFE_SETUID) && defined(SAFE_SETUID_POSIX)
#include "main-unix/unix-user-ids.h"
#endif

/*!
 * @brief ファイルのドロップパーミッションチェック / Check drop permissions
 */
void safe_setuid_drop()
{
#if defined(SET_UID) && defined(SAFE_SETUID)
#ifdef SAFE_SETUID_POSIX
    if (auto ret = setuid(getuid()); ret != 0) {
        auto msg = _("setuid(): 正しく許可が取れません！ エラーコード：%d", "setuid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }

    if (auto ret = setgid(getgid()); ret != 0) {
        auto msg = _("setgid(): 正しく許可が取れません！ エラーコード：%d", "setgid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }
#else
    if (auto ret = setreuid(geteuid(), getuid()); ret != 0) {
        auto msg = _("setreuid(): 正しく許可が取れません！ エラーコード：%d", "setreuid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }

    if (auto ret = setregid(getegid(), getgid()); ret != 0) {
        auto msg = _("setregid(): 正しく許可が取れません！ エラーコード：%d", "setregid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }
#endif
#endif
}

/*!
 * @brief ファイルのグラブパーミッションチェック / Check grab permissions
 */
void safe_setuid_grab()
{
#if defined(SET_UID) && defined(SAFE_SETUID)
#ifdef SAFE_SETUID_POSIX
    auto &ids = UnixUserIds::get_instance();
    if (auto ret = setuid(ids.get_effective_user_id()); ret != 0) {
        auto msg = _("setuid(): 正しく許可が取れません！ エラーコード：%d", "setuid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }

    if (auto ret = setgid(ids.get_effective_group_id()); ret != 0) {
        auto msg = _("setgid(): 正しく許可が取れません！ エラーコード：%d", "setgid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }
#else
    if (auto ret = setreuid(geteuid(), getuid()); ret != 0) {
        auto msg = _("setreuid(): 正しく許可が取れません！ エラーコード：%d", "setreuid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }

    if (auto ret = setregid(getegid(), getgid()); ret != 0) {
        auto msg = _("setregid(): 正しく許可が取れません！ エラーコード：%d", "setregid(): cannot set permissions correctly! Error code: %d");
        quit_fmt(msg, ret);
    }
#endif
#else
#endif
}
