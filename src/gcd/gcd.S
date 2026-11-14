# gcd_u32_asm: binary GCD, hand-written RV32I.
#
# uint32_t gcd_u32_asm(uint32_t a, uint32_t b)   a in a0, b in a1, result in a0
#
# RV32I and Zicsr only: no mul, div, rem, no floating point, no compressed
# instructions. Tests: tests/tgcd.c.

    .text

    .globl  gcd_u32_asm
    .type   gcd_u32_asm, @function
    .align  2
gcd_u32_asm:
    mv    a5,a0 
    beqz  a0, RET_B
    beqz  a1, RET
    or    a0,a0,a1 
    andi  a0,a0,1
    bnez  a0, SHIFT0
COMMON:
    srli  a5,a5,1
    srli  a1,a1,1 
    or    a4,a5,a1
    andi  a4,a4,1
    addi  a0,a0,1 
    beqz  a4, COMMON
ODD_A:
    andi  a4,a5,1
    bnez  a4, LOOP
STRIP_A:
    srli  a5,a5,1
    andi  a4,a5,1
    beqz  a4, STRIP_A
LOOP:
    andi  a4,a1,1
    bnez  a4, SUB1
HALF_B1:
    srli  a1,a1,1
    andi  a4,a1,1
    beqz  a4, HALF_B1
SUB1:
    bltu  a1,a5, SWAP1
    sub   a1,a1,a5
    bnez  a1, HALF_B1
    sll   a0,a5,a0
    ret
SWAP1:
    sub   a5,a5,a1

HALF_B2:
    srli  a5,a5,1
    andi  a4,a5,1
    beqz  a4, HALF_B2
    bltu  a5,a1, SWAP2
    sub   a5,a5,a1
    bnez  a5, HALF_B2
    sll   a0,a1,a0
    ret
SWAP2:
    sub   a1,a1,a5
    j     HALF_B1

RET_B:
    mv    a0,a1
RET:
    ret
SHIFT0:
    li    a0,0
    j     ODD_A
    .size   gcd_u32_asm, .-gcd_u32_asm