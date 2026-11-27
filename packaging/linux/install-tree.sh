#!/bin/sh
# ビルド済みの src/hengband とゲームデータを、AppImage・Flatpak 用の配置で <prefix> の下に置く。
#
# 使い方: install-tree.sh <prefix> <app-id>
#   <prefix>  AppImage では AppDir/usr、Flatpak では /app
#   <app-id>  io.github.hengband.Hengband (日本語版) または io.github.hengband.HengbandEn (英語版)
#
# リポジトリのトップで、configure と make を済ませてから実行する。
set -eu

if [ $# -ne 2 ]; then
    echo "usage: $0 <prefix> <app-id>" >&2
    exit 2
fi

prefix=$1
app_id=$2
pkgdir=$(dirname "$0")

# variant はデータディレクトリの名前。ほかはデスクトップエントリと AppStream のメタデータに入れる名前
case "$app_id" in
io.github.hengband.Hengband)
    variant=hengband
    name=Hengband name_ja=変愚蛮怒 lang=Japanese lang_ja=日本語
    ;;
io.github.hengband.HengbandEn)
    variant=hengband-en
    name='Hengband (English)' name_ja='変愚蛮怒 (英語版)' lang=English lang_ja=英語
    ;;
*)
    echo "$0: unknown app-id: $app_id" >&2
    exit 2
    ;;
esac

if [ ! -x src/hengband ]; then
    echo "$0: src/hengband is not built. Run this at the top of the repository after make." >&2
    exit 1
fi

install -Dm755 src/hengband "$prefix/libexec/hengband/hengband"
install -Dm755 "$pkgdir/hengband-launcher.sh" "$prefix/bin/hengband"

datadir="$prefix/share/hengband"
mkdir -p "$datadir"
printf '%s\n' "$variant" > "$datadir/variant"

# 読み取るだけのデータを置く。書き込むディレクトリ (save・bone・data・user) は起動スクリプトが利用者の場所に向けるので、空で作る
rm -rf "$datadir/lib"
for d in apex edit file help pref; do
    mkdir -p "$datadir/lib/$d"
    (cd "lib/$d" && find . -type f ! -name 'Makefile*' ! -name 'delete.me' ! -name 'scores.*' -print) | while IFS= read -r f; do
        install -Dm644 "lib/$d/$f" "$datadir/lib/$d/$f"
    done
done
for d in bone data info save script user; do
    mkdir -p "$datadir/lib/$d"
done

# タイル (サブモジュール lib/xtra) はあれば入れる。BGM・効果音は外部のプレイヤーに頼るため入れない
mkdir -p "$datadir/lib/xtra"
if [ -f lib/xtra/graf/8x8.bmp ]; then
    for f in 8x8.bmp original32.png original32.README.txt original32.LICENSE.txt; do
        install -Dm644 "lib/xtra/graf/$f" "$datadir/lib/xtra/graf/$f"
    done
fi

# 版は configure.ac から取り、リリースの日付はビルドした日 (SOURCE_DATE_EPOCH があればその日) にする
version=$(sed -n 's/^AC_INIT(hengband, *\([^)]*\))/\1/p' configure.ac)
case "$version" in
*-Alpha* | *-Beta*) release_type=development ;;
*) release_type=stable ;;
esac
if [ -n "${SOURCE_DATE_EPOCH:-}" ]; then
    release_date=$(date -u -d "@$SOURCE_DATE_EPOCH" +%Y-%m-%d)
else
    release_date=$(date -u +%Y-%m-%d)
fi
fill_template() {
    sed -e "s|@APP_ID@|$app_id|g" -e "s|@NAME@|$name|g" -e "s|@NAME_JA@|$name_ja|g" \
        -e "s|@LANG@|$lang|g" -e "s|@LANG_JA@|$lang_ja|g" -e "s|@VERSION@|$version|g" \
        -e "s|@DATE@|$release_date|g" -e "s|@RELEASE_TYPE@|$release_type|g" "$1"
}
mkdir -p "$prefix/share/applications" "$prefix/share/metainfo"
fill_template "$pkgdir/hengband.desktop.in" > "$prefix/share/applications/$app_id.desktop"
fill_template "$pkgdir/hengband.metainfo.xml.in" > "$prefix/share/metainfo/$app_id.metainfo.xml"
install -Dm644 "$pkgdir/hengband.png" "$prefix/share/icons/hicolor/256x256/apps/$app_id.png"

for f in readme.md readme-eng.md readme_angband THIRD-PARTY-NOTICES.txt lib/help/jlicense.txt; do
    install -Dm644 "$f" "$prefix/share/doc/hengband/${f##*/}"
done
