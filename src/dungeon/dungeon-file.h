#pragma once

#include "system/angband.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"

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
