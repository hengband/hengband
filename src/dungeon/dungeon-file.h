#pragma once

#include "system/angband.h"
#include "main/init.h" // 相互参照、後で何とかする.

/* Random dungeon grid effects */
#define RANDOM_NONE         0x00000000
#define RANDOM_FEATURE      0x00000001
#define RANDOM_MONSTER      0x00000002
#define RANDOM_OBJECT       0x00000004
#define RANDOM_EGO          0x00000008
#define RANDOM_ARTIFACT     0x00000010
#define RANDOM_TRAP         0x00000020

#define PARSE_ERROR_GENERIC                  1
#define PARSE_ERROR_INVALID_FLAG             5
#define PARSE_ERROR_UNDEFINED_DIRECTIVE      6
#define PARSE_ERROR_OUT_OF_MEMORY            7
#define PARSE_ERROR_OUT_OF_BOUNDS            8
#define PARSE_ERROR_TOO_FEW_ARGUMENTS        9
#define PARSE_ERROR_UNDEFINED_TERRAIN_TAG   10
#define PARSE_ERROR_MAX                     11

extern concptr err_str[PARSE_ERROR_MAX];

errr init_v_info(void);
errr init_buildings(void);
errr init_info_txt(FILE *fp, char *buf, angband_header *head, parse_info_txt_func parse_info_txt_line);
errr parse_v_info(char *buf, angband_header *head);
errr parse_s_info(char *buf, angband_header *head);
errr parse_m_info(char *buf, angband_header *head);
errr parse_f_info(char *buf, angband_header *head);
s16b f_tag_to_index(concptr str);
void retouch_f_info(angband_header *head);
errr parse_k_info(char *buf, angband_header *head);
errr parse_a_info(char *buf, angband_header *head);
errr parse_e_info(char *buf, angband_header *head);
errr parse_r_info(char *buf, angband_header *head);
errr parse_d_info(char *buf, angband_header *head);
errr process_dungeon_file(player_type *player_ptr, concptr name, int ymin, int xmin, int ymax, int xmax);
