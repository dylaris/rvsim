#!/bin/sh

set -e

SRC_DIR="test/c"
BIN_DIR="bin"
RISCV_PREFIX="$HOME/opt/rv64-unknown-elf/bin/riscv64-unknown-elf-"

mkdir -p "$BIN_DIR"

for src_file in "$SRC_DIR"/*.c; do
    [ -f "$src_file" ] || continue

    base_name=$(basename "$src_file" .c)
    exe_file="$BIN_DIR/$base_name"

    echo "Compiling: $src_file"
    echo "Output:    $exe_file"

    "${RISCV_PREFIX}gcc" \
        -static \
        -march=rv64gc \
        -mabi=lp64d \
        -O2 \
        -g \
        -o "$exe_file" \
        "$src_file"

    chmod +x "$exe_file"
    echo "done."
    echo
done

echo "All files compiled successfully!"
