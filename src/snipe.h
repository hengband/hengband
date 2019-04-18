#pragma once

/* snipe.c */
extern void reset_concentration(bool msg);
extern void display_snipe_list(void);
extern MULTIPLY tot_dam_aux_snipe(MULTIPLY mult, monster_type *m_ptr, SPELL_IDX snipe_type);
extern void do_cmd_snipe(void);
extern void do_cmd_snipe_browse(void);
extern int boost_concentration_damage(int tdam);
