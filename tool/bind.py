import ctypes
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
lib_path = os.path.join(script_dir, "../librvsim.so")

lib = ctypes.CDLL(lib_path)

class Instr(ctypes.Structure):
    pass

lib.decode_instr.argtypes = [ctypes.c_uint32, ctypes.POINTER(Instr)]
lib.decode_instr.restype = ctypes.c_bool

lib.instr_to_string.argtypes = [ctypes.POINTER(Instr)]
lib.instr_to_string.restype = ctypes.c_char_p

GPR_ZERO = 0
GPR_RA   = 1
GPR_SP   = 2
GPR_GP   = 3
GPR_TP   = 4
GPR_T0   = 5
GPR_T1   = 6
GPR_T2   = 7
GPR_S0   = 8
GPR_S1   = 9
GPR_A0   = 10
GPR_A1   = 11
GPR_A2   = 12
GPR_A3   = 13
GPR_A4   = 14
GPR_A5   = 15
GPR_A6   = 16
GPR_A7   = 17
GPR_S2   = 18
GPR_S3   = 19
GPR_S4   = 20
GPR_S5   = 21
GPR_S6   = 22
GPR_S7   = 23
GPR_S8   = 24
GPR_S9   = 25
GPR_S10  = 26
GPR_S11  = 27
GPR_T3   = 28
GPR_T4   = 29
GPR_T5   = 30
GPR_T6   = 31
NUM_GPRS = 32

FPR_FT0  = 0
FPR_FT1  = 1
FPR_FT2  = 2
FPR_FT3  = 3
FPR_FT4  = 4
FPR_FT5  = 5
FPR_FT6  = 6
FPR_FT7  = 7
FPR_FS0  = 8
FPR_FS1  = 9
FPR_FA0  = 10
FPR_FA1  = 11
FPR_FA2  = 12
FPR_FA3  = 13
FPR_FA4  = 14
FPR_FA5  = 15
FPR_FA6  = 16
FPR_FA7  = 17
FPR_FS2  = 18
FPR_FS3  = 19
FPR_FS4  = 20
FPR_FS5  = 21
FPR_FS6  = 22
FPR_FS7  = 23
FPR_FS8  = 24
FPR_FS9  = 25
FPR_FS10 = 26
FPR_FS11 = 27
FPR_FT8  = 28
FPR_FT9  = 29
FPR_FT10 = 30
FPR_FT11 = 31
NUM_FPRS = 32

FPR_VIEW_Q = 0
FPR_VIEW_D = 1
FPR_VIEW_W = 2
FPR_VIEW_S = 3

CSR_FFLAGS = 1
CSR_FRM    = 2
CSR_FCSR   = 3

TRAP_MASK           = 1 << 4
FLOW_NONE           = 0
FLOW_BRANCH         = 1
FLOW_JUMP           = 2
FLOW_SKIP_CODEGEN   = 3
FLOW_ECALL          = TRAP_MASK | 1
FLOW_ILLEGAL_INSTR  = TRAP_MASK | 2

class Flow(ctypes.Structure):
    _fields_ = [
        ("pc", ctypes.c_uint64),
        ("ctl", ctypes.c_int),
    ]

class FPR(ctypes.Union):
    _fields_ = [
        ("q", ctypes.c_uint64),
        ("d", ctypes.c_double),
        ("w", ctypes.c_uint32),
        ("s", ctypes.c_float),
    ]

class CPUState(ctypes.Structure):
    _fields_ = [
        ("gprs", ctypes.c_uint64 * NUM_GPRS),
        ("pc", ctypes.c_uint64),
        ("flow", Flow),
        ("fprs", FPR * NUM_FPRS),
    ]

    def get_gpr(self, index):
        if not 0 <= index < NUM_GPRS:
            raise ValueError(f"invalid GPR index: {index}")
        return self.gprs[index]

    def get_fpr(self, index, view):
        if not 0 <= index < NUM_FPRS:
            raise ValueError(f"invalid FPR index: {index}")
        if view == FPR_VIEW_Q:
            return self.fprs[index].q
        elif view == FPR_VIEW_D:
            return self.fprs[index].d
        elif view == FPR_VIEW_W:
            return self.fprs[index].w
        elif view == FPR_VIEW_S:
            return self.fprs[index].s
        else:
            raise ValueError(f"invalid FPR view : {view}")

    def get_flow_ctl(self):
        return self.flow.ctl

    def get_flow_pc(self):
        return self.flow.pc

Instr._fields_ = [
    ("rd",   ctypes.c_int8),
    ("rs1",  ctypes.c_int8),
    ("rs2",  ctypes.c_int8),
    ("rs3",  ctypes.c_int8),
    ("imm",  ctypes.c_int32),
    ("kind", ctypes.c_int),
    ("rvc",  ctypes.c_bool),
    ("cfc",  ctypes.c_bool),
]

def decode(self, raw):
    return lib.decode_instr(raw, ctypes.byref(self))

def __str__(self):
    return lib.instr_to_string(ctypes.byref(self))

Instr.decode = decode
Instr.__str__ = __str__
