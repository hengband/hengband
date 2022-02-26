#!/bin/sh

# InsertBraces オプションに対応するため、clang-format-15 をインストールする
# そのため LLVM が用意している APT リポジトリを追加する
# 将来的に GitHub Actions の Ubuntu runner が 22.04 になれば Ubuntu の APT リポジトリからインストールできるかもしれない
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key 2>/dev/null | sudo apt-key add - >/dev/null

cat <<EOF | sudo tee /etc/apt/sources.list.d/llvm.list >/dev/null
deb http://apt.llvm.org/focal/ llvm-toolchain-focal main
deb-src http://apt.llvm.org/focal/ llvm-toolchain-focal main
# 13
deb http://apt.llvm.org/focal/ llvm-toolchain-focal-13 main
deb-src http://apt.llvm.org/focal/ llvm-toolchain-focal-13 main
# 14
deb http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main
deb-src http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main

EOF

sudo apt-get update >/dev/null
sudo apt-get install clang-format-15 >/dev/null

SRC_FILES=$(find src/ -name \*.cpp -or -name \*.h)

for file in $SRC_FILES; do
    clang-format-15 -style=file:.github/scripts/check-clang-format-style -i $file
done

DIFF_FILE=$(mktemp)
git diff >$DIFF_FILE

if [ -s $DIFF_FILE ]; then
    echo "Found incorrectly formatted source file(s)."
    cat $DIFF_FILE
    exit 1
fi

exit 0
