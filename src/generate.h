#define ALLOW_CAVERNS_AND_LAKES

#define SAFE_MAX_ATTEMPTS 5000

/*
 * Dungeon generation values
 */
#define DUN_UNUSUAL 250 /* Level/chance of unusual room (was 200) */
#define DUN_DEST    18  /* 1/chance of having a destroyed level */
#define SMALL_LEVEL 3   /* 1/chance of smaller size (3) */
#define EMPTY_LEVEL 24  /* 1/chance of being 'empty' (15) */
#define LAKE_LEVEL  24  /* 1/chance of being a lake on the level */
#define DARK_EMPTY  5   /* 1/chance of arena level NOT being lit (2) */
#define DUN_CAVERN  20	/* 1/chance of having a cavern level */

/*
 * Dungeon tunnel generation values
 */
#define DUN_TUN_RND_MIN	 5 /* Chance of random direction (was 10) */
#define DUN_TUN_RND_MAX	20
#define DUN_TUN_CHG_MIN	20 /* Chance of changing direction (was 30) */
#define DUN_TUN_CHG_MAX	60
#define DUN_TUN_CON_MIN 10 /* Chance of extra tunneling (was 15) */
#define DUN_TUN_CON_MAX	40
#define DUN_TUN_PEN_MIN 30 /* Chance of doors at room entrances (was 25) */
#define DUN_TUN_PEN_MAX 70
#define DUN_TUN_JCT_MIN 60 /* Chance of doors at tunnel junctions (was 90) */
#define DUN_TUN_JCT_MAX 90

extern int dun_rooms;

extern int dun_tun_rnd;
extern int dun_tun_chg;
extern int dun_tun_con;
extern int dun_tun_pen;
extern int dun_tun_jct;

/*
 * Dungeon streamer generation values
 */
#define DUN_STR_DEN     5	/* Density of streamers */
#define DUN_STR_RNG     5	/* Width of streamers */
#define DUN_STR_MAG     6	/* Number of magma streamers */
#define DUN_STR_MC     30	/* 1/chance of treasure per magma */
#define DUN_STR_QUA	4	/* Number of quartz streamers */
#define DUN_STR_QC     15	/* 1/chance of treasure per quartz */
#define DUN_STR_WLW     1	/* Width of lava & water streamers -KMW- */
#define DUN_STR_DWLW    8	/* Density of water & lava streams -KMW- */

#define DUN_MOS_DEN	2	/* Density of moss streamers */
#define DUN_MOS_RNG	10	/* Width of moss streamers */
#define DUN_STR_MOS	2	/* Number of moss streamers */
#define DUN_WAT_DEN	15	/* Density of rivers */
#define DUN_WAT_RNG	2	/* Width of rivers */
#define DUN_STR_WAT	3	/* Max number of rivers */
#define DUN_WAT_CHG	50	/* 1 in 50 chance of junction in river */

/*
 * Dungeon treausre allocation values
 */
#define DUN_AMT_ROOM	9	/* Amount of objects for rooms */
#define DUN_AMT_ITEM	3	/* Amount of objects for rooms/corridors */
#define DUN_AMT_GOLD	3	/* Amount of treasure for rooms/corridors */
#define DUN_AMT_INVIS 3	/* Amount of invisible walls for rooms/corridors */

/*
 * Hack -- Dungeon allocation "places"
 */
#define ALLOC_SET_CORR		1	/* Hallway */
#define ALLOC_SET_ROOM		2	/* Room */
#define ALLOC_SET_BOTH		3	/* Anywhere */

/*
 * Hack -- Dungeon allocation "types"
 */
#define ALLOC_TYP_RUBBLE	1	/* Rubble */
#define ALLOC_TYP_TRAP		3	/* Trap */
#define ALLOC_TYP_GOLD		4	/* Gold */
#define ALLOC_TYP_OBJECT	5	/* Object */
#define ALLOC_TYP_INVIS		6	/* Invisible wall */



/*
 * The "size" of a "generation block" in grids
 */
#define BLOCK_HGT	11
#define BLOCK_WID	11

/*
 * Maximum numbers of rooms along each axis (currently 6x6)
 */
#define MAX_ROOMS_ROW	(MAX_HGT / BLOCK_HGT)
#define MAX_ROOMS_COL	(MAX_WID / BLOCK_WID)


/*
 * Bounds on some arrays used in the "dun_data" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX	100
#define DOOR_MAX	200
#define WALL_MAX	500
#define TUNN_MAX	900


/*
 * Structure to hold all "dungeon generation" data
 */

typedef struct dun_data dun_data;

struct dun_data
{
	/* Array of centers of rooms */
	int cent_n;
	coord cent[CENT_MAX];

	/* Array of possible door locations */
	int door_n;
	coord door[DOOR_MAX];

	/* Array of wall piercing locations */
	int wall_n;
	coord wall[WALL_MAX];

	/* Array of tunnel grids */
	int tunn_n;
	coord tunn[TUNN_MAX];

	/* Number of blocks along each axis */
	int row_rooms;
	int col_rooms;

	/* Array of which blocks are used */
	bool room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];

	/* Various type of dungeon floors */
	bool destroyed;
	bool empty_level;
	bool cavern;
	int laketype;
};

extern dun_data *dun;
