  .data
  .globl arr
arr:
  .zero 40

  .text
  .globl f1
f1:
  addi sp, sp, -16
  mv t0, a0
  lw t1, 0(sp)
  sw t0, 0(t1)
  lw t0, 0(sp)
  sw t0, 4(sp)
  addi t0, sp, 4
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  sw t0, 8(sp)
  lw t0, 8(sp)
  lw t0, 0(t0)
  sw t0, 12(sp)
  lw a0, 12(sp)
  addi sp, sp, 16
  ret 

  .text
  .globl main
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  la t0, arr
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  sw t0, 0(sp)
  lw a0, 0(sp)
  call f1
  sw a0, 4(sp)
  lw a0, 4(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret 

