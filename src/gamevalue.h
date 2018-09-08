
/*!
* @brief ベースアイテム生成階層が加算される確率
* @details
* There is a 1/10 (10%) chance of inflating the requested object_level
* during the creation of an object (see "get_obj_num()" in "object.c").
* Lower values yield better objects more often.
*/
#define GREAT_OBJ       10

/*!
* @brief 深層モンスターが生成される(NASTY生成)の基本確率(1/x)
* @details
* There is a 1/25 (4%) chance of inflating the requested monster_level
* during the creation of a monsters (see "get_mon_num()" in "monster.c").
* Lower values yield harder monsters more often.
*/
#define NASTY_MON_BASE     25
#define NASTY_MON_MAX      3	/*!< 深層モンスターが1フロアに生成される最大数  */
#define NASTY_MON_PLUS_MAX 25	/*!< 深層モンスターの階層加算最大量 */

#define PENETRATE_INVULNERABILITY 13 /*!< 無敵化が破られる確率(1/x) / 1/x chance of hurting even if invulnerable! */

#define MAX_TELEPORT_DISTANCE 200 /*!< テレポート最大距離 */

/*
* Refueling constants
*/
#define FUEL_TORCH      5000    /*!< 松明の基本寿命値 / Maximum amount of fuel in a torch */
#define FUEL_LAMP       15000   /*!< ランタンの基本寿命値 / Maximum amount of fuel in a lantern */

/*
* More maximum values
*/
#define MAX_SIGHT       20      /*!< プレイヤーの最大視界範囲(マス) / Maximum view distance */
#define MAX_RANGE       (p_ptr->inside_battle ? 36 : 18)      /*!< プレイヤーの攻撃射程(マス) / Maximum range (spells, etc) */
#define AAF_LIMIT       100     /*!< モンスターの限界感知範囲(マス) Limit of sensing radius */

#define MIN_M_ALLOC_TD          4 /*!< 街(昼間)の最低住人配置数 / The town starts out with 4 residents during the day */
#define MIN_M_ALLOC_TN          8 /*!< 街(夜間)の最低住人配置数 / The town starts out with 8 residents during the night */

/*!
* @brief モンスター増殖の最大数
* @details
* A monster can only "multiply" (reproduce) if there are fewer than 100
* monsters on the level capable of such spontaneous reproduction.  This
* is a hack which prevents the "m_list[]" array from exploding due to
* reproducing monsters.  Messy, but necessary.
*/
#define MAX_REPRO       100


