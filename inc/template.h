#ifndef TEMPLATE_H
#define TEMPLATE_H

#define INSTRUCTION_LIST(X) \
    X(lb,  LOAD, i8,  _, _, _) \
    X(lh,  LOAD, i16, _, _, _) \
    X(lw,  LOAD, i32, _, _, _) \
    X(ld,  LOAD, i64, _, _, _) \
    X(lbu, LOAD, u8,  _, _, _) \
    X(lhu, LOAD, u16, _, _, _) \
    X(lwu, LOAD, u32, _, _, _) \
    X(fence,   EMPTY, _, _, _, _) \
    X(fence_i, EMPTY, _, _, _, _) \
    X(ebreak,  EMPTY, _, _, _, _) \
    X(auipc,   AUIPC, _, _, _, _) \
    X(lui,     LUI,   _, _, _, _) \
    X(ecall,   ECALL, _, _, _, _) \
    X(addi,  OP_IMM, rs1 + imm,                            rs1 + %ld,                            _, _) \
    X(slli,  OP_IMM, rs1 << (imm & 0x3f),                  rs1 << (%ld & 0x3f),                  _, _) \
    X(slti,  OP_IMM, (i64)rs1 < (i64)imm,                  (i64)rs1 < (i64)%ld,                  _, _) \
    X(sltiu, OP_IMM, (u64)rs1 < (u64)imm,                  (u64)rs1 < (u64)%ld,                  _, _) \
    X(xori,  OP_IMM, rs1 ^ imm,                            rs1 ^ %ld,                            _, _) \
    X(srli,  OP_IMM, rs1 >> (imm & 0x3f),                  rs1 >> (%ld & 0x3f),                  _, _) \
    X(srai,  OP_IMM, (i64)rs1 >> (imm & 0x3f),             (i64)rs1 >> (%ld & 0x3f),             _, _) \
    X(ori,   OP_IMM, rs1 | (u64)imm,                       rs1 | (u64)%ld,                       _, _) \
    X(andi,  OP_IMM, rs1 & (u64)imm,                       rs1 & (u64)%ld,                       _, _) \
    X(addiw, OP_IMM, (i64)(i32)(rs1 + imm),                (i64)(i32)(rs1 + %ld),                _, _) \
    X(slliw, OP_IMM, (i64)(i32)(rs1 << (imm & 0x1f)),      (i64)(i32)(rs1 << (%ld & 0x1f)),      _, _) \
    X(srliw, OP_IMM, (i64)(i32)((u32)rs1 >> (imm & 0x1f)), (i64)(i32)((u32)rs1 >> (%ld & 0x1f)), _, _) \
    X(sraiw, OP_IMM, (i64)((i32)rs1 >> (imm & 0x1f)),      (i64)((i32)rs1 >> (%ld & 0x1f)),      _, _) \
    X(sb, STORE, u8,  _, _, _) \
    X(sh, STORE, u16, _, _, _) \
    X(sw, STORE, u32, _, _, _) \
    X(sd, STORE, u64, _, _, _) \
    X(add,    OP_REG, rs1 + rs2,                                                            _, _, _) \
    X(sll,    OP_REG, rs1 << (rs2 & 0x3f),                                                  _, _, _) \
    X(slt,    OP_REG, (i64)rs1 < (i64)rs2,                                                  _, _, _) \
    X(sltu,   OP_REG, (u64)rs1 < (u64)rs2,                                                  _, _, _) \
    X(xor,    OP_REG, rs1 ^ rs2,                                                            _, _, _) \
    X(srl,    OP_REG, rs1 >> (rs2 & 0x3f),                                                  _, _, _) \
    X(or,     OP_REG, rs1 | rs2,                                                            _, _, _) \
    X(and,    OP_REG, rs1 & rs2,                                                            _, _, _) \
    X(mul,    OP_REG, rs1 * rs2,                                                            _, _, _) \
    X(div,    OP_REG, help_div(rs1, rs2),                                                  _, _, _) \
    X(divu,   OP_REG, rs2 == 0 ? UINT64_MAX : rs1 / rs2,                                    _, _, _) \
    X(rem,    OP_REG, help_rem(rs1, rs2),                                                  _, _, _) \
    X(remu,   OP_REG, rs2 == 0 ? rs1 : rs1 % rs2,                                           _, _, _) \
    X(sub,    OP_REG, rs1 - rs2,                                                            _, _, _) \
    X(sra,    OP_REG, (i64)rs1 >> (rs2 & 0x3f),                                             _, _, _) \
    X(addw,   OP_REG, (i64)(i32)(rs1 + rs2),                                                _, _, _) \
    X(sllw,   OP_REG, (i64)(i32)(rs1 << (rs2 & 0x1f)),                                      _, _, _) \
    X(srlw,   OP_REG, (i64)(i32)((u32)rs1 >> (rs2 & 0x1f)),                                 _, _, _) \
    X(mulw,   OP_REG, (i64)(i32)(rs1 * rs2),                                                _, _, _) \
    X(divw,   OP_REG, rs2 == 0 ? UINT64_MAX : (i32)((i64)(i32)rs1 / (i64)(i32)rs2),         _, _, _) \
    X(divuw,  OP_REG, rs2 == 0 ? UINT64_MAX : (i32)((u32)rs1 / (u32)rs2),                   _, _, _) \
    X(remw,   OP_REG, rs2 == 0 ? (i64)(i32)rs1 : (i64)(i32)((i64)(i32)rs1 % (i64)(i32)rs2), _, _, _) \
    X(remuw,  OP_REG, rs2 == 0 ? (i64)(i32)(u32)rs1 : (i64)(i32)((u32)rs1 % (u32)rs2),      _, _, _) \
    X(subw,   OP_REG, (i64)(i32)(rs1 - rs2),                                                _, _, _) \
    X(sraw,   OP_REG, (i64)(i32)((i32)rs1 >> (rs2 & 0x1f)),                                 _, _, _) \
    X(mulh,   OP_HMUL, help_mulh(rs1, rs2),                                                 _, _, _) \
    X(mulhsu, OP_HMUL, help_mulhsu(rs1, rs2),                                               _, _, _) \
    X(mulhu,  OP_HMUL, help_mulhu(rs1, rs2),                                                _, _, _) \
    X(beq,  BRANCH, (u64)rs1 == (u64)rs2, _, _, _) \
    X(bne,  BRANCH, (u64)rs1 != (u64)rs2, _, _, _) \
    X(blt,  BRANCH, (i64)rs1 <  (i64)rs2, _, _, _) \
    X(bge,  BRANCH, (i64)rs1 >= (i64)rs2, _, _, _) \
    X(bltu, BRANCH, (u64)rs1 <  (u64)rs2, _, _, _) \
    X(bgeu, BRANCH, (u64)rs1 >= (u64)rs2, _, _, _) \
    X(jalr, JUMP, (rs1 + (i64)imm) & ~(u64)1, (rs1 + (i64)%ld) & ~(u64)1, _, _) \
    X(jal,  JUMP, pc + (i64)imm,              pc + (i64)%ld,              _, _) \
    X(csrrc,  CSR, _, _, _, _) \
    X(csrrci, CSR, _, _, _, _) \
    X(csrrs,  CSR, _, _, _, _) \
    X(csrrsi, CSR, _, _, _, _) \
    X(csrrw,  CSR, _, _, _, _) \
    X(csrrwi, CSR, _, _, _, _) \
    X(flw, FP_LOAD, u32, _, _, _) \
    X(fld, FP_LOAD, u64, _, _, _) \
    X(fsw, FP_STORE, u32, _, _, _) \
    X(fsd, FP_STORE, u64, _, _, _) \
    X(fmadd_s,   FP_FMA4, s, f32, rs1 * rs2 + rs3,    _) \
    X(fmsub_s,   FP_FMA4, s, f32, rs1 * rs2 - rs3,    _) \
    X(fnmsub_s,  FP_FMA4, s, f32, -(rs1 * rs2) + rs3, _) \
    X(fnmadd_s,  FP_FMA4, s, f32, -(rs1 * rs2) - rs3, _) \
    X(fmadd_d,   FP_FMA4, d, f64, rs1 * rs2 + rs3,    _) \
    X(fmsub_d,   FP_FMA4, d, f64, rs1 * rs2 - rs3,    _) \
    X(fnmsub_d,  FP_FMA4, d, f64, -(rs1 * rs2) + rs3, _) \
    X(fnmadd_d,  FP_FMA4, d, f64, -(rs1 * rs2) - rs3, _) \
    X(fadd_s,  FP_BINOP, s, f32, rs1 + rs2,             _) \
    X(fsub_s,  FP_BINOP, s, f32, rs1 - rs2,             _) \
    X(fmul_s,  FP_BINOP, s, f32, rs1 * rs2,             _) \
    X(fdiv_s,  FP_BINOP, s, f32, rs1 / rs2,             _) \
    X(fsqrt_s, FP_BINOP, s, f32, sqrtf(rs1),            _) \
    X(fmin_s,  FP_BINOP, s, f32, rs1 < rs2 ? rs1 : rs2, _) \
    X(fmax_s,  FP_BINOP, s, f32, rs1 > rs2 ? rs1 : rs2, _) \
    X(fadd_d,  FP_BINOP, d, f64, rs1 + rs2,             _) \
    X(fsub_d,  FP_BINOP, d, f64, rs1 - rs2,             _) \
    X(fmul_d,  FP_BINOP, d, f64, rs1 * rs2,             _) \
    X(fdiv_d,  FP_BINOP, d, f64, rs1 / rs2,             _) \
    X(fsqrt_d, FP_BINOP, d, f64, sqrt(rs1),             _) \
    X(fmin_d,  FP_BINOP, d, f64, rs1 < rs2 ? rs1 : rs2, _) \
    X(fmax_d,  FP_BINOP, d, f64, rs1 > rs2 ? rs1 : rs2, _) \
    X(fsgnj_s,  FP_SGNJ, w, u32, false, false) \
    X(fsgnjn_s, FP_SGNJ, w, u32, true,  false) \
    X(fsgnjx_s, FP_SGNJ, w, u32, false, true)  \
    X(fsgnj_d,  FP_SGNJ, q, u64, false, false) \
    X(fsgnjn_d, FP_SGNJ, q, u64, true,  false) \
    X(fsgnjx_d, FP_SGNJ, q, u64, false, true)  \
    X(fcvt_w_s,  FP_XFER_F2I, s, f32, (i64)(i32)llrintf(src),      _) \
    X(fcvt_wu_s, FP_XFER_F2I, s, f32, (i64)(i32)(u32)llrintf(src), _) \
    X(fcvt_l_s,  FP_XFER_F2I, s, f32, (i64)llrintf(src),           _) \
    X(fcvt_lu_s, FP_XFER_F2I, s, f32, (u64)llrintf(src),           _) \
    X(fcvt_w_d,  FP_XFER_F2I, d, f64, (i64)(i32)llrint(src),       _) \
    X(fcvt_wu_d, FP_XFER_F2I, d, f64, (i64)(i32)(u32)llrint(src),  _) \
    X(fcvt_l_d,  FP_XFER_F2I, d, f64, (i64)llrint(src),            _) \
    X(fcvt_lu_d, FP_XFER_F2I, d, f64, (u64)llrint(src),            _) \
    X(fmv_x_w,   FP_XFER_F2I, w, u32, (i64)(i32)src,               _) \
    X(fmv_x_d,   FP_XFER_F2I, q, u64, src,                         _) \
    X(fcvt_s_w,  FP_XFER_I2F, s, (f32)(i32)src, _, _) \
    X(fcvt_s_wu, FP_XFER_I2F, s, (f32)(u32)src, _, _) \
    X(fcvt_s_l,  FP_XFER_I2F, s, (f32)(i64)src, _, _) \
    X(fcvt_s_lu, FP_XFER_I2F, s, (f32)(u64)src, _, _) \
    X(fcvt_d_w,  FP_XFER_I2F, d, (f64)(i64)src, _, _) \
    X(fcvt_d_wu, FP_XFER_I2F, d, (f64)(u64)src, _, _) \
    X(fcvt_d_l,  FP_XFER_I2F, d, (f64)(i64)src, _, _) \
    X(fcvt_d_lu, FP_XFER_I2F, d, (f64)(u64)src, _, _) \
    X(fmv_w_x,   FP_XFER_I2F, w, (u32)src,      _, _) \
    X(fmv_d_x,   FP_XFER_I2F, q, src,           _, _) \
    X(fcvt_s_d, FP_XFER_F2F, d, s, f64, (f32)src) \
    X(fcvt_d_s, FP_XFER_F2F, s, d, f32, (f64)src) \
    X(feq_s, FP_CMP, s, f32, rs1 == rs2, _) \
    X(flt_s, FP_CMP, s, f32, rs1 <  rs2, _) \
    X(fle_s, FP_CMP, s, f32, rs1 <= rs2, _) \
    X(feq_d, FP_CMP, d, f64, rs1 == rs2, _) \
    X(flt_d, FP_CMP, d, f64, rs1 <  rs2, _) \
    X(fle_d, FP_CMP, d, f64, rs1 <= rs2, _) \
    X(fclass_s, FP_CLASS, s, f32, _, _) \
    X(fclass_d, FP_CLASS, d, f64, _, _)


#define SYSCALL_LIST(X)      \
    X(exit,            93)   \
    X(exit_group,      94)   \
    X(getpid,          172)  \
    X(kill,            129)  \
    X(tgkill,          131)  \
    X(read,            63)   \
    X(write,           64)   \
    X(openat,          56)   \
    X(close,           57)   \
    X(lseek,           62)   \
    X(brk,             214)  \
    X(linkat,          37)   \
    X(unlinkat,        35)   \
    X(mkdirat,         34)   \
    X(renameat,        38)   \
    X(chdir,           49)   \
    X(getcwd,          17)   \
    X(fstat,           80)   \
    X(fstatat,         79)   \
    X(faccessat,       48)   \
    X(pread,           67)   \
    X(pwrite,          68)   \
    X(uname,           160)  \
    X(getuid,          174)  \
    X(geteuid,         175)  \
    X(getgid,          176)  \
    X(getegid,         177)  \
    X(gettid,          178)  \
    X(sysinfo,         179)  \
    X(mmap,            222)  \
    X(munmap,          215)  \
    X(mremap,          216)  \
    X(mprotect,        226)  \
    X(prlimit64,       261)  \
    X(getmainvars,     2011) \
    X(rt_sigaction,    134)  \
    X(writev,          66)   \
    X(gettimeofday,    169)  \
    X(times,           153)  \
    X(fcntl,           25)   \
    X(ftruncate,       46)   \
    X(getdents,        61)   \
    X(dup,             23)   \
    X(dup3,            24)   \
    X(readlinkat,      78)   \
    X(rt_sigprocmask,  135)  \
    X(ioctl,           29)   \
    X(getrlimit,       163)  \
    X(setrlimit,       164)  \
    X(getrusage,       165)  \
    X(clock_gettime,   113)  \
    X(set_tid_address, 96)   \
    X(set_robust_list, 99)   \
    X(madvise,         233)  \
    X(statx,           291)  \
    X(open,            1024) \
    X(link,            1025) \
    X(unlink,          1026) \
    X(mkdir,           1030) \
    X(access,          1033) \
    X(stat,            1038) \
    X(lstat,           1039) \
    X(time,            1062)

#endif // TEMPLATE_H
