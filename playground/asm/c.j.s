.option rvc
.option nopic

# @ident: name=c.j, copcode=0x1, cfunct3=0x5
# @expect: brk=true, rvc=true, rd=0, imm=2
c.j 1f
1:

