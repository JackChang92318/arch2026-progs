# op1, op2, op3 and q_mul: hand-written RV32I.
#
# uint32_t op1_asm(uint32_t a, uint32_t b, int *n)   a0, a1, a2 -> a0
# uint32_t op2_asm(uint32_t a, uint32_t b)           a0, a1 -> a0
# uint32_t op3_asm(uint32_t a, uint32_t b)           a0, a1 -> a0
# int32_t  q_mul_asm(int32_t a, int32_t b)           a0, a1 -> a0
#
# RV32I and Zicsr only: no mul, div, rem, no floating point, no compressed
# instructions. q_mul forms a 64-bit product in a register pair. Tests:
# tests/tfixed.c.

    .text

    .globl  op1_asm
    .type   op1_asm, @function
    .align  2
op1_asm:
    li     t1, 0
    beqz   a1, .Lop1_done
.Lop1_loop:
    and    t0, a0, a1
    xor    a0, a0, a1
    slli   a1, t0, 1
    addi   t1, t1, 1
    bnez   a1, .Lop1_loop
.Lop1_done:
    sw     t1, 0(a2)
    ret
    .size   op1_asm, .-op1_asm

    .globl  op2_asm
    .type   op2_asm, @function
    .align  2

op2_asm:
    sub     a0, a0, a1
    ret
    .size   op2_asm, .-op2_asm

    .globl  op3_asm
    .type   op3_asm, @function
    .align  2
op3_asm:
    and    t0, a0, a1
    xor    a0, a0, a1
    srli   a0, a0, 1
    add    a0, t0, a0
    ret
    .size   op3_asm, .-op3_asm

    .globl  q_mul_asm
    .type   q_mul_asm, @function
    .align  2
q_mul_asm:
    li    t0, 0
    li    t1, 0
    mv    t2, a0
    li    t3, 0
    mv    t4, a1
    beqz  t4, .Lq_done

.Lq_loop:
    andi  t5, t4, 1
    beqz  t5, .Lq_skip
    add   t0, t0, t2
    sltu  t6, t0, t2
    add   t1, t1, t3
    add   t1, t1, t6
.Lq_skip:
    srli  t5, t2, 31
    slli  t2, t2, 1
    slli  t3, t3, 1
    or    t3, t3, t5
    srli  t4, t4, 1
    bnez  t4, .Lq_loop
.Lq_done:
    bgez  a0, .Lq_bpos
    sub  t1, t1, a1
.Lq_bpos:
    bgez  a1, .Lq_round
    sub   t1, t1, a0
.Lq_round:
    srli  t0, t0, 19
    addi  t0, t0, 1
    srli  t0, t0, 1
    slli  t1, t1, 12
    add   a0, t1, t0
    ret
    .size   q_mul_asm, .-q_mul_asm
