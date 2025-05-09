.text
.globl main
main:
	addi   sp, sp, -12
	li    t0, 1
	sw   t0, 4(sp)
	bnez  t0, then0
	jal x0, 16
then0:
	li    t1, 1
	mv  a0, t1
	addi   sp, sp, 12
	ret
end0:
	li    t2, 0
	mv  a0, t2
	addi   sp, sp, 12
	ret
