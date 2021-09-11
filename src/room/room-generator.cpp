#include "room/room-generator.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
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
static bool room_build(player_type *player_ptr, dun_data_type *dd_ptr, EFFECT_ID typ)
{
    switch (typ) {
    case ROOM_T_NORMAL:
        return build_type1(player_ptr, dd_ptr);
    case ROOM_T_OVERLAP:
        return build_type2(player_ptr, dd_ptr);
    case ROOM_T_CROSS:
        return build_type3(player_ptr, dd_ptr);
    case ROOM_T_INNER_FEAT:
        return build_type4(player_ptr, dd_ptr);
    case ROOM_T_NEST:
        return build_type5(player_ptr, dd_ptr);
    case ROOM_T_PIT:
        return build_type6(player_ptr, dd_ptr);
    case ROOM_T_LESSER_VAULT:
        return build_type7(player_ptr, dd_ptr);
    case ROOM_T_GREATER_VAULT:
        return build_type8(player_ptr, dd_ptr);
    case ROOM_T_FRACAVE:
        return build_type9(player_ptr, dd_ptr);
    case ROOM_T_RANDOM_VAULT:
        return build_type10(player_ptr, dd_ptr);
    case ROOM_T_OVAL:
        return build_type11(player_ptr, dd_ptr);
    case ROOM_T_CRYPT:
        return build_type12(player_ptr, dd_ptr);
    case ROOM_T_TRAP_PIT:
        return build_type13(player_ptr, dd_ptr);
    case ROOM_T_TRAP:
        return build_type14(player_ptr, dd_ptr);
    case ROOM_T_GLASS:
        return build_type15(player_ptr, dd_ptr);
    case ROOM_T_ARCADE:
        return build_type16(player_ptr, dd_ptr);
    case ROOM_T_FIXED:
        return build_type17(player_ptr, dd_ptr);
    default:
        return false;
    }
}

/*!
 * @brief 指定した部屋の生成確率を別の部屋に加算し、指定した部屋の生成率を0にする
 * @param dst 確率を移す先の部屋種ID
 * @param src 確率を与える元の部屋種ID
 */
static void move_prob_list(room_type dst, room_type src, int *prob_list)
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
bool generate_rooms(player_type *player_ptr, dun_data_type *dd_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int crowded = 0;
    int prob_list[ROOM_T_MAX];
    int rooms_built = 0;
    int area_size = 100 * (floor_ptr->height * floor_ptr->width) / (MAX_HGT * MAX_WID);
    int level_index = MIN(10, div_round(floor_ptr->dun_level, 10));
    int16_t room_num[ROOM_T_MAX];
    int dun_rooms = DUN_ROOMS_MAX * area_size / 100;
    room_info_type *room_info_ptr = room_info_normal;
    for (int i = 0; i < ROOM_T_MAX; i++) {
        if (floor_ptr->dun_level < room_info_ptr[i].min_level)
            prob_list[i] = 0;
        else
            prob_list[i] = room_info_ptr[i].prob[level_index];
    }

    /*!
     * @details ダンジョンにBEGINNER、CHAMELEON、SMALLESTいずれのフラグもなく、
     * かつ「常に通常でない部屋を生成する」フラグがONならば、
     * GRATER_VAULTのみを生成対象とする。 / Ironman sees only Greater Vaults
     */
    if (ironman_rooms && d_info[floor_ptr->dungeon_idx].flags.has_none_of( {DF::BEGINNER, DF::CHAMELEON, DF::SMALLEST })) {
        for (int i = 0; i < ROOM_T_MAX; i++) {
            if (i == ROOM_T_GREATER_VAULT)
                prob_list[i] = 1;
            else
                prob_list[i] = 0;
        }
    } else if (d_info[floor_ptr->dungeon_idx].flags.has(DF::NO_VAULT)) {
        /*! @details ダンジョンにNO_VAULTフラグがあるならば、LESSER_VAULT / GREATER_VAULT/ RANDOM_VAULTを除外 / Forbidden vaults */
        prob_list[ROOM_T_LESSER_VAULT] = 0;
        prob_list[ROOM_T_GREATER_VAULT] = 0;
        prob_list[ROOM_T_RANDOM_VAULT] = 0;
    }

    /*! @details ダンジョンにBEGINNERフラグがあるならば、FIXED_ROOMを除外 / Forbidden vaults */
    if (d_info[floor_ptr->dungeon_idx].flags.has(DF::BEGINNER))
        prob_list[ROOM_T_FIXED] = 0;

    /*! @details ダンジョンにNO_CAVEフラグがある場合、FRACAVEの生成枠がNORMALに与えられる。CRIPT、OVALの生成枠がINNER_Fに与えられる。/ NO_CAVE dungeon */
    if (d_info[floor_ptr->dungeon_idx].flags.has(DF::NO_CAVE)) {
        move_prob_list(ROOM_T_NORMAL, ROOM_T_FRACAVE, prob_list);
        move_prob_list(ROOM_T_INNER_FEAT, ROOM_T_CRYPT, prob_list);
        move_prob_list(ROOM_T_INNER_FEAT, ROOM_T_OVAL, prob_list);
    } else if (d_info[floor_ptr->dungeon_idx].flags.has(DF::CAVE)) {
        /*! @details ダンジョンにCAVEフラグがある場合、NORMALの生成枠がFRACAVEに与えられる。/ CAVE dungeon (Orc floor_ptr->grid_array etc.) */
        move_prob_list(ROOM_T_FRACAVE, ROOM_T_NORMAL, prob_list);
    } else if (dd_ptr->cavern || dd_ptr->empty_level) {
        /*! @details ダンジョンの基本地形が最初から渓谷かアリーナ型の場合 FRACAVE は生成から除外。 /  No caves when a (random) cavern exists: they look bad */
        prob_list[ROOM_T_FRACAVE] = 0;
    }

    /*! @details ダンジョンに最初からGLASS_ROOMフラグがある場合、GLASS を生成から除外。/ Forbidden glass rooms */
    if (d_info[floor_ptr->dungeon_idx].flags.has_not(DF::GLASS_ROOM))
        prob_list[ROOM_T_GLASS] = 0;

    /*! @details ARCADEは同フラグがダンジョンにないと生成されない。 / Forbidden glass rooms */
    if (d_info[floor_ptr->dungeon_idx].flags.has_not(DF::ARCADE))
        prob_list[ROOM_T_ARCADE] = 0;

    ProbabilityTable<int> prob_table;
    for (int i = 0; i < ROOM_T_MAX; i++) {
        room_num[i] = 0;
        prob_table.entry_item(i, prob_list[i]);
    }

    for (int i = dun_rooms; i > 0; i--) {
        int room_type;
        if (prob_table.empty())
            room_type = ROOM_T_NORMAL;
        else
            room_type = prob_table.pick_one_at_random();

        room_num[room_type]++;
        switch (room_type) {
        case ROOM_T_NEST:
        case ROOM_T_PIT:
        case ROOM_T_LESSER_VAULT:
        case ROOM_T_TRAP_PIT:
        case ROOM_T_GLASS:
        case ROOM_T_ARCADE:
            /* Large room */
            i -= 2;
            break;
        case ROOM_T_GREATER_VAULT:
        case ROOM_T_RANDOM_VAULT:
            /* Largest room */
            i -= 3;
            break;
        }
    }

    bool remain;
    while (true) {
        remain = false;
        for (int i = 0; i < ROOM_T_MAX; i++) {
            int room_type = room_build_order[i];
            if (!room_num[room_type])
                continue;

            room_num[room_type]--;
            if (!room_build(player_ptr, dd_ptr, room_type))
                continue;

            rooms_built++;
            remain = true;
            switch (room_type) {
            case ROOM_T_PIT:
            case ROOM_T_NEST:
            case ROOM_T_TRAP_PIT:
                if (++crowded >= 2) {
                    room_num[ROOM_T_PIT] = 0;
                    room_num[ROOM_T_NEST] = 0;
                    room_num[ROOM_T_TRAP_PIT] = 0;
                }

                break;
            case ROOM_T_ARCADE:
                room_num[ROOM_T_ARCADE] = 0;
                break;
            }
        }

        if (!remain)
            break;
    }

    /*! @details 部屋生成数が2未満の場合生成失敗を返す */
    if (rooms_built < 2) {
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("部屋数が2未満でした。生成を再試行します。", "Number of rooms was under 2. Retry."), rooms_built);
        return false;
    }

    msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("このダンジョンの部屋数は %d です。", "Number of Rooms: %d"), rooms_built);
    return true;
}
