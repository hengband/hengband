#pragma once

#include "system/angband.h"

int increase_insults(void);
void decrease_insults(void);
int haggle_insults(void);
void updatebargain(PRICE price, PRICE minprice, int num);
bool receive_offer(concptr pmt, s32b *poffer, s32b last_offer, int factor, PRICE price, int final);
