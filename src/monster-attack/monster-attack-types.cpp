#include "monster-attack/monster-attack-types.h"
#include "monster-attack/monster-attack-effect.h"
#include "spell/spell-types.h"

/*!
 * @brief モンスターの打撃効力テーブル /
 * The table of monsters' blow effects
 */
const mbe_info_type mbe_info[NB_RBE_TYPE] = {
    {
        0,
        0,
    }, /* None      */

    {
        60,
        GF_MISSILE,
    }, /* HURT      */

    {
        5,
        GF_POIS,
    }, /* POISON    */

    {
        20,
        GF_DISENCHANT,
    }, /* UN_BONUS  */

    {
        15,
        GF_MISSILE,
    },
    /* UN_POWER  */ /* ToDo: Apply the correct effects */

    {
        5,
        GF_MISSILE,
    }, /* EAT_GOLD  */

    {
        5,
        GF_MISSILE,
    }, /* EAT_ITEM  */

    {
        5,
        GF_MISSILE,
    }, /* EAT_FOOD  */

    {
        5,
        GF_MISSILE,
    }, /* EAT_LITE  */

    {
        0,
        GF_ACID,
    }, /* ACID      */

    {
        10,
        GF_ELEC,
    }, /* ELEC      */

    {
        10,
        GF_FIRE,
    }, /* FIRE      */

    {
        10,
        GF_COLD,
    }, /* COLD      */

    {
        2,
        GF_MISSILE,
    }, /* BLIND     */

    {
        10,
        GF_CONFUSION,
    }, /* CONFUSE   */

    {
        10,
        GF_MISSILE,
    }, /* TERRIFY   */

    {
        2,
        GF_MISSILE,
    }, /* PARALYZE  */

    {
        0,
        GF_MISSILE,
    }, /* LOSE_STR  */

    {
        0,
        GF_MISSILE,
    }, /* LOSE_INT  */

    {
        0,
        GF_MISSILE,
    }, /* LOSE_WIS  */

    {
        0,
        GF_MISSILE,
    }, /* LOSE_DEX  */

    {
        0,
        GF_MISSILE,
    }, /* LOSE_CON  */

    {
        0,
        GF_MISSILE,
    }, /* LOSE_CHR  */

    {
        2,
        GF_MISSILE,
    }, /* LOSE_ALL  */

    {
        60,
        GF_ROCKET,
    }, /* SHATTER   */

    {
        5,
        GF_MISSILE,
    }, /* EXP_10    */

    {
        5,
        GF_MISSILE,
    }, /* EXP_20    */

    {
        5,
        GF_MISSILE,
    }, /* EXP_40    */

    {
        5,
        GF_MISSILE,
    }, /* EXP_80    */

    {
        5,
        GF_POIS,
    }, /* DISEASE   */

    {
        5,
        GF_TIME,
    }, /* TIME      */

    {
        5,
        GF_MISSILE,
    }, /* EXP_VAMP  */

    {
        5,
        GF_MANA,
    }, /* DR_MANA   */

    {
        60,
        GF_MISSILE,
    }, /* SUPERHURT */
    {
        5,
        GF_MISSILE,
    }, /* INERTIA */
    {
        5,
        GF_MISSILE,
    }, /* STUN */
    {
        5,
        GF_MISSILE,
    }, /* HUNGRY */
    {
        0,
        GF_NONE,
    }, /* FLAVOR */
};
