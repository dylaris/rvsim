.option norvc
.option nopic

# @ident: name=srai, opcode=0b0010011, funct3=0x5, funct6=0x10

# @expect: rd=1, rs1=2
srai x1, x2, 0
# @expect: rd=2, rs1=3
srai x2, x3, 1
# @expect: rd=3, rs1=4
srai x3, x4, 7
# @expect: rd=4, rs1=5
srai x4, x5, 15
# @expect: rd=5, rs1=6
srai x5, x6, 31
