#include "load/angband-version-comparer.h"
#include "system/angband-version.h"
#include "world/world.h"

/*!
 * @brief 変愚蛮怒のバージョン比較処理 / This function determines if the version of the savefile currently being read is older than version
 * "major.minor.patch.extra".
 * @param major メジャーバージョン値
 * @param minor マイナーバージョン値
 * @param patch パッチバージョン値
 * @param extra エクストラパージョン値
 * @return 現在のバージョンより値が古いならtrue
 */
bool h_older_than(byte major, byte minor, byte patch, byte extra)
{
    if (VARIANT_NAME != ROOT_VARIANT_NAME) {
        return false;
    }

    if (w_ptr->h_ver_major < major) {
        return true;
    }
    if (w_ptr->h_ver_major > major) {
        return false;
    }

    if (w_ptr->h_ver_minor < minor) {
        return true;
    }
    if (w_ptr->h_ver_minor > minor) {
        return false;
    }

    if (w_ptr->h_ver_patch < patch) {
        return true;
    }
    if (w_ptr->h_ver_patch > patch) {
        return false;
    }

    if (w_ptr->h_ver_extra < extra) {
        return true;
    }
    if (w_ptr->h_ver_extra > extra) {
        return false;
    }

    return false;
}

/*!
 * @brief [互換性用/新規仕様禁止]変愚蛮怒のバージョン比較処理 / The above function, adapted for Hengband
 * @param major メジャーバージョン値
 * @param minor マイナーバージョン値
 * @param patch パッチバージョン値
 * @return 現在のバージョンより値が古いならtrue
 * @details
 * 旧バージョン比較の互換性のためにのみ保持。
 */
bool h_older_than(byte major, byte minor, byte patch)
{
    if (VARIANT_NAME != ROOT_VARIANT_NAME) {
        return false;
    }

    if (w_ptr->h_ver_major < major) {
        return true;
    }
    if (w_ptr->h_ver_major > major) {
        return false;
    }

    if (w_ptr->h_ver_minor < minor) {
        return true;
    }
    if (w_ptr->h_ver_minor > minor) {
        return false;
    }

    if (w_ptr->h_ver_patch < patch) {
        return true;
    }
    if (w_ptr->h_ver_patch > patch) {
        return false;
    }

    return false;
}
