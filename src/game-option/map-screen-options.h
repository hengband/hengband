#pragma once

#include "system/angband.h"

extern bool center_player; /* Center map while walking (*slow*) */
extern bool center_running; /* Centering even while running */
extern bool view_yellow_lite; /* Use special colors for torch-lit grids */
extern bool view_bright_lite; /* Use special colors for 'viewable' grids */
extern bool view_granite_lite; /* Use special colors for wall grids (slow) */
extern bool view_special_lite; /* Use special colors for floor grids (slow) */
extern bool view_perma_grids; /* Map remembers all perma-lit grids */
extern bool view_torch_grids; /* Map remembers all torch-lit grids */
extern bool view_unsafe_grids; /* Map marked by detect traps */
extern bool view_reduce_view; /* Reduce view-radius in town */
extern bool view_hidden_walls; /* Map walls hidden in other walls. */
extern bool view_unsafe_walls; /* Map hidden walls not marked by detect traps. */
extern bool fresh_before; /* Flush output while continuous command */
extern bool fresh_after; /* Flush output after monster's move */
extern bool fresh_once; /* Flush output only once per key input */
extern bool fresh_message; /* Flush output after every message */
extern bool hilite_player; /* Hilite the player with the cursor */
extern bool display_path; /* Display actual path before shooting */
