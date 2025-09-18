#!/bin/bash
# run_examples.sh
# Demonstrates running the cache simulator on sample traces

echo "Running stream trace..."
./cache_sim traces/stream.txt 32768 64 4 LRU 32 -v

echo ""
echo "Running conflict trace..."
./cache_sim traces/conflict.txt 1024 64 2 LRU 32 -v

echo ""
echo "Running random trace..."
./cache_sim traces/random.txt 8192 64 2 FIFO 32 -v