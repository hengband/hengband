# AppImage・Flatpak のパッケージ

Linux 向けに、X11 版の変愚蛮怒を AppImage と Flatpak にまとめるための定義を置く。
日本語版と英語版は別のパッケージにする。

| 版     | アプリ ID                      | データディレクトリの名前 |
| ------ | ------------------------------ | ------------------------ |
| 日本語 | `io.github.hengband.Hengband`   | `hengband`               |
| 英語   | `io.github.hengband.HengbandEn` | `hengband-en`            |

GitHub Actions の `.github/workflows/build-linux-packages.yml` が両方を作る。成果物 (Artifacts) のストレージには上限があるので、
PR ではビルドとチェックだけを行い、パッケージを成果物に置かない。成果物に置くのは、リリース (`create-release.yml` が
`upload: true` で呼ぶ) と手動の実行 (`upload` の既定は有効) のときだけで、1 日で消える。

## ファイル

| ファイル                             | 役割                                                                   |
| ------------------------------------ | ---------------------------------------------------------------------- |
| `install-tree.sh`                    | ビルドした `src/hengband` とゲームデータを、パッケージ用の配置で置く   |
| `hengband-launcher.sh`               | 起動スクリプト。書き込むディレクトリを利用者の場所に向けて本体を起動する |
| `build-appimage.sh`                  | AppImage を作る (起動スクリプトを AppRun として置く)                   |
| `io.github.hengband.*.yml`           | Flatpak のマニフェスト (版ごとの違いだけを書く)                        |
| `hengband-module.yml`                | Flatpak のマニフェストで共通のモジュール (ビルドの手順)                |
| `hengband.desktop.in`                | デスクトップエントリのひな形 (`install-tree.sh` が版ごとに埋める)      |
| `hengband.metainfo.xml.in`           | AppStream のメタデータのひな形 (版は `configure.ac` から入れる)        |
| `hengband.png`                       | アイコン (`src/angband.ico` の 256x256 のもの)                         |

## 保存先

パッケージの中の `lib` は読み取り専用なので、ゲームが書き込むディレクトリは起動スクリプトが次に向ける。

- セーブ・スコア・骨ファイルなど (`lib/save`・`lib/apex`・`lib/bone`・`lib/data`):
  `$XDG_DATA_HOME/<データディレクトリの名前>` (既定は `~/.local/share/hengband` など)
  - Flatpak では `~/.var/app/<アプリ ID>/data/<データディレクトリの名前>`
  - 環境変数 `HENGBAND_VAR_DIR` で変えられる
- 設定やダンプ (ユーザーディレクトリ): 通常のインストールと同じく `~/.angband/Hengband`
  - Flatpak では `~/.var/app/<アプリ ID>/.angband/Hengband`

BGM と効果音は外部のプレイヤーに頼るため入れていない。タイルはサブモジュール `lib/xtra` があれば入れる。

フォントはどちらもホストのものを使う。日本語版の表示には、ホストに日本語のフォント (Noto Sans CJK など) が要る。

## フォントとウィンドウの大きさを変える

X11 版の既定のフォントは、メインウィンドウが `monospace-24:lang=ja:spacing=90`、サブウィンドウが
`sans-serif-16:lang=ja` (英語版は `lang=en`) である。1 行の高さはフォントが申告する行の高さで決まるので、
同じ 24pt でも、どのフォントに解決されるかでウィンドウの大きさが変わる。たとえば 96 DPI では、
Noto Sans Mono CJK JP に解決されるとメインウィンドウの高さが約 1100px になり、1920x1080 の画面に収まらない
(BIZ UDGothic なら約 770px)。どのフォントに解決されるかは次で確かめられる。

```bash
fc-match "monospace-24:lang=ja:spacing=90"
```

フォントは、パッケージでも通常のビルドと同じく環境変数で変えられる (起動スクリプトはそのまま本体に渡す)。

- `ANGBAND_X11_FONT_0`: メインウィンドウのフォント
- `ANGBAND_X11_FONT_<1〜7>`: 各サブウィンドウのフォント
- `ANGBAND_X11_FONT`: 番号を付けて指定していないウィンドウすべてのフォント

値は Xft のフォント名 (`ファミリ名-大きさ:属性`) で書く。ほかの環境変数は `readme-eng.md` の「Configuring display」を参照。

AppImage では、起動するときに環境変数を付ける。

```bash
ANGBAND_X11_FONT_0="monospace-18:lang=ja:spacing=90" ./Hengband-<版>-x86_64.AppImage
```

デスクトップのアイコンから起動するときにも効かせるには、`~/.config/environment.d/hengband.conf` などに
`ANGBAND_X11_FONT_0=monospace-18:lang=ja:spacing=90` と書き、ログインし直す (systemd を使うデスクトップの場合)。

Flatpak では、`flatpak override` で設定しておくと、アイコンから起動したときにも効く。

```bash
flatpak override --user --env=ANGBAND_X11_FONT_0="monospace-18:lang=ja:spacing=90" io.github.hengband.Hengband
# 元に戻す
flatpak override --user --reset io.github.hengband.Hengband
```

一度だけ試すなら `flatpak run --env=ANGBAND_X11_FONT_0=... io.github.hengband.Hengband` でもよい。

## Flatpak を手元で作る

`org.freedesktop.Sdk//26.08` (GCC 16 を含む) を使う。

```bash
flatpak install --user flathub org.freedesktop.Platform//26.08 org.freedesktop.Sdk//26.08
git submodule update --init lib/xtra   # タイルを入れる場合
flatpak-builder --user --force-clean --install build-flatpak/build-ja packaging/linux/io.github.hengband.Hengband.yml
flatpak run io.github.hengband.Hengband
```

マニフェストは作業ツリーをそのまま使う。作業ツリーで `configure` を実行していると止まるので、
先に `make distclean` するか、別の作業ツリー (`git worktree add` など) で作る。

## AppImage を手元で作る

[linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) と
[linuxdeploy-plugin-appimage](https://github.com/linuxdeploy/linuxdeploy-plugin-appimage)、patchelf が要る。
PATH に置くか、環境変数 `LINUXDEPLOY` に linuxdeploy のパスを渡す。

ビルドした環境より glibc の古いディストリビューションでは動かない。配布用には CI と同じく Ubuntu 24.04 で作る。
GCC 16 の libstdc++ は利用者の環境に無いことが多いので、静的にリンクする。

```bash
./bootstrap
CC=gcc-16 CXX=g++-16 ./configure --enable-xft CXXFLAGS="-O2" LDFLAGS="-static-libstdc++ -static-libgcc"   # 英語版は --disable-japanese を足す
make -j$(getconf _NPROCESSORS_ONLN)
sh packaging/linux/build-appimage.sh io.github.hengband.Hengband dist
```
