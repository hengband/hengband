#include "system/angband-version.h"
#include "system/angband-exceptions.h"
#include "system/angband.h"

std::string get_version()
{
    std::string_view expr;
    switch (VERSION_STATUS) {
    case VersionStatusType::ALPHA:
        expr = "Alpha";
        break;
    case VersionStatusType::BETA:
        expr = "Beta";
        break;
    case VersionStatusType::RELEASE_CANDIDATE:
        expr = "RC";
        break;
    case VersionStatusType::RELEASE:
        expr = "";
        break;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid version status was specified!");
    }

    if (VERSION_STATUS != VersionStatusType::RELEASE) {
        return format(_("変愚蛮怒 %d.%d.%d%s%d", "Hengband %d.%d.%d%s%d"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, expr.data(), H_VER_EXTRA);
    } else {
        concptr mode = IS_STABLE_VERSION ? _("安定版", "Stable") : _("開発版", "Developing");
        return format(_("変愚蛮怒 %d.%d.%d.%d(%s)", "Hengband %d.%d.%d.%d(%s)"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA, mode);
    }
}
