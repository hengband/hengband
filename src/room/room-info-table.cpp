#include "room/room-info-table.h"

/*!
 * 各部屋タイプの生成比定義
 *[from SAngband (originally from OAngband)]\n
 *\n
 * Table of values that control how many times each type of room will\n
 * appear.  Each type of room has its own row, and each column\n
 * corresponds to dungeon levels 0, 10, 20, and so on.  The final\n
 * value is the minimum depth the room can appear at.  -LM-\n
 *\n
 * Level 101 and below use the values for level 100.\n
 *\n
 * Rooms with lots of monsters or loot may not be generated if the\n
 * object or monster lists are already nearly full.  Rooms will not\n
 * appear above their minimum depth.  Tiny levels will not have space\n
 * for all the rooms you ask for.\n
 */
room_info_type room_info_normal[ROOM_TYPE_MAX] = {
    /* Depth */
    /*  0  10  20  30  40  50  60  70  80  90 100  min limit */
    { { 999, 900, 800, 700, 600, 500, 400, 300, 200, 100, 0 }, 0 }, /*NORMAL   */
    { { 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 }, 1 }, /*OVERLAP  */
    { { 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 }, 3 }, /*CROSS    */
    { { 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 }, 3 }, /*INNER_F  */
    { { 0, 1, 1, 1, 2, 3, 5, 6, 8, 10, 13 }, 10 }, /*NEST     */
    { { 0, 1, 1, 2, 3, 4, 6, 8, 10, 13, 16 }, 10 }, /*PIT      */
    { { 0, 1, 1, 1, 2, 2, 3, 5, 6, 8, 10 }, 10 }, /*LESSER_V */
    { { 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4 }, 20 }, /*GREATER_V*/
    { { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 999 }, 10 }, /*FRACAVE  */
    { { 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 }, 10 }, /*RANDOM_V */
    { { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40 }, 3 }, /*OVAL     */
    { { 1, 6, 12, 18, 24, 30, 36, 42, 48, 54, 60 }, 10 }, /*CRYPT    */
    { { 0, 0, 1, 1, 1, 2, 3, 4, 5, 6, 8 }, 20 }, /*TRAP_PIT */
    { { 0, 0, 1, 1, 1, 2, 3, 4, 5, 6, 8 }, 20 }, /*TRAP     */
    { { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 }, 40 }, /*GLASS    */
    { { 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3 }, 1 }, /*ARCADE   */
    { { 1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80 }, 1 }, /*FIX      */
};

/*! 部屋の生成処理順 / Build rooms in descending order of difficulty. */
RoomType room_build_order[ROOM_TYPE_MAX] = {
    RoomType::GREATER_VAULT,
    RoomType::ARCADE,
    RoomType::RANDOM_VAULT,
    RoomType::LESSER_VAULT,
    RoomType::TRAP_PIT,
    RoomType::PIT,
    RoomType::NEST,
    RoomType::TRAP,
    RoomType::GLASS,
    RoomType::INNER_FEAT,
    RoomType::FIXED,
    RoomType::OVAL,
    RoomType::CRYPT,
    RoomType::OVERLAP,
    RoomType::CROSS,
    RoomType::FRACAVE,
    RoomType::NORMAL,
};
