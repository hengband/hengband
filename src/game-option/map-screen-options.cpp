#include "game-option/map-screen-options.h"

bool center_player; /* Center map while walking (*slow*) */
bool center_running; /* Centering even while running */
bool view_yellow_lite; /* Use special colors for torch-lit grids */
bool view_bright_lite; /* Use special colors for 'viewable' grids */
bool view_granite_lite; /* Use special colors for wall grids (slow) */
bool view_special_lite; /* Use special colors for floor grids (slow) */
bool view_perma_grids; /* Map remembers all perma-lit grids */
bool view_torch_grids; /* Map remembers all torch-lit grids */
bool view_unsafe_grids; /* Map marked by detect traps */
bool view_reduce_view; /* Reduce view-radius in town */
bool view_hidden_walls; /* Map walls hidden in other walls. */
bool view_unsafe_walls; /* Map hidden walls not marked by detect traps. */
bool fresh_before; /* Flush output while in repeated command */
bool fresh_after; /* Flush output after monster's move */
bool fresh_once; /* Flush output only once per key input */
bool fresh_message; /* Flush output after every message */
bool hilite_player; /* Hilite the player with the cursor */
bool display_path; /* Display actual path before shooting */
