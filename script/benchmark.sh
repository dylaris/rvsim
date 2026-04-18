#!/bin/sh

set -xe

echo "Testing tbcache..."
./nob tbcache
./rvsim --elf bin/nbench > tbcache.txt
sync

echo "Testing dbcache..."
./nob dbcache
./rvsim --elf bin/nbench > dbcache.txt
sync

echo "Testing interp..."
./nob interp
./rvsim --elf bin/nbench > interp.txt
sync

echo "All tests completed."
