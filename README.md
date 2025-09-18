# Cache Simulator (C++)

A simple set-associative cache simulator in C++ with LRU/FIFO replacement and miss classification.

## Features
- Configurable cache size, block size, associativity
- Replacement policies: LRU / FIFO
- Miss classification: Compulsory / Conflict / Capacity
- Verbose mode (`-v`) for per-access logging

## Build
```bash
make