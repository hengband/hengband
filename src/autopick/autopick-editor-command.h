#pragma once

/*! APE means AutoPickEditor*/
enum ape_quittance {
    APE_QUIT = 0,
    APE_QUIT_WITHOUT_SAVE = 1,
    APE_QUIT_AND_SAVE = 2
};

enum class EditorCommandId;
class PlayerType;
struct text_body_type;
ape_quittance do_editor_command(PlayerType *player_ptr, text_body_type *tb, EditorCommandId com_id);
