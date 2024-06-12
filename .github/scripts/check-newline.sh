#!/bin/sh

# ./VisualStudio/ 以下のファイルはVisual StudioがCRLFで上書きするため除外
CHECK_FILES=$(find . -type f -not -path './.git/*' -and -not -path './VisualStudio/*' -and -not -name \*.wav -and -not -name \*.mp3)

STATUS=0

for file in $CHECK_FILES; do
    file $file | grep "CRLF" >/dev/null
    if [ $? -eq 0 ]; then
        echo "$file: newline is CRLF."
        STATUS=1
    fi
done

exit $STATUS
