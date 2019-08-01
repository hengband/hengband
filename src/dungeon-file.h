#pragma once

typedef struct dungeon_grid dungeon_grid;

struct dungeon_grid
{
	FEAT_IDX feature;		/* Terrain feature */
	MONSTER_IDX	monster;		/* Monster */
	OBJECT_IDX object;			/* Object */
	EGO_IDX	ego;			/* Ego-Item */
	ARTIFACT_IDX artifact;		/* Artifact */
	IDX trap;			/* Trap */
	BIT_FLAGS cave_info;		/* Flags for CAVE_MARK, CAVE_GLOW, CAVE_ICKY, CAVE_ROOM */
	s16b special; /* Reserved for special terrain info */
	int random;			/* Number of the random effect */
};

/* Random dungeon grid effects */
#define RANDOM_NONE         0x00000000
#define RANDOM_FEATURE      0x00000001
#define RANDOM_MONSTER      0x00000002
#define RANDOM_OBJECT       0x00000004
#define RANDOM_EGO          0x00000008
#define RANDOM_ARTIFACT     0x00000010
#define RANDOM_TRAP         0x00000020

/*
 * Parse errors
 */
#define PARSE_ERROR_GENERIC                  1
#define PARSE_ERROR_ABSOLETE_FILE            2
#define PARSE_ERROR_MISSING_RECORD_HEADER    3
#define PARSE_ERROR_NON_SEQUENTIAL_RECORDS   4
#define PARSE_ERROR_INVALID_FLAG             5
#define PARSE_ERROR_UNDEFINED_DIRECTIVE      6
#define PARSE_ERROR_OUT_OF_MEMORY            7
#define PARSE_ERROR_OUT_OF_BOUNDS            8
#define PARSE_ERROR_TOO_FEW_ARGUMENTS        9
#define PARSE_ERROR_UNDEFINED_TERRAIN_TAG   10
#define PARSE_ERROR_MAX                     11

extern concptr err_str[PARSE_ERROR_MAX];
extern errr process_dungeon_file(concptr name, int ymin, int xmin, int ymax, int xmax);
extern errr init_v_info(void);
extern errr init_buildings(void);

extern dungeon_grid letter[255];

