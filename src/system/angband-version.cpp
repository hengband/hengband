#include "system/angband-version.h"
#include "system/angband-exceptions.h"
#include "system/angband.h"

namespace {
std::string get_version_status()
{
    switch (VERSION_STATUS) {
    case VersionStatusType::ALPHA:
        return "Alpha";
    case VersionStatusType::BETA:
        return "Beta";
    case VersionStatusType::RELEASE_CANDIDATE:
        return "RC";
    case VersionStatusType::RELEASE:
        return "";
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid version status was specified!");
    }
}
}

std::string AngbandVersion::build_expression(VersionExpression expression) const
{
    switch (expression) {
    case VersionExpression::WITHOUT_EXTRA:
        return format("%d.%d.%d", this->major, this->minor, this->patch);
    case VersionExpression::WITH_EXTRA:
        return format("%d.%d.%d.%d", this->major, this->minor, this->patch, this->extra);
    case VersionExpression::FULL: {
        const auto expr = get_version_status();
        if (VERSION_STATUS == VersionStatusType::RELEASE) {
            constexpr auto mode = IS_STABLE_VERSION ? _("安定版", "Stable") : _("開発版", "Developing");
            return format(_("変愚蛮怒 %d.%d.%d.%d(%s)", "Hengband %d.%d.%d.%d(%s)"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA, mode);
        }

        return format(_("変愚蛮怒 %d.%d.%d%s%d", "Hengband %d.%d.%d%s%d"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, expr.data(), H_VER_EXTRA);
    }
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid version expression!");
    }
}
