#!/bin/sh
# 日本語版の AppImage に同梱する Takao ゴシック (TakaoGothic.ttf) を、公式の配布物から取り出す。
#
# 使い方: fetch-takao-fonts.sh <出力先ディレクトリ>
#
# 出力先に TakaoGothic.ttf、IPA_Font_License_Agreement_v1.0.txt (使用許諾契約書)、README (派生の経緯) を置く。
# Takao フォントは IPA フォントライセンス v1.0 で配布されている。再配布では名前を変えず、改変せず、
# 契約書の写しを添付すること (契約書の第 3 条 2 項)。
# 版とハッシュは io.github.hengband.Hengband.yml の takao-fonts モジュールと揃える。
set -eu

if [ $# -ne 1 ]; then
    echo "usage: $0 <output-dir>" >&2
    exit 2
fi

url=https://launchpad.net/takao-fonts/trunk/15.03/+download/TakaoFonts_00303.01.tar.xz
sha256=e9871f72ac69acb3e277aebbee7ca01fbebf54800733e29fafdc46133fc3552f
topdir=TakaoFonts_00303.01

outdir=$1
mkdir -p "$outdir"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

curl -fsSL -o "$work/takao.tar.xz" "$url"
echo "$sha256  $work/takao.tar.xz" | sha256sum -c - > /dev/null
tar -C "$work" -xJf "$work/takao.tar.xz" \
    "$topdir/TakaoGothic.ttf" "$topdir/IPA_Font_License_Agreement_v1.0.txt" "$topdir/README"
for f in TakaoGothic.ttf IPA_Font_License_Agreement_v1.0.txt README; do
    install -m644 "$work/$topdir/$f" "$outdir/$f"
done
