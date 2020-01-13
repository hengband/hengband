#pragma once

/*
 * A store owner
 */
typedef struct owner_type owner_type;

struct owner_type
{
	concptr owner_name;	/* Name */
	PRICE max_cost;		/* Purse limit */
	byte max_inflate;	/* Inflation (max) */
	byte min_inflate;	/* Inflation (min) */
	byte haggle_per;	/* Haggle unit */
	byte insult_max;	/* Insult limit */
	byte owner_race;	/* Owner race */
};


/*
 * A store, with an owner, various state flags, a current stock
 * of items, and a table of items that are often purchased.
 */
typedef struct store_type store_type;

struct store_type
{
	byte type;				/* Store type */

	byte owner;				/* Owner index */
	byte extra;				/* Unused for now */

	s16b insult_cur;		/* Insult counter */

	s16b good_buy;			/* Number of "good" buys */
	s16b bad_buy;			/* Number of "bad" buys */

	s32b store_open;		/* Closed until this turn */

	s32b last_visit;		/* Last visited on this turn */

	s16b table_num;			/* Table -- Number of entries */
	s16b table_size;		/* Table -- Total Size of Array */
	s16b *table;			/* Table -- Legal item kinds */

	s16b stock_num;			/* Stock -- Number of entries */
	s16b stock_size;		/* Stock -- Total Size of Array */
	object_type *stock;		/* Stock -- Actual stock items */
};

/*
 * Store types
 */
#define STORE_GENERAL   0 /*!< 店舗の種類: 雑貨屋 */
#define STORE_ARMOURY   1 /*!< 店舗の種類: 防具屋 */
#define STORE_WEAPON    2 /*!< 店舗の種類: 武器屋 */
#define STORE_TEMPLE    3 /*!< 店舗の種類: 寺院 */
#define STORE_ALCHEMIST 4 /*!< 店舗の種類: 錬金術の店 */
#define STORE_MAGIC     5 /*!< 店舗の種類: 魔道具屋 */
#define STORE_BLACK     6 /*!< 店舗の種類: ブラック・マーケット */
#define STORE_HOME      7 /*!< 店舗の種類: 我が家 */
#define STORE_BOOK      8 /*!< 店舗の種類: 書店 */
#define STORE_MUSEUM    9 /*!< 店舗の種類: 博物館 */
#define MAX_STORES      10 /*!< store.c用の店舗の種類最大数 / Total number of stores (see "store.c", etc) */

#define MAX_OWNERS      32 /*!< 各店舗毎の店主定義最大数 / Total number of owners per store (see "store.c", etc) */

 /*
  * Store constants
  */
#define STORE_INVEN_MAX 24              /* Max number of discrete objs in inven */
#define STORE_CHOICES   48              /* Number of items to choose stock from */
#define STORE_OBJ_LEVEL 5               /* Magic Level for normal stores */
#define STORE_TURNOVER  9               /* Normal shop turnover, per day */
#define STORE_MIN_KEEP  6               /* Min slots to "always" keep full */
#define STORE_MAX_KEEP  18              /* Max slots to "always" keep full */
#define STORE_SHUFFLE   21              /* 1/Chance (per day) of an owner changing */
#define STORE_TICKS     1000            /* Number of ticks between turnovers */

/* store.c */
extern bool combine_and_reorder_home(int store_num);
extern void do_cmd_store(player_type *player_ptr);
extern void store_shuffle(player_type *player_ptr, int which);
extern void store_maint(player_type *player_ptr, int town_num, int store_num);
extern void store_init(int town_num, int store_num);

// どこからも呼ばれていない.
extern void move_to_black_market(player_type *player_ptr, object_type *o_ptr);

extern const owner_type owners[MAX_STORES][MAX_OWNERS];

extern byte store_table[MAX_STORES][STORE_CHOICES][2];
