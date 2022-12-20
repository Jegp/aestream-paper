#!/bin/env bash

for i in `seq 5`; do
    for n in cpu cuda; do
        echo Executing $n / $i
        nsys profile -o "$n_$i_with_c" python3 src/gpu_benchmark.py $n
        nsys profile -o "$n_$i_no_c" python3 src/gpu_benchmark.py $n --no_coroutines
    done
done