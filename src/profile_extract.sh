#!/bin/env bash

REPORTS=`ls *.nsys-rep | sort -V`
for r in $REPORTS; do
    echo "#########"
    echo $r
    echo "#########"
    nsys stats -r cudaapisum $r --format json | tail -n +2 | jq '.[] | select(.Name == "cudaMemcpyAsync")'
    nsys stats -r gpumemtimesum $w --format table | grep memcpy | cut -d'|' -f3
done