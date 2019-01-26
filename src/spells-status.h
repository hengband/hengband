
extern bool heal_monster(DIRECTION dir, HIT_POINT dam);
extern bool speed_monster(DIRECTION dir, int power);
extern bool slow_monster(DIRECTION dir, int power);
extern bool sleep_monster(DIRECTION dir, int power);
extern bool stasis_monster(DIRECTION dir);    /* Like sleep, affects undead as well */
extern bool stasis_evil(DIRECTION dir);    /* Like sleep, affects undead as well */
extern bool confuse_monster(DIRECTION dir, PLAYER_LEVEL plev);
extern bool stun_monster(DIRECTION dir, PLAYER_LEVEL plev);
extern bool fear_monster(DIRECTION dir, PLAYER_LEVEL plev);
extern bool poly_monster(DIRECTION dir, int power);
extern bool clone_monster(DIRECTION dir);
extern void stop_singing(void);

