#!/bin/bash - 

set -o nounset                              # Treat unset variables as an error

rm -rf /tmp/timing_data
mkdir /tmp/timing_data

for i in `seq 100`; do
    num=${i}0000
    ./program $num /tmp/timing_data/${num}.dat
done
