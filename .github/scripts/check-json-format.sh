#!/bin/sh

JSON_FILES=$(find lib/ -type f -regextype posix-egrep -regex ".*\.(json|jsonc)")

npm install -g prettier
npx prettier --write $JSON_FILES

DIFF_FILE=$(mktemp)
git diff >$DIFF_FILE

if [ -s $DIFF_FILE ]; then
    echo "Some json/jsonc files are not properly formatted."
    cat $DIFF_FILE
    exit 1
fi

exit 0
