  .text
  .globl init
init:
  li t0, -236
  add sp, sp, t0
  mv t0, a0
  li t2, 0
  add t1, sp, t2
  sw t0, (t1)
  li t0, 0
  li t1, 0
  add t0, t0, t1
  li t2, 8
  add t2, sp, t2
  sw t0, (t2)
  li t2, 8
  add t2, sp, t2
  lw t0, (t2)
  li t2, 4
  add t1, sp, t2
  sw t0, (t1)
  j while_entry_0
while_entry_0:
  li t2, 4
  add t2, sp, t2
  lw t0, (t2)
  li t2, 12
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 10
  add t0, t0, t1
  li t2, 16
  add t2, sp, t2
  sw t0, (t2)
  li t2, 12
  add t2, sp, t2
  lw t0, (t2)
  li t2, 16
  add t2, sp, t2
  lw t1, (t2)
  slt t0, t0, t1
  li t2, 20
  add t2, sp, t2
  sw t0, (t2)
  li t2, 20
  add t2, sp, t2
  lw t0, (t2)
  bnez t0, STATION_0
  j while_exit_0
STATION_0:
  j while_body_0
while_body_0:
  li t0, 0
  li t1, 0
  add t0, t0, t1
  li t2, 28
  add t2, sp, t2
  sw t0, (t2)
  li t2, 28
  add t2, sp, t2
  lw t0, (t2)
  li t2, 24
  add t1, sp, t2
  sw t0, (t1)
  j while_entry_1
while_exit_0:
  li t0, 236
  add sp, t0, sp
  ret
while_entry_1:
  li t2, 24
  add t2, sp, t2
  lw t0, (t2)
  li t2, 32
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 10
  add t0, t0, t1
  li t2, 36
  add t2, sp, t2
  sw t0, (t2)
  li t2, 32
  add t2, sp, t2
  lw t0, (t2)
  li t2, 36
  add t2, sp, t2
  lw t1, (t2)
  slt t0, t0, t1
  li t2, 40
  add t2, sp, t2
  sw t0, (t2)
  li t2, 40
  add t2, sp, t2
  lw t0, (t2)
  bnez t0, STATION_1
  j while_exit_1
STATION_1:
  j while_body_1
while_body_1:
  li t0, 0
  li t1, 0
  add t0, t0, t1
  li t2, 48
  add t2, sp, t2
  sw t0, (t2)
  li t2, 48
  add t2, sp, t2
  lw t0, (t2)
  li t2, 44
  add t1, sp, t2
  sw t0, (t1)
  j while_entry_2
while_exit_1:
  li t2, 4
  add t2, sp, t2
  lw t0, (t2)
  li t2, 52
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 1
  add t0, t0, t1
  li t2, 56
  add t2, sp, t2
  sw t0, (t2)
  li t2, 52
  add t2, sp, t2
  lw t0, (t2)
  li t2, 56
  add t2, sp, t2
  lw t1, (t2)
  add t0, t0, t1
  li t2, 60
  add t2, sp, t2
  sw t0, (t2)
  li t2, 60
  add t2, sp, t2
  lw t0, (t2)
  li t2, 4
  add t1, sp, t2
  sw t0, (t1)
  j while_entry_0
while_entry_2:
  li t2, 44
  add t2, sp, t2
  lw t0, (t2)
  li t2, 64
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 10
  add t0, t0, t1
  li t2, 68
  add t2, sp, t2
  sw t0, (t2)
  li t2, 64
  add t2, sp, t2
  lw t0, (t2)
  li t2, 68
  add t2, sp, t2
  lw t1, (t2)
  slt t0, t0, t1
  li t2, 72
  add t2, sp, t2
  sw t0, (t2)
  li t2, 72
  add t2, sp, t2
  lw t0, (t2)
  bnez t0, STATION_2
  j while_exit_2
STATION_2:
  j while_body_2
while_body_2:
  li t2, 4
  add t2, sp, t2
  lw t0, (t2)
  li t2, 76
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 100
  add t0, t0, t1
  li t2, 80
  add t2, sp, t2
  sw t0, (t2)
  li t2, 76
  add t2, sp, t2
  lw t0, (t2)
  li t2, 80
  add t2, sp, t2
  lw t1, (t2)
  mul t0, t0, t1
  li t2, 84
  add t2, sp, t2
  sw t0, (t2)
  li t2, 24
  add t2, sp, t2
  lw t0, (t2)
  li t2, 88
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 10
  add t0, t0, t1
  li t2, 92
  add t2, sp, t2
  sw t0, (t2)
  li t2, 88
  add t2, sp, t2
  lw t0, (t2)
  li t2, 92
  add t2, sp, t2
  lw t1, (t2)
  mul t0, t0, t1
  li t2, 96
  add t2, sp, t2
  sw t0, (t2)
  li t2, 84
  add t2, sp, t2
  lw t0, (t2)
  li t2, 96
  add t2, sp, t2
  lw t1, (t2)
  add t0, t0, t1
  li t2, 100
  add t2, sp, t2
  sw t0, (t2)
  li t2, 44
  add t2, sp, t2
  lw t0, (t2)
  li t2, 104
  add t2, sp, t2
  sw t0, (t2)
  li t2, 100
  add t2, sp, t2
  lw t0, (t2)
  li t2, 104
  add t2, sp, t2
  lw t1, (t2)
  add t0, t0, t1
  li t2, 108
  add t2, sp, t2
  sw t0, (t2)
  li t2, 0
  add t2, sp, t2
  lw t0, (t2)
  li t2, 112
  add t2, sp, t2
  sw t0, (t2)
  li t2, 4
  add t2, sp, t2
  lw t0, (t2)
  li t2, 116
  add t2, sp, t2
  sw t0, (t2)
  li t2, 112
  add t0, sp, t2
  lw t0, (t0)
  li t2, 116
  add t2, sp, t2
  lw t1, (t2)
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t2, 120
  add t2, sp, t2
  sw t0, (t2)
  li t2, 24
  add t2, sp, t2
  lw t0, (t2)
  li t2, 124
  add t2, sp, t2
  sw t0, (t2)
  li t2, 120
  add t0, sp, t2
  lw t0, (t0)
  li t2, 124
  add t2, sp, t2
  lw t1, (t2)
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t2, 128
  add t2, sp, t2
  sw t0, (t2)
  li t2, 44
  add t2, sp, t2
  lw t0, (t2)
  li t2, 132
  add t2, sp, t2
  sw t0, (t2)
  li t2, 128
  add t0, sp, t2
  lw t0, (t0)
  li t2, 132
  add t2, sp, t2
  lw t1, (t2)
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t2, 136
  add t2, sp, t2
  sw t0, (t2)
  li t2, 108
  add t2, sp, t2
  lw t0, (t2)
  li t2, 136
  add t1, sp, t2
  lw t1, (t1)
  sw t0, (t1)
  li t2, 44
  add t2, sp, t2
  lw t0, (t2)
  li t2, 140
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 1
  add t0, t0, t1
  li t2, 144
  add t2, sp, t2
  sw t0, (t2)
  li t2, 140
  add t2, sp, t2
  lw t0, (t2)
  li t2, 144
  add t2, sp, t2
  lw t1, (t2)
  add t0, t0, t1
  li t2, 148
  add t2, sp, t2
  sw t0, (t2)
  li t2, 148
  add t2, sp, t2
  lw t0, (t2)
  li t2, 44
  add t1, sp, t2
  sw t0, (t1)
  j while_entry_2
while_exit_2:
  li t2, 24
  add t2, sp, t2
  lw t0, (t2)
  li t2, 152
  add t2, sp, t2
  sw t0, (t2)
  li t0, 0
  li t1, 1
  add t0, t0, t1
  li t2, 156
  add t2, sp, t2
  sw t0, (t2)
  li t2, 152
  add t2, sp, t2
  lw t0, (t2)
  li t2, 156
  add t2, sp, t2
  lw t1, (t2)
  add t0, t0, t1
  li t2, 160
  add t2, sp, t2
  sw t0, (t2)
  li t2, 160
  add t2, sp, t2
  lw t0, (t2)
  li t2, 24
  add t1, sp, t2
  sw t0, (t1)
  j while_entry_1

  .text
  .globl main
main:
  li t0, -4
  add t0, sp, t0
  sw ra, (t0)
  li t0, -4048
  add sp, sp, t0
  li t0, 0
  li t1, 0
  add t0, t0, t1
  li t2, 4004
  add t2, sp, t2
  sw t0, (t2)
  li t2, 4004
  add t2, sp, t2
  lw t0, (t2)
  li t2, 4000
  add t1, sp, t2
  sw t0, (t1)
  li t2, 0
  add t0, sp, t2
  li t1, 0
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t2, 4008
  add t2, sp, t2
  sw t0, (t2)
  li t2, 4008
  add t2, sp, t2
  lw a0, (t2)
  call init
  li t2, 4000
  add t2, sp, t2
  lw t0, (t2)
  li t2, 4012
  add t2, sp, t2
  sw t0, (t2)
  li t2, 4012
  add t2, sp, t2
  lw a0, (t2)
  call putint
  li t0, 0
  li t1, 10
  add t0, t0, t1
  li t2, 4016
  add t2, sp, t2
  sw t0, (t2)
  li t2, 4016
  add t2, sp, t2
  lw a0, (t2)
  call putch
  li t0, 0
  li t1, 0
  add t0, t0, t1
  li t2, 4020
  add t2, sp, t2
  sw t0, (t2)
  li t2, 4020
  add t2, sp, t2
  lw a0, (t2)
  li t0, 4044
  add t0, sp, t0
  lw ra, (t0)
  li t0, 4048
  add sp, t0, sp
  ret

