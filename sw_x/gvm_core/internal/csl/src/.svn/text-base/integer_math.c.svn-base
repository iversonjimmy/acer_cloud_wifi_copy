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
 * implementations for integer math routines needed for the 
 * ECDSA and RSA  algorithm. implemented using the array of  words big 
 * integer notation
 * 
 */

#include "integer_math.h"

#include "csl_impl.h"

#include "poly_math.h"

#if defined(USE_INTEGER_ASM)
bigint_digit bigint_add_digit_mult(bigint_digit *a, bigint_digit *b, bigint_digit c, bigint_digit *d, int digits);
bigint_digit bigint_sub_digit_mult(bigint_digit *a, bigint_digit *b, bigint_digit c, bigint_digit *d, int digits);
#else
static 
void bigint_digit_mult(bigint_digit a[2], bigint_digit b, 
                       bigint_digit c)
{
    
    
    unsigned long long llres;
    
    llres = ((unsigned long long ) b) * ((unsigned long long) c);
    a[0] = (unsigned int) (llres & 0xffffffff);
    a[1] = (unsigned int) (llres >> 32);

}

/*
 * computes a = b + c*d 
 * return carry
 * a, b, d, arrays. c is digit.
 */
static 
bigint_digit bigint_add_digit_mult(bigint_digit *a, bigint_digit *b, bigint_digit c, bigint_digit *d, int digits)
{
    bigint_digit carry, t[2];
    int i;
    
    if(c==0)
        return (0);
    
    carry = 0;
    for(i=0; i<digits; i++){
        bigint_digit_mult(t, c, d[i]);
        if((a[i] = b[i] + carry) < carry)
            carry = 1;
        else 
            carry = 0;
        if((a[i] += t[0]) < t[0])
            carry++;
        carry += t[1];
    }
    return carry;
}

/*
 * computes a = b - c*d for 
 * a[] b[] d[] and c is a digit
 * return borrow
 */
static
bigint_digit bigint_sub_digit_mult(bigint_digit *a, bigint_digit *b, 
                                   bigint_digit c, bigint_digit *d,
                                   int digits)
{
    bigint_digit borrow, t[2];
    int i;
    
    if(c==0)
        return (0);
    
    borrow = 0;
    for(i=0; i<digits; i++){
        bigint_digit_mult(t, c, d[i]);
        if((a[i] = b[i] - borrow) > (MAX_BIGINT_DIGIT - borrow))
            borrow = 1;
        else 
            borrow = 0;
        if((a[i] -= t[0]) > (MAX_BIGINT_DIGIT - t[0]))
            borrow++;
        borrow += t[1];
    }
    return borrow;
}
#endif

#if defined(USE_INTEGER_ASM) && defined(USE_ARMV5)
int bigint_digit_bits(bigint_digit a);
#else

/* 
 * returns how many bits are significant 
 */
static 
int bigint_digit_bits(bigint_digit a)
{
    int i;
    for(i=0; i<BIGINT_DIGIT_BITS; i++, a>>=1)
        if(a==0)
            break;
    return i;
}
#endif

/*
 * a = b/ c where all are digits 
 * length of b is b[2]
 * b[1] < c and HIGHER_HALF(c) > 0
 */
static void
bigint_digit_div(bigint_digit *a, bigint_digit b[2], bigint_digit c)
{
    bigint_digit t[2], u, v;
    bigint_half_digit a_high, a_low, c_high, c_low;

    c_high = (bigint_half_digit) HIGHER_HALF(c);
    c_low = (bigint_half_digit) LOWER_HALF(c);

    t[0] = b[0];
    t[1] = b[1];
    
    /* under-estimate higher half of quotient and subtract */
    if(c_high == LOW_MASK)
        a_high = (bigint_half_digit) HIGHER_HALF (t[1]);
    else
        a_high = (bigint_half_digit) (t[1]/(c_high + 1));
    
    u = (bigint_digit) a_high * (bigint_digit) c_low;
    v = (bigint_digit) a_high * (bigint_digit) c_high;
    
    if((t[0] -= TO_HIGHER_HALF(u)) > (MAX_BIGINT_DIGIT - TO_HIGHER_HALF(u)))
        t[1]--;
    t[1] -= HIGHER_HALF(u);
    t[1] -= v;
    
    /* correct estimate */
    while((t[1] > c_high) ||
          ((t[1] == c_high) && (t[0] >= TO_HIGHER_HALF(c_low)))){
        if((t[0] -= TO_HIGHER_HALF(c_low)) > MAX_BIGINT_DIGIT - TO_HIGHER_HALF(c_low))
            t[1]--;
        t[1] -= c_high;
        a_high++;
    }
    
    /* underestimate lower half of quotient and subtract */
    if(c_high == LOW_MASK)
        a_low = (bigint_half_digit) LOWER_HALF(t[1]);
    else
        a_low = (bigint_half_digit) ((TO_HIGHER_HALF(t[1])+HIGHER_HALF(t[0]))/(c_high + 1));
    u = (bigint_digit) a_low * (bigint_digit) c_low;
    v = (bigint_digit) a_low * (bigint_digit) c_high;
    if((t[0] -= u) > (MAX_BIGINT_DIGIT -u))
        t[1]--;
    if((t[0] -= TO_HIGHER_HALF(v)) > (MAX_BIGINT_DIGIT - TO_HIGHER_HALF(v)))
        t[1]--;
    t[1] -= HIGHER_HALF(v); 
    
    /* correct estimate */
    while((t[1] > 0) || ((t[1] == 0) && t[0] >= c)){
        if((t[0] -= c) > (MAX_BIGINT_DIGIT -c))
            t[1]--;
        a_low++;
    }
    *a = TO_HIGHER_HALF(a_high) + a_low;
}

/* 
 * copies b to a 
 */
static void
bigint_copy(bigint_digit *a, bigint_digit *b, int digits)
{
    int i;
    for(i=0; i<digits; i++){
        a[i] = b[i];
    }
}

/* 
 * zeros 
 */
void bigint_zero(bigint_digit *a, int digits)
{
    int i;
    for(i=0; i<digits; i++){
        a[i] = 0;
    }
}

/* 
 * a = b+c
 * a,b,c all bigint_digit  arrays
 */
bigint_digit 
bigint_add(bigint_digit *a, bigint_digit *b, bigint_digit *c, int digits)
{
    bigint_digit ai, carry;
    int i;
    carry = 0;
    
    for(i=0; i<digits; i++){
        if((ai = b[i] + carry) < carry)
            ai = c[i];
        else if((ai += c[i]) < c[i])
            carry = 1;
        else 
            carry = 0;
        a[i] = ai;
    }
    return (carry);
}

/*
 * a = b - c all arrays, returns borrow
 */
bigint_digit 
bigint_sub(bigint_digit *a, bigint_digit *b, bigint_digit *c, int digits)
{
    bigint_digit ai, borrow;
    int i;
    borrow = 0;
    
    for(i=0; i<digits; i++){
        if((ai = b[i] - borrow)>(MAX_BIGINT_DIGIT - borrow))
            ai = MAX_BIGINT_DIGIT - c[i];
        else if ((ai -= c[i]) > (MAX_BIGINT_DIGIT - c[i]))
            borrow = 1;
        else 
            borrow = 0;
        a[i] = ai;
    }
    return (borrow);
}


/*
 * return length in digits
 */
int
bigint_digits(bigint_digit *a, int digits)
{
    int i;
    for(i=digits-1; i>=0; i--)
        if(a[i])
            break;
    
    return(i+1);
}

/* Computes a = b * c.
   Lengths: a[2*digits], b[digits], c[digits].
*/
void
bigint_mult(bigint_digit *a, bigint_digit *b, bigint_digit *c, int digits)
{
    int b_digits, c_digits, i;
#if defined(__KERNEL__)
    // for linux kernel, put the big array in the heap
    bigint_digit *integer_math_res = kmalloc(sizeof(bigint_digit) * 2 * MAX_BIGINT_DIGITS, GFP_KERNEL);
    if (!integer_math_res)
        return;  // FIXME tell caller of error
#elif defined(LINUX)
    // for linux user space, put the big array on the stack
    bigint_digit integer_math_res[2*MAX_BIGINT_DIGITS];
#else
    // otherwise, statically allocate the big array
    static bigint_digit integer_math_res[2*MAX_BIGINT_DIGITS];
#endif
    bigint_zero(integer_math_res, 2*digits);
    
    b_digits = bigint_digits(b, digits);
    c_digits = bigint_digits(c, digits);
    
    for(i=0; i<b_digits; i++){
        integer_math_res[i+c_digits] += bigint_add_digit_mult(&integer_math_res[i], &integer_math_res[i], b[i], c, c_digits);
    }
    bigint_copy(a, integer_math_res, 2*digits);
#if defined(__KERNEL__)
    kfree(integer_math_res);
#endif
}

/*
 * a = b << c (shifts b left c bits)
 *return carry 
 */
static bigint_digit
bigint_left_shift(bigint_digit *a, bigint_digit *b, int c, int digits)
{
    bigint_digit bi, carry;
    int i, t;
    
    if(c >= BIGINT_DIGIT_BITS)
        return (0);
    
    t = BIGINT_DIGIT_BITS - c;
    carry = 0;
    for(i=0; i<digits; i++){
        bi = b[i];
        a[i] = (bi << c) | carry;
        carry = c ? (bi >> t) : 0;
    }
    return (carry);
}

/*
 * shifts b right c times and returns in a 
 */
static bigint_digit
bigint_right_shift(bigint_digit *a, bigint_digit *b, int c, int digits)
{
    bigint_digit bi, carry;
    int i;
    unsigned int t;

    if(c >= BIGINT_DIGIT_BITS)
        return (0);
    
    t = BIGINT_DIGIT_BITS - c;
    carry = 0;
    
    for(i=digits-1; i >= 0; i--){
        bi = b[i];
        a[i] = (bi >> c) | carry;
        carry = c ? (bi << t) : 0;
    }
    return (carry);
}


/*
 * compare and return sign of a-b
 */
int 
bigint_cmp(bigint_digit *a, bigint_digit *b, int digits)
{
    int i;
    
    for(i=digits-1; i>=0; i--){
        if(a[i] > b[i])
            return (1);
        if(a[i] < b[i])
            return (-1);
    }
    return (0);
}



/*
 * a = c/d and b = c % d
 * a[c_digits], b[d_digits], c[c_digits], d[d_digits], 
 * c_digits < 2*MAX_BIGINT_DIGITS
 */
void
bigint_div(bigint_digit *a, bigint_digit *b, bigint_digit *c, 
           int c_digits, bigint_digit *d, int d_digits)
{
    bigint_digit ai;
    bigint_digit t;
    int i, dd_digits, shift;
#if defined(__KERNEL__)
    // for linux kernel, put the big arrays in the heap
    bigint_digit *integer_math_cc = kmalloc(sizeof(bigint_digit) * (2 * MAX_BIGINT_DIGITS + 1), GFP_KERNEL);
    bigint_digit *integer_math_dd = kmalloc(sizeof(bigint_digit) * MAX_BIGINT_DIGITS, GFP_KERNEL);
    if (!integer_math_cc || !integer_math_dd)
        goto end;
#elif defined(LINUX)
    // for linux user space, put the big arrays on the stack
    bigint_digit integer_math_cc[2*MAX_BIGINT_DIGITS+1], integer_math_dd[MAX_BIGINT_DIGITS];
#else
    // otherwise, statically allocate the big arrays
    static bigint_digit integer_math_cc[2*MAX_BIGINT_DIGITS+1], integer_math_dd[MAX_BIGINT_DIGITS];
#endif

    dd_digits = bigint_digits(d, d_digits);
    if(dd_digits == 0)
        return;

    /* normalize */
    shift = BIGINT_DIGIT_BITS - bigint_digit_bits(d[dd_digits-1]);
    bigint_zero(integer_math_cc, dd_digits);
    integer_math_cc[c_digits] = bigint_left_shift(integer_math_cc, c, shift, c_digits);
    bigint_left_shift(integer_math_dd, d, shift, dd_digits);
    t = integer_math_dd[dd_digits-1];
    
    bigint_zero(a, c_digits);

    for(i=c_digits-dd_digits; i>=0; i--){
        /* underestimate quotient and subtract */
        if(t==MAX_BIGINT_DIGIT)
            ai = integer_math_cc[i+dd_digits];
        else
            bigint_digit_div(&ai, &integer_math_cc[i+dd_digits-1], t+1);
        integer_math_cc[i+dd_digits] -= bigint_sub_digit_mult(&integer_math_cc[i], &integer_math_cc[i], ai, integer_math_dd, dd_digits);
        /*correct estimate */
        while(integer_math_cc[i+dd_digits] || (bigint_cmp(&integer_math_cc[i], integer_math_dd, dd_digits) >= 0)){
            ai++;
            integer_math_cc[i+dd_digits] -= bigint_sub(&integer_math_cc[i], &integer_math_cc[i], integer_math_dd, dd_digits);
        }
        a[i] = ai;
    }
    bigint_zero(b, d_digits);
    bigint_right_shift(b, integer_math_cc, shift, dd_digits);

#if defined(__KERNEL__)
 end:
    if (integer_math_cc)
        kfree(integer_math_cc);
    if (integer_math_dd)
        kfree(integer_math_dd);
#endif
}

/*
 * compute a = b mod c 
 * a[c_digits], b[b_digits], c[c_digits]
 * c > 0, b_digits < 2*MAX_BIGINT_DIGITS, c_digits < MAX_BIGINT_DIGITS
 */

void
bigint_mod(bigint_digit *a, bigint_digit *b, int b_digits, bigint_digit *c, int c_digits)
{
#if defined(__KERNEL__)
    // for linux kernel, put the big array in the heap
    bigint_digit *integer_math_mod_t = kmalloc(sizeof(bigint_digit) * 2 * MAX_BIGINT_DIGITS, GFP_KERNEL);
    if (!integer_math_mod_t)
        return;  // FIXME tell caller of error
#elif defined(LINUX)
    // for linux user space, put the big array on the stack
    bigint_digit integer_math_mod_t[2*MAX_BIGINT_DIGITS];
#else
    // otherwise, statically allocate the big array
    static bigint_digit integer_math_mod_t[2*MAX_BIGINT_DIGITS];
#endif
    bigint_div(integer_math_mod_t, a, b, b_digits, c, c_digits);
#if defined(__KERNEL__)
    kfree(integer_math_mod_t);
#endif
}

/*
 * a = b*c mod d 
 */
void
bigint_mod_mult(bigint_digit *a, bigint_digit *b, bigint_digit *c, bigint_digit *d, int digits)
{
#if defined(__KERNEL__)
    // for linux kernel, put the big array in the heap
    bigint_digit *integer_math_mod_mult_t = kmalloc(sizeof(bigint_digit) * 2 * MAX_BIGINT_DIGITS, GFP_KERNEL);
    if (!integer_math_mod_mult_t)
        return;  // FIXME tell caller of error
#elif defined(LINUX)
    // for linux user space, put the big array on the stack
    bigint_digit integer_math_mod_mult_t[2*MAX_BIGINT_DIGITS];
#else
    // otherwise, statically allocate the big array
    static bigint_digit integer_math_mod_mult_t[2*MAX_BIGINT_DIGITS];
#endif
    bigint_mult(integer_math_mod_mult_t, b, c, digits);
    bigint_mod(a, integer_math_mod_mult_t, 2*digits, d, digits);
#if defined(__KERNEL__)
    kfree(integer_math_mod_mult_t);
#endif
}


/* a = b^c mod d.
 * Lengths: a[d_digits, b[d_digits], c[c_digits], d[d_digits].
 * Assumes d > 0, c_digits > 0, d_digits < MAX_BIGINT_DIGITS.
 */
void
bigint_mod_exp(bigint_digit *a, bigint_digit *b, bigint_digit *c, 
               int c_digits, bigint_digit *d, int d_digits)
{

    bigint_digit ci;
    int i;
    unsigned int ci_bits, j, s;
    bigint_digit exp;
    unsigned int need[4];
    bigint_digit set_bits = 0;
#if defined(__KERNEL__)
    // for linux kernel, put the big arrays in the heap
    bigint_digit *integer_math_b_power[3] = { NULL, NULL, NULL };
    bigint_digit *integer_math_tt = NULL;
    for (i = 0; i < 3; i++) {
        integer_math_b_power[i] = kmalloc(sizeof(bigint_digit) * MAX_BIGINT_DIGITS, GFP_KERNEL);
        if (!integer_math_b_power[i])
            goto end;
    }
    integer_math_tt = kmalloc(sizeof(bigint_digit) * MAX_BIGINT_DIGITS, GFP_KERNEL);
    if (!integer_math_tt)
        goto end;
#elif defined(LINUX)
    // for linux user space, put the big arrays on the stack
    bigint_digit integer_math_b_power[3][MAX_BIGINT_DIGITS], integer_math_tt[MAX_BIGINT_DIGITS];
#else
    // otherwise, statically allocate the big arrays
    static bigint_digit integer_math_b_power[3][MAX_BIGINT_DIGITS], integer_math_tt[MAX_BIGINT_DIGITS];
#endif
    /* zero and power one come for free */
    need[0] = 1;
    need[1] = 1;
    need[2] = 0;
    need[3] = 0;
    /* check input to see if power 3 is needed, then turn on 2. */
    exp = c[0];
    
    for(i=0; i<16; i++){
        /* take last two bits */
        set_bits = exp & 0x00000003;
        /* you need to compute that power */
        need[set_bits]++;
        /* shift right two bits */
        exp = exp >> 2;
    }
    if(need[3] > 0){
        need[2] = 1; /* need to compute anyway */
    }
    
    /* store b, b^2 mod d, and b^3 mod d */
    bigint_copy(integer_math_b_power[0], b, d_digits);
    if(need[2] > 0){
        bigint_mod_mult(integer_math_b_power[1], integer_math_b_power[0], b, d, d_digits);
    }
    if(need[3] > 0){
        bigint_mod_mult(integer_math_b_power[2], integer_math_b_power[1], b, d, d_digits);
    }
    BIGINT_ASSIGN_DIGIT(integer_math_tt, 1, d_digits);
    
    c_digits = bigint_digits(c, c_digits);

    for(i=c_digits-1; i>=0; i--){
        ci = c[i];
        ci_bits = BIGINT_DIGIT_BITS;
        
        /* scan past leading zero bits of most significant digit */
        if(i==(int)(c_digits-1)){
            while(! BIGINT_DIGIT_2MSBS(ci)){
                ci <<= 2;
                ci_bits -= 2;
            }
        }

        for(j=0; j <ci_bits; j+= 2, ci<<=2){ 
            /* compute t = t^4*b^s mod d, where s = two MSBs of ci */
            bigint_mod_mult(integer_math_tt, integer_math_tt, integer_math_tt, d, d_digits);
            bigint_mod_mult(integer_math_tt, integer_math_tt, integer_math_tt, d, d_digits);
            if((s = BIGINT_DIGIT_2MSBS(ci)) != 0)
                bigint_mod_mult(integer_math_tt, integer_math_tt, integer_math_b_power[s-1], d, d_digits);
        }
    }
    bigint_copy(a, integer_math_tt, d_digits);

#if defined(__KERNEL__)
 end:
    for (i = 0; i < 3; i++)
        if (integer_math_b_power[i])
            kfree(integer_math_b_power[i]);
    if (integer_math_tt)
        kfree(integer_math_tt);
#endif
}

/* 
 * return nonzero iff a is zero
 */
int
bigint_iszero(bigint_digit *a, int digits)
{
    int i;
    
    for(i=0; i<digits; i++)
        if(a[i])
            return (0);

    return (1);
}

#if 0
/*
 * return length of a in bits 
 */
int
bigint_bits(bigint_digit *a, int digits)
{
    if((digits=bigint_digits(a, digits))==0)
        return (0);

    return ((digits-1)*BIGINT_DIGIT_BITS+bigint_digit_bits(a[digits-1]));
}
#endif


/* compute a = 1/b mod c
 * this is only for ecc, so use the smaller MAX_ECC_DIGITS
 * gcd(b,c)=1
 */

typedef struct {
    bigint_digit q[MAX_ECC_DIGITS], t1[MAX_ECC_DIGITS], t3[MAX_ECC_DIGITS],
        u1[MAX_ECC_DIGITS], u3[MAX_ECC_DIGITS], v1[MAX_ECC_DIGITS],
        v3[MAX_ECC_DIGITS], w[2*MAX_ECC_DIGITS];
} bigint_mod_inv_big_locals;

void
bigint_mod_inv(bigint_digit *a, bigint_digit *b,
               bigint_digit *c, int digits)
{
    int u1_sign;
    bigint_mod_inv_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(bigint_mod_inv_big_locals), GFP_KERNEL);
    if (!big_locals)
        return;  // FIXME tell caller of error
#else
    bigint_mod_inv_big_locals _big_locals;
    big_locals = &_big_locals;
#endif
    
    /* apply extended euclidian alg modified to avoid neg numbers */
    BIGINT_ASSIGN_DIGIT(big_locals->u1, 1, digits);
    bigint_zero(big_locals->v1, digits);
    bigint_copy(big_locals->u3, b, digits);
    bigint_copy(big_locals->v3, c, digits);
    u1_sign = 1;
    
    while(!bigint_iszero(big_locals->v3, digits)){
        bigint_div(big_locals->q, big_locals->t3, big_locals->u3, digits, big_locals->v3, digits);
        bigint_mult(big_locals->w, big_locals->q, big_locals->v1, digits);
        bigint_add(big_locals->t1, big_locals->u1, big_locals->w, digits);
        bigint_copy(big_locals->u1, big_locals->v1, digits);
        bigint_copy(big_locals->v1, big_locals->t1, digits);
        bigint_copy(big_locals->u3, big_locals->v3, digits);
        bigint_copy(big_locals->v3, big_locals->t3, digits);
        u1_sign = -u1_sign;
    }
    /* negate if sign is negative */
    if(u1_sign<0)
        bigint_sub(a, c, big_locals->u1, digits);
    else
        bigint_copy(a, big_locals->u1, digits);

#ifdef __KERNEL__
    kfree(big_locals);
#endif
}
