#include "system/angband-version.h"
#include "system/angband.h"

void put_version(char *buf)
{
    if (IS_ALPHA_VERSION) {
        sprintf(buf, _("変愚蛮怒 %d.%d.%dAlpha%d", "Hengband %d.%d.%dAlpha%d"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA);
    } else {
        concptr mode = IS_STABLE_VERSION ? _("安定版", "Stable") : _("開発版", "Developing");
        sprintf(buf, _("変愚蛮怒 %d.%d.%d.%d(%s)", "Hengband %d.%d.%d.%d(%s)"), H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA, mode);
    }
}
