#include "room/rooms-maze-vault.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "room/treasure-deployment.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "wizard/wizard-messages.h"

/*
 * maze vault -- rectangular labyrinthine rooms
 *
 * maze vault uses two routines:
 *    r_visit - a recursive routine that builds the labyrinth
 *    build_maze_vault - a driver routine that calls r_visit and adds
 *                   monsters, traps and treasure
 *
 * The labyrinth is built by creating a spanning tree of a graph.
 * The graph vertices are at
 *    (x, y) = (2j + x1, 2k + y1)   j = 0,...,m-1    k = 0,...,n-1
 * and the edges are the vertical and horizontal nearest neighbors.
 *
 * The spanning tree is created by performing a suitably randomized
 * depth-first traversal of the graph. The only adjustable parameter
 * is the randint0(3) below; it governs the relative density of
 * twists and turns in the labyrinth: smaller number, more twists.
 */
void r_visit(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited)
{
    int adj[4];
    int m = (x2 - x1) / 2 + 1;
    int n = (y2 - y1) / 2 + 1;
    visited[node] = 1;
    int x = 2 * (node % m) + x1;
    int y = 2 * (node / m) + y1;
    place_bold(player_ptr, y, x, GB_FLOOR);

    if (one_in_(3)) {
        for (int i = 0; i < 4; i++)
            adj[i] = i;

        for (int i = 0; i < 4; i++) {
            int j = randint0(4);
            int temp = adj[i];
            adj[i] = adj[j];
            adj[j] = temp;
        }

        dir = adj[0];
    } else {
        adj[0] = dir;
        for (int i = 1; i < 4; i++)
            adj[i] = i;

        for (int i = 1; i < 4; i++) {
            int j = 1 + randint0(3);
            int temp = adj[i];
            adj[i] = adj[j];
            adj[j] = temp;
        }
    }

    for (int i = 0; i < 4; i++) {
        switch (adj[i]) {
        case 0:
            /* (0,+) - check for bottom boundary */
            if ((node / m < n - 1) && (visited[node + m] == 0)) {
                place_bold(player_ptr, y + 1, x, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node + m, dir, visited);
            }
            break;
        case 1:
            /* (0,-) - check for top boundary */
            if ((node / m > 0) && (visited[node - m] == 0)) {
                place_bold(player_ptr, y - 1, x, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node - m, dir, visited);
            }
            break;
        case 2:
            /* (+,0) - check for right boundary */
            if ((node % m < m - 1) && (visited[node + 1] == 0)) {
                place_bold(player_ptr, y, x + 1, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node + 1, dir, visited);
            }
            break;
        case 3:
            /* (-,0) - check for left boundary */
            if ((node % m > 0) && (visited[node - 1] == 0)) {
                place_bold(player_ptr, y, x - 1, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node - 1, dir, visited);
            }
        }
    }
}

void build_maze_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize, bool is_vault)
{
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("迷路ランダムVaultを生成しました。", "Maze Vault."));
    auto *floor_ptr = player_ptr->current_floor_ptr;
    bool light = ((floor_ptr->dun_level <= randint1(25)) && is_vault && d_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS));
    POSITION dy = ysize / 2 - 1;
    POSITION dx = xsize / 2 - 1;
    POSITION y1 = y0 - dy;
    POSITION x1 = x0 - dx;
    POSITION y2 = y0 + dy;
    POSITION x2 = x0 + dx;
    for (POSITION y = y1 - 1; y <= y2 + 1; y++) {
        for (POSITION x = x1 - 1; x <= x2 + 1; x++) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info |= CAVE_ROOM;
            if (is_vault)
                g_ptr->info |= CAVE_ICKY;
            if ((x == x1 - 1) || (x == x2 + 1) || (y == y1 - 1) || (y == y2 + 1)) {
                place_grid(player_ptr, g_ptr, GB_OUTER);
            } else if (!is_vault) {
                place_grid(player_ptr, g_ptr, GB_EXTRA);
            } else {
                place_grid(player_ptr, g_ptr, GB_INNER);
            }

            if (light)
                g_ptr->info |= (CAVE_GLOW);
        }
    }

    int m = dx + 1;
    int n = dy + 1;
    int num_vertices = m * n;

    std::vector<int> visited(num_vertices);
    r_visit(player_ptr, y1, x1, y2, x2, randint0(num_vertices), 0, visited.data());
    if (is_vault)
        fill_treasure(player_ptr, x1, x2, y1, y2, randint1(5));
}
