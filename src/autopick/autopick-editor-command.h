#pragma once

/*! APE means AutoPickEditor*/
enum ape_quittance {
	APE_QUIT = 0,
	APE_QUIT_WITHOUT_SAVE = 1,
	APE_QUIT_AND_SAVE = 2
};

struct player_type;
struct text_body_type;
ape_quittance do_editor_command(player_type *player_ptr, text_body_type *tb, int com_id);
