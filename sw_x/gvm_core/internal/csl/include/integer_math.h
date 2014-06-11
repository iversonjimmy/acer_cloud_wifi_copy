/*
 *               Copyright (C) 2005, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
/* 
 * integer_math.h
 * declarations for integer math routines needed for the 
 * ECDSA algorithm and RSA verification algorithms. 
 * 
 */

#ifndef __INTEGER_MATH_H__
#define __INTEGER_MATH_H__


#include "poly_math.h"


typedef unsigned int bigint_digit;
typedef unsigned short bigint_half_digit;

#define BIGINT_DIGIT_BITS 32
#define BIGINT_HALF_DIGIT_BITS 16
#define BIGINT_DIGIT_BYTES 4
/* in bytes */
#define BIGINT_DIGIT_LEN (BIGINT_DIGIT_BITS/8)

/* maxima */
#define MAX_BIGINT_DIGIT 0xffffffff
#define HIGH_MASK 0xffff0000
#define LOW_MASK 0x0000ffff
#define MIN_RSA_MODULUS_BITS 508
#define MAX_RSA_MODULUS_BITS 4096
#define MAX_RSA_MODULUS_LEN ((MAX_RSA_MODULUS_BITS +7)/8)
#define MAX_RSA_PRIME_BITS ((MAX_RSA_MODULUS_BITS + 1)/2)
#define MAX_RSA_PRIME_LEN ((MAX_RSA_PRIME_BITS + 7)/8)

#define LOWER_HALF(x) ((x) & LOW_MASK)
#define HIGHER_HALF(x) (((x) >> BIGINT_HALF_DIGIT_BITS)&LOW_MASK)
#define TO_HIGHER_HALF(x) (((bigint_digit)(x)) << BIGINT_HALF_DIGIT_BITS)
#define BIGINT_DIGIT_MSB(x) (bigint_digit) (((x) >> (BIGINT_DIGIT_BITS -1)) & 1)
#define BIGINT_DIGIT_2MSBS(x) (bigint_digit)(((x) >>(BIGINT_DIGIT_BITS -2)) & 3)
#define BIGINT_ASSIGN_DIGIT(a, b, digits) {bigint_zero(a, digits); a[0] = b;}

/* maximum length in digits */
#define MAX_BIGINT_DIGITS ((MAX_RSA_MODULUS_LEN + BIGINT_DIGIT_LEN - 1)/BIGINT_DIGIT_LEN + 1)

/* supports only 233 bit ECC*/
#define MAX_ECC_DIGITS 8


int bigint_iszero(bigint_digit *a, int digits);
int bigint_cmp(bigint_digit *a, bigint_digit *b, int digits);
void bigint_mod(bigint_digit *a, bigint_digit *b, int bDigits, 
                bigint_digit *c, int cDigits);
void bigint_mod_mult(bigint_digit *a,  bigint_digit *b, 
                     bigint_digit *c, bigint_digit *d, int digits);
void bigint_mod_exp (bigint_digit *a, bigint_digit *b, bigint_digit *c, int cDigits, bigint_digit *d, int dDigits);
void bigint_mod_inv (bigint_digit *a, bigint_digit *b, 
		     bigint_digit *c, int digits);
bigint_digit bigint_add(bigint_digit *a, bigint_digit *b, 
                        bigint_digit *c, int digits);
bigint_digit bigint_sub(bigint_digit *a, bigint_digit *b, 
                        bigint_digit *c, int digits);
void bigint_mult (bigint_digit *a, bigint_digit *b, bigint_digit *c, 
		  int digits);
void bigint_div (bigint_digit *a, bigint_digit *b, bigint_digit *c, 
		 int cDigits, bigint_digit  *d, int dDigits);
int bigint_digits(bigint_digit *a, int digits);
void bigint_zero(bigint_digit *a, int digits);
#endif /* __INTEGER_MATH_H__ */
