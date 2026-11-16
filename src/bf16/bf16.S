# BFloat16 routines: hand-written RV32I.
#
# uint16_t bf16_from_f32_asm(uint32_t f32bits)   a0 -> a0
# uint16_t bf16_mul_asm(uint16_t a, uint16_t b)  a0, a1 -> a0
# uint16_t bf16_sqrt_asm(uint16_t a)             a0 -> a0
#
# RV32I and Zicsr only: no mul, div, rem, no floating point, no compressed
# instructions. Tests: tests/tbf16.c.

    .text

    .globl  bf16_from_f32_asm
    .type   bf16_from_f32_asm, @function
    .align  2
bf16_from_f32_asm:
    lui  t1, 0x7F800
    and  t0, a0, t1
    beq  t0, t1, .Lf32_top
    srli t0, a0, 16
    andi t0, t0, 1
    li   t1, 0x7FFF
    add  t0, t0, t1
    add  a0, a0, t0
.Lf32_top:
    srli a0, a0, 16
    ret
    .size   bf16_from_f32_asm, .-bf16_from_f32_asm

    .globl  bf16_mul_asm
    .type   bf16_mul_asm, @function
    .align  2
bf16_mul_asm:
    xor  t0, a0, a1
    lui  t6, 8
    and  t0, t0, t6
    srli t1, a0, 7
    andi t1, t1, 0xFF
    srli t2, a1, 7
    andi t2, t2, 0xFF
    andi t3, a0, 0x7F
    andi t4, a1, 0x7F
    li   t5, 0xFF
    beq  t1, t5, .Lm_a_special
    beq  t2, t5, .Lm_b_special
    or   t6, t1, t3
    beqz t6, .Lm_zero
    or   t6, t2, t4
    beqz t6, .Lm_zero
    beqz t1, .Lm_a_den
    ori  t3, t3, 0x80
.Lm_b:
    beqz t2, .Lm_b_den
    ori  t4, t4, 0x80
.Lm_mul_start:
    li   a2, 0
.Lm_mul:
    andi t6, t4, 1
    beqz t6, .Lm_mul_skip
    add  a2, a2, t3
.Lm_mul_skip:
    slli t3, t3, 1
    srli t4, t4, 1
    bnez t4, .Lm_mul
    add  t1, t1, t2
    addi t1, t1, -127
    srli t6, a2, 15
    srl  a2, a2, t6
    srli a2, a2, 7
    andi a2, a2, 0x7F
    add  t1, t1, t6
    bge  t1, t5, .Lm_inf
    blez t1, .Lm_small
    slli t1, t1, 7
    or   a0, t0, t1
    or   a0, a0, a2
    ret
.Lm_small:
    li   t6, -6
    blt  t1, t6, .Lm_zero
    li   t6, 1
    sub  t6, t6, t1
    srl  a2, a2, t6
    or   a0, t0, a2
    ret
.Lm_zero:
    mv   a0, t0
    ret
.Lm_a_den:
    li   t1, 1
.Lm_a_loop:
    slli t3, t3, 1
    addi t1, t1, -1
    andi t6, t3, 0x80
    beqz t6, .Lm_a_loop
    j    .Lm_b
.Lm_b_den:
    li   t2, 1
.Lm_b_loop:
    slli t4, t4, 1
    addi t2, t2, -1
    andi t6, t4, 0x80
    beqz t6, .Lm_b_loop
    j    .Lm_mul_start
.Lm_a_special:
    bnez t3, .Lm_ret
    or   t6, t2, t4
    beqz t6, .Lm_qnan
    j    .Lm_inf
.Lm_b_special:
    mv   a0, a1
    bnez t4, .Lm_ret
    or   t6, t1, t3
    beqz t6, .Lm_qnan
.Lm_inf:
    li   t6, 0x7F80
    or   a0, t0, t6
.Lm_ret:
    ret
.Lm_qnan:
    li   a0, 0x7FC0
    ret
    .size   bf16_mul_asm, .-bf16_mul_asm

    .globl  bf16_sqrt_asm
    .type   bf16_sqrt_asm, @function
    .align  2
bf16_sqrt_asm:
    srli  t0, a0, 7
    andi  t0, t0, 0xFF
    andi  t1, a0, 0x7F
    li    t2, 0xFF
    beq   t0, t2, .Ls_special
    or    t3, t0, t1
    beqz  t3, .Ls_zero
    srli  t3, a0, 15
    bnez  t3, .Ls_qnan
    beqz  t0, .Ls_zero

    addi  t0, t0, -127
    ori   t1, t1, 0x80
    andi  t3, t0, 1
    beqz  t3, .Ls_even
    slli  t1, t1, 1
    addi  t0, t0, -1
.Ls_even:
    srai  t0, t0, 1
    addi  t0, t0, 127
    slli  t1, t1, 7
    addi  t1, t1, 127
    li    t2, 128
    li    t3, 16384
    mv    t4, t3
    li    t5, 64
    li    t6, 4096
.Ls_loop:
    add   a1, t3, t4
    add   a1, a1, t6
    bltu  t1, a1, .Ls_next
    mv    t3, a1
    add   t2, t2, t5
    slli  a2, t6, 1
    add   t4, t4, a2
.Ls_next:
    srli  t5, t5, 1
    srli  t6, t6, 2
    srli  t4, t4, 1
    bnez  t5, .Ls_loop
    slli  t0, t0, 7
    andi  t2, t2, 0x7F
    or    a0, t0, t2
    ret

.Ls_special:
    bnez  t1, .Ls_ret
    srli  t3, a0, 15
    bnez  t3, .Ls_qnan
.Ls_ret:
    ret
.Ls_zero:
    li   a0, 0
    ret
.Ls_qnan:
    li   a0, 0x7FC0
    ret
    .size   bf16_sqrt_asm, .-bf16_sqrt_asm
