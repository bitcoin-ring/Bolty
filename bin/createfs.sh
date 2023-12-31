#/bin/bash
currentversion="$(egrep 'fsversion =' ../tardata.cpp |awk '{print $4}' |tr -d ';')"
nextversion=$((currentversion + 1))
/usr/bin/tar czf data.tar -C ../data/ .
/usr/bin/xxd --include data.tar > data.h
header="$(cat  <<EOF
#include <Arduino.h>

int fsversion = $nextversion;

EOF
)"
echo "$header" > ../tardata.cpp
cat data.h >> ../tardata.cpp
rm data.h data.tar
