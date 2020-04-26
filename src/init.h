/*!
 * @file init.h
 * @brief ゲームデータ初期化処理のヘッダファイル
 * @date 2015/01/02
 * @author
 * Copyright (c) 2000 Robert Ruehlmann
 * @details
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#ifndef INCLUDED_INIT_H
#define INCLUDED_INIT_H

#include "h-basic.h"

typedef struct header header;

typedef errr(*parse_info_txt_func)(char *buf, header *head);

/*
 * Size of memory reserved for initialization of some arrays
 */
#define FAKE_NAME_SIZE  40 * 1024L /*!< ゲーム情報の種別毎に用意される名前用バッファの容量 */
#define FAKE_TEXT_SIZE 150 * 1024L /*!< ゲーム情報の種別毎に用意されるテキスト用バッファの容量 */
#define FAKE_TAG_SIZE   10 * 1024L /*!< ゲーム情報の種別毎に用意されるタグ用バッファの容量 */

#define VER_INFO_ROW 3   //!< タイトル表記(行)

 /*!
  * @brief マクロ登録の最大数 / Maximum number of macros (see "io.c")
  * @note Default: assume at most 256 macros are used
  */
#define MACRO_MAX       256


  /*!
   * @struct header
   * @brief 各初期データ用ヘッダ構造体 / Template file header information (see "init.c").  16 bytes.
   * @details
   * Note that the sizes of many of the "arrays" are between 32768 and
   * 65535, and so we must use "unsigned" values to hold the "sizes" of
   * these arrays below.  Normally, I try to avoid using unsigned values,
   * since they can cause all sorts of bizarre problems, but I have no
   * choice here, at least, until the "race" array is split into "normal"
   * and "unique" monsters, which may or may not actually help.
   *
   * Note that, on some machines, for example, the Macintosh, the standard
   * "read()" and "write()" functions cannot handle more than 32767 bytes
   * at one time, so we need replacement functions, see "util.c" for details.
   *
   * Note that, on some machines, for example, the Macintosh, the standard
   * "malloc()" function cannot handle more than 32767 bytes at one time,
   * but we may assume that the "ralloc()" function can handle up to 65535
   * butes at one time.  We should not, however, assume that the "ralloc()"
   * function can handle more than 65536 bytes at a time, since this might
   * result in segmentation problems on certain older machines, and in fact,
   * we should not assume that it can handle exactly 65536 bytes at a time,
   * since the internal functions may use an unsigned short to specify size.
   *
   * In general, these problems occur only on machines (such as most personal
   * computers) which use 2 byte "int" values, and which use "int" for the
   * arguments to the relevent functions.
   */
struct header
{
	byte v_major;		/* Version -- major */
	byte v_minor;		/* Version -- minor */
	byte v_patch;		/* Version -- patch */
	byte v_extra;		/* Version -- extra */

	u16b info_num;		/* Number of "info" records */
	int info_len;		/* Size of each "info" record */
	u16b head_size;		/* Size of the "header" in bytes */

	STR_OFFSET info_size;		/* Size of the "info" array in bytes */
	STR_OFFSET name_size;		/* Size of the "name" array in bytes */
	STR_OFFSET text_size;		/* Size of the "text" array in bytes */
	STR_OFFSET tag_size;		/* Size of the "tag" array in bytes */

	void *info_ptr;
	char *name_ptr;
	char *text_ptr;
	char *tag_ptr;

	parse_info_txt_func parse_info_txt;

	void(*retouch)(header *head);
};

extern errr init_info_txt(FILE *fp, char *buf, header *head,
	parse_info_txt_func parse_info_txt_line);

extern errr parse_v_info(char *buf, header *head);
extern errr parse_f_info(char *buf, header *head);
extern void retouch_f_info(header *head);
extern errr parse_k_info(char *buf, header *head);
extern errr parse_a_info(char *buf, header *head);
extern errr parse_e_info(char *buf, header *head);
extern errr parse_r_info(char *buf, header *head);
extern errr parse_d_info(char *buf, header *head);
extern errr parse_s_info(char *buf, header *head);
extern errr parse_m_info(char *buf, header *head);

/*
 * Error tracking
 */
extern int error_idx;
extern int error_line;

/*
 * File headers
 */
extern header z_head;
extern header v_head;
extern header f_head;
extern header k_head;
extern header a_head;
extern header e_head;
extern header r_head;
extern header p_head;
extern header h_head;
extern header b_head;
extern header g_head;

#endif /* INCLUDED_INIT_H */

extern s16b f_tag_to_index(concptr str);
extern s16b f_tag_to_index_in_init(concptr str);
extern void init_angband(player_type *player_ptr, void(*process_autopick_file_command)(char*));
extern concptr get_check_sum(void);
extern void init_file_paths(char *path);
