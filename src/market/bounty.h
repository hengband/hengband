#pragma once

struct player_type;
bool exchange_cash(player_type *player_ptr);
void today_target(player_type *player_ptr);
void tsuchinoko(void);
void show_bounty(void);
void determine_daily_bounty(player_type *player_ptr, bool conv_old);
void determine_bounty_uniques(player_type *player_ptr);
