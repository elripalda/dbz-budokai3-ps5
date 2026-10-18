#!/bin/bash
# Build the PS5 app (all of it); log to out/build.log.
cd "$(dirname "$0")/../.."
start=$(date +%s)
ninja -C out/ps5 -k 0 dbz3 > out/build.log 2>&1
echo "exit $? after $(( $(date +%s)-start ))s" >> out/build.log
