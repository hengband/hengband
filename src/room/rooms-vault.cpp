/*!
 * @brief Vaultの生成処理
 * @date 2018/09/11
 * @author deskull
 */

#include "room/rooms-vault.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-generator.h"
#include "floor/floor-town.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/item-apply-magic.h"
#include "room/cave-filler.h"
#include "room/door-definition.h"
#include "room/lake-types.h"
#include "room/rooms-builder.h"
#include "room/rooms-maze-vault.h"
#include "room/space-finder.h"
#include "room/treasure-deployment.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/dungeon/dungeon-data-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/probability-table.h"
#include "wizard/wizard-messages.h"

/*
 * The vault generation arrays
 */
std::vector<vault_type> vaults_info;

constexpr auto NUM_BUBBLES = 10;

namespace {
std::array<Pos2D, NUM_BUBBLES> allocate_bubbles_center(const Pos2DVec &vec)
{
    std::array<Pos2D, NUM_BUBBLES> center_points{ {
        { randint1(vec.y - 3) + 1, randint1(vec.x - 3) + 1 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
    } };
    return center_points;
}

std::array<Pos2D, NUM_BUBBLES> create_bubbles_center(const Pos2DVec &vec)
{
    auto center_points = allocate_bubbles_center(vec);
    Pos2D pos(0, 0);
    bool is_center_checked;
    for (auto i = 1; i < NUM_BUBBLES; i++) {
        is_center_checked = false;
        while (!is_center_checked) {
            is_center_checked = true;
            pos = { randint1(vec.y - 3) + 1, randint1(vec.x - 3) + 1 };
            for (auto j = 0; j < i; j++) {
                if (pos == center_points[j]) {
                    is_center_checked = false;
                }
            }
        }

        center_points[i] = pos;
    }

    return center_points;
}

void set_boundaries(PlayerType *player_ptr, const Pos2D &pos)
{
    place_bold(player_ptr, pos.y, pos.x, GB_OUTER_NOPERM);
    player_ptr->current_floor_ptr->get_grid(pos).add_info(CAVE_ROOM | CAVE_ICKY);
}
}

/*!
 * @brief 泡型Vaultを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos0 Vault全体の中心座標
 * @param vec 中心から部屋の隅までの線分長
 */
static void build_bubble_vault(PlayerType *player_ptr, const Pos2D &pos0, const Pos2DVec &vec)
{
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("泡型ランダムVaultを生成しました。", "Bubble-shaped Vault."));
    auto center_points = create_bubbles_center(vec);

    const Pos2DVec vec_half(vec.y / 2, vec.x / 2);

    /* Top and bottom boundaries */
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto i = 0; i < vec.x; i++) {
        const auto side_x = pos0.x - vec_half.x + i;
        set_boundaries(player_ptr, { pos0.y - vec_half.y, side_x });
        set_boundaries(player_ptr, { pos0.y - vec_half.y + vec.y - 1, side_x });
    }

    /* Left and right boundaries */
    for (auto i = 1; i < vec.y - 1; i++) {
        const auto side_y = pos0.y - vec_half.y + i;
        set_boundaries(player_ptr, { side_y, pos0.x - vec_half.x });
        set_boundaries(player_ptr, { side_y, pos0.x - vec_half.x + vec.x - 1 });
    }

    /* Fill in middle with bubbles */
    for (auto x = 1; x < vec.x - 1; x++) {
        for (auto y = 1; y < vec.y - 1; y++) {
            /* Get distances to two closest centers */
            auto min1 = distance(x, y, center_points[0].x, center_points[0].y);
            auto min2 = distance(x, y, center_points[1].x, center_points[1].y);
            if (min1 > min2) {
                std::swap(min1, min2);
            }

            /* Scan the rest */
            for (auto i = 2; i < NUM_BUBBLES; i++) {
                auto temp = distance(x, y, center_points[i].x, center_points[i].y);
                if (temp < min1) {
                    min2 = min1;
                    std::swap(temp, min1);
                } else if (temp < min2) {
                    std::swap(min2, temp);
                }
            }

            if (((min2 - min1) <= 2) && (!(min1 < 3))) {
                /* Boundary at midpoint+ not at inner region of bubble */
                place_bold(player_ptr, pos0.y - vec_half.y + y, pos0.x - vec_half.x + x, GB_OUTER_NOPERM);
            } else {
                /* middle of a bubble */
                place_bold(player_ptr, pos0.y - vec_half.y + y, pos0.x - vec_half.x + x, GB_FLOOR);
            }

            /* clean up rest of flags */
            floor.get_grid({ pos0.y - vec_half.y + y, pos0.x - vec_half.x + x }).add_info(CAVE_ROOM | CAVE_ICKY);
        }
    }

    /* Try to add some random doors */
    for (auto i = 0; i < 500; i++) {
        add_door(player_ptr, randint1(vec.x - 3) - vec_half.x + pos0.x + 1, randint1(vec.y - 3) - vec_half.y + pos0.y + 1);
    }

    /* Fill with monsters and treasure, low difficulty */
    fill_treasure(player_ptr, pos0.x - vec_half.x + 1, pos0.x - vec_half.x + vec.x - 2, pos0.y - vec_half.y + 1, pos0.y - vec_half.y + vec.y - 2, randint1(5));
}

/* Create a random vault that looks like a collection of overlapping rooms */
static void build_room_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
    /* get offset from center */
    const auto xhsize = xsize / 2;
    const auto yhsize = ysize / 2;

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("部屋型ランダムVaultを生成しました。", "Room Vault."));

    /* fill area so don't get problems with on_defeat_arena_monster levels */
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto x1 = 0; x1 < xsize; x1++) {
        const auto x = x0 - xhsize + x1;
        for (auto y1 = 0; y1 < ysize; y1++) {
            const Pos2D pos(y0 - yhsize + y1, x);
            place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
            floor.get_grid(pos).info &= (~CAVE_ICKY);
        }
    }

    /* add ten random rooms */
    for (auto i = 0; i < 10; i++) {
        const auto x1 = randint1(xhsize) * 2 + x0 - xhsize;
        const auto x2 = randint1(xhsize) * 2 + x0 - xhsize;
        const auto y1 = randint1(yhsize) * 2 + y0 - yhsize;
        const auto y2 = randint1(yhsize) * 2 + y0 - yhsize;
        build_room(player_ptr, x1, x2, y1, y2);
    }

    /* Add some random doors */
    for (auto i = 0; i < 500; i++) {
        const auto x1 = randint1(xsize - 3) - xhsize + x0 + 1;
        const auto y1 = randint1(ysize - 3) - yhsize + y0 + 1;
        add_door(player_ptr, x1, y1);
    }

    /* Fill with monsters and treasure, high difficulty */
    fill_treasure(player_ptr, x0 - xhsize + 1, x0 - xhsize + xsize - 2, y0 - yhsize + 1, y0 - yhsize + ysize - 2, randint1(5) + 5);
}

/* Create a random vault out of a fractal grid */
static void build_cave_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsiz, POSITION ysiz)
{
    /* round to make sizes even */
    const auto xhsize = xsiz / 2;
    const auto yhsize = ysiz / 2;
    const auto xsize = xhsize * 2;
    const auto ysize = yhsize * 2;

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("洞穴ランダムVaultを生成しました。", "Cave Vault."));

    auto light = false;
    auto done = false;
    auto room = true;

    auto &floor = *player_ptr->current_floor_ptr;
    while (!done) {
        /* testing values for these parameters feel free to adjust */
        const auto grd = 1 << randint0(4);

        /* want average of about 16 */
        const auto roug = randint1(8) * randint1(4);

        /* about size/2 */
        const auto cutoff = randint1(xsize / 4) + randint1(ysize / 4) + randint1(xsize / 4) + randint1(ysize / 4);

        /* make it */
        generate_hmap(&floor, y0, x0, xsize, ysize, grd, roug, cutoff);

        /* Convert to normal format+ clean up */
        done = generate_fracave(player_ptr, y0, x0, xsize, ysize, cutoff, light, room);
    }

    /* Set icky flag because is a vault */
    for (auto x = 0; x <= xsize; x++) {
        for (auto y = 0; y <= ysize; y++) {
            floor.get_grid({ y0 - yhsize + y, x0 - xhsize + x }).info |= CAVE_ICKY;
        }
    }

    /* Fill with monsters and treasure, low difficulty */
    fill_treasure(player_ptr, x0 - xhsize + 1, x0 - xhsize + xsize - 1, y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint1(5));
}

/*!
 * @brief Vault地形を回転、上下左右反転するための座標変換を返す
 * @param pos_initial 変換したい点
 * @param offset Vault生成時の座標オフセット
 * @param transno 処理ID
 * @return 回転後の座標
 */
static Pos2D coord_trans(const Pos2D &pos_initial, const Pos2DVec &offset, int transno)
{
    /*
     * transno specifies what transformation is required. (0-7)
     * The lower two bits indicate by how much the vault is rotated,
     * and the upper bit indicates a reflection.
     * This is done by using rotation matrices... however since
     * these are mostly zeros for rotations by 90 degrees this can
     * be expressed simply in terms of swapping and inverting the
     * x and y coordinates.
     */
    Pos2D pos = pos_initial;
    for (auto i = 0; i < transno % 4; i++) {
        /* rotate by 90 degrees */
        auto temp = pos.x;
        pos.x = -pos.y;
        pos.y = temp;
    }

    if (transno / 4) {
        /* Reflect depending on status of 3rd bit. */
        pos.x = -pos.x;
    }

    /* Add offsets so vault stays in the first quadrant */
    pos += offset;
    return pos;
}

/*!
 * @brief Vaultをフロアに配置する / Hack -- fill in "vault" rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param yval 生成基準Y座標
 * @param xval 生成基準X座標
 * @param ymax VaultのYサイズ
 * @param xmax VaultのXサイズ
 * @param data Vaultのデータ文字列
 * @param xoffset 変換基準X座標
 * @param yoffset 変換基準Y座標
 * @param transno 変換ID
 */
static void build_vault(
    PlayerType *player_ptr, POSITION yval, POSITION xval, POSITION ymax, POSITION xmax, concptr data, POSITION xoffset, POSITION yoffset, int transno)
{
    /* Place dungeon features and objects */
    auto &floor = *player_ptr->current_floor_ptr;
    auto t = data;
    for (auto dy = 0; dy < ymax; dy++) {
        for (auto dx = 0; dx < xmax; dx++, t++) {
            /* prevent loop counter from being overwritten */
            auto i = dx;
            auto j = dy;

            /* Flip / rotate */
            const auto pos_trans = coord_trans({ j, i }, { yoffset, xoffset }, transno);
            i = pos_trans.x;
            j = pos_trans.y;
            int x;
            int y;
            if (transno % 2 == 0) {
                /* no swap of x/y */
                x = xval - (xmax / 2) + i;
                y = yval - (ymax / 2) + j;
            } else {
                /* swap of x/y */
                x = xval - (ymax / 2) + i;
                y = yval - (xmax / 2) + j;
            }

            /* Hack -- skip "non-grids" */
            if (*t == ' ') {
                continue;
            }

            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);

            /* Lay down a floor */
            place_grid(player_ptr, &grid, GB_FLOOR);

            /* Remove any mimic */
            grid.mimic = 0;

            /* Part of a vault */
            grid.info |= (CAVE_ROOM | CAVE_ICKY);

            /* Analyze the grid */
            switch (*t) {
                /* Granite wall (outer) */
            case '%':
                place_grid(player_ptr, &grid, GB_OUTER_NOPERM);
                break;

                /* Granite wall (inner) */
            case '#':
                place_grid(player_ptr, &grid, GB_INNER);
                break;

                /* Glass wall (inner) */
            case '$':
                place_grid(player_ptr, &grid, GB_INNER);
                grid.feat = feat_glass_wall;
                break;

                /* Permanent wall (inner) */
            case 'X':
                place_grid(player_ptr, &grid, GB_INNER_PERM);
                break;

                /* Permanent glass wall (inner) */
            case 'Y':
                place_grid(player_ptr, &grid, GB_INNER_PERM);
                grid.feat = feat_permanent_glass_wall;
                break;

                /* Treasure/trap */
            case '*':
                if (evaluate_percent(75)) {
                    place_object(player_ptr, pos.y, pos.x, 0L);
                } else {
                    place_trap(&floor, pos.y, pos.x);
                }
                break;

                /* Treasure */
            case '[':
                place_object(player_ptr, pos.y, pos.x, 0L);
                break;

                /* Tree */
            case ':':
                grid.feat = feat_tree;
                break;

                /* Secret doors */
            case '+':
                place_secret_door(player_ptr, pos.y, pos.x, DOOR_DEFAULT);
                break;

                /* Secret glass doors */
            case '-':
                place_secret_door(player_ptr, pos.y, pos.x, DOOR_GLASS_DOOR);
                if (floor.is_closed_door(pos)) {
                    grid.mimic = feat_glass_wall;
                }

                break;

                /* Curtains */
            case '\'':
                place_secret_door(player_ptr, pos.y, pos.x, DOOR_CURTAIN);
                break;

                /* Trap */
            case '^':
                place_trap(&floor, pos.y, pos.x);
                break;

                /* Black market in a dungeon */
            case 'S':
                set_cave_feat(&floor, pos.y, pos.x, feat_black_market);
                store_init(VALID_TOWNS, StoreSaleType::BLACK);
                break;

                /* The Pattern */
            case 'p':
                set_cave_feat(&floor, pos.y, pos.x, feat_pattern_start);
                break;

            case 'a':
                set_cave_feat(&floor, pos.y, pos.x, feat_pattern_1);
                break;

            case 'b':
                set_cave_feat(&floor, pos.y, pos.x, feat_pattern_2);
                break;

            case 'c':
                set_cave_feat(&floor, pos.y, pos.x, feat_pattern_3);
                break;

            case 'd':
                set_cave_feat(&floor, pos.y, pos.x, feat_pattern_4);
                break;

            case 'P':
                set_cave_feat(&floor, pos.y, pos.x, feat_pattern_end);
                break;

            case 'B':
                set_cave_feat(&floor, pos.y, pos.x, feat_pattern_exit);
                break;

            case 'A':
                /* Reward for Pattern walk */
                floor.object_level = floor.base_level + 12;
                place_object(player_ptr, pos.y, pos.x, AM_GOOD | AM_GREAT);
                floor.object_level = floor.base_level;
                break;

            case '~':
                set_cave_feat(&floor, pos.y, pos.x, feat_shallow_water);
                break;

            case '=':
                set_cave_feat(&floor, pos.y, pos.x, feat_deep_water);
                break;

            case 'v':
                set_cave_feat(&floor, pos.y, pos.x, feat_shallow_lava);
                break;

            case 'w':
                set_cave_feat(&floor, pos.y, pos.x, feat_deep_lava);
                break;

            case 'f':
                set_cave_feat(&floor, pos.y, pos.x, feat_shallow_acid_puddle);
                break;

            case 'F':
                set_cave_feat(&floor, pos.y, pos.x, feat_deep_acid_puddle);
                break;

            case 'g':
                set_cave_feat(&floor, pos.y, pos.x, feat_shallow_poisonous_puddle);
                break;

            case 'G':
                set_cave_feat(&floor, pos.y, pos.x, feat_deep_poisonous_puddle);
                break;

            case 'h':
                set_cave_feat(&floor, pos.y, pos.x, feat_cold_zone);
                break;

            case 'H':
                set_cave_feat(&floor, pos.y, pos.x, feat_heavy_cold_zone);
                break;

            case 'i':
                set_cave_feat(&floor, pos.y, pos.x, feat_electrical_zone);
                break;

            case 'I':
                set_cave_feat(&floor, pos.y, pos.x, feat_heavy_electrical_zone);
                break;
            }
        }
    }

    /* Place dungeon monsters and objects */
    t = data;
    for (auto dy = 0; dy < ymax; dy++) {
        for (auto dx = 0; dx < xmax; dx++, t++) {
            /* prevent loop counter from being overwritten */
            auto i = dx;
            auto j = dy;

            /* Flip / rotate */
            const auto pos = coord_trans({ j, i }, { yoffset, xoffset }, transno);
            i = pos.x;
            j = pos.y;
            int y;
            int x;
            if (transno % 2 == 0) {
                /* no swap of x/y */
                x = xval - (xmax / 2) + i;
                y = yval - (ymax / 2) + j;
            } else {
                /* swap of x/y */
                x = xval - (ymax / 2) + i;
                y = yval - (xmax / 2) + j;
            }

            /* Hack -- skip "non-grids" */
            if (*t == ' ') {
                continue;
            }

            /* Analyze the symbol */
            switch (*t) {
            case '&': {
                floor.monster_level = floor.base_level + 5;
                place_random_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                floor.monster_level = floor.base_level;
                break;
            }

            /* Meaner monster */
            case '@': {
                floor.monster_level = floor.base_level + 11;
                place_random_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                floor.monster_level = floor.base_level;
                break;
            }

            /* Meaner monster, plus treasure */
            case '9': {
                floor.monster_level = floor.base_level + 9;
                place_random_monster(player_ptr, y, x, PM_ALLOW_SLEEP);
                floor.monster_level = floor.base_level;
                floor.object_level = floor.base_level + 7;
                place_object(player_ptr, y, x, AM_GOOD);
                floor.object_level = floor.base_level;
                break;
            }

            /* Nasty monster and treasure */
            case '8': {
                floor.monster_level = floor.base_level + 40;
                place_random_monster(player_ptr, y, x, PM_ALLOW_SLEEP);
                floor.monster_level = floor.base_level;
                floor.object_level = floor.base_level + 20;
                place_object(player_ptr, y, x, AM_GOOD | AM_GREAT);
                floor.object_level = floor.base_level;
                break;
            }

            /* Monster and/or object */
            case ',': {
                if (one_in_(2)) {
                    floor.monster_level = floor.base_level + 3;
                    place_random_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                    floor.monster_level = floor.base_level;
                }
                if (one_in_(2)) {
                    floor.object_level = floor.base_level + 7;
                    place_object(player_ptr, y, x, 0L);
                    floor.object_level = floor.base_level;
                }
                break;
            }
            }
        }
    }
}

/*
 * Build target vault.
 * This is made by two concentric "crypts" with perpendicular
 * walls creating the cross-hairs.
 */
static void build_target_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
    /* Make a random metric */
    const auto h1 = randint1(32) - 16;
    const auto h2 = randint1(16);
    const auto h3 = randint1(32);
    const auto h4 = randint1(32) - 16;

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("対称形ランダムVaultを生成しました。", "Target Vault"));

    const auto rad = xsize > ysize ? ysize / 2 : xsize / 2;

    /* Make floor */
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto x = x0 - rad; x <= x0 + rad; x++) {
        for (auto y = y0 - rad; y <= y0 + rad; y++) {
            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);
            grid.info &= ~(CAVE_ROOM);
            grid.info |= CAVE_ICKY;

            if (dist2(y0, x0, pos.y, pos.x, h1, h2, h3, h4) <= rad - 1) {
                /* inside- so is floor */
                place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
            } else {
                /* make granite outside so on_defeat_arena_monster works */
                place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
            }

            /* proper boundary for on_defeat_arena_monster */
            if (((pos.y + rad) == y0) || ((pos.y - rad) == y0) || ((pos.x + rad) == x0) || ((pos.x - rad) == x0)) {
                place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
            }
        }
    }

    /* Find visible outer walls and set to be FEAT_OUTER */
    add_outer_wall(player_ptr, x0, y0, false, x0 - rad - 1, y0 - rad - 1, x0 + rad + 1, y0 + rad + 1);

    /* Add inner wall */
    for (auto x = x0 - rad / 2; x <= x0 + rad / 2; x++) {
        for (auto y = y0 - rad / 2; y <= y0 + rad / 2; y++) {
            if (dist2(y0, x0, y, x, h1, h2, h3, h4) == rad / 2) {
                /* Make an internal wall */
                place_bold(player_ptr, y, x, GB_INNER);
            }
        }
    }

    /* Add perpendicular walls */
    for (auto x = x0 - rad; x <= x0 + rad; x++) {
        place_bold(player_ptr, y0, x, GB_INNER);
    }

    for (auto y = y0 - rad; y <= y0 + rad; y++) {
        place_bold(player_ptr, y, x0, GB_INNER);
    }

    /* Make inner vault */
    for (auto y = y0 - 1; y <= y0 + 1; y++) {
        place_bold(player_ptr, y, x0 - 1, GB_INNER);
        place_bold(player_ptr, y, x0 + 1, GB_INNER);
    }
    for (auto x = x0 - 1; x <= x0 + 1; x++) {
        place_bold(player_ptr, y0 - 1, x, GB_INNER);
        place_bold(player_ptr, y0 + 1, x, GB_INNER);
    }

    place_bold(player_ptr, y0, x0, GB_FLOOR);

    /* Add doors to vault */
    /* get two distances so can place doors relative to centre */
    auto x = (rad - 2) / 4 + 1;
    auto y = rad / 2 + x;

    add_door(player_ptr, x0 + x, y0);
    add_door(player_ptr, x0 + y, y0);
    add_door(player_ptr, x0 - x, y0);
    add_door(player_ptr, x0 - y, y0);
    add_door(player_ptr, x0, y0 + x);
    add_door(player_ptr, x0, y0 + y);
    add_door(player_ptr, x0, y0 - x);
    add_door(player_ptr, x0, y0 - y);

    /* Fill with stuff - medium difficulty */
    fill_treasure(player_ptr, x0 - rad, x0 + rad, y0 - rad, y0 + rad, randint1(3) + 3);
}

/*
 * This routine uses a modified version of the lake code to make a
 * distribution of some terrain type over the vault.  This type
 * depends on the dungeon depth.
 *
 * Miniture rooms are then scattered across the vault.
 */
static void build_elemental_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsiz, POSITION ysiz)
{
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("精霊界ランダムVaultを生成しました。", "Elemental Vault"));

    /* round to make sizes even */
    const auto xhsize = xsiz / 2;
    const auto yhsize = ysiz / 2;
    const auto xsize = xhsize * 2;
    const auto ysize = yhsize * 2;

    auto &floor = *player_ptr->current_floor_ptr;
    lake_type type;
    if (floor.dun_level < 25) {
        /* Earth vault  (Rubble) */
        type = LAKE_T_EARTH_VAULT;
    } else if (floor.dun_level < 50) {
        /* Air vault (Trees) */
        type = LAKE_T_AIR_VAULT;
    } else if (floor.dun_level < 75) {
        /* Water vault (shallow water) */
        type = LAKE_T_WATER_VAULT;
    } else {
        /* Fire vault (shallow lava) */
        type = LAKE_T_FIRE_VAULT;
    }

    auto done = false;
    while (!done) {
        /* testing values for these parameters: feel free to adjust */
        const auto grd = 1 << (randint0(3));

        /* want average of about 16 */
        const auto roug = randint1(8) * randint1(4);

        /* Make up size of various componants */
        /* Floor */
        const auto c3 = 2 * xsize / 3;

        /* Deep water/lava */
        const auto c1 = randint0(c3 / 2) + randint0(c3 / 2) - 5;

        /* Shallow boundary */
        const auto c2 = (c1 + c3) / 2;

        /* make it */
        generate_hmap(&floor, y0, x0, xsize, ysize, grd, roug, c3);

        /* Convert to normal format+ clean up */
        done = generate_lake(player_ptr, y0, x0, xsize, ysize, c1, c2, c3, type);
    }

    /* Set icky flag because is a vault */
    for (auto x = 0; x <= xsize; x++) {
        for (auto y = 0; y <= ysize; y++) {
            floor.grid_array[y0 - yhsize + y][x0 - xhsize + x].info |= CAVE_ICKY;
        }
    }

    /* make a few rooms in the vault */
    for (auto i = 1; i <= (xsize * ysize) / 50; i++) {
        build_small_room(player_ptr, x0 + randint0(xsize - 4) - xsize / 2 + 2, y0 + randint0(ysize - 4) - ysize / 2 + 2);
    }

    /* Fill with monsters and treasure, low difficulty */
    fill_treasure(player_ptr, x0 - xhsize + 1, x0 - xhsize + xsize - 1, y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint1(5));
}

/* Build a "mini" checkerboard vault
 *
 * This is done by making a permanent wall maze and setting
 * the diagonal sqaures of the checker board to be granite.
 * The vault has two entrances on opposite sides to guarantee
 * a way to get in even if the vault abuts a side of the dungeon.
 */
static void build_mini_c_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("小型チェッカーランダムVaultを生成しました。", "Mini Checker Board Vault."));

    /* Pick a random room size */
    const auto dy = ysize / 2 - 1;
    const auto dx = xsize / 2 - 1;
    const auto y1 = y0 - dy;
    const auto x1 = x0 - dx;
    const auto y2 = y0 + dy;
    const auto x2 = x0 + dx;

    /* generate the room */
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto x = x1 - 2; x <= x2 + 2; x++) {
        const Pos2D pos(y1 - 2, x);
        if (!in_bounds(&floor, pos.y, pos.x)) {
            break;
        }

        floor.get_grid(pos).info |= (CAVE_ROOM | CAVE_ICKY);
        place_bold(player_ptr, pos.y, pos.x, GB_OUTER_NOPERM);
    }

    for (auto x = x1 - 2; x <= x2 + 2; x++) {
        const Pos2D pos(y2 + 2, x);
        if (!in_bounds(&floor, pos.y, pos.x)) {
            break;
        }

        floor.get_grid(pos).info |= (CAVE_ROOM | CAVE_ICKY);
        place_bold(player_ptr, pos.y, pos.x, GB_OUTER_NOPERM);
    }

    for (auto y = y1 - 2; y <= y2 + 2; y++) {
        const Pos2D pos(y, x1 - 2);
        if (!in_bounds(&floor, pos.y, pos.x)) {
            break;
        }

        floor.get_grid(pos).info |= (CAVE_ROOM | CAVE_ICKY);
        place_bold(player_ptr, pos.y, pos.x, GB_OUTER_NOPERM);
    }

    for (auto y = y1 - 2; y <= y2 + 2; y++) {
        if (!in_bounds(&floor, y, x2 + 2)) {
            break;
        }

        const Pos2D pos(y, x2 + 2);
        floor.get_grid(pos).info |= (CAVE_ROOM | CAVE_ICKY);
        place_bold(player_ptr, pos.y, pos.x, GB_OUTER_NOPERM);
    }

    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        for (auto x = x1 - 1; x <= x2 + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            grid.info |= (CAVE_ROOM | CAVE_ICKY);

            /* Permanent walls */
            place_grid(player_ptr, &grid, GB_INNER_PERM);
        }
    }

    /* dimensions of vertex array */
    const auto m = dx + 1;
    const auto n = dy + 1;
    const auto num_vertices = m * n;

    /* initialize array of visited vertices */
    std::vector<int> visited(num_vertices);

    /* traverse the graph to create a spannng tree, pick a random root */
    r_visit(player_ptr, y1, x1, y2, x2, randint0(num_vertices), 0, visited.data());

    /* Make it look like a checker board vault */
    for (auto x = x1; x <= x2; x++) {
        for (auto y = y1; y <= y2; y++) {
            const auto total = x - x1 + y - y1;
            /* If total is odd- and is a floor then make a wall */
            if ((total % 2 == 1) && floor.grid_array[y][x].is_floor()) {
                place_bold(player_ptr, y, x, GB_INNER);
            }
        }
    }

    /* Make a couple of entrances */
    if (one_in_(2)) {
        /* left and right */
        const auto y = randint1(dy) + dy / 2;
        place_bold(player_ptr, y1 + y, x1 - 1, GB_INNER);
        place_bold(player_ptr, y1 + y, x2 + 1, GB_INNER);
    } else {
        /* top and bottom */
        const auto x = randint1(dx) + dx / 2;
        place_bold(player_ptr, y1 - 1, x1 + x, GB_INNER);
        place_bold(player_ptr, y2 + 1, x1 + x, GB_INNER);
    }

    /* Fill with monsters and treasure, highest difficulty */
    fill_treasure(player_ptr, x1, x2, y1, y2, 10);
}

/* Build a castle */
/* Driver routine: clear the region and call the recursive
 * room routine.
 *
 *This makes a vault that looks like a castle/ city in the dungeon.
 */
static void build_castle_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
    /* Pick a random room size */
    const auto dy = ysize / 2 - 1;
    const auto dx = xsize / 2 - 1;
    const auto y1 = y0 - dy;
    const auto x1 = x0 - dx;
    const auto y2 = y0 + dy;
    const auto x2 = x0 + dx;

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("城型ランダムVaultを生成しました。", "Castle Vault"));

    /* generate the room */
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        for (auto x = x1 - 1; x <= x2 + 1; x++) {
            const Pos2D pos(y, x);
            floor.get_grid(pos).info |= (CAVE_ROOM | CAVE_ICKY);
            /* Make everything a floor */
            place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        }
    }

    /* Make the castle */
    build_recursive_room(player_ptr, x1, y1, x2, y2, randint1(5));

    /* Fill with monsters and treasure, low difficulty */
    fill_treasure(player_ptr, x1, x2, y1, y2, randint1(3));
}

/*!
 * @brief タイプ10の部屋…ランダム生成vault / Type 10 -- Random vaults
 * @param player_ptr プレイヤーへの参照ポインタ
 */
bool build_type10(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto xsize = randint1(22) + 22;
    const auto ysize = randint1(11) + 11;
    int y;
    int x;
    if (!find_space(player_ptr, dd_ptr, &y, &x, ysize + 1, xsize + 1)) {
        return false;
    }

    int vault_type;
    do {
        vault_type = randint1(15);
    } while (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_CAVE) && ((vault_type == 1) || (vault_type == 3) || (vault_type == 8) || (vault_type == 9) || (vault_type == 11)));

    switch (vault_type) {
    case 1:
    case 9:
        build_bubble_vault(player_ptr, { y, x }, { ysize, xsize });
        break;
    case 2:
    case 10:
        build_room_vault(player_ptr, x, y, xsize, ysize);
        break;
    case 3:
    case 11:
        build_cave_vault(player_ptr, x, y, xsize, ysize);
        break;
    case 4:
    case 12:
        build_maze_vault(player_ptr, x, y, xsize, ysize, true);
        break;
    case 5:
    case 13:
        build_mini_c_vault(player_ptr, x, y, xsize, ysize);
        break;
    case 6:
    case 14:
        build_castle_vault(player_ptr, x, y, xsize, ysize);
        break;
    case 7:
    case 15:
        build_target_vault(player_ptr, x, y, xsize, ysize);
        break;
    case 8:
        build_elemental_vault(player_ptr, x, y, xsize, ysize);
        break;
    default:
        return false;
    }

    return true;
}

/*!
 * @brief VaultDefinitions からの部屋生成
 */
bool build_fixed_room(PlayerType *player_ptr, DungeonData *dd_ptr, int typ, bool more_space)
{
    ProbabilityTable<int> prob_table;
    for (const auto &vault : vaults_info) {
        if (vault.typ == typ) {
            prob_table.entry_item(vault.idx, 1);
        }
    }

    const auto result = prob_table.pick_one_at_random();
    const auto &vault = vaults_info[result];
    auto num_transformation = randint0(8);

    /* calculate offsets */
    auto x = vault.wid;
    auto y = vault.hgt;

    /* Some huge vault cannot be ratated to fit in the dungeon */
    const auto &floor = *player_ptr->current_floor_ptr;
    if (x + 2 > floor.height - 2) {
        /* Forbid 90 or 270 degree ratation */
        num_transformation &= ~1;
    }

    const auto pos = coord_trans({ y, x }, { 0, 0 }, num_transformation);
    y = pos.y;
    x = pos.x;
    const auto y_offset = y < 0 ? -y - 1 : 0;
    const auto x_offset = x < 0 ? -x - 1 : 0;

    /*
     * Try to allocate space for room.  If fails, exit
     *
     * Hack -- Prepare a bit larger space (+2, +2) to
     * prevent generation of vaults with no-entrance.
     */
    const auto xsize = more_space ? std::abs(x) + 2 : std::abs(x);
    const auto ysize = more_space ? std::abs(y) + 2 : std::abs(y);
    /* Find and reserve some space in the dungeon.  Get center of room. */
    int yval;
    int xval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, ysize, xsize)) {
        return false;
    }

    msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("固定部屋(%s)を生成しました。", "Fixed room (%s)."), vault.name.data());
    build_vault(player_ptr, yval, xval, vault.hgt, vault.wid, vault.text.data(), x_offset, y_offset, num_transformation);
    return true;
}
