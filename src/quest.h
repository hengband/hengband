/*
 * Quest constants
 */
#define MIN_RANDOM_QUEST 40 /*<! ランダムクエストを割り当てるクエストIDの開始値 */
#define MAX_RANDOM_QUEST 49 /*<! ランダムクエストを割り当てるクエストIDの終了値 */

 /*!
  * @brief 該当IDが固定クエストかどうかを判定する / Check is the quest index is "fixed"
  * @param Q_IDX クエストID
  * @return 固定クエストならばTRUEを返す
  */
#define is_fixed_quest_idx(Q_IDX) (((Q_IDX) < MIN_RANDOM_QUEST) || ((Q_IDX) > MAX_RANDOM_QUEST))

#define QUEST_TOWER1 5 /*<! 塔クエスト(第1階層)に割り振るクエストID */
#define QUEST_TOWER2 6 /*<! 塔クエスト(第2階層)に割り振るクエストID */
#define QUEST_TOWER3 7 /*<! 塔クエスト(第3階層)に割り振るクエストID */
#define QUEST_OBERON 8 /*<! オベロン打倒クエストに割り振るクエストID */
#define QUEST_SERPENT 9 /*<! サーペント打倒クエストに割り振るクエストID */


extern void determine_random_questor(quest_type *q_ptr);
extern void complete_quest(QUEST_IDX quest_num);
extern void check_quest_completion(monster_type *m_ptr);
extern void check_find_art_quest_completion(object_type *o_ptr);
void quest_discovery(QUEST_IDX q_idx);
extern QUEST_IDX quest_number(DEPTH level);
extern QUEST_IDX random_quest_number(DEPTH level);
