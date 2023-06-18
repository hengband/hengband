#!/bin/sh

SRC_FILES=$(find src/ -type f ! -path "src/external-lib/*" \( -name \*.cpp -or -name \*.h \))

STATUS=0

for file in $SRC_FILES; do
    file $file | grep "BOM" >/dev/null
    if [ $? != 0 ]; then
        echo "$file: BOM does not exists."
        STATUS=1
    fi
done

exit $STATUS
