#!/bin/bash
./cache_sim traces/stream.txt 32768 64 4 LRU 32 -v
./cache_sim traces/conflict.txt 1024 64 2 LRU 32 -v
./cache_sim traces/random.txt 8192 64 2 FIFO 32 -v