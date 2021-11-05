#pragma once

class PlayerType;
bool exchange_cash(PlayerType *player_ptr);
void today_target(PlayerType *player_ptr);
void tsuchinoko(void);
void show_bounty(void);
void determine_daily_bounty(PlayerType *player_ptr, bool conv_old);
void determine_bounty_uniques(PlayerType *player_ptr);
