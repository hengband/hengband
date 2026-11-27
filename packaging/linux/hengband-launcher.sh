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

export ANGBAND_PATH="$libdir/"
exec "$binary" \
    "-ds=$vardir/save" \
    "-db=$vardir/bone" \
    "-dd=$vardir/data" \
    "-da=$vardir/apex" \
    "$@"
