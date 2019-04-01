/*
 * Bit flags for the place_monster_???() (etc)
 */
#define PM_ALLOW_SLEEP    0x00000001    /*!< モンスター生成フラグ: 眠っている状態で生成されても良い */
#define PM_ALLOW_GROUP    0x00000002    /*!< モンスター生成フラグ: 集団生成されても良い */
#define PM_FORCE_FRIENDLY 0x00000004    /*!< モンスター生成フラグ: 必ず友好的に生成される */
#define PM_FORCE_PET      0x00000008    /*!< モンスター生成フラグ: 必ずペットとして生成される */
#define PM_NO_KAGE        0x00000010    /*!< モンスター生成フラグ: 必ずあやしい影としては生成されない */
#define PM_NO_PET         0x00000020    /*!< モンスター生成フラグ: 必ずペットとして生成されない */
#define PM_ALLOW_UNIQUE   0x00000040    /*!< モンスター生成フラグ: ユニークの選択生成を許可する */
#define PM_IGNORE_TERRAIN 0x00000080    /*!< モンスター生成フラグ: 侵入可能地形を考慮せずに生成する */
#define PM_HASTE          0x00000100    /*!< モンスター生成フラグ: 加速状態で生成する */
#define PM_KAGE           0x00000200    /*!< モンスター生成フラグ: 必ずあやしい影として生成する */
#define PM_MULTIPLY       0x00000400    /*!< モンスター生成フラグ: 増殖処理時として生成する */

extern bool place_monster_aux(MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
extern bool place_monster(POSITION y, POSITION x, BIT_FLAGS mode);
extern bool alloc_horde(POSITION y, POSITION x);
extern bool alloc_guardian(bool def_val);
extern bool alloc_monster(POSITION dis, BIT_FLAGS mode);


extern void monster_desc(char *desc, monster_type *m_ptr, BIT_FLAGS mode);
/* Bit flags for monster_desc() */
#define MD_OBJECTIVE      0x00000001 /* Objective (or Reflexive) */
#define MD_POSSESSIVE     0x00000002 /* Possessive (or Reflexive) */
#define MD_INDEF_HIDDEN   0x00000004 /* Use indefinites for hidden monsters ("something") */
#define MD_INDEF_VISIBLE  0x00000008 /* Use indefinites for visible monsters ("a kobold") */
#define MD_PRON_HIDDEN    0x00000010 /* Pronominalize hidden monsters */
#define MD_PRON_VISIBLE   0x00000020 /* Pronominalize visible monsters */
#define MD_ASSUME_HIDDEN  0x00000040 /* Assume the monster is hidden */
#define MD_ASSUME_VISIBLE 0x00000080 /* Assume the monster is visible */
#define MD_TRUE_NAME      0x00000100 /* Chameleon's true name */
#define MD_IGNORE_HALLU   0x00000200 /* Ignore hallucination, and penetrate shape change */

#define MD_WRONGDOER_NAME (MD_IGNORE_HALLU | MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE) /* 加害明記向け */

extern void roff_top(MONRACE_IDX r_idx);
extern void screen_roff(MONRACE_IDX r_idx, BIT_FLAGS mode);
extern void display_roff(MONRACE_IDX r_idx);
extern void output_monster_spoiler(MONRACE_IDX r_idx, void(*roff_func)(TERM_COLOR attr, concptr str));
extern void create_name(int type, char *name);
extern concptr extract_note_dies(MONRACE_IDX r_idx);
extern void monster_death(MONSTER_IDX m_idx, bool drop_item);
extern monsterrace_hook_type get_monster_hook(void);
extern monsterrace_hook_type get_monster_hook2(POSITION y, POSITION x);
extern void set_friendly(monster_type *m_ptr);
extern void set_pet(monster_type *m_ptr);
extern void set_hostile(monster_type *m_ptr);
extern void anger_monster(monster_type *m_ptr);
extern bool monster_can_cross_terrain(FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode);
extern bool monster_can_enter(POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode);
extern bool are_enemies(monster_type *m_ptr1, monster_type *m_ptr2);
extern bool monster_has_hostile_align(monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr);
extern void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char* msg);
extern concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode);
