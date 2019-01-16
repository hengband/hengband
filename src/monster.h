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
