#include "monster-attack/monster-attack-types.h"
#include "monster-attack/monster-attack-effect.h"
#include "effect/attribute-types.h"

/*!
 * @brief モンスターの打撃効力テーブル /
 * The table of monsters' blow effects
 */
const mbe_info_type mbe_info[static_cast<int>(RaceBlowEffectType::MAX)] = {
    {
        0,
        AttributeType::NONE,
    }, /* None      */

    {
        60,
        AttributeType::MISSILE,
    }, /* HURT      */

    {
        5,
        AttributeType::POIS,
    }, /* POISON    */

    {
        20,
        AttributeType::DISENCHANT,
    }, /* UN_BONUS  */

    {
        15,
        AttributeType::MISSILE,
    },
    /* UN_POWER  */ /* ToDo: Apply the correct effects */

    {
        5,
        AttributeType::MISSILE,
    }, /* EAT_GOLD  */

    {
        5,
        AttributeType::MISSILE,
    }, /* EAT_ITEM  */

    {
        5,
        AttributeType::MISSILE,
    }, /* EAT_FOOD  */

    {
        5,
        AttributeType::MISSILE,
    }, /* EAT_LITE  */

    {
        0,
        AttributeType::ACID,
    }, /* ACID      */

    {
        10,
        AttributeType::ELEC,
    }, /* ELEC      */

    {
        10,
        AttributeType::FIRE,
    }, /* FIRE      */

    {
        10,
        AttributeType::COLD,
    }, /* COLD      */

    {
        2,
        AttributeType::MISSILE,
    }, /* BLIND     */

    {
        10,
        AttributeType::CONFUSION,
    }, /* CONFUSE   */

    {
        10,
        AttributeType::MISSILE,
    }, /* TERRIFY   */

    {
        2,
        AttributeType::MISSILE,
    }, /* PARALYZE  */

    {
        0,
        AttributeType::MISSILE,
    }, /* LOSE_STR  */

    {
        0,
        AttributeType::MISSILE,
    }, /* LOSE_INT  */

    {
        0,
        AttributeType::MISSILE,
    }, /* LOSE_WIS  */

    {
        0,
        AttributeType::MISSILE,
    }, /* LOSE_DEX  */

    {
        0,
        AttributeType::MISSILE,
    }, /* LOSE_CON  */

    {
        0,
        AttributeType::MISSILE,
    }, /* LOSE_CHR  */

    {
        2,
        AttributeType::MISSILE,
    }, /* LOSE_ALL  */

    {
        60,
        AttributeType::ROCKET,
    }, /* SHATTER   */

    {
        5,
        AttributeType::MISSILE,
    }, /* EXP_10    */

    {
        5,
        AttributeType::MISSILE,
    }, /* EXP_20    */

    {
        5,
        AttributeType::MISSILE,
    }, /* EXP_40    */

    {
        5,
        AttributeType::MISSILE,
    }, /* EXP_80    */

    {
        5,
        AttributeType::POIS,
    }, /* DISEASE   */

    {
        5,
        AttributeType::TIME,
    }, /* TIME      */

    {
        5,
        AttributeType::MISSILE,
    }, /* EXP_VAMP  */

    {
        5,
        AttributeType::MANA,
    }, /* DR_MANA   */

    {
        60,
        AttributeType::MISSILE,
    }, /* SUPERHURT */
    {
        5,
        AttributeType::MISSILE,
    }, /* INERTIA */
    {
        5,
        AttributeType::MISSILE,
    }, /* STUN */
    {
        5,
        AttributeType::MISSILE,
    }, /* HUNGRY */
    {
        0,
        AttributeType::NONE,
    }, /* FLAVOR */
};
