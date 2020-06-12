/* File: z-term.h */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#ifndef INCLUDED_Z_TERM_H
#define INCLUDED_Z_TERM_H

#include "system/angband.h"
#include "system/h-basic.h"

typedef struct term_win term_win;
 /*!
 * @brief A term_win is a "window" for a Term
 */
 struct term_win
{
	bool cu, cv; //!< Cursor Useless / Visible codes
	TERM_LEN cx, cy; //!< Cursor Location (see "Useless")

	TERM_COLOR **a; //!< Array[h*w] -- Attribute array 
	char **c; //!< Array[h*w] -- Character array

	TERM_COLOR *va; //!< Array[h] -- Access to the attribute array
	char *vc; //!< Array[h] -- Access to the character array

	TERM_COLOR **ta; //!< Note that the attr pair at(x, y) is a[y][x]
	char **tc; //!< Note that the char pair at(x, y) is c[y][x]

	TERM_COLOR *vta; //!< Note that the row of attr at(0, y) is a[y]
	char *vtc;  //!< Note that the row of chars at(0, y) is c[y]
};



/*!
 * @brief term実装構造体 / An actual "term" structure
 */
typedef struct term term;
struct term
{
	vptr user; //!< Extra "user" info (used by application)
	vptr data; //!< Extra "data" info (used by implementation)

	bool user_flag; //!< Flag "user_flag" An extra "user" flag (used by application)
	bool data_flag; //!< Flag "data_flag" An extra "data" flag (used by implementation)

	bool active_flag; //!< Flag "active_flag" This "term" is "active"
	bool mapped_flag; //!< Flag "mapped_flag" This "term" is "mapped"
	bool total_erase; //!< Flag "total_erase" This "term" should be fully erased
	bool fixed_shape; //!< Flag "fixed_shape" This "term" is not allowed to resize
	bool icky_corner; //!< Flag "icky_corner" This "term" has an "icky" corner grid
	bool soft_cursor; //!< Flag "soft_cursor" This "term" uses a "software" cursor
	bool always_pict; //!< Flag "always_pict" Use the "Term_pict()" routine for all text
	bool higher_pict; //!< Flag "higher_pict" Use the "Term_pict()" routine for special text
	bool always_text; //!< Flag "always_text" Use the "Term_text()" routine for invisible text
	bool unused_flag; //!< Flag "unused_flag" Reserved for future use
	bool never_bored; //!< Flag "never_bored" Never call the "TERM_XTRA_BORED" action
	bool never_frosh; //!< Flag "never_frosh" Never call the "TERM_XTRA_FROSH" action

	byte attr_blank; //!< Value "attr_blank" Use this "attr" value for "blank" grids
	char char_blank; //!< Value "char_blank" Use this "char" value for "blank" grids

	char *key_queue; //!< Keypress Queue -- various data / Keypress Queue -- pending keys
	u16b key_head;
	u16b key_tail;
	u16b key_xtra;
	u16b key_size;

	TERM_LEN wid; //!< Window Width(max 255)
	TERM_LEN hgt; //!< Window Height(max 255)

	TERM_LEN y1; //!< Minimum modified row
	TERM_LEN y2; //!< Maximum modified row

	TERM_LEN *x1; //!< Minimum modified column(per row)
	TERM_LEN *x2; //!< Maximum modified column(per row)

	term_win *old; //!< Displayed screen image
	term_win *scr; //!< Requested screen image

	term_win *tmp; //!< Temporary screen image
	term_win *mem; //!< Memorized screen image

	void (*init_hook)(term *t); //!< Hook for init - ing the term
	void (*nuke_hook)(term *t); //!< Hook for nuke - ing the term

	errr (*user_hook)(int n); //!< ユーザ設定項目実装部 / Hook for user actions
	errr (*xtra_hook)(int n, int v); //!< 拡張機能実装部 / Hook for extra actions
	errr (*curs_hook)(TERM_LEN x, TERM_LEN y); //!< カーソル描画実装部 / Hook for placing the cursor
	errr (*bigcurs_hook)(TERM_LEN x, TERM_LEN y); //!< 大型タイル時カーソル描画実装部 / Hook for placing the cursor on bigtile mode
	errr (*wipe_hook)(TERM_LEN x, TERM_LEN y, int n); //!< 指定座標テキスト消去実装部 / Hook for drawing some blank spaces
	errr (*text_hook)(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr s); //!< テキスト描画実装部 / Hook for drawing a string of chars using an attr
	void (*resize_hook)(void); //!< 画面リサイズ実装部
	errr (*pict_hook)(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, concptr cp, const TERM_COLOR *tap, concptr tcp); //!< タイル描画実装部 / Hook for drawing a sequence of special attr / char pairs
};







/**** Available Constants ****/


/*
 * Definitions for the "actions" of "Term_xtra()"
 *
 * These values may be used as the first parameter of "Term_xtra()",
 * with the second parameter depending on the "action" itself.  Many
 * of the actions shown below are optional on at least one platform.
 *
 * The "TERM_XTRA_EVENT" action uses "v" to "wait" for an event
 * The "TERM_XTRA_SHAPE" action uses "v" to "show" the cursor
 * The "TERM_XTRA_FROSH" action uses "v" for the index of the row
 * The "TERM_XTRA_SOUND" action uses "v" for the index of a sound
 * The "TERM_XTRA_ALIVE" action uses "v" to "activate" (or "close")
 * The "TERM_XTRA_LEVEL" action uses "v" to "resume" (or "suspend")
 * The "TERM_XTRA_DELAY" action uses "v" as a "millisecond" value
 *
 * The other actions do not need a "v" code, so "zero" is used.
 */
#define TERM_XTRA_EVENT	1	/* Process some pending events */
#define TERM_XTRA_FLUSH 2	/* Flush all pending events */
#define TERM_XTRA_CLEAR 3	/* Clear the entire window */
#define TERM_XTRA_SHAPE 4	/* Set cursor shape (optional) */
#define TERM_XTRA_FROSH 5	/* Flush one row (optional) */
#define TERM_XTRA_FRESH 6	/* Flush all rows (optional) */
#define TERM_XTRA_NOISE 7	/* Make a noise (optional) */
#define TERM_XTRA_SOUND 8	/* Make a sound (optional) */
#define TERM_XTRA_BORED 9	/* Handle stuff when bored (optional) */
#define TERM_XTRA_REACT 10	/* React to global changes (optional) */
#define TERM_XTRA_ALIVE 11	/* Change the "hard" level (optional) */
#define TERM_XTRA_LEVEL 12	/* Change the "soft" level (optional) */
#define TERM_XTRA_DELAY 13	/* Delay some milliseconds (optional) */
#define TERM_XTRA_MUSIC_BASIC 14   /* Play a music(basic)   (optional) */
#define TERM_XTRA_MUSIC_DUNGEON 15 /* Play a music(dungeon) (optional) */
#define TERM_XTRA_MUSIC_QUEST 16   /* Play a music(quest)   (optional) */
#define TERM_XTRA_MUSIC_TOWN 17    /* Play a music(floor)   (optional) */
#define TERM_XTRA_MUSIC_MUTE 18

/**** Available Variables ****/

extern term *Term;


/**** Available Functions ****/

extern errr Term_user(int n);
extern errr Term_xtra(int n, int v);

extern void Term_queue_char(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c, TERM_COLOR ta, char tc);
extern void Term_queue_bigchar(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c, TERM_COLOR ta, char tc);

extern void Term_queue_line(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR *a, char *c, TERM_COLOR *ta, char *tc);

extern void Term_queue_chars(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr s);

extern errr Term_fresh(void);
extern errr Term_set_cursor(int v);
extern errr Term_gotoxy(TERM_LEN x, TERM_LEN y);
extern errr Term_draw(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c);
extern errr Term_addch(TERM_COLOR a, char c);
extern errr Term_add_bigch(TERM_COLOR a, char c);
extern errr Term_addstr(int n, TERM_COLOR a, concptr s);
extern errr Term_putch(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c);
extern errr Term_putstr(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr s);
#ifdef JP
extern errr Term_putstr_v(TERM_LEN x, TERM_LEN y, int n, byte a, concptr s);
#endif
extern errr Term_erase(TERM_LEN x, TERM_LEN y, int n);
extern errr Term_clear(void);
extern errr Term_redraw(void);
extern errr Term_redraw_section(TERM_LEN x1, TERM_LEN y1, TERM_LEN x2, TERM_LEN y2);

extern errr Term_get_cursor(int *v);
extern errr Term_get_size(TERM_LEN *w, TERM_LEN *h);
extern errr Term_locate(TERM_LEN *x, TERM_LEN *y);
extern errr Term_what(TERM_LEN x, TERM_LEN y, TERM_COLOR *a, char *c);

extern errr Term_flush(void);
extern errr Term_keypress(int k);
extern errr Term_key_push(int k);
extern errr Term_inkey(char *ch, bool wait, bool take);

extern errr Term_save(void);
extern errr Term_load(void);

extern errr Term_exchange(void);

extern errr Term_resize(TERM_LEN w, TERM_LEN h);

extern errr Term_activate(term *t);

extern errr term_nuke(term *t);
extern errr term_init(term *t, TERM_LEN w, TERM_LEN h, int k);


#endif


