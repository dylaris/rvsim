#!/bin/sh

# Compile single C file (RISCV64)

SRC="$1"
EXE="${SRC%.c}"

RISCV_PREFIX="$HOME/opt/rv64-unknown-elf/bin/riscv64-unknown-elf-"
${RISCV_PREFIX}gcc -static -march=rv64gc -mabi=lp64d -o "$EXE" "$SRC"
