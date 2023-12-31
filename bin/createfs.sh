#/bin/bash
currentversion="$(egrep 'fsversion =' ../lib/tardata/tardata.cpp |awk '{print $4}' |tr -d ';')"
nextversion=$((currentversion + 1))
/usr/bin/tar czf data.tar -C ../data/ .
/usr/bin/xxd --include data.tar > data.h
header="$(cat  <<EOF
#include <Arduino.h>

int fsversion = $nextversion;

EOF
)"
echo "$header" > ../lib/tardata/tardata.cpp
cat data.h >> ../lib/tardata/tardata.cpp
rm data.h data.tar
