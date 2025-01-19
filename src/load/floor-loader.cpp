#include "load/floor-loader.h"
#include "floor/floor-generator.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "load/angband-version-comparer.h"
#include "load/item/item-loader-factory.h"
#include "load/load-util.h"
#include "load/monster/monster-loader-factory.h"
#include "load/old-feature-types.h"
#include "load/old/item-loader-savefile50.h"
#include "load/old/load-v1-5-0.h"
#include "load/old/monster-loader-savefile50.h"
#include "locale/character-encoding.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "save/floor-writer.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/finalizer.h"

/*!
 * @brief 保存されたフロアを読み込む / Read the saved floor
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 最後に保存されたフロアへの参照ポインタ
 * @return info読み込みエラーコード
 * @details
 * この関数は、セーブデータの互換性を保つために多くのデータ改変処理を備えている。
 * 現在確認している処理は以下の通り、
 * <ul>
 * <li>1.7.0.2で8bitだったGridのfeat,mimicのID値を16bitに拡張する処理。</li>
 * <li>1.7.0.8までに廃止、IDなどを差し替えたクエスト番号を置換する処理。</li>
 * </ul>
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
errr rd_saved_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    clear_cave(player_ptr);
    player_ptr->x = player_ptr->y = 0;

    if (!sf_ptr) {
        floor.dun_level = rd_s16b();
        floor.base_level = floor.dun_level;
    } else {
        if (rd_s16b() != sf_ptr->floor_id) {
            return 171;
        }

        if (rd_byte() != sf_ptr->savefile_id) {
            return 171;
        }

        if (rd_s16b() != sf_ptr->dun_level) {
            return 171;
        }
        floor.dun_level = sf_ptr->dun_level;

        if (rd_s32b() != sf_ptr->last_visit) {
            return 171;
        }

        if (rd_u32b() != sf_ptr->visit_mark) {
            return 171;
        }

        if (rd_s16b() != sf_ptr->upper_floor_id) {
            return 171;
        }

        if (rd_s16b() != sf_ptr->lower_floor_id) {
            return 171;
        }
    }

    floor.base_level = rd_s16b();
    floor.num_repro = rd_s16b();

    player_ptr->y = rd_u16b();

    player_ptr->x = rd_u16b();

    floor.height = rd_s16b();
    floor.width = rd_s16b();

    player_ptr->feeling = rd_byte();

    auto limit = rd_u16b();
    std::vector<GridTemplate> templates(limit);

    for (auto &ct_ref : templates) {
        ct_ref.info = rd_u16b();
        if (h_older_than(1, 7, 0, 2)) {
            ct_ref.feat = rd_byte();
            ct_ref.mimic = rd_byte();
        } else {
            ct_ref.feat = rd_s16b();
            ct_ref.mimic = rd_s16b();
        }

        ct_ref.special = rd_s16b();
    }

    POSITION ymax = floor.height;
    POSITION xmax = floor.width;
    for (POSITION x = 0, y = 0; y < ymax;) {
        auto count = rd_byte();

        uint16_t id = 0;
        byte tmp8u;
        do {
            tmp8u = rd_byte();
            id += tmp8u;
        } while (tmp8u == MAX_UCHAR);

        for (int i = count; i > 0; i--) {
            auto *g_ptr = &floor.grid_array[y][x];
            g_ptr->info = templates[id].info;
            g_ptr->feat = templates[id].feat;
            g_ptr->mimic = templates[id].mimic;
            g_ptr->special = templates[id].special;

            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax) {
                    break;
                }
            }
        }
    }

    /* Quest 18 was removed */
    if (h_older_than(1, 7, 0, 6) && !vanilla_town) {
        for (auto y = 0; y < ymax; y++) {
            for (auto x = 0; x < xmax; x++) {
                auto &grid = floor.get_grid({ y, x });
                if ((grid.special == OLD_QUEST_WATER_CAVE) && !floor.is_underground()) {
                    if (grid.feat == OLD_FEAT_QUEST_ENTER) {
                        grid.set_terrain_id(TerrainTag::TREE);
                        grid.special = 0;
                    } else if (grid.feat == OLD_FEAT_BLDG_1) {
                        grid.special = lite_town ? QUEST_OLD_CASTLE : QUEST_ROYAL_CRYPT;
                    }
                } else if ((grid.feat == OLD_FEAT_QUEST_EXIT) && (floor.quest_number == i2enum<QuestId>(OLD_QUEST_WATER_CAVE))) {
                    grid.set_terrain_id(TerrainTag::UP_STAIR);
                    grid.special = 0;
                }
            }
        }
    }

    limit = rd_u16b();
    if (limit > MAX_FLOOR_ITEMS) {
        return 151;
    }

    auto item_loader = ItemLoaderFactory::create_loader();
    for (int i = 1; i < limit; i++) {
        const auto item_idx = floor.pop_empty_index_item();
        if (i != item_idx) {
            return 152;
        }

        auto &item = floor.o_list[item_idx];
        item_loader->rd_item(&item);
        auto &list = get_o_idx_list_contains(&floor, item_idx);
        list.add(&floor, item_idx, item.stack_idx);
    }

    limit = rd_u16b();
    if (limit > MAX_FLOOR_MONSTERS) {
        return 161;
    }

    auto monster_loader = MonsterLoaderFactory::create_loader(player_ptr);
    for (auto i = 1; i < limit; i++) {
        const auto m_idx = floor.pop_empty_index_monster();
        if (i != m_idx) {
            return 162;
        }

        auto &monster = floor.m_list[m_idx];
        monster_loader->rd_monster(&monster);
        auto &grid = floor.get_grid(monster.get_position());
        grid.m_idx = m_idx;
        monster.get_real_monrace().increment_current_numbers();
    }

    return 0;
}

/*!
 * @brief 保存フロア読み込みのサブ関数 / Actually load and verify a floor save data
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 保存フロア読み込み先
 * @return 成功したらtrue
 */
static bool load_floor_aux(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    load_xor_byte = 0;
    strip_bytes(1);

    v_check = 0L;
    x_check = 0L;

    auto &system = AngbandSystem::get_instance();
    system.set_version({ H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA });
    loading_savefile_version = SAVEFILE_VERSION;

    if (saved_floor_file_sign != rd_u32b()) {
        return false;
    }

    if (rd_saved_floor(player_ptr, sf_ptr)) {
        return false;
    }

    auto n_v_check = v_check;
    if (rd_u32b() != n_v_check) {
        return false;
    }

    auto n_x_check = x_check;
    return rd_u32b() == n_x_check;
}

/*!
 * @brief 一時保存フロア情報を読み込む / Attempt to load the temporarily saved-floor data
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 保存フロア読み込み先
 * @param mode オプション
 * @return 成功したらtrue
 */
bool load_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode)
{
    const auto finalizer = util::make_finalizer([backup = loading_character_encoding]() {
        loading_character_encoding = backup;
    });

    /*
     * Temporary files are always written in system depended kanji
     * code.
     */
#ifdef JP
#ifdef EUC
    loading_character_encoding = CharacterEncoding::EUC_JP;
#endif
#ifdef SJIS
    loading_character_encoding = CharacterEncoding::SHIFT_JIS;
#endif
#else
    loading_character_encoding = CharacterEncoding::US_ASCII;
#endif

    FILE *old_fff = nullptr;
    byte old_xor_byte = 0;
    uint32_t old_v_check = 0;
    uint32_t old_x_check = 0;
    AngbandVersion version_backup{};
    uint32_t old_loading_savefile_version = 0;
    auto &system = AngbandSystem::get_instance();
    if (mode & SLF_SECOND) {
        old_fff = loading_savefile;
        old_xor_byte = load_xor_byte;
        old_v_check = v_check;
        old_x_check = x_check;
        version_backup = system.get_version();
        old_loading_savefile_version = loading_savefile_version;
    }

    auto floor_savefile = savefile.string();
    const auto ext = format(".F%02d", (int)sf_ptr->savefile_id);
    floor_savefile.append(ext);

    safe_setuid_grab();
    loading_savefile = angband_fopen(floor_savefile, FileOpenMode::READ, true);
    safe_setuid_drop();

    bool is_save_successful = true;
    if (!loading_savefile) {
        is_save_successful = false;
    }

    if (is_save_successful) {
        is_save_successful = load_floor_aux(player_ptr, sf_ptr);
        if (ferror(loading_savefile)) {
            is_save_successful = false;
        }

        angband_fclose(loading_savefile);
        safe_setuid_grab();
        if (!(mode & SLF_NO_KILL)) {
            (void)fd_kill(floor_savefile);
        }

        safe_setuid_drop();
    }

    if (mode & SLF_SECOND) {
        loading_savefile = old_fff;
        load_xor_byte = old_xor_byte;
        v_check = old_v_check;
        x_check = old_x_check;
        system.set_version(version_backup);
        loading_savefile_version = old_loading_savefile_version;
    }

    return is_save_successful;
}
