    .data
    .text
    .global init
init:
    sw ra, -4(sp)
    li t0, -224
    add sp, sp, t0
init_Block0:
    li t0, 0
    add t0, t0, sp
    sw t0, 4(sp)
    mv t0, a0
    lw t1, 4(sp)
    sw t0, 0(t1)
    li t0, 12
    add t0, t0, sp
    sw t0, 16(sp)
    li t0, 0
    lw t1, 16(sp)
    sw t0, 0(t1)
    j init_Block1
init_Block1:
    lw t0, 16(sp)
    lw t0, 0(t0)
    sw t0, 28(sp)
    lw t0, 28(sp)
    li t1, 10
    slt t0, t0, t1
    sw t0, 32(sp)
    lw t0, 32(sp)
    beqz t0, init_Block3
    j init_Block2
init_Block2:
    li t0, 40
    add t0, t0, sp
    sw t0, 44(sp)
    li t0, 0
    lw t1, 44(sp)
    sw t0, 0(t1)
    j init_Block4
init_Block3:
    li t0, 224
    add sp, sp, t0
    lw ra, -4(sp)
    ret
init_Block4:
    lw t0, 44(sp)
    lw t0, 0(t0)
    sw t0, 60(sp)
    lw t0, 60(sp)
    li t1, 10
    slt t0, t0, t1
    sw t0, 64(sp)
    lw t0, 64(sp)
    beqz t0, init_Block6
    j init_Block5
init_Block5:
    li t0, 72
    add t0, t0, sp
    sw t0, 76(sp)
    li t0, 0
    lw t1, 76(sp)
    sw t0, 0(t1)
    j init_Block7
init_Block6:
    lw t0, 16(sp)
    lw t0, 0(t0)
    sw t0, 88(sp)
    lw t0, 88(sp)
    li t1, 1
    add t0, t0, t1
    sw t0, 92(sp)
    lw t0, 92(sp)
    lw t1, 16(sp)
    sw t0, 0(t1)
    j init_Block1
init_Block7:
    lw t0, 76(sp)
    lw t0, 0(t0)
    sw t0, 104(sp)
    lw t0, 104(sp)
    li t1, 10
    slt t0, t0, t1
    sw t0, 108(sp)
    lw t0, 108(sp)
    beqz t0, init_Block9
    j init_Block8
init_Block8:
    lw t0, 16(sp)
    lw t0, 0(t0)
    sw t0, 116(sp)
    lw t0, 116(sp)
    li t1, 100
    mul t0, t0, t1
    sw t0, 120(sp)
    lw t0, 44(sp)
    lw t0, 0(t0)
    sw t0, 124(sp)
    lw t0, 124(sp)
    li t1, 10
    mul t0, t0, t1
    sw t0, 128(sp)
    lw t0, 120(sp)
    lw t1, 128(sp)
    add t0, t0, t1
    sw t0, 132(sp)
    lw t0, 76(sp)
    lw t0, 0(t0)
    sw t0, 136(sp)
    lw t0, 132(sp)
    lw t1, 136(sp)
    add t0, t0, t1
    sw t0, 140(sp)
    lw t0, 4(sp)
    lw t0, 0(t0)
    sw t0, 144(sp)
    lw t0, 16(sp)
    lw t0, 0(t0)
    sw t0, 148(sp)
    lw t0, 148(sp)
    li t1, 400
    mul t0, t0, t1
    lw t1, 144(sp)
    add t0, t0, t1
    sw t0, 152(sp)
    lw t0, 44(sp)
    lw t0, 0(t0)
    sw t0, 156(sp)
    lw t0, 156(sp)
    li t1, 40
    mul t0, t0, t1
    lw t1, 152(sp)
    add t0, t0, t1
    sw t0, 160(sp)
    lw t0, 76(sp)
    lw t0, 0(t0)
    sw t0, 164(sp)
    lw t0, 164(sp)
    li t1, 4
    mul t0, t0, t1
    lw t1, 160(sp)
    add t0, t0, t1
    sw t0, 168(sp)
    lw t0, 140(sp)
    lw t1, 168(sp)
    sw t0, 0(t1)
    lw t0, 76(sp)
    lw t0, 0(t0)
    sw t0, 176(sp)
    lw t0, 176(sp)
    li t1, 1
    add t0, t0, t1
    sw t0, 180(sp)
    lw t0, 180(sp)
    lw t1, 76(sp)
    sw t0, 0(t1)
    j init_Block7
init_Block9:
    lw t0, 44(sp)
    lw t0, 0(t0)
    sw t0, 192(sp)
    lw t0, 192(sp)
    li t1, 1
    add t0, t0, t1
    sw t0, 196(sp)
    lw t0, 196(sp)
    lw t1, 44(sp)
    sw t0, 0(t1)
    j init_Block4
    .global f1
f1:
    sw ra, -4(sp)
    li t0, -288
    add sp, sp, t0
f1_Block10:
    li t0, 0
    add t0, t0, sp
    sw t0, 4(sp)
    lw t0, 292(sp)
    lw t1, 4(sp)
    sw t0, 0(t1)
    li t0, 12
    add t0, t0, sp
    sw t0, 16(sp)
    mv t0, a5
    lw t1, 16(sp)
    sw t0, 0(t1)
    li t0, 24
    add t0, t0, sp
    sw t0, 28(sp)
    mv t0, a4
    lw t1, 28(sp)
    sw t0, 0(t1)
    li t0, 36
    add t0, t0, sp
    sw t0, 40(sp)
    mv t0, a3
    lw t1, 40(sp)
    sw t0, 0(t1)
    li t0, 48
    add t0, t0, sp
    sw t0, 52(sp)
    mv t0, a6
    lw t1, 52(sp)
    sw t0, 0(t1)
    li t0, 60
    add t0, t0, sp
    sw t0, 64(sp)
    mv t0, a2
    lw t1, 64(sp)
    sw t0, 0(t1)
    li t0, 72
    add t0, t0, sp
    sw t0, 76(sp)
    mv t0, a1
    lw t1, 76(sp)
    sw t0, 0(t1)
    li t0, 84
    add t0, t0, sp
    sw t0, 88(sp)
    lw t0, 288(sp)
    lw t1, 88(sp)
    sw t0, 0(t1)
    li t0, 96
    add t0, t0, sp
    sw t0, 100(sp)
    mv t0, a7
    lw t1, 100(sp)
    sw t0, 0(t1)
    li t0, 108
    add t0, t0, sp
    sw t0, 112(sp)
    mv t0, a0
    lw t1, 112(sp)
    sw t0, 0(t1)
    lw t0, 112(sp)
    lw t0, 0(t0)
    sw t0, 120(sp)
    li t0, 0
    li t1, 4
    mul t0, t0, t1
    lw t1, 120(sp)
    add t0, t0, t1
    sw t0, 124(sp)
    lw t0, 124(sp)
    lw t0, 0(t0)
    sw t0, 128(sp)
    lw t0, 76(sp)
    lw t0, 0(t0)
    sw t0, 132(sp)
    li t0, 1
    li t1, 4
    mul t0, t0, t1
    lw t1, 132(sp)
    add t0, t0, t1
    sw t0, 136(sp)
    lw t0, 136(sp)
    lw t0, 0(t0)
    sw t0, 140(sp)
    lw t0, 128(sp)
    lw t1, 140(sp)
    add t0, t0, t1
    sw t0, 144(sp)
    lw t0, 64(sp)
    lw t0, 0(t0)
    sw t0, 148(sp)
    li t0, 2
    li t1, 4
    mul t0, t0, t1
    lw t1, 148(sp)
    add t0, t0, t1
    sw t0, 152(sp)
    lw t0, 152(sp)
    lw t0, 0(t0)
    sw t0, 156(sp)
    lw t0, 144(sp)
    lw t1, 156(sp)
    add t0, t0, t1
    sw t0, 160(sp)
    lw t0, 40(sp)
    lw t0, 0(t0)
    sw t0, 164(sp)
    li t0, 3
    li t1, 4
    mul t0, t0, t1
    lw t1, 164(sp)
    add t0, t0, t1
    sw t0, 168(sp)
    lw t0, 168(sp)
    lw t0, 0(t0)
    sw t0, 172(sp)
    lw t0, 160(sp)
    lw t1, 172(sp)
    add t0, t0, t1
    sw t0, 176(sp)
    lw t0, 28(sp)
    lw t0, 0(t0)
    sw t0, 180(sp)
    li t0, 4
    li t1, 4
    mul t0, t0, t1
    lw t1, 180(sp)
    add t0, t0, t1
    sw t0, 184(sp)
    lw t0, 184(sp)
    lw t0, 0(t0)
    sw t0, 188(sp)
    lw t0, 176(sp)
    lw t1, 188(sp)
    add t0, t0, t1
    sw t0, 192(sp)
    lw t0, 16(sp)
    lw t0, 0(t0)
    sw t0, 196(sp)
    li t0, 5
    li t1, 4
    mul t0, t0, t1
    lw t1, 196(sp)
    add t0, t0, t1
    sw t0, 200(sp)
    lw t0, 200(sp)
    lw t0, 0(t0)
    sw t0, 204(sp)
    lw t0, 192(sp)
    lw t1, 204(sp)
    add t0, t0, t1
    sw t0, 208(sp)
    lw t0, 52(sp)
    lw t0, 0(t0)
    sw t0, 212(sp)
    li t0, 6
    li t1, 4
    mul t0, t0, t1
    lw t1, 212(sp)
    add t0, t0, t1
    sw t0, 216(sp)
    lw t0, 216(sp)
    lw t0, 0(t0)
    sw t0, 220(sp)
    lw t0, 208(sp)
    lw t1, 220(sp)
    add t0, t0, t1
    sw t0, 224(sp)
    lw t0, 100(sp)
    lw t0, 0(t0)
    sw t0, 228(sp)
    li t0, 7
    li t1, 4
    mul t0, t0, t1
    lw t1, 228(sp)
    add t0, t0, t1
    sw t0, 232(sp)
    lw t0, 232(sp)
    lw t0, 0(t0)
    sw t0, 236(sp)
    lw t0, 224(sp)
    lw t1, 236(sp)
    add t0, t0, t1
    sw t0, 240(sp)
    lw t0, 88(sp)
    lw t0, 0(t0)
    sw t0, 244(sp)
    li t0, 8
    li t1, 4
    mul t0, t0, t1
    lw t1, 244(sp)
    add t0, t0, t1
    sw t0, 248(sp)
    lw t0, 248(sp)
    lw t0, 0(t0)
    sw t0, 252(sp)
    lw t0, 240(sp)
    lw t1, 252(sp)
    add t0, t0, t1
    sw t0, 256(sp)
    lw t0, 4(sp)
    lw t0, 0(t0)
    sw t0, 260(sp)
    li t0, 9
    li t1, 4
    mul t0, t0, t1
    lw t1, 260(sp)
    add t0, t0, t1
    sw t0, 264(sp)
    lw t0, 264(sp)
    lw t0, 0(t0)
    sw t0, 268(sp)
    lw t0, 256(sp)
    lw t1, 268(sp)
    add t0, t0, t1
    sw t0, 272(sp)
    lw a0, 272(sp)
    li t0, 288
    add sp, sp, t0
    lw ra, -4(sp)
    ret
    .global f2
f2:
    sw ra, -4(sp)
    li t0, -288
    add sp, sp, t0
f2_Block12:
    li t0, 0
    add t0, t0, sp
    sw t0, 4(sp)
    lw t0, 292(sp)
    lw t1, 4(sp)
    sw t0, 0(t1)
    li t0, 12
    add t0, t0, sp
    sw t0, 16(sp)
    mv t0, a5
    lw t1, 16(sp)
    sw t0, 0(t1)
    li t0, 24
    add t0, t0, sp
    sw t0, 28(sp)
    mv t0, a4
    lw t1, 28(sp)
    sw t0, 0(t1)
    li t0, 36
    add t0, t0, sp
    sw t0, 40(sp)
    mv t0, a3
    lw t1, 40(sp)
    sw t0, 0(t1)
    li t0, 48
    add t0, t0, sp
    sw t0, 52(sp)
    mv t0, a6
    lw t1, 52(sp)
    sw t0, 0(t1)
    li t0, 60
    add t0, t0, sp
    sw t0, 64(sp)
    mv t0, a2
    lw t1, 64(sp)
    sw t0, 0(t1)
    li t0, 72
    add t0, t0, sp
    sw t0, 76(sp)
    mv t0, a1
    lw t1, 76(sp)
    sw t0, 0(t1)
    li t0, 84
    add t0, t0, sp
    sw t0, 88(sp)
    lw t0, 288(sp)
    lw t1, 88(sp)
    sw t0, 0(t1)
    li t0, 96
    add t0, t0, sp
    sw t0, 100(sp)
    mv t0, a7
    lw t1, 100(sp)
    sw t0, 0(t1)
    li t0, 108
    add t0, t0, sp
    sw t0, 112(sp)
    mv t0, a0
    lw t1, 112(sp)
    sw t0, 0(t1)
    lw t0, 112(sp)
    lw t0, 0(t0)
    sw t0, 120(sp)
    li t0, 0
    li t1, 40
    mul t0, t0, t1
    lw t1, 120(sp)
    add t0, t0, t1
    sw t0, 124(sp)
    li t0, 9
    li t1, 4
    mul t0, t0, t1
    lw t1, 124(sp)
    add t0, t0, t1
    sw t0, 128(sp)
    lw t0, 128(sp)
    lw t0, 0(t0)
    sw t0, 132(sp)
    lw t0, 76(sp)
    lw t0, 0(t0)
    sw t0, 136(sp)
    li t0, 1
    li t1, 4
    mul t0, t0, t1
    lw t1, 136(sp)
    add t0, t0, t1
    sw t0, 140(sp)
    lw t0, 140(sp)
    lw t0, 0(t0)
    sw t0, 144(sp)
    lw t0, 132(sp)
    lw t1, 144(sp)
    add t0, t0, t1
    sw t0, 148(sp)
    lw t0, 64(sp)
    lw t0, 0(t0)
    sw t0, 152(sp)
    lw t0, 148(sp)
    lw t1, 152(sp)
    add t0, t0, t1
    sw t0, 156(sp)
    lw t0, 40(sp)
    lw t0, 0(t0)
    sw t0, 160(sp)
    li t0, 3
    li t1, 4
    mul t0, t0, t1
    lw t1, 160(sp)
    add t0, t0, t1
    sw t0, 164(sp)
    lw t0, 164(sp)
    lw t0, 0(t0)
    sw t0, 168(sp)
    lw t0, 156(sp)
    lw t1, 168(sp)
    add t0, t0, t1
    sw t0, 172(sp)
    lw t0, 28(sp)
    lw t0, 0(t0)
    sw t0, 176(sp)
    li t0, 4
    li t1, 4
    mul t0, t0, t1
    lw t1, 176(sp)
    add t0, t0, t1
    sw t0, 180(sp)
    lw t0, 180(sp)
    lw t0, 0(t0)
    sw t0, 184(sp)
    lw t0, 172(sp)
    lw t1, 184(sp)
    add t0, t0, t1
    sw t0, 188(sp)
    lw t0, 16(sp)
    lw t0, 0(t0)
    sw t0, 192(sp)
    li t0, 5
    li t1, 400
    mul t0, t0, t1
    lw t1, 192(sp)
    add t0, t0, t1
    sw t0, 196(sp)
    li t0, 5
    li t1, 40
    mul t0, t0, t1
    lw t1, 196(sp)
    add t0, t0, t1
    sw t0, 200(sp)
    li t0, 5
    li t1, 4
    mul t0, t0, t1
    lw t1, 200(sp)
    add t0, t0, t1
    sw t0, 204(sp)
    lw t0, 204(sp)
    lw t0, 0(t0)
    sw t0, 208(sp)
    lw t0, 188(sp)
    lw t1, 208(sp)
    add t0, t0, t1
    sw t0, 212(sp)
    lw t0, 52(sp)
    lw t0, 0(t0)
    sw t0, 216(sp)
    li t0, 6
    li t1, 4
    mul t0, t0, t1
    lw t1, 216(sp)
    add t0, t0, t1
    sw t0, 220(sp)
    lw t0, 220(sp)
    lw t0, 0(t0)
    sw t0, 224(sp)
    lw t0, 212(sp)
    lw t1, 224(sp)
    add t0, t0, t1
    sw t0, 228(sp)
    lw t0, 100(sp)
    lw t0, 0(t0)
    sw t0, 232(sp)
    li t0, 7
    li t1, 4
    mul t0, t0, t1
    lw t1, 232(sp)
    add t0, t0, t1
    sw t0, 236(sp)
    lw t0, 236(sp)
    lw t0, 0(t0)
    sw t0, 240(sp)
    lw t0, 228(sp)
    lw t1, 240(sp)
    add t0, t0, t1
    sw t0, 244(sp)
    lw t0, 88(sp)
    lw t0, 0(t0)
    sw t0, 248(sp)
    lw t0, 244(sp)
    lw t1, 248(sp)
    add t0, t0, t1
    sw t0, 252(sp)
    lw t0, 4(sp)
    lw t0, 0(t0)
    sw t0, 256(sp)
    li t0, 9
    li t1, 40
    mul t0, t0, t1
    lw t1, 256(sp)
    add t0, t0, t1
    sw t0, 260(sp)
    li t0, 8
    li t1, 4
    mul t0, t0, t1
    lw t1, 260(sp)
    add t0, t0, t1
    sw t0, 264(sp)
    lw t0, 264(sp)
    lw t0, 0(t0)
    sw t0, 268(sp)
    lw t0, 252(sp)
    lw t1, 268(sp)
    add t0, t0, t1
    sw t0, 272(sp)
    lw a0, 272(sp)
    li t0, 288
    add sp, sp, t0
    lw ra, -4(sp)
    ret
    .global f3
f3:
    sw ra, -4(sp)
    li t0, -192
    add sp, sp, t0
f3_Block14:
    li t0, 0
    add t0, t0, sp
    sw t0, 4(sp)
    mv t0, a6
    lw t1, 4(sp)
    sw t0, 0(t1)
    li t0, 12
    add t0, t0, sp
    sw t0, 16(sp)
    mv t0, a5
    lw t1, 16(sp)
    sw t0, 0(t1)
    li t0, 24
    add t0, t0, sp
    sw t0, 28(sp)
    lw t0, 192(sp)
    lw t1, 28(sp)
    sw t0, 0(t1)
    li t0, 36
    add t0, t0, sp
    sw t0, 40(sp)
    mv t0, a4
    lw t1, 40(sp)
    sw t0, 0(t1)
    li t0, 48
    add t0, t0, sp
    sw t0, 52(sp)
    mv t0, a2
    lw t1, 52(sp)
    sw t0, 0(t1)
    li t0, 60
    add t0, t0, sp
    sw t0, 64(sp)
    mv t0, a7
    lw t1, 64(sp)
    sw t0, 0(t1)
    li t0, 72
    add t0, t0, sp
    sw t0, 76(sp)
    mv t0, a1
    lw t1, 76(sp)
    sw t0, 0(t1)
    li t0, 84
    add t0, t0, sp
    sw t0, 88(sp)
    mv t0, a3
    lw t1, 88(sp)
    sw t0, 0(t1)
    li t0, 96
    add t0, t0, sp
    sw t0, 100(sp)
    mv t0, a0
    lw t1, 100(sp)
    sw t0, 0(t1)
    lw t0, 100(sp)
    lw t0, 0(t0)
    sw t0, 108(sp)
    lw t0, 76(sp)
    lw t0, 0(t0)
    sw t0, 112(sp)
    lw t0, 108(sp)
    lw t1, 112(sp)
    add t0, t0, t1
    sw t0, 116(sp)
    lw t0, 52(sp)
    lw t0, 0(t0)
    sw t0, 120(sp)
    lw t0, 116(sp)
    lw t1, 120(sp)
    add t0, t0, t1
    sw t0, 124(sp)
    lw t0, 88(sp)
    lw t0, 0(t0)
    sw t0, 128(sp)
    lw t0, 124(sp)
    lw t1, 128(sp)
    add t0, t0, t1
    sw t0, 132(sp)
    lw t0, 40(sp)
    lw t0, 0(t0)
    sw t0, 136(sp)
    lw t0, 132(sp)
    lw t1, 136(sp)
    add t0, t0, t1
    sw t0, 140(sp)
    lw t0, 16(sp)
    lw t0, 0(t0)
    sw t0, 144(sp)
    lw t0, 140(sp)
    lw t1, 144(sp)
    add t0, t0, t1
    sw t0, 148(sp)
    lw t0, 4(sp)
    lw t0, 0(t0)
    sw t0, 152(sp)
    lw t0, 148(sp)
    lw t1, 152(sp)
    add t0, t0, t1
    sw t0, 156(sp)
    lw t0, 64(sp)
    lw t0, 0(t0)
    sw t0, 160(sp)
    lw t0, 156(sp)
    lw t1, 160(sp)
    add t0, t0, t1
    sw t0, 164(sp)
    lw t0, 28(sp)
    lw t0, 0(t0)
    sw t0, 168(sp)
    lw t0, 164(sp)
    lw t1, 168(sp)
    add t0, t0, t1
    sw t0, 172(sp)
    lw a0, 172(sp)
    li t0, 192
    add sp, sp, t0
    lw ra, -4(sp)
    ret
    .global main
main:
    sw ra, -4(sp)
    li t0, -4176
    add sp, sp, t0
main_Block16:
    li t0, 0
    add t0, t0, sp
    li t1, 4000
    add t1, t1, sp
    sw t0, 0(t1)
    li t0, 4004
    add t0, t0, sp
    li t1, 4008
    add t1, t1, sp
    sw t0, 0(t1)
    li t0, 0
    li t6, 4008
    add t6, t6, sp
    lw t1, 0(t6)
    sw t0, 0(t1)
    li t0, 0
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4016
    add t6, t6, sp
    sw t0, 0(t6)
    li t6, 4016
    add t6, t6, sp
    lw a0, 0(t6)
    call init
    li t6, 4020
    add t6, t6, sp
    sw a0, 0(t6)
    li t6, 4008
    add t6, t6, sp
    lw t0, 0(t6)
    lw t0, 0(t0)
    li t6, 4024
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4028
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 40
    mul t0, t0, t1
    li t6, 4028
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4032
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 1
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4036
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 1
    li t1, 40
    mul t0, t0, t1
    li t6, 4036
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4040
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 4
    mul t0, t0, t1
    li t6, 4040
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4044
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 2
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4048
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 2
    li t1, 40
    mul t0, t0, t1
    li t6, 4048
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4052
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 2
    li t1, 4
    mul t0, t0, t1
    li t6, 4052
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4056
    add t6, t6, sp
    sw t0, 0(t6)
    li t6, 4056
    add t6, t6, sp
    lw t0, 0(t6)
    lw t0, 0(t0)
    li t6, 4060
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 3
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4064
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 3
    li t1, 40
    mul t0, t0, t1
    li t6, 4064
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4068
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 4
    mul t0, t0, t1
    li t6, 4068
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4072
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 4
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4076
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 4
    li t1, 40
    mul t0, t0, t1
    li t6, 4076
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4080
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 4
    mul t0, t0, t1
    li t6, 4080
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4084
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4088
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 6
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4092
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 6
    li t1, 40
    mul t0, t0, t1
    li t6, 4092
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4096
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 4
    mul t0, t0, t1
    li t6, 4096
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4100
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 7
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4104
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 7
    li t1, 40
    mul t0, t0, t1
    li t6, 4104
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4108
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 4
    mul t0, t0, t1
    li t6, 4108
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4112
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 8
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4116
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 8
    li t1, 40
    mul t0, t0, t1
    li t6, 4116
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4120
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 8
    li t1, 4
    mul t0, t0, t1
    li t6, 4120
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4124
    add t6, t6, sp
    sw t0, 0(t6)
    li t6, 4124
    add t6, t6, sp
    lw t0, 0(t6)
    lw t0, 0(t0)
    li t6, 4128
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 9
    li t1, 400
    mul t0, t0, t1
    li t6, 4000
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4132
    add t6, t6, sp
    sw t0, 0(t6)
    li t0, 0
    li t1, 40
    mul t0, t0, t1
    li t6, 4132
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4136
    add t6, t6, sp
    sw t0, 0(t6)
    li t6, 4032
    add t6, t6, sp
    lw a0, 0(t6)
    li t6, 4044
    add t6, t6, sp
    lw a1, 0(t6)
    li t6, 4060
    add t6, t6, sp
    lw a2, 0(t6)
    li t6, 4072
    add t6, t6, sp
    lw a3, 0(t6)
    li t6, 4084
    add t6, t6, sp
    lw a4, 0(t6)
    li t6, 4088
    add t6, t6, sp
    lw a5, 0(t6)
    li t6, 4100
    add t6, t6, sp
    lw a6, 0(t6)
    li t6, 4112
    add t6, t6, sp
    lw a7, 0(t6)
    addi sp, sp, -8
    li t6, 4128
    add t6, t6, sp
    lw t0, 0(t6)
    sw t0, 0(sp)
    li t6, 4136
    add t6, t6, sp
    lw t0, 0(t6)
    sw t0, 4(sp)
    call f2
    addi sp, sp, 8
    li t6, 4140
    add t6, t6, sp
    sw a0, 0(t6)
    li t6, 4024
    add t6, t6, sp
    lw t0, 0(t6)
    li t6, 4140
    add t6, t6, sp
    lw t1, 0(t6)
    add t0, t0, t1
    li t6, 4144
    add t6, t6, sp
    sw t0, 0(t6)
    li t6, 4144
    add t6, t6, sp
    lw t0, 0(t6)
    li t6, 4008
    add t6, t6, sp
    lw t1, 0(t6)
    sw t0, 0(t1)
    li t6, 4008
    add t6, t6, sp
    lw t0, 0(t6)
    lw t0, 0(t0)
    li t6, 4152
    add t6, t6, sp
    sw t0, 0(t6)
    li t6, 4152
    add t6, t6, sp
    lw a0, 0(t6)
    call putint
    li t6, 4156
    add t6, t6, sp
    sw a0, 0(t6)
    li a0, 10
    call putch
    li t6, 4160
    add t6, t6, sp
    sw a0, 0(t6)
    li a0, 0
    li t0, 4176
    add sp, sp, t0
    lw ra, -4(sp)
    ret

