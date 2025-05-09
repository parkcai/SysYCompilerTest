
  .text
  .globl main
main:
  addi  sp, sp, -352
  li    t0, 1
  mv    s8, t0
  li    t0, 0
  mv    s9, t0
  mv    s7, s8
  addi  s6, s7, 2
  mv    s8, s6
  mv    s5, s8
  addi  s4, s5, 3
  mv    s3, s4
  mv    s10, s3
  addi  s2, s10, 4
  mv    s3, s2
  mv    s1, s9
  sw    s8, 0(sp)
  lw    s8, 0(sp)
  sw    s9, 4(sp)
  add  s9, s1, s8
  sw    s7, 8(sp)
  mv    s7, s3
  sw    s6, 12(sp)
  add  s6, s9, s7
  sw    s5, 16(sp)
  mv    s5, s6
  sw    s4, 20(sp)
  mv    s4, s3
  sw    s3, 24(sp)
  addi  s3, s4, 5
  sw    s10, 28(sp)
  mv    s10, s3
  sw    s2, 32(sp)
  mv    s2, s10
  sw    s1, 36(sp)
  addi  s1, s2, 6
  sw    s8, 0(sp)
  mv    s8, s1
  sw    s5, 4(sp)
  lw    s9, 0(sp)
  sw    s7, 8(sp)
  mv    s7, s8
  sw    s6, 12(sp)
  add  s6, s9, s7
  sw    s5, 16(sp)
  mv    s5, s6
  sw    s4, 20(sp)
  lw    s4, 4(sp)
  sw    s10, 24(sp)
  mv    s3, s5
  sw    s10, 28(sp)
  add  s10, s4, s3
  sw    s2, 32(sp)
  lw    s2, 24(sp)
  sw    s1, 36(sp)
  add  s1, s10, s2
  sw    s5, 0(sp)
  mv    s8, s8
  sw    s5, 4(sp)
  add  s9, s1, s8
  sw    s7, 8(sp)
  mv    s7, s9
  sw    s6, 12(sp)
  lw    s6, 24(sp)
  sw    s5, 16(sp)
  lw    s5, 0(sp)
  sw    s4, 20(sp)
  add  s4, s6, s5
  sw    s10, 24(sp)
  mv    s3, s4
  sw    s10, 28(sp)
  mv    s10, s8
  sw    s2, 32(sp)
  addi  s2, s10, 7
  sw    s1, 36(sp)
  mv    s1, s2
  sw    s5, 0(sp)
  mv    s8, s1
  sw    s7, 4(sp)
  addi  s9, s8, 8
  mv    s1, s9
  sw    s7, 8(sp)
  lw    s7, 4(sp)
  sw    s6, 12(sp)
  mv    s6, s1
  sw    s5, 16(sp)
  add  s5, s7, s6
  sw    s4, 20(sp)
  mv    s4, s3
  sw    s3, 24(sp)
  add  s3, s5, s4
  sw    s10, 28(sp)
  mv    s10, s8
  sw    s2, 32(sp)
  add  s2, s3, s10
  sw    s1, 36(sp)
  mv    s1, s2
  sw    s5, 0(sp)
  lw    s8, 24(sp)
  sw    s1, 4(sp)
  mv    s9, s1
  sw    s7, 8(sp)
  add  s7, s8, s9
  sw    s6, 12(sp)
  mv    s6, s7
  sw    s5, 16(sp)
  mv    s5, s8
  sw    s4, 20(sp)
  addi  s4, s5, 9
  sw    s6, 24(sp)
  mv    s3, s4
  sw    s10, 28(sp)
  mv    s10, s1
  sw    s2, 32(sp)
  addi  s2, s10, 10
  mv    s1, s2
  sw    s1, 36(sp)
  mv    s1, s3
  sw    s5, 0(sp)
  addi  s8, s1, 12
  mv    s3, s8
  sw    s1, 4(sp)
  lw    s9, 4(sp)
  sw    s7, 8(sp)
  addi  s7, s9, 11
  sw    s6, 12(sp)
  mv    s6, s3
  sw    s5, 16(sp)
  add  s5, s7, s6
  sw    s4, 20(sp)
  mv    s4, s8
  sw    s6, 24(sp)
  add  s3, s5, s4
  sw    s10, 28(sp)
  mv    s10, s3
  sw    s2, 32(sp)
  mv    s2, s8
  sw    s1, 36(sp)
  mv    s1, s3
  sw    s5, 0(sp)
  add  s8, s2, s1
  mv    s8, s8
  sw    s10, 4(sp)
  mv    s9, s3
  sw    s7, 8(sp)
  addi  s7, s9, 13
  sw    s6, 12(sp)
  mv    s6, s7
  sw    s5, 16(sp)
  mv    s5, s6
  sw    s4, 20(sp)
  addi  s4, s5, 11
  mv    s6, s4
  sw    s6, 24(sp)
  lw    s3, 4(sp)
  sw    s10, 28(sp)
  addi  s10, s3, 11
  sw    s2, 32(sp)
  mv    s2, s3
  sw    s1, 36(sp)
  add  s1, s10, s2
  sw    s5, 0(sp)
  mv    s8, s6
  sw    s10, 4(sp)
  add  s9, s1, s8
  sw    s7, 8(sp)
  mv    s7, s9
  sw    s6, 12(sp)
  mv    s6, s7
  sw    s5, 16(sp)
  mv    s5, s8
  sw    s4, 20(sp)
  sub   s4, s6, s5
  mv    s7, s4
  sw    s6, 24(sp)
  mv    s3, s7
  sw    s10, 28(sp)
  lw    s10, 24(sp)
  sw    s2, 32(sp)
  sub   s2, s3, s10
  mv    s7, s2
  sw    s1, 36(sp)
  mv    s1, s7
  sw    s5, 0(sp)
  lw    s8, 0(sp)
  sw    s7, 4(sp)
  sub   s9, s1, s8
  sw    s7, 8(sp)
  mv    s7, s9
  sw    s6, 12(sp)
  mv    s6, s7
  sw    s5, 16(sp)
  li    t1, 77
  rem   s5, s6, t1
  mv    a0, s5
  addi  sp, sp, 352
  ret

