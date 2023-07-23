#pragma once

/*!
 * @file angband.h
 * @brief Angband(変愚蛮怒)メインヘッダファイル
 * Main "Angband" header file
 * @date 2014/08/08
 * @author
 * Copyright (c) 1989 James E. Wilson
 * @details *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 * This file has been modified for use with "Angband 2.8.2"
 */

/*
 * Then, include the header files for the low-level code
 */
#include "term/z-form.h"
#include "term/z-rand.h"
#include "term/z-term.h"
#include "term/z-util.h"
#include "term/z-virt.h"

#include "locale/language-switcher.h"

/*
 * Include the "Angband" configuration header
 */
#include "system/gamevalue.h"

/***** Some copyright messages follow below *****/

/*!
 *
 * @mainpage 変愚蛮怒仕様書
 *
 * @par
 * 変愚蛮怒の仕様をangband.hをソースにDoxygenで読めるようまとめておきます。<br>
 *
 * @par
 * - @ref page_basic "基本方針"
 * - @ref page_license "Angbandソース内ライセンス記述"
 *
 *
 *
 *
 */

/*!
 * @page page_basic リファクタリング基本方針
 *
 * 現在まとめ中。
 *
 * @ref index "メインページへ"
 */

/*!
 * @page page_license Angbandソース内ライセンス記述
 *
 * 以下、angband.hに当初から在った記述となります。
 * <pre>
 * Note that these copyright messages apply to an ancient version
 * of Angband, as in, from pre-2.4.frog-knows days, and thus the
 * reference to "5.0" is rather misleading...
 *
 * UNIX ANGBAND Version 5.0
 *
 * Original copyright message follows.
 *
 * ANGBAND Version 4.8	COPYRIGHT (c) Robert Alan Koeneke
 *
 *	 I lovingly dedicate this game to hackers and adventurers
 *	 everywhere...
 *
 *	 Designer and Programmer:
 *		Robert Alan Koeneke
 *		University of Oklahoma
 *
 *	 Assistant Programmer:
 *		Jimmey Wayne Todd
 *		University of Oklahoma
 *
 *	 Assistant Programmer:
 *		Gary D. McAdoo
 *		University of Oklahoma
 *
 *	 UNIX Port:
 *		James E. Wilson
 *		UC Berkeley
 *		wilson@ernie.Berkeley.EDU
 *		ucbvax!ucbernie!wilson
 *
 *	 ANGBAND may be copied and modified freely as long as the above
 *	 credits are retained.	No one who-so-ever may sell or market
 *	 this software in any form without the expressed written consent
 *	 of the author Robert Alan Koeneke.
 * </pre>
 *
 * @ref index "メインページへ"
 */

/*!
 * @dir action プレイヤーの行動実行処理(アイテム発動/移動/変異能力/開閉/レイシャル/走る/投擲/トラベル機能/掘削/装備持ち替え)
 * @dir artifact 固定アーティファクト/ランダムアーティファクトの生成処理
 * @dir autopick 自動拾い処理
 * @dir birth プレイヤーキャラクターメイク
 * @dir blue-magic 青魔法処理全般
 * @dir cmd-action プレイヤーの行動コマンド処理(攻撃/剣術/ものまね/超能力等職業固有能力/移動/開閉/ペット命令/レイシャル/射撃/トラベル/掘削/その他)
 * @dir cmd-building 施設の各サービス処理
 * @dir cmd-io 入出力コマンド処理(自動拾い設定/記録設定/ステータスダンプ/フロア地点指定/ゲームオプション/ヘルプ/各情報/思い出/マクロ/メニューウィンドウ/スクリーンダンプ/セーブ/階の雰囲気)
 * @dir cmd-item アイテムコマンド処理(壊す/食べる/装備する/魔道具吸収/飲む/読む/光源燃料補充/鍛冶/投擲/杖、魔法棒、ロッドの使用)
 * @dir cmd-visual 描画/表示に関するコマンド(再描画/全体マップ表示/シンボル変更)
 * @dir combat 白兵/射撃戦闘(命中処理/クリティカル処理/威力計算/オーラ反撃処理/剣術定義/幻覚時メッセージ/マーシャルアーツ/射撃武器処理/スレイ効果)
 * @dir core ゲーム進行処理部（TODO:各ファイル概要は後で追記）
 * @dir dungeon ダンジョンの進行補助処理(ダンジョン内のクエストなどが主)
 * @dir effect 各属性効果の判定や効力発揮
 * @dir flavor アイテムの特性やフレーバーテキスト表現
 * @dir floor フロア生成/管理処理
 * @dir game-option 詐欺モードも含めたゲームオプション
 * @dir grid グリッド(各マス)に関する処理
 * @dir info-reader *_info.txtの読み込み処理
 * @dir inventory 所持品に関する処理(フロアからの取得/呪いの品/攻撃による損壊/所持状態の記述/所持数表記/アイテムスロット/ザック溢れ/選択処理/自然充填)
 * @dir store 店舗処理
 */
