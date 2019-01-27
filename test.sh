#!/bin/sh

set -e

race64=./race64
temp=test.tmp
checksum=sha1sum

len=0
while [ $len -lt 300 ]; do
    len=$((len + 1))
    dd if=/dev/urandom of=$temp count=1 bs=$len 2>/dev/null
    check1="$($checksum <$temp)"
    check2="$($race64 <$temp | $race64 -d | $checksum)"
    if [ "$check1" != "$check2" ]; then
        printf 'Mismatch len=%d\n' $len
        exit 1
    fi
done

rm $temp
