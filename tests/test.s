.section my_code
my_start:
  ld $0xFFFFFEFE, %sp
  ld $0x1, %r1
  ld $0x2, %r2
  ld $0x3, %r3
  push %r1
  push %r2
  push %r3
  pop %r1   #r3
  pop %r3   #r2
  pop %r2   #r1 

  halt
.end
