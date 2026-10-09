#pragma once

struct text_body_type;
class PlayerType;
void update_autopick_expression_states(PlayerType *player_ptr, text_body_type *tb);
void draw_text_editor(PlayerType *player_ptr, text_body_type *tb);
