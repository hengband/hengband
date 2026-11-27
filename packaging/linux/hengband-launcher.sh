#!/bin/sh
# AppImage・Flatpak 用の起動スクリプト。
#
# パッケージの中の lib は読み取り専用なので、ゲームが書き込むディレクトリ (save・bone・data・apex) だけを
# 利用者のデータディレクトリ ($XDG_DATA_HOME/<HENGBAND_VARIANT>) に向けて本体を起動する。
# 設定やダンプを置くユーザーディレクトリは、通常のインストールと同じく ~/.angband/Hengband を使う。
#
# 配置は次を前提とする (<prefix> は AppImage では AppDir/usr、Flatpak では /app)。
#   <prefix>/bin/hengband                  このスクリプト (AppImage では AppDir/AppRun)
#   <prefix>/libexec/hengband/hengband     本体
#   <prefix>/share/hengband/lib            ゲームデータ
#   <prefix>/share/hengband/variant        データディレクトリの名前 (hengband または hengband-en)
set -eu

here=$(dirname "$(readlink -f "$0")")
if [ -d "$here/usr/share/hengband" ]; then
    # AppImage の AppRun として置いたとき
    prefix="$here/usr"
else
    prefix=$(dirname "$here")
fi
binary="$prefix/libexec/hengband/hengband"
libdir="$prefix/share/hengband/lib"

if [ -z "${HENGBAND_VARIANT:-}" ]; then
    HENGBAND_VARIANT=$(cat "$prefix/share/hengband/variant" 2>/dev/null || echo hengband)
fi

if [ -z "${HOME:-}" ]; then
    echo "hengband: HOME is not set." >&2
    exit 1
fi

vardir="${HENGBAND_VAR_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/$HENGBAND_VARIANT}"
for d in save bone data apex; do
    mkdir -p "$vardir/$d"
done

# スコアの初期ファイルを、まだ無ければ置く
for f in "$libdir"/apex/*; do
    [ -f "$f" ] || continue
    [ -e "$vardir/apex/${f##*/}" ] || cp "$f" "$vardir/apex/"
done

# AppImage に同梱したフォントは、ホストの fontconfig からは見えない。ホストの設定に同梱のフォントの
# ディレクトリを足した設定ファイル (build-appimage.sh が置く) を使う。Flatpak では /app/share/fonts が最初から見える
if [ -z "${FONTCONFIG_FILE:-}" ] && [ -f "$prefix/share/hengband/fonts.conf" ]; then
    export FONTCONFIG_FILE="$prefix/share/hengband/fonts.conf"
fi

# 日本語版に Takao ゴシックを同梱したときは、メインウィンドウの既定のフォントにする。
# 既定の monospace は環境によって行の高い日本語フォント (Noto Sans Mono CJK JP など) に解決され、
# ウィンドウが画面に収まらないことがあるため。大きさは本体の既定 (24) のまま。利用者の指定があればそちらを使う。
# フォント名は Xft の書式なので、TrueType フォントで描くときだけ設定する。描き方は "--" の後の -ft・-fb のうち
# 最後のもので決まり、どちらも無ければ TrueType フォント (パッケージは Xft を有効にしてビルドする)
use_xft=yes
after_dashdash=no
for arg in "$@"; do
    if [ "$after_dashdash" = no ]; then
        [ "$arg" = -- ] && after_dashdash=yes
        continue
    fi
    case "$arg" in
    -ft) use_xft=yes ;;
    -fb) use_xft=no ;;
    esac
done
if [ "$use_xft" = yes ] && [ -z "${ANGBAND_X11_FONT_0:-}" ] && [ -z "${ANGBAND_X11_FONT:-}" ] && [ -f "$prefix/share/fonts/takao/TakaoGothic.ttf" ]; then
    export ANGBAND_X11_FONT_0="TakaoGothic-24:lang=ja:spacing=90"
fi

export ANGBAND_PATH="$libdir/"
exec "$binary" \
    "-ds=$vardir/save" \
    "-db=$vardir/bone" \
    "-dd=$vardir/data" \
    "-da=$vardir/apex" \
    "$@"
