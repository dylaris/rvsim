#!/bin/sh

set -xe

echo "Testing tbcache..."
./nob tbcache
./rvsim bin/nbench > tbcache.txt
sync

echo "Testing dbcache..."
./nob dbcache
./rvsim bin/nbench > dbcache.txt
sync

echo "Testing interp..."
./nob interp
./rvsim bin/nbench > interp.txt
sync

echo "All tests completed."
