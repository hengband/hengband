#!/bin/sh

SRC_FILES=$(find src/ -name \*.cpp -or -name \*.h)

STATUS=0

for file in $SRC_FILES; do
    file $file | grep "BOM" >/dev/null
    if [ $? == 0 ]; then
        echo "$file: BOM exists."
        STATUS=1
    fi
done

exit $STATUS
