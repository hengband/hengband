#!/bin/sh
# ビルド済みの src/hengband から AppImage を作る。
#
# 使い方: build-appimage.sh <app-id> [出力先ディレクトリ]
#   <app-id>  io.github.hengband.Hengband (日本語版) または io.github.hengband.HengbandEn (英語版)
#
# リポジトリのトップで、configure と make を済ませてから実行する。
# 共有ライブラリの同梱には linuxdeploy と linuxdeploy-plugin-appimage を、RUNPATH の設定には patchelf を使う。
# PATH に無ければ、環境変数 LINUXDEPLOY に linuxdeploy (AppImage 版) のパスを渡す。
set -eu

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "usage: $0 <app-id> [output-dir]" >&2
    exit 2
fi

app_id=$1
outdir=$(mkdir -p "${2:-.}" && cd "${2:-.}" && pwd)
pkgdir=$(cd "$(dirname "$0")" && pwd)
linuxdeploy=${LINUXDEPLOY:-linuxdeploy}
# 後でディレクトリを移るので、スラッシュを含む相対パスは絶対パスにする (スラッシュの無いコマンド名は PATH から探す)
case "$linuxdeploy" in
/*) ;;
*/*) linuxdeploy="$(cd "$(dirname "$linuxdeploy")" && pwd)/$(basename "$linuxdeploy")" ;;
esac

case "$app_id" in
io.github.hengband.Hengband) name=Hengband ;;
io.github.hengband.HengbandEn) name=Hengband-en ;;
*)
    echo "$0: unknown app-id: $app_id" >&2
    exit 2
    ;;
esac

version=$(sed -n 's/^AC_INIT(hengband, *\([^)]*\))/\1/p' configure.ac)
arch=$(uname -m)
appdir="build-appimage/$app_id.AppDir"

rm -rf "$appdir"
# 日本語版には Takao ゴシックを同梱する。HENGBAND_FONT_DIR が無ければ公式の配布物から取り出す
if [ "$app_id" = io.github.hengband.Hengband ] && [ -z "${HENGBAND_FONT_DIR:-}" ]; then
    sh "$pkgdir/fetch-takao-fonts.sh" build-appimage/takao-fonts
    HENGBAND_FONT_DIR=build-appimage/takao-fonts
    export HENGBAND_FONT_DIR
fi
sh "$pkgdir/install-tree.sh" "$appdir/usr" "$app_id"
if [ -d "$appdir/usr/share/fonts" ]; then
    install -Dm644 "$pkgdir/fonts.conf" "$appdir/usr/share/hengband/fonts.conf"
fi
# linuxdeploy は usr/bin のファイルをすべて ELF として扱い、シェルスクリプトがあると失敗する。
# AppImage では起動スクリプトを AppRun として置き、usr/bin は使わない
rm -rf "$appdir/usr/bin"
# 本体は usr/libexec/hengband に置くので、linuxdeploy が usr/lib に同梱したライブラリを探せるよう RUNPATH を設定する
# (linuxdeploy は usr/bin の外の実行ファイルには設定しない)
patchelf --set-rpath '$ORIGIN/../../lib' "$appdir/usr/libexec/hengband/hengband"

# linuxdeploy は --icon-file のファイル名を .desktop の Icon= と照らし合わせる
icon="build-appimage/$app_id.png"
cp "$pkgdir/hengband.png" "$icon"

cd "$(dirname "$appdir")"
appdir_name=$(basename "$appdir")
ARCH=$arch OUTPUT="$outdir/$name-$version-$arch.AppImage" "$linuxdeploy" \
    --appdir "$appdir_name" \
    --executable "$appdir_name/usr/libexec/hengband/hengband" \
    --desktop-file "$appdir_name/usr/share/applications/$app_id.desktop" \
    --icon-file "$app_id.png" \
    --custom-apprun "$pkgdir/hengband-launcher.sh" \
    --output appimage
