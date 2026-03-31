.option norvc
.option nopic

# @ident: name=jal, opcode=0x6f
# @expect: brk=true, rd=1, imm=16
jal x1, 4f
1:
# @expect: brk=true, rd=2, imm=0
jal x2, 1b
2:
# @expect: brk=true, rd=3, imm=4
jal x3, 3f
3:
# @expect: brk=true, rd=4, imm=-4
jal x4, 2b
4:
