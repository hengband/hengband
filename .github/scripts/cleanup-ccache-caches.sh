#!/bin/bash

# Actions のキャッシュのうち、ccache のキャッシュの不要なものを削除する。
# - ブランチ・PR とビルド構成の組ごとに、最新の 1 件だけを残す
#   (読み込みでは最新のものしか使わない)
# - 閉じた PR と、削除されたブランチ、ブランチでも PR でもない ref (タグなど) のキャッシュはすべて削除する
# - PR のキャッシュは、同じ構成の PR のベースのブランチのキャッシュより古ければ削除する。
#   PR では自分のキャッシュがベースのブランチのものより優先して読まれるため、残すと
#   ベースのブランチの新しいキャッシュが使われなくなる
# 対象はキーが ccache- で始まるものだけで、ほかのキャッシュには触れない。
#
# 使い方: cleanup-ccache-caches.sh [--dry-run]
#   --dry-run を付けると、削除せずに削除する対象を表示するだけにする。
#   対象のリポジトリは gh の規則で決まる (GH_REPO 環境変数、またはカレントディレクトリのリポジトリ)。

set -euo pipefail

# 引数を書き間違えたときに本当に削除してしまわないよう、知らない引数ではエラーにする
dry_run=false
case "${1:-}" in
    --dry-run) dry_run=true ;;
    "") ;;
    *)
        echo "Usage: $0 [--dry-run]" >&2
        exit 2
        ;;
esac

declare -A kept=()
declare -A pr_bases=()
declare -A branches=()

delete_cache() {
    local id=$1 key=$2 ref=$3 reason=$4
    echo "delete: $ref $key ($reason)"
    if ! $dry_run; then
        # 一覧を取ってから削除するまでに GitHub が自分で削除していることもあるので、失敗しても続ける
        if ! gh cache delete "$id" </dev/null; then
            echo "::warning::Failed to delete cache $key ($ref)"
        fi
    fi
}

# 取得に失敗したときに何も消さずにエラーで止まるよう、ループに渡す前に変数に受ける。
# 取得の合間に作られた PR・ブランチのキャッシュを消してしまわないよう、キャッシュを先に取る
caches=$(gh cache list --key ccache- --limit 1000 --sort created_at --order desc --json id,key,ref --jq '.[] | [.id, .key, .ref] | @tsv')
max_open_prs=1000
open_prs=$(gh pr list --state open --limit $max_open_prs --json number,baseRefName --jq '.[] | [.number, .baseRefName] | @tsv')
branch_names=$(gh api 'repos/{owner}/{repo}/branches?per_page=100' --paginate --jq '.[].name')

# 開いている PR を取りこぼすと、その PR のキャッシュを閉じた PR のものとして消してしまうので止める
if [ "$(grep -c . <<<"$open_prs")" -ge $max_open_prs ]; then
    echo "Too many open PRs to list them all" >&2
    exit 1
fi
while IFS=$'\t' read -r number base; do
    [ -n "$number" ] || continue
    pr_bases[$number]=$base
done <<<"$open_prs"
for name in $branch_names; do
    branches[$name]=1
done

# 新しいものから順に処理し、ブランチ・PR と構成の組で最初に出てきたものを残す
while IFS=$'\t' read -r id key ref; do
    # 一覧が空のときも空の行が 1 行渡される
    [ -n "$id" ] || continue
    # キーは ccache-<構成のハッシュ>-<時刻> (.github/workflows/build-with-autotools.yml で決めている)。
    # 構成のハッシュは - を含まない
    config=${key#ccache-}
    config=${config%%-*}

    # 削除する理由。空なら残す
    reason=
    if [[ $ref =~ ^refs/pull/([0-9]+)/merge$ ]]; then
        number=${BASH_REMATCH[1]}
        base=${pr_bases[$number]:-}
        if [ -z "${pr_bases[$number]+x}" ]; then
            reason="PR is closed"
        # 新しい順に処理しているので、ベースのブランチの同じ構成のものを残し済みなら、それのほうが新しい
        elif [ -n "${kept["refs/heads/$base $config"]+x}" ]; then
            reason="older than $base"
        fi
    elif [[ $ref != refs/heads/* ]]; then
        # タグなどで手動実行したときのもの。ほかの実行からは使われない
        reason="neither a branch nor a PR"
    elif [ -z "${branches[${ref#refs/heads/}]+x}" ]; then
        reason="branch is deleted"
    fi
    if [ -z "$reason" ] && [ -n "${kept["$ref $config"]+x}" ]; then
        reason="older generation"
    fi

    if [ -n "$reason" ]; then
        delete_cache "$id" "$key" "$ref" "$reason"
    else
        kept["$ref $config"]=1
        echo "keep:   $ref $key"
    fi
done <<<"$caches"
