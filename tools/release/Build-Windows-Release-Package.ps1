<#
.SYNOPSIS
    Windows 版のリリースパッケージ (zip) を作成する。

.DESCRIPTION
    Hengband.sln を Release / English-Release 構成でリビルドし、日本語版・英語版
    それぞれの配布用 zip を作成する。
    MSBuild.exe は PATH 上のものを優先し、見つからなければ vswhere でインストール
    済みの最新の Visual Studio から探索する。
    NuGet パッケージの復元は事前に済ませておくこと。

.NOTES
    このファイルは UTF-8 (BOM 付き) で保存すること。
    BOM を外すと Windows PowerShell 5.1 が CP932 として読むため、日本語のメッセージが
    文字化けして構文エラーになる。

.EXAMPLE
    .\tools\release\Build-Windows-Release-Package.ps1 -Version 3.0.0
#>
Param(
    # パッケージに付加するバージョン
    [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$Version,

    # zip の出力先ディレクトリ (既定: リポジトリルート)
    [string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# カレントディレクトリに依存しないよう、スクリプトの位置からリポジトリルートを求める
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if (-not $OutputDirectory) {
    $OutputDirectory = $repoRoot
}
# 既存ファイルを指された場合、Join-Path がファイル配下の不正なパスを作って
# Compress-Archive が分かりにくく失敗するため、ここで弾いておく
if (Test-Path -LiteralPath $OutputDirectory -PathType Leaf) {
    throw "OutputDirectory にファイルが指定されています。ディレクトリを指定してください。($OutputDirectory)"
}
if (-not (Test-Path -LiteralPath $OutputDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
}
# Push-Location の前に、呼び出し元のカレントディレクトリ基準で絶対パス化しておく
$outputDirFull = (Resolve-Path $OutputDirectory).Path

function Find-MSBuild {
    # CI では setup-msbuild が PATH を通しているので、それを優先する
    $msbuild = Get-Command MSBuild.exe -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty Source
    if ($msbuild) {
        return $msbuild
    }

    # PATH にない場合はインストール済みの最新の Visual Studio から探す
    # ソリューションは C++ プロジェクトなので、C++ ツールセットの有無も条件に含める
    # (-requires は指定したコンポーネントをすべて備えるインスタンスのみを返す)
    $vswhere = Join-Path ${Env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $msbuild = & $vswhere -latest -prerelease -products * `
            -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if ($msbuild) {
            return $msbuild
        }
    }

    throw 'MSBuild.exe が見つかりませんでした。Visual Studio (C++ によるデスクトップ開発) がインストールされているか確認してください。'
}

function BuildPackage ($msbuild, $package_name, $package_unique_files, $build_conf) {
    # バイナリをリビルド
    # 構成によらず出力先が同じ (リポジトリルート) なので、構成を切り替えるたびにリビルドする
    & $msbuild .\VisualStudio\Hengband.sln /t:Rebuild "/p:Configuration=$build_conf" /m
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild の実行に失敗しました。(Configuration=$build_conf, ExitCode=$LASTEXITCODE)"
    }

    # 作業用テンポラリフォルダ (失敗時も確実に消すため try/finally で囲む)
    $tempDir = New-Item -ItemType Directory -Path (Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString()))
    try {
        $hengbandDir = New-Item -ItemType Directory -Path (Join-Path $tempDir $package_name)

        # 必要なファイルをコピーして、その中で不要になりえるものを削除
        Copy-Item -Verbose -Path .\Hengband.exe, .\Hengband.pdb, .\readme_angband, .\THIRD-PARTY-NOTICES.txt -Destination $hengbandDir
        Copy-Item -Verbose -Path $package_unique_files -Destination $hengbandDir
        Copy-Item -Verbose -Recurse -Path .\lib -Destination $hengbandDir -Exclude Makefile.am, *.raw, .gitattributes
        Copy-Item -Verbose -Path .\lib\apex\h_scores.raw -Destination (Join-Path $hengbandDir 'lib\apex')
        Remove-Item -Verbose -Exclude delete.me -Recurse -Path (Join-Path $hengbandDir 'lib\save\*'), (Join-Path $hengbandDir 'lib\user\*')
        Remove-Item -Verbose -Exclude music.cfg, readme.txt, *.mp3 -Path (Join-Path $hengbandDir 'lib\xtra\music\*')

        # zipアーカイブ作成
        $package_path = Join-Path $outputDirFull "${package_name}.zip"
        Get-ChildItem -Path $tempDir | Compress-Archive -Force -Verbose -DestinationPath $package_path
    }
    finally {
        Remove-Item -Recurse -Force -Path $tempDir
    }
}

# 以降の相対パスがリポジトリルート基準になるようカレントディレクトリを移す
Push-Location $repoRoot
try {
    $msbuild = Find-MSBuild

    # 日本語版
    BuildPackage -msbuild $msbuild -package_name Hengband-$Version-jp -package_unique_files .\readme.md, .\autopick.txt -build_conf Release
    # 英語版
    BuildPackage -msbuild $msbuild -package_name Hengband-$Version-en -package_unique_files .\readme-eng.md, .\autopick_eng.txt -build_conf English-Release
}
finally {
    Pop-Location
}
