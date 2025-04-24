  .text
  .global main
main:

  // function prologue
  addi sp, sp, -64

  //getelemptr instruction: integer index of local array
  addi t0, sp, 24
  addi t0, t0, 4
  sw t0, 20(sp)

  //store instruction: store integer to pointed position
  li t0, 1
  lw t1, 20(sp)
  sw t0, 0(t1)

  //getelemptr instruction: integer index of local array
  addi t0, sp, 24
  addi t0, t0, 4
  sw t0, 16(sp)

  //load instruction: load from pointed position
  lw t0, 16(sp)
  lw t0, 0(t0)
  sw t0, 12(sp)

  //getelemptr instruction: variable index of local array
  addi t0, sp, 24
  lw t1, 12(sp)
  li t3, 2
  sll t1, t1, t3
  add t0, t0, t1
  sw t0, 8(sp)

  //load instruction: load from pointed position
  lw t0, 8(sp)
  lw t0, 0(t0)
  sw t0, 4(sp)

  //return instruction: return variable
  lw a0, 4(sp)
  addi sp, sp, 64
  ret
