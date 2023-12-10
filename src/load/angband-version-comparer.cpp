#include "load/angband-version-comparer.h"
#include "system/angband-system.h"
#include "system/angband-version.h"

/*!
 * @brief 変愚蛮怒のバージョン比較処理
 * @param major メジャーバージョン値
 * @param minor マイナーバージョン値
 * @param state パッチバージョン値
 * @param build エクストラパージョン値
 * @return 現在のバージョンより値が古いならtrue
 */
bool h_older_than(uint8_t major, uint8_t minor, uint8_t state, uint8_t build)
{
    if (VARIANT_NAME != ROOT_VARIANT_NAME) {
        return false;
    }

    const auto &system = AngbandSystem::get_instance();
    if (system.version_major < major) {
        return true;
    }

    if (system.version_major > major) {
        return false;
    }

    if (system.version_minor < minor) {
        return true;
    }

    if (system.version_minor > minor) {
        return false;
    }

    if (system.version_state < state) {
        return true;
    }

    if (system.version_state > state) {
        return false;
    }

    if (system.version_build < build) {
        return true;
    }

    if (system.version_build > build) {
        return false;
    }

    return false;
}

/*!
 * @brief [互換性用/新規使用禁止]変愚蛮怒のバージョン比較処理
 * @param major メジャーバージョン値
 * @param minor マイナーバージョン値
 * @param state パッチバージョン値
 * @return 現在のバージョンより値が古いならtrue
 * @details 旧バージョン比較の互換性のためにのみ保持する.
 */
bool h_older_than(uint8_t major, uint8_t minor, uint8_t state)
{
    if (VARIANT_NAME != ROOT_VARIANT_NAME) {
        return false;
    }

    const auto &system = AngbandSystem::get_instance();
    if (system.version_major < major) {
        return true;
    }

    if (system.version_major > major) {
        return false;
    }

    if (system.version_minor < minor) {
        return true;
    }

    if (system.version_minor > minor) {
        return false;
    }

    if (system.version_state < state) {
        return true;
    }

    if (system.version_state > state) {
        return false;
    }

    return false;
}
