#include "room/room-generator.h"
#include "dungeon/dungeon-flag-types.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "room/door-definition.h"
#include "room/room-info-table.h"
#include "room/room-types.h"
#include "room/rooms-city.h"
#include "room/rooms-fractal.h"
#include "room/rooms-normal.h"
#include "room/rooms-pit-nest.h"
#include "room/rooms-special.h"
#include "room/rooms-trap.h"
#include "room/rooms-vault.h"
#include "system/dungeon-data-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/probability-table.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief 与えられた部屋型IDに応じて部屋の生成処理分岐を行い結果を返す / Attempt to build a room of the given type at the given block
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param type 部屋型ID
 * @note that we restrict the number of "crowded" rooms to reduce the chance of overflowing the monster list during level creation.
 * @return 部屋の生成に成功した場合 TRUE を返す。
 */
static bool room_build(PlayerType *player_ptr, dun_data_type *dd_ptr, RoomType typ)
{
    switch (typ) {
    case RoomType::NORMAL:
        return build_type1(player_ptr, dd_ptr);
    case RoomType::OVERLAP:
        return build_type2(player_ptr, dd_ptr);
    case RoomType::CROSS:
        return build_type3(player_ptr, dd_ptr);
    case RoomType::INNER_FEAT:
        return build_type4(player_ptr, dd_ptr);
    case RoomType::NEST:
        return build_type5(player_ptr, dd_ptr);
    case RoomType::PIT:
        return build_type6(player_ptr, dd_ptr);
    case RoomType::LESSER_VAULT:
        return build_fixed_room(player_ptr, dd_ptr, 7, false);
    case RoomType::GREATER_VAULT:
        return build_fixed_room(player_ptr, dd_ptr, 8, true);
    case RoomType::FRACAVE:
        return build_type9(player_ptr, dd_ptr);
    case RoomType::RANDOM_VAULT:
        return build_type10(player_ptr, dd_ptr);
    case RoomType::OVAL:
        return build_type11(player_ptr, dd_ptr);
    case RoomType::CRYPT:
        return build_type12(player_ptr, dd_ptr);
    case RoomType::TRAP_PIT:
        return build_type13(player_ptr, dd_ptr);
    case RoomType::TRAP:
        return build_type14(player_ptr, dd_ptr);
    case RoomType::GLASS:
        return build_type15(player_ptr, dd_ptr);
    case RoomType::ARCADE:
        return build_type16(player_ptr, dd_ptr);
    case RoomType::FIXED:
        return build_fixed_room(player_ptr, dd_ptr, 17, false);
    default:
        return false;
    }
}

/*!
 * @brief 指定した部屋の生成確率を別の部屋に加算し、指定した部屋の生成率を0にする
 * @param dst 確率を移す先の部屋種ID
 * @param src 確率を与える元の部屋種ID
 */
static void move_prob_list(RoomType dst, RoomType src, std::map<RoomType, int> &prob_list)
{
    prob_list[dst] += prob_list[src];
    prob_list[src] = 0;
}

/*!
 * @brief 部屋生成処理のメインルーチン(Sangbandを経由してOangbandからの実装を引用) / Generate rooms in dungeon.  Build bigger rooms at first.　[from SAngband
 * (originally from OAngband)]
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 部屋生成に成功した場合 TRUE を返す。
 */
bool generate_rooms(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    int crowded = 0;
    std::map<RoomType, int> prob_list;
    int rooms_built = 0;
    int area_size = 100 * (floor_ptr->height * floor_ptr->width) / (MAX_HGT * MAX_WID);
    int level_index = std::min(10, div_round(floor_ptr->dun_level, 10));
    std::map<RoomType, int> room_num;
    int dun_rooms = DUN_ROOMS_MAX * area_size / 100;
    room_info_type *room_info_ptr = room_info_normal;
    for (auto r : ROOM_TYPE_LIST) {
        if (floor_ptr->dun_level < room_info_ptr[enum2i(r)].min_level) {
            prob_list[r] = 0;
        } else {
            prob_list[r] = room_info_ptr[enum2i(r)].prob[level_index];
        }
    }

    /*!
     * @details ダンジョンにBEGINNER、CHAMELEON、SMALLESTいずれのフラグもなく、
     * かつ「常に通常でない部屋を生成する」フラグがONならば、
     * GRATER_VAULTのみを生成対象とする。 / Ironman sees only Greater Vaults
     */
    const auto &dungeon = floor_ptr->get_dungeon_definition();
    if (ironman_rooms && dungeon.flags.has_none_of({ DungeonFeatureType::BEGINNER, DungeonFeatureType::CHAMELEON, DungeonFeatureType::SMALLEST })) {
        for (auto r : ROOM_TYPE_LIST) {
            if (r == RoomType::GREATER_VAULT) {
                prob_list[r] = 1;
            } else {
                prob_list[r] = 0;
            }
        }
    } else if (dungeon.flags.has(DungeonFeatureType::NO_VAULT)) {
        /*! @details ダンジョンにNO_VAULTフラグがあるならば、LESSER_VAULT / GREATER_VAULT/ RANDOM_VAULTを除外 / Forbidden vaults */
        prob_list[RoomType::LESSER_VAULT] = 0;
        prob_list[RoomType::GREATER_VAULT] = 0;
        prob_list[RoomType::RANDOM_VAULT] = 0;
    }

    /*! @details ダンジョンにBEGINNERフラグがあるならば、FIXED_ROOMを除外 / Forbidden vaults */
    if (dungeon.flags.has(DungeonFeatureType::BEGINNER)) {
        prob_list[RoomType::FIXED] = 0;
    }

    /*! @details ダンジョンにNO_CAVEフラグがある場合、FRACAVEの生成枠がNORMALに与えられる。CRIPT、OVALの生成枠がINNER_Fに与えられる。/ NO_CAVE dungeon */
    if (dungeon.flags.has(DungeonFeatureType::NO_CAVE)) {
        move_prob_list(RoomType::NORMAL, RoomType::FRACAVE, prob_list);
        move_prob_list(RoomType::INNER_FEAT, RoomType::CRYPT, prob_list);
        move_prob_list(RoomType::INNER_FEAT, RoomType::OVAL, prob_list);
    } else if (dungeon.flags.has(DungeonFeatureType::CAVE)) {
        /*! @details ダンジョンにCAVEフラグがある場合、NORMALの生成枠がFRACAVEに与えられる。/ CAVE dungeon (Orc floor_ptr->grid_array etc.) */
        move_prob_list(RoomType::FRACAVE, RoomType::NORMAL, prob_list);
    } else if (dd_ptr->cavern || dd_ptr->empty_level) {
        /*! @details ダンジョンの基本地形が最初から渓谷かアリーナ型の場合 FRACAVE は生成から除外。 /  No caves when a (random) cavern exists: they look bad */
        prob_list[RoomType::FRACAVE] = 0;
    }

    /*! @details ダンジョンに最初からGLASS_ROOMフラグがある場合、GLASS を生成から除外。/ Forbidden glass rooms */
    if (dungeon.flags.has_not(DungeonFeatureType::GLASS_ROOM)) {
        prob_list[RoomType::GLASS] = 0;
    }

    /*! @details ARCADEは同フラグがダンジョンにないと生成されない。 / Forbidden glass rooms */
    if (dungeon.flags.has_not(DungeonFeatureType::ARCADE)) {
        prob_list[RoomType::ARCADE] = 0;
    }

    ProbabilityTable<RoomType> prob_table;
    for (auto i : ROOM_TYPE_LIST) {
        room_num[i] = 0;
        prob_table.entry_item(i, prob_list[i]);
    }

    for (int i = dun_rooms; i > 0; i--) {
        auto r = prob_table.empty() ? RoomType::NORMAL : prob_table.pick_one_at_random();

        room_num[r]++;
        switch (r) {
        case RoomType::NEST:
        case RoomType::PIT:
        case RoomType::LESSER_VAULT:
        case RoomType::TRAP_PIT:
        case RoomType::GLASS:
        case RoomType::ARCADE:
            /* Large room */
            i -= 2;
            break;
        case RoomType::GREATER_VAULT:
        case RoomType::RANDOM_VAULT:
            /* Largest room */
            i -= 3;
            break;
        default:
            break;
        }
    }

    bool remain;
    while (true) {
        remain = false;
        for (auto i = 0; i < ROOM_TYPE_MAX; i++) {
            auto room_type = room_build_order[i];
            if (!room_num[room_type]) {
                continue;
            }

            room_num[room_type]--;
            if (!room_build(player_ptr, dd_ptr, room_type)) {
                continue;
            }

            rooms_built++;
            remain = true;
            switch (room_type) {
            case RoomType::PIT:
            case RoomType::NEST:
            case RoomType::TRAP_PIT:
                if (++crowded >= 2) {
                    room_num[RoomType::PIT] = 0;
                    room_num[RoomType::NEST] = 0;
                    room_num[RoomType::TRAP_PIT] = 0;
                }

                break;
            case RoomType::ARCADE:
                room_num[RoomType::ARCADE] = 0;
                break;
            default:
                break;
            }
        }

        if (!remain) {
            break;
        }
    }

    /*! @details 部屋生成数が2未満の場合生成失敗を返す */
    if (rooms_built < 2) {
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("部屋数が2未満でした。生成を再試行します。", "Number of rooms was under 2. Retry."));
        return false;
    }

    msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("このダンジョンの部屋数は %d です。", "Number of Rooms: %d"), rooms_built);
    return true;
}
