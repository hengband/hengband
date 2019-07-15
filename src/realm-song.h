
/*
 * Music songs
 */
#define MUSIC_NONE              0
#define MUSIC_SLOW              1
#define MUSIC_BLESS             2
#define MUSIC_STUN              3
#define MUSIC_L_LIFE            4
#define MUSIC_FEAR              5
#define MUSIC_HERO              6
#define MUSIC_MIND              7
#define MUSIC_STEALTH           8
#define MUSIC_ID                9
#define MUSIC_CONF              10
#define MUSIC_SOUND             11
#define MUSIC_CHARM             12
#define MUSIC_WALL              13
#define MUSIC_RESIST            14
#define MUSIC_SPEED             15
#define MUSIC_DISPEL            16
#define MUSIC_SARUMAN           17
#define MUSIC_QUAKE             18
#define MUSIC_STASIS            19
#define MUSIC_SHERO             20
#define MUSIC_H_LIFE            21
#define MUSIC_INVULN            22
#define MUSIC_PSI               23

#define MUSIC_DETECT            101

#define SINGING_SONG_EFFECT(P_PTR) ((P_PTR)->magic_num1[0])
#define INTERUPTING_SONG_EFFECT(P_PTR) ((P_PTR)->magic_num1[1])
#define SINGING_COUNT(P_PTR) ((P_PTR)->magic_num1[2])
#define SINGING_SONG_ID(P_PTR) ((P_PTR)->magic_num2[0])
#define music_singing(p_ptr, X) ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0] == (X)))
#define music_singing_any(CREATURE_PTR) (((CREATURE_PTR)->pclass == CLASS_BARD) && (CREATURE_PTR)->magic_num1[0])

extern concptr do_music_spell(SPELL_IDX spell, BIT_FLAGS mode);
