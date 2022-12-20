#!/bin/env bash

for i in `seq 5`; do
    for n in cpu cuda; do
        echo Executing $n / $i
        python3 src/gpu_benchmark.py $n
        python3 src/gpu_benchmark.py $n --no_coroutines
    done
done