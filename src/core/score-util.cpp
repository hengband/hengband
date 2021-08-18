#include "core/score-util.h"
#include "util/angband-files.h"

/*
 * The "highscore" file descriptor, if available.
 */
int highscore_fd = -1;

/*!
 * @brief i番目のスコア情報にバッファ位置をシークする / Seek score 'i' in the highscore file
 * @param i スコア情報ID
 * @return 問題がなければ0を返す
 */
int highscore_seek(int i)
{
    return (fd_seek(highscore_fd, (ulong)(i) * sizeof(high_score)));
}

/*!
 * @brief 所定ポインタからスコア情報を読み取る / Read one score from the highscore file
 * @param score スコア情報参照ポインタ
 * @return エラーコード
 */
errr highscore_read(high_score *score)
{
    return (fd_read(highscore_fd, (char *)(score), sizeof(high_score)));
}
