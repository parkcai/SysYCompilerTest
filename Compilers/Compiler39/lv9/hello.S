  
.text
  .globl init
init:
  li t0, 224
  sub sp, sp, t0
  li t0, 212
  add t0, t0, sp
  li t1, 216
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 200
  add t0, t0, sp
  li t1, 204
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a0
  li t1, 200
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 184
  add t0, t0, sp
  li t1, 188
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 0
  li t1, 184
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  j while_entry_0

while_entry_0:
  li t0, 184
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 172
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 172
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 10
  slt t0, t0, t1
  li t1, 168
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 168
  add t0, sp, t0
  lw t0, 0(t0)
  bnez t0, jmpboard_0
  j end_0
jmpboard_0:
  j while_body_0

while_body_0:
  li t0, 152
  add t0, t0, sp
  li t1, 156
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 0
  li t1, 152
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  j while_entry_1

end_0:
  li t1, 224
  add sp, sp, t1
  ret

while_entry_1:
  li t0, 152
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 136
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 136
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 10
  slt t0, t0, t1
  li t1, 132
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 132
  add t0, sp, t0
  lw t0, 0(t0)
  bnez t0, jmpboard_1
  j end_1
jmpboard_1:
  j while_body_1

while_body_1:
  li t0, 116
  add t0, t0, sp
  li t1, 120
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 0
  li t1, 116
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  j while_entry_2

end_1:
  li t0, 184
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 104
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 104
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  add t0, t0, t1
  li t1, 100
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 100
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 184
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  j while_entry_0

while_entry_2:
  li t0, 116
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 88
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 88
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 10
  slt t0, t0, t1
  li t1, 84
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 84
  add t0, sp, t0
  lw t0, 0(t0)
  bnez t0, jmpboard_2
  j end_2
jmpboard_2:
  j while_body_2

while_body_2:
  li t0, 116
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 76
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 152
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 72
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 72
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 10
  mul t0, t0, t1
  li t1, 68
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 184
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 64
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 64
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 100
  mul t0, t0, t1
  li t1, 60
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 60
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 68
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, 56
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 56
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 76
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, 52
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 184
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 48
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 152
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 44
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 116
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 40
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 200
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 36
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 36
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 48
  add t1, sp, t1
  lw t1, 0(t1)
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 32
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 32
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 44
  add t1, sp, t1
  lw t1, 0(t1)
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 28
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 28
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 40
  add t1, sp, t1
  lw t1, 0(t1)
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 24
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 52
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 24
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 116
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 16
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 16
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  add t0, t0, t1
  li t1, 12
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 12
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 116
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  j while_entry_2

end_2:
  li t0, 152
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 0
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 0
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  add t0, t0, t1
  li t1, -4
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -4
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 152
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  j while_entry_1

  
.text
  .globl f1
f1:
  li t0, 288
  sub sp, sp, t0
  li t0, 276
  add t0, t0, sp
  li t1, 280
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 264
  add t0, t0, sp
  li t1, 268
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a0
  li t1, 264
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 248
  add t0, t0, sp
  li t1, 252
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a1
  li t1, 248
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 232
  add t0, t0, sp
  li t1, 236
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a2
  li t1, 232
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 216
  add t0, t0, sp
  li t1, 220
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a3
  li t1, 216
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 200
  add t0, t0, sp
  li t1, 204
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a4
  li t1, 200
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 184
  add t0, t0, sp
  li t1, 188
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a5
  li t1, 184
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 168
  add t0, t0, sp
  li t1, 172
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a6
  li t1, 168
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 152
  add t0, t0, sp
  li t1, 156
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a7
  li t1, 152
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 136
  add t0, t0, sp
  li t1, 140
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 288
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 136
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 120
  add t0, t0, sp
  li t1, 124
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 292
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 120
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 120
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 112
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 112
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 9
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 108
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 108
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 104
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 136
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 100
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 100
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 96
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 96
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 92
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 152
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 88
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 88
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 7
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 84
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 84
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 80
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 168
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 76
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 76
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 6
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 72
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 72
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 68
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 184
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 64
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 64
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 5
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 60
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 60
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 56
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 200
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 52
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 52
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 4
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 48
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 48
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 44
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 216
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 40
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 40
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 3
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 36
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 36
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 32
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 232
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 28
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 28
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 2
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 24
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 24
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 20
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 248
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 16
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 16
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 12
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 12
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 8
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 264
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 4
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 4
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 0
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 0
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, -4
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -4
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -8
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -8
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 20
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -12
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -12
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 32
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -16
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -16
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 44
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -20
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -20
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 56
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -24
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -24
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 68
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -28
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -28
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 80
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -32
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -32
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 92
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -36
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -36
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 104
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -40
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -40
  add t0, sp, t0
  lw t0, 0(t0)
  mv a0, t0
  li t1, 288
  add sp, sp, t1
  ret

  
.text
  .globl f2
f2:
  li t0, 288
  sub sp, sp, t0
  li t0, 276
  add t0, t0, sp
  li t1, 280
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 264
  add t0, t0, sp
  li t1, 268
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a0
  li t1, 264
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 248
  add t0, t0, sp
  li t1, 252
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a1
  li t1, 248
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 232
  add t0, t0, sp
  li t1, 236
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a2
  li t1, 232
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 216
  add t0, t0, sp
  li t1, 220
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a3
  li t1, 216
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 200
  add t0, t0, sp
  li t1, 204
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a4
  li t1, 200
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 184
  add t0, t0, sp
  li t1, 188
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a5
  li t1, 184
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 168
  add t0, t0, sp
  li t1, 172
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a6
  li t1, 168
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 152
  add t0, t0, sp
  li t1, 156
  add t1, t1, sp
  sw t1, 0(t0)

  mv t0, a7
  li t1, 152
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 136
  add t0, t0, sp
  li t1, 140
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 288
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 136
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 120
  add t0, t0, sp
  li t1, 124
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 292
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 120
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, 120
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 112
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 112
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 9
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 108
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 108
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 104
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 104
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 100
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 136
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 96
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 152
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 92
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 92
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 7
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 88
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 88
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 84
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 168
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 80
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 80
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 6
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 76
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 76
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 72
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 184
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 68
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 68
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 5
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 64
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 64
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 5
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 60
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 60
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 5
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 56
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 56
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 52
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 200
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 48
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 48
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 4
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 44
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 44
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 40
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 216
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 36
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 36
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 3
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 32
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 32
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 28
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 232
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 24
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 248
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 20
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 20
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 16
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 16
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 12
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 264
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, 8
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, 8
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 4
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 4
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 9
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, 0
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 0
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, -4
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -4
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 12
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -8
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -8
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 24
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -12
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -12
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 28
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -16
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -16
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 40
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -20
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -20
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 52
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -24
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -24
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 72
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -28
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -28
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 84
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -32
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -32
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 96
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -36
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -36
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 100
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -40
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -40
  add t0, sp, t0
  lw t0, 0(t0)
  mv a0, t0
  li t1, 288
  add sp, sp, t1
  ret

  
.text
  .globl main
main:
  li t0, 4336
  sub sp, sp, t0
  li t1, 4332
  add t1, sp, t1
  sw ra, 0(t1)
  li t0, 4320
  add t0, t0, sp
  li t1, 4324
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, -3684
  add t0, t0, sp
  li t1, -3680
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, -3696
  add t0, t0, sp
  li t1, -3692
  add t1, t1, sp
  sw t1, 0(t0)

  li t0, 0
  li t1, -3696
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3704
  add t1, t1, sp
  sw t0, 0(t1)
  mv t1, sp
  li t0, -3704
  add t0, sp, t0
  lw t0, 0(t0)
  mv a0, t0
  call init
  li t1, -3708
  add t1, sp, t1
  sw a0, 0(t1)

  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3712
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3712
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3716
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3716
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3720
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3724
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3724
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3728
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3728
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3732
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 2
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3736
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3736
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 2
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3740
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3740
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3744
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 3
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3748
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3748
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 3
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3752
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3752
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3756
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 4
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3760
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3760
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 4
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3764
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3764
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3768
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 5
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3772
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3772
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 5
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3776
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3776
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3780
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 6
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3784
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3784
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 6
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3788
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3788
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3792
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 7
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3796
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3796
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 7
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3800
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3800
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3804
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3808
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3808
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3812
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3812
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3816
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 9
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3820
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3820
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 9
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3824
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3824
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3828
  add t1, t1, sp
  sw t0, 0(t1)
  mv t1, sp
  li t0, -3720
  add t0, sp, t0
  lw t0, 0(t0)
  mv a0, t0
  li t0, -3732
  add t0, sp, t0
  lw t0, 0(t0)
  mv a1, t0
  li t0, -3744
  add t0, sp, t0
  lw t0, 0(t0)
  mv a2, t0
  li t0, -3756
  add t0, sp, t0
  lw t0, 0(t0)
  mv a3, t0
  li t0, -3768
  add t0, sp, t0
  lw t0, 0(t0)
  mv a4, t0
  li t0, -3780
  add t0, sp, t0
  lw t0, 0(t0)
  mv a5, t0
  li t0, -3792
  add t0, sp, t0
  lw t0, 0(t0)
  mv a6, t0
  li t0, -3804
  add t0, sp, t0
  lw t0, 0(t0)
  mv a7, t0
  li t0, -3816
  add t0, sp, t0
  lw t0, 0(t0)
  sw t0, 0(t1)
  addi t1, t1, 4
  li t0, -3828
  add t0, sp, t0
  lw t0, 0(t0)
  sw t0, 0(t1)
  addi t1, t1, 4
  call f1
  li t1, -3832
  add t1, sp, t1
  sw a0, 0(t1)

  li t0, -3696
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, -3836
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -3836
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, -3832
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -3840
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -3840
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, -3696
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3848
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3848
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3852
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3856
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3856
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 1
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3860
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3860
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3864
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 2
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3868
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3868
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 2
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3872
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3872
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 2
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3876
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3876
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, -3880
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 3
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3884
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3884
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 3
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3888
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3888
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3892
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 4
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3896
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3896
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 4
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3900
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3900
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3904
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3908
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 6
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3912
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3912
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 6
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3916
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3916
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3920
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 7
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3924
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3924
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 7
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3928
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3928
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3932
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3936
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3936
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3940
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3940
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 8
  li t2, 4
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3944
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3944
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, -3948
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -3684
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 9
  li t2, 400
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3952
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, -3952
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, 0
  li t2, 40
  mul t1, t1, t2
  add t0, t0, t1
  li t1, -3956
  add t1, t1, sp
  sw t0, 0(t1)
  mv t1, sp
  li t0, -3852
  add t0, sp, t0
  lw t0, 0(t0)
  mv a0, t0
  li t0, -3864
  add t0, sp, t0
  lw t0, 0(t0)
  mv a1, t0
  li t0, -3880
  add t0, sp, t0
  lw t0, 0(t0)
  mv a2, t0
  li t0, -3892
  add t0, sp, t0
  lw t0, 0(t0)
  mv a3, t0
  li t0, -3904
  add t0, sp, t0
  lw t0, 0(t0)
  mv a4, t0
  li t0, -3908
  add t0, sp, t0
  lw t0, 0(t0)
  mv a5, t0
  li t0, -3920
  add t0, sp, t0
  lw t0, 0(t0)
  mv a6, t0
  li t0, -3932
  add t0, sp, t0
  lw t0, 0(t0)
  mv a7, t0
  li t0, -3948
  add t0, sp, t0
  lw t0, 0(t0)
  sw t0, 0(t1)
  addi t1, t1, 4
  li t0, -3956
  add t0, sp, t0
  lw t0, 0(t0)
  sw t0, 0(t1)
  addi t1, t1, 4
  call f2
  li t1, -3960
  add t1, sp, t1
  sw a0, 0(t1)

  li t0, -3696
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, -3964
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -3964
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, -3960
  add t1, sp, t1
  lw t1, 0(t1)
  add t0, t0, t1
  li t1, -3968
  add t1, sp, t1
  sw t0, 0(t1)

  li t0, -3968
  add t0, sp, t0
  lw t0, 0(t0)
  li t1, -3696
  add t1, sp, t1
  lw t1, 0(t1)
  sw t0, 0(t1)

  li t0, -3696
  add t0, sp, t0
  lw t0, 0(t0)
  lw t0, 0(t0)
  li t1, -3976
  add t1, sp, t1
  sw t0, 0(t1)

  mv t1, sp
  li t0, -3976
  add t0, sp, t0
  lw t0, 0(t0)
  mv a0, t0
  call putint
  li t1, -3980
  add t1, sp, t1
  sw a0, 0(t1)

  mv t1, sp
  li t0, 10
  mv a0, t0
  call putch
  li t1, -3984
  add t1, sp, t1
  sw a0, 0(t1)

  li t0, 0
  mv a0, t0
  li t1, 4332
  add t1, sp, t1
  lw ra, 0(t1)
  li t1, 4336
  add sp, sp, t1
  ret

