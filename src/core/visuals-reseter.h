#pragma once

class PlayerType;
void reset_visuals(PlayerType *player_ptr);

/*!
 * @brief スコープの間だけグラフィックを外し、文字で描き直す
 * @details 記念撮影やスクリーンダンプのように、画面を文字として読み取る間に使う。
 * グラフィックを使っていなければ何もしない。例外で抜けるときは戻さない (戻す処理の例外で終了しないように)。
 */
class GraphicsSuspender {
public:
    explicit GraphicsSuspender(PlayerType *player_ptr);
    ~GraphicsSuspender() noexcept(false);
    GraphicsSuspender(const GraphicsSuspender &) = delete;
    GraphicsSuspender(GraphicsSuspender &&) = delete;
    GraphicsSuspender &operator=(const GraphicsSuspender &) = delete;
    GraphicsSuspender &operator=(GraphicsSuspender &&) = delete;

private:
    PlayerType *player_ptr;
    bool is_suspended;
    int uncaught_exceptions_on_suspend; //!< 外したときに投げられている途中の例外の数
};
