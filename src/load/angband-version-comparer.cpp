#include "load/angband-version-comparer.h"
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
    if (current_world_ptr->h_ver_major < major)
        return TRUE;
    if (current_world_ptr->h_ver_major > major)
        return FALSE;

    if (current_world_ptr->h_ver_minor < minor)
        return TRUE;
    if (current_world_ptr->h_ver_minor > minor)
        return FALSE;

    if (current_world_ptr->h_ver_patch < patch)
        return TRUE;
    if (current_world_ptr->h_ver_patch > patch)
        return FALSE;

    if (current_world_ptr->h_ver_extra < extra)
        return TRUE;
    if (current_world_ptr->h_ver_extra > extra)
        return FALSE;

    return FALSE;
}

/*!
 * @brief [互換性用/新規仕様禁止]変愚蛮怒のバージョン比較処理 / The above function, adapted for Hengband
 * @param major メジャーバージョン値
 * @param minor マイナーバージョン値
 * @param patch パッチバージョン値
 * @return 現在のバージョンより値が古いならtrue
 * @detail
 * 旧バージョン比較の互換性のためにのみ保持。
 */
bool h_older_than(byte major, byte minor, byte patch)
{
    if (current_world_ptr->h_ver_major < major)
        return TRUE;
    if (current_world_ptr->h_ver_major > major)
        return FALSE;

    if (current_world_ptr->h_ver_minor < minor)
        return TRUE;
    if (current_world_ptr->h_ver_minor > minor)
        return FALSE;

    if (current_world_ptr->h_ver_patch < patch)
        return TRUE;
    if (current_world_ptr->h_ver_patch > patch)
        return FALSE;

    return FALSE;
}
