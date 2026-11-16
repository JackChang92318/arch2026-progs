#include "bf16.h"

/* Dummies: every function returns 0. Replace with the real routines. */
uint16_t bf16_from_f32(uint32_t f32bits)
{
    if (((f32bits >> 23) & 0xFF) == 0xFF)
        return (uint16_t) (f32bits >> 16);

    f32bits += 0x7FFF + ((f32bits >> 16) & 1);
    return (uint16_t) (f32bits >> 16);
}

uint16_t bf16_mul(uint16_t a, uint16_t b)
{
    uint16_t sign = (a ^ b) & 0x8000;
    int exp_a = (a >> 7) & 0xFF;
    int exp_b = (b >> 7) & 0xFF;
    int mant_a = a & 0x7F;
    int mant_b = b & 0x7F;

    if (exp_a == 0xFF) {
        if (mant_a)
            return a;
        if (!exp_b && !mant_b)
            return 0x7FC0;
        return sign | 0x7F80;
    }

    if (exp_b == 0xFF) {
        if (mant_b)
            return b;
        if (!exp_a && !mant_a)
            return 0x7FC0;
        return sign | 0x7F80;
    }

    if ((!exp_a && !mant_a) || (!exp_b && !mant_b))
        return sign;

    int exp_adjust = 0;
    if (!exp_a) {
        while (!(mant_a & 0x80)) {
            mant_a <<= 1;
            exp_adjust--;
        }
        exp_a = 1;
    } else {
        mant_a |= 0x80;
    }

    if (!exp_b) {
        while (!(mant_b & 0x80)) {
            mant_b <<= 1;
            exp_adjust--;
        }
        exp_b = 1;
    } else {
        mant_b |= 0x80;
    }

    uint32_t prod = (uint32_t) mant_a * mant_b;
    int mant;
    if (prod & 0x8000) {
        mant = (prod >> 8) & 0x7F;
        exp_adjust++;
    } else {
        mant = (prod >> 7) & 0x7F;
    }

    int exp = exp_a + exp_b - 127 + exp_adjust;
    if (exp >= 0xFF)
        return sign | 0x7F80;
    if (exp <= 0) {
        if (exp < -6)
            return sign;
        mant >>= 1 - exp;
        exp = 0;
    }
    return sign | (exp << 7) | mant;
}

uint16_t bf16_sqrt(uint16_t a)
{
    int sign = a >> 15;
    int exp = (a >> 7) & 0xFF;
    int mant = a & 0x7F;

    if (exp == 0xFF) {
        if (mant)
            return a;
        if (sign)
            return 0x7FC0;
        return a;
    }
    if (!exp && !mant)
        return 0;
    if (sign)
        return 0x7FC0;
    if (!exp)
        return 0;

    int e = exp - 127;
    uint32_t m = 0x80 | mant;
    int new_exp;
    if (e & 1) {
        m <<= 1;
        new_exp = ((e - 1) >> 1) + 127;
    } else {
        new_exp = (e >> 1) + 127;
    }

    uint32_t low = 90, high = 256, result = 128;
    while (low <= high) {
        uint32_t mid = (low + high) >> 1;
        if ((mid * mid) / 128 <= m) {
            result = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    if (result >= 256) {
        result >>= 1;
        new_exp++;
    }
    while (result < 128 && new_exp > 1) {
        result <<= 1;
        new_exp--;
    }

    if (new_exp >= 0xFF)
        return 0x7F80;
    if (new_exp <= 0)
        return 0;
    return (uint16_t) ((new_exp << 7) | (result & 0x7F));
}
