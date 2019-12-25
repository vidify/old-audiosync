#!/bin/bash


NUM=25
time_c=0
time_py=0

counter=0
# Testing with different millisecond cuts
for ms in 5000 10000 15000 20000 25000 30000 35000; do
    # And `NUM` times each
    for i in $(seq 1 $NUM); do
        time=$(./main "audio/youtube.wav" "audio/recorded.wav" "$ms" | awk '/in .* secs/{print $4}')
        time_c=$(awk "BEGIN {print $time_c+$time}")
        time=$(python sketch.py "audio/youtube.wav" "audio/recorded.wav" "$ms" 2>/dev/null | awk '/in .* secs/{print $4}')
        time_py=$(awk "BEGIN {print $time_py+$time}")
        counter=$((counter + 1))
    done
    echo "Iteration $counter"
done

bench_c=$(awk "BEGIN {print $time_c/$counter}")
bench_py=$(awk "BEGIN {print $time_py/$counter}")

echo "$counter iterations in total with lengths 5000 10000 15000 20000 25000 30000 35000. Averages:"
echo "    C: $bench_c"
echo "    Python: $bench_py"
