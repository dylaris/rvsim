import ctypes
import re
import os

lib = ctypes.CDLL("./librvemu.so")

class Instruction(ctypes.Structure):
    _fields_ = [
        ("rd",  ctypes.c_int8),
        ("rs1", ctypes.c_int8),
        ("rs2", ctypes.c_int8),
        ("imm", ctypes.c_int8),
        ("brk", ctypes.c_bool),
        ("rvc", ctypes.c_bool),
    ]

lib.api_decode.argtypes = [ctypes.c_uint32, ctypes.POINTER(Instruction)]
lib.api_decode.restype = ctypes.c_bool

def parse_int(s):
    s = s.strip()
    if s.startswith("0b"): return int(s[:2], 2)
    if s.startswith("0x"): return int(s[:2], 16)
    return int(s)

def run_test(filepath):
    with open(filepath, "r") as f:
        lines = [l.strip() for l in f.readlines()]
    print(f"{os.path.basename(filepath)}")

    prefix = "/home/aris/opt/riscv/bin/riscv64-unknown-elf-"
    os.system(f"{prefix}gcc -c -o tmp.o {filepath}")
    os.system(f"{prefix}objcopy -O binary tmp.o tmp.bin")
    data = open("tmp.bin", "rb").read()
    os.system("rm -f tmp.o tmp.bin")

    insts = [int.from_bytes(data[i:i+4], "little") for i in range(0, len(data), 4)]
    inst_idx = 0

    for line in lines:
        if not line.startswith("# @expect"):
            continue

        # parse
        exp_str = line.replace("# @expect:", "").strip()
        expect = dict(kv.split("=") for kv in exp_str.split(", "))

        # decode
        inst = Instruction()
        lib.api_decode(insts[inst_idx], ctypes.byref(inst))
        inst_idx += 1

        # compare
        ok = True
        if "rd"  in expect and inst.rd  != parse_int(expect["rd"]):  ok = False
        if "rs1" in expect and inst.rs1 != parse_int(expect["rs1"]): ok = False
        if "rs1" in expect and inst.rs2 != parse_int(expect["rs2"]): ok = False
        if "imm" in expect and inst.imm != parse_int(expect["imm"]): ok = False

        if ok:
            print(f"  PASS")
        else:
            print(f"  FAIL | expect: {expect} | actual: rd={inst.rd}, rs1={inst.rs1}, rs2={inst.rs2}, imm={inst.imm}")

if __name__ == "__main__":
    for filename in os.listdir("playground/asm"):
        if filename.endswith(".s"):
            run_test(f"playground/asm/{filename}")
