#!/usr/bin/env bash

for i in 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29
do
echo "Experiment for $i"
./precise_parallel_fp $i
done