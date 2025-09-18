#!/bin/bash
# tests/test_stream.sh
# Simple smoke test for cache_sim with stream trace

BIN=../cache_sim
TRACE=../traces/stream.txt

if [ ! -x "$BIN" ]; then
  echo "Error: $BIN not found or not executable."
  exit 1
fi

echo "Running stream trace test..."
OUTPUT=$($BIN $TRACE 32768 64 4 LRU 32)

echo "$OUTPUT"

# Example check: ensure the word "Hits" and "Misses" appear
if [[ "$OUTPUT" == *"Hits"* && "$OUTPUT" == *"Misses"* ]]; then
  echo "✅ Test passed: Output contains hit/miss stats"
  exit 0
else
  echo "❌ Test failed: Stats not found in output"
  exit 1
fi