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
/* poly_math.c
 * implementation of GF 2^m arithmetic needed for elliptic math
 * field_2n poly_prime = {0x00080000,0x00000000,0x00000000,
 * 0x00000000,0x00000000,0x00000400, 0x00000000, 0x00000001};  
 * 233 bit u^233 + u^74 + 1
 * Reference:
 * all these routines are derived from the method in "fast key 
 * exchange with elliptic curve systems" by schroeppel, orman, malley
 * and the partial multiplication, is the LR window method surveyed 
 * in "software implementation of elliptic curve crypt.
 * over binary fields" hankerson, hernandez, menezes
 * 
 */

#include "poly_math.h"

#include "csl_impl.h"

#include "integer_math.h"

const field_2n poly_prime = {{0x00000200,0x00000000,0x00000000,
                              0x00000000,0x00000000,0x00000400, 
                              0x00000000, 0x00000001}};  
const unsigned char shift_by[256] = {0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x05,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x06,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x05,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x07,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x05,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x06,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x05,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00, 
                                     0x03,0x00,0x01,0x00,0x02,0x00,0x01,0x00};
 
unsigned short int zero_inserted[256] = 
{0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 
 0x0014, 0x0015, 0x0040, 0x0041, 0x0044, 0x0045, 
 0x0050, 0x0051, 0x0054, 0x0055, 0x0100, 0x0101, 
 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 
 0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 
 0x0154, 0x0155, 0x0400, 0x0401, 0x0404, 0x0405, 
 0x0410, 0x0411, 0x0414, 0x0415, 0x0440, 0x0441, 
 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 
 0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 
 0x0514, 0x0515, 0x0540, 0x0541, 0x0544, 0x0545, 
 0x0550, 0x0551, 0x0554, 0x0555, 0x1000, 0x1001, 
 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 
 0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 
 0x1054, 0x1055, 0x1100, 0x1101, 0x1104, 0x1105, 
 0x1110, 0x1111, 0x1114, 0x1115, 0x1140, 0x1141, 
 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 
 0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 
 0x1414, 0x1415, 0x1440, 0x1441, 0x1444, 0x1445, 
 0x1450, 0x1451, 0x1454, 0x1455, 0x1500, 0x1501, 
 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 
 0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 
 0x1554, 0x1555, 0x4000, 0x4001, 0x4004, 0x4005, 
 0x4010, 0x4011, 0x4014, 0x4015, 0x4040, 0x4041, 
 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055, 
 0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 
 0x4114, 0x4115, 0x4140, 0x4141, 0x4144, 0x4145, 
 0x4150, 0x4151, 0x4154, 0x4155, 0x4400, 0x4401, 
 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 
 0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 
 0x4454, 0x4455, 0x4500, 0x4501, 0x4504, 0x4505, 
 0x4510, 0x4511, 0x4514, 0x4515, 0x4540, 0x4541, 
 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555, 
 0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 
 0x5014, 0x5015, 0x5040, 0x5041, 0x5044, 0x5045, 
 0x5050, 0x5051, 0x5054, 0x5055, 0x5100, 0x5101, 
 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 
 0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 
 0x5154, 0x5155, 0x5400, 0x5401, 0x5404, 0x5405, 
 0x5410, 0x5411, 0x5414, 0x5415, 0x5440, 0x5441, 
 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 
 0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 
 0x5514, 0x5515, 0x5540, 0x5541, 0x5544, 0x5545, 
 0x5550, 0x5551, 0x5554, 0x5555};

/*
 * void 
 * null()
 * Purpose : null a field_2n variable
 * input: pointer to allocated field_2n, can do in place
 * 
 */

void 
poly_null(field_2n *a)
{
    int i;
    element *eptr = a->e;
    for (i = MAX_LONG - 1; i >= 0; i--){
        *eptr++ = 0L;
    }
}


/*
 * void 
 * double_null()
 * Purpose : null a double variable
 * input: pointer to allocated double, can do in place
 * 
 */

static void
double_null(field_double *a)
{
    int i;
    element *eptr = a->e;
    for (i = MAX_DOUBLE - 1; i >= 0; i--){
        *eptr++ = 0L;
    }
}



/*
 * void
 * double_add()
 * Purpose : add double variable
 * input: pointer to allocated doubles, cannot do in place
 * 
 */

static void
double_add(field_double *a, field_double *b, field_double *c)
{
    int i;
    element *aptr = a->e;
    element *bptr = b->e;
    element *cptr = c->e;
    
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *cptr = *aptr ^ *bptr;
        aptr++;
        bptr++;
        cptr++;
    }
}


/*
 * void 
 * copy()
 * Purpose : copy field_2n variables
 * input: pointer to allocated field_2n, cannot do in place
 * 
 */

void
poly_copy(const field_2n *from, field_2n *to)
{
    int i;
    const element *fromptr = from->e;
    element *toptr = to->e;
    
    for (i = MAX_LONG - 1; i >= 0; --i) {
        *toptr++ = *fromptr++;
    }
}



/*
 * void 
 * double_copy()
 * Purpose : copy double variables
 * input: pointers to allocated double, cannot do in place
 * 
 */

static void 
double_copy(field_double *from, field_double *to)
{
    int i;
    const element *fromptr = from->e;
    element *toptr = to->e;
    
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *toptr++ = *fromptr++;
    }
}

/*
 * void 
 * single_to_double()
 * Purpose : copy single to double
 * input: pointers to allocated double, cannot do in place
 * 
 */

static void 
single_to_double(field_2n *from, field_double *to)
{
    int i;
    
    double_null(to);
    for (i =0 ; i < MAX_LONG; i++){
        to->e[DOUBLE_WORD - NUM_WORD + i] = from->e[i];
    }
    return;
}


/*
 * void 
 * double_to_single()
 * Purpose : copy bottom of double to single
 * input: pointers to allocated double, cannot do in place
 * 
 */
static void 
double_to_single(field_double *from, field_2n *to)
{
    int i;
    element *t = to->e;
    element *f = from->e + DOUBLE_WORD - NUM_WORD;
    
    for (i = MAX_LONG - 1; i >= 0; --i) {
        *t++ = *f++;
    }
}

/*
 * void
 * multiply_shift()
 * Purpose : shift one bit to do partial multiply: double field.
 * input: pointer to allocated field_double, can do in place
 * 
 */
static void 
multiply_shift(field_double *a)
{
    element *eptr, temp, bit;
    int i;
    
    /* this is bigendian representation
     */
    eptr = &a->e[DOUBLE_WORD];
    bit = 0;
    
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        temp = (*eptr << 1) | bit;
        bit = (*eptr & MSB) ? 1L : 0L;
        *eptr-- = temp;
    }
}

#if !defined (LINUX) || defined(__KERNEL__)
field_double poly_math_bprep[16]; /* precompute b multiples from 0-15 */
#endif

/* this is optimised partial polynomial multiplication, 
 * L-R algorithm with precomputation of 4 bit wide multipliers 
 */

void 
poly_mul_partial(field_2n *a, field_2n *b, field_double *c)
{
    int i, bit_count, word;
    element mask;
    field_double local_b;
    int k, num_shift;
    int b_start;
    element *eptr, temp, bit;
    element *local_bptr;
    element *cptr;
    element *dst, *src;
    
    element multiplier;
#if defined (LINUX) && !defined(__KERNEL__)
    field_double poly_math_bprep[16]; /* precompute b multiples from 0-15 */
#endif
    /* precompute 2^window size -1 multiples*/
  
    /* hand code the 16 values, otherwise its too costly */
    dst = poly_math_bprep[0].e;
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *dst++ = 0;
    }
    single_to_double(b, &poly_math_bprep[1]);
    dst = local_b.e;
    src = poly_math_bprep[1].e;
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *dst++ = *src++;
    }
    multiply_shift(&local_b);
    dst = poly_math_bprep[2].e;
    src = local_b.e;
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *dst++ = *src++;
    }
    multiply_shift(&local_b);
    dst = poly_math_bprep[4].e;
    src = local_b.e;
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *dst++ = *src++;
    }
    multiply_shift(&local_b);
    dst = poly_math_bprep[8].e;
    src = local_b.e;
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *dst++ = *src++;
    }
    
    /* 3 = 2+ 1 */
    for (i= MAX_DOUBLE - 1; i >= 0; --i) {
        poly_math_bprep[3].e[i] = poly_math_bprep[2].e[i] ^ poly_math_bprep[1].e[i]; 
        /* 5 = 4+ 1 */  
        poly_math_bprep[5].e[i] = poly_math_bprep[4].e[i] ^ poly_math_bprep[1].e[i];  
        /* 6 = 4 + 2 */ 
        poly_math_bprep[6].e[i] = poly_math_bprep[4].e[i] ^ poly_math_bprep[2].e[i];  
        /* 7 = 6 + 1 */  
        poly_math_bprep[7].e[i] = poly_math_bprep[6].e[i] ^ poly_math_bprep[1].e[i];  
        /* 9 = 8 + 1 */ 
        poly_math_bprep[9].e[i] = poly_math_bprep[8].e[i] ^ poly_math_bprep[1].e[i];
        /* 10 = 8 + 2 */ 
        poly_math_bprep[10].e[i] = poly_math_bprep[8].e[i] ^ poly_math_bprep[2].e[i];
        /* 11 = 10 + 1 */
        poly_math_bprep[11].e[i] = poly_math_bprep[10].e[i] ^ poly_math_bprep[1].e[i];
        /* 12 = 8 + 4 */
        poly_math_bprep[12].e[i] = poly_math_bprep[8 ].e[i] ^ poly_math_bprep[4].e[i];
        /* 13 = 12 + 1 */
        poly_math_bprep[13].e[i] = poly_math_bprep[12 ].e[i] ^ poly_math_bprep[1].e[i];
        /* 14 = 12 + 2 */
        poly_math_bprep[14].e[i] = poly_math_bprep[12 ].e[i] ^ poly_math_bprep[2].e[i];
        /* 15 = 14 + 1 */
        poly_math_bprep[15].e[i] = poly_math_bprep[14].e[i] ^ poly_math_bprep[1].e[i];
    }
    /*clear bits in the result */
    double_null(c);

    mask = 0xf0000000;
    b_start = DOUBLE_WORD- NUM_WORD;
    for (bit_count = 7; bit_count >=0; bit_count--){
        
        for (word = MAX_LONG-1; word >=0; word--){
            multiplier = (mask & a->e[word]) >> (4*bit_count);
            num_shift = (MAX_LONG-1) - word;
            local_bptr = &(poly_math_bprep[multiplier].e[b_start]);
            cptr = &(c->e[b_start-num_shift]);
            for (i = MAX_LONG - 1; i >= 0; --i){
                *cptr = *local_bptr ^ *cptr;
                cptr++;
                local_bptr++;
            }
        }
        if (bit_count !=0){
            /*
              for (i=0; i< 4; i++){
              multiply_shift(c);
              }
            */
            eptr = &(c->e[DOUBLE_WORD]);
            bit = 0;
            
            for (k = MAX_DOUBLE - 1; k >= 0; --k){
                temp = (*eptr << 4) | bit;
                bit = (*eptr & 0xf0000000)>>28;
                *eptr-- = temp;
            }
        }
        mask >>= 4;
    }
}



/* shift left several bits: for multiplication */

static void 
multiply_shift_n(field_double *a, int n){
    element upper_bits;
    int i;
    int word_shift = n / WORD_SIZE;
    int upper_bit_shift = n % WORD_SIZE;
    int lower_bit_shift = 32 - upper_bit_shift;

    element *dst = a->e;
    element *src = a->e + word_shift;

    if (lower_bit_shift != 32) {
        upper_bits = *src << upper_bit_shift;
        ++src;
        for (i = MAX_DOUBLE - 2 - word_shift; i >= 0; --i) {
            element next = *src++;
            *dst++ = upper_bits | (next >> lower_bit_shift);
            upper_bits = next << upper_bit_shift;
        }
        *dst++ = upper_bits;
    } else {
        for (i = MAX_DOUBLE - 1 - word_shift; i >= 0; --i) {
            *dst++ = *src++;
        }
    }
    for (i = word_shift - 1; i >= 0; --i) {
        *dst++ = 0;
    }
}

/* shift right several bits for division */

static void 
divide_shift_n(field_double *a, int n)
{
    element upper_bits;
    int i;
    int word_shift = n / WORD_SIZE;
    int lower_bit_shift = n % WORD_SIZE;
    int upper_bit_shift = 32 - lower_bit_shift;

    element *dst = a->e + MAX_DOUBLE - 1;
    element *src = dst - word_shift;

    if (lower_bit_shift) {
        upper_bits = src[-1] << upper_bit_shift;
        for (i = MAX_DOUBLE - 2 - word_shift; i >= 0; --i) {
            element next = *src--;
            *dst-- = upper_bits | (next >> lower_bit_shift);
            upper_bits = src[-1] << upper_bit_shift;
        }
        *dst-- = *src >> lower_bit_shift;
    } else {
        for (i = MAX_DOUBLE - 1 - word_shift; i >= 0; --i) {
            *dst-- = *src--;
        }
    }

    for (i = word_shift - 1; i >= 0; --i) {
        *dst-- = 0;
    }
}

/* supporting functions for poly_mul later */
/* extract bits of the double field corresponding to mask
 */
static void 
extract_masked_bits(field_double * a, field_double *mask, field_double *result)
{
    int i;
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        result->e[i] = a->e[i] & mask->e[i];
    }
}

/* put zeros in nonzero locations of mask in the input*/
static void 
zero_masked_bits(field_double *a, field_double *mask)
{
    int i;
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        /* a - mask = a intersection (mask)complement */
        a->e[i] = a->e[i] & (~mask->e[i]);
    }
}

#if 0
/* just utility function for the partial multiply 
 */

static void
shift_and_add(field_double *temp, field_double *extract_mask)
{
    field_double temp1, temp_masked;
    /*first 159 bits */
    extract_masked_bits(temp, extract_mask, &temp_masked);
    zero_masked_bits(temp, extract_mask);
    divide_shift_n(&temp_masked, 159);
    double_add(temp, &temp_masked, &temp1); /* result in temp1*/
    
    /* then shift again by 74 to achieve shift of 233*/
    divide_shift_n(&temp_masked, 74);
    double_add(&temp1, &temp_masked, temp); /* result in temp*/
    return;
}
#endif // 0

void
poly_sqr(field_2n *a, field_double *sqra)
{
    int i;
    unsigned int temp[MAX_LONG*2];
    for (i = MAX_LONG - 1; i >= 0; --i) {
        /* expand it into each word */
        temp[2*i] = ((zero_inserted[((a->e[i] )& 0xff000000)>>24])<<16) |
            ((zero_inserted[((a->e[i]) & 0x00ff0000)>>16]));
        temp[(2*i)+1] =((zero_inserted[((a->e[i]) & 0x0000ff00)>>8])<<16)|
            (zero_inserted[((a->e[i]) & 0x000000ff)]);
    }
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        sqra->e[i] = temp[i+1];
    }
}
	
/* this is polynomial multiplication modulo a primitive
 * polynomial. The primitive polynomial is 233 bits
 * u^233 + u^74 + 1. defined in the header field
 * generally this will work for any trinomials changing the
 * shift numbers, check word
 * size. Need to generalise for pentanomials.
 * field_2n poly_prime = {0x00080000,0x00000000,0x00000000,
 * 0x00000000,0x0000000 * 0,0x00000400, 0x00000000, 0x00000001};  
 * 233 bit u^233 + u^74 + 1

 * this is derived from the method in "fast key exchange with 
 * elliptic curve systems"
 * by schroeppel, orman, malley
 */

typedef struct {
    field_double temp;
    field_double temp1, temp2, temp3, temp_masked, c_copy; 
    element C[16];
} poly_mul_big_locals;

void 
poly_mul( field_2n *a, field_2n *b, field_2n *c)
{
    element t;
    int i;
    int isnotequal = 0;
    element *fromptr;
    element *temp1ptr;
    element *temp2ptr;
    poly_mul_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (big_locals == NULL)
        return;  // currently has no way to tell caller of error
#else
    poly_mul_big_locals _big_locals;
    big_locals = &_big_locals;
#endif
    
    /* optimisation below: if equal, call square */
    
    for (i = MAX_LONG - 1; i >= 0; --i) {
        if (a->e[i] !=  b->e[i]) {
            isnotequal = 1;
            break;
        }
    }
    if (isnotequal == 0){ /* is equal */
    	poly_sqr(a, &big_locals->temp);
    } else {
    	poly_mul_partial(a, b, &big_locals->temp);
    }

    
#if 0
    /* check if this is 16*32=512 bits!*/
    /* bits 464-306 */
    
    double_null(&extract_mask);
    /*MSB is in 0*/
    extract_mask.e[0] = 0x000fffff;
    extract_mask.e[1] = 0xffffffff;
    extract_mask.e[2] = 0xffffffff;
    extract_mask.e[3] = 0xffffffff;
    extract_mask.e[4] = 0xffffffff;
    extract_mask.e[5] = 0xffe00000;
    
    
    /* extract first 159 bits from degree= 468 and shift it by 159, 
       and 233 and add to temp.
    */
    shift_and_add(&big_locals->temp, &extract_mask);
    
    /* temp has first 159 bits reduced*/
    /* second iteration */
    /* repeat only for 74 points extract_mask shifted by 159 inclusive */
    
    /*MSB is in 0*/
    divide_shift_n(&extract_mask, 159);
    /* keep least significant 233 bits : so make them zero*/
    extract_mask.e[7] = 0xfffffe00;
    extract_mask.e[8] = 0x00000000;
    extract_mask.e[9] = 0x00000000;
    extract_mask.e[10] = 0x00000000;
    extract_mask.e[11] = 0x00000000;
    extract_mask.e[12] = 0x00000000;
    extract_mask.e[13] = 0x00000000;
    extract_mask.e[14] = 0x00000000;
    
    shift_and_add(&big_locals->temp, &extract_mask);
    double_to_single(&big_locals->temp, c);
#endif

    for (i = 0; i < 15; i++) {
        big_locals->C[15-i] = big_locals->temp.e[i];
    }
    
    big_locals->C[0] = 0x0;
    

    /* new algorithm : "guide to elliptic curve cryptography*/
    for (i=15; i >= 8; i--){
	t = big_locals->C[i];
	big_locals->C[i - 8] = big_locals->C[i - 8] ^ ((t & 0xffffffff) << 23);
	big_locals->C[i - 7] = big_locals->C[i - 7] ^ ((t & 0xffffffff) >> 9);
	big_locals->C[i - 5] = big_locals->C[i - 5] ^ ((t & 0xffffffff) << 1);
	big_locals->C[i - 4] = big_locals->C[i - 4] ^ ((t & 0xffffffff) >> 31);
    }
    
    
    t = ((big_locals->C[7] & 0xffffffff)  >> 9);
    big_locals->C[0] = (big_locals->C[0] & 0xffffffff) ^ t;
    big_locals->C[2] = big_locals->C[2]^ ((t & 0xffffffff) << 10);
    big_locals->C[3] = big_locals->C[3] ^ ((t & 0xffffffff) >> 22);
    big_locals->C[7] = big_locals->C[7] & 0x1ff;
    
    for (i=0; i < 8; i++){
        c->e[i] = big_locals->C[7 - i];
    }
    /*
      cus_times_u_to_n(c, 32, c);
    */
    
    single_to_double(c, &big_locals->c_copy);
    
    for (i=0; i< DOUBLE_WORD; i++){
        big_locals->temp_masked.e[i] = 0;
    }
    big_locals->temp_masked.e[DOUBLE_WORD] = big_locals->c_copy.e[DOUBLE_WORD];
    
    /*
      double_copy(&temp_masked, &temp1);
      double_copy(&temp_masked, &temp2);
    */
    fromptr = &(big_locals->temp_masked.e[0]);
    temp1ptr = &(big_locals->temp1.e[0]);
    temp2ptr = &(big_locals->temp2.e[0]);	     
    for (i = MAX_DOUBLE - 1; i >= 0; --i) {
        *temp1ptr = *fromptr;
        *temp2ptr = *fromptr;
        fromptr++;
        temp1ptr++;
        temp2ptr++;
    }
    
    multiply_shift_n(&big_locals->temp1, 233);
    multiply_shift_n(&big_locals->temp2, 74);
    
    
    double_add(&big_locals->c_copy, &big_locals->temp1, &big_locals->temp3);
    double_add(&big_locals->temp3, &big_locals->temp2, &big_locals->temp1);
#if 0
    /* last 32 bits must be zero anyway, check */
    
    zero_masked_bits(&big_locals->temp1, &extract_mask);
#endif
    divide_shift_n(&big_locals->temp1, 32);
    double_to_single(&big_locals->temp1, c);

#ifdef __KERNEL__
    kfree(big_locals);
#endif
}

typedef struct {
    field_double extract_mask, temp1, temp2, temp3, temp_masked;
    field_double a_copy;
} cus_times_u_to_n_big_locals;

static void 
cus_times_u_to_n(field_2n *a, int n, field_2n *b)
{
    unsigned int moving_one;
    int num_words_divide;
    int num_bits_divide;
    int i,j;
    element *fromptr;
    element *temp1ptr;
    element *temp2ptr;
    cus_times_u_to_n_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (big_locals == NULL)
        return;  // currently has no way to tell caller of error
#else
    cus_times_u_to_n_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    single_to_double(a, &big_locals->a_copy);
    
    double_null(&big_locals->extract_mask);
    
    num_words_divide = n/WORD_SIZE;
    num_bits_divide = n % WORD_SIZE;
    
    
    
    /* last word */
    big_locals->extract_mask.e[DOUBLE_WORD] = 0xffffffff;
    for (j =0; j < num_words_divide; j++){
        extract_masked_bits(&big_locals->a_copy, &big_locals->extract_mask, &big_locals->temp_masked);
        /*
          double_copy(&big_locals->temp_masked, &big_locals->temp1);
          double_copy(&big_locals->temp_masked, &big_locals->temp2);
        */
        fromptr = &(big_locals->temp_masked.e[0]);
        temp1ptr = &(big_locals->temp1.e[0]);
        temp2ptr = &(big_locals->temp2.e[0]);	     
        for (i = MAX_DOUBLE - 1; i >= 0; --i) {
            *temp1ptr = *fromptr;
            *temp2ptr = *fromptr;
            fromptr++;
            temp1ptr++;
            temp2ptr++;
        }
        
        multiply_shift_n(&big_locals->temp1, 233);
        multiply_shift_n(&big_locals->temp2, 74);
        
        
        double_add(&big_locals->a_copy, &big_locals->temp1, &big_locals->temp3);
        double_add(&big_locals->temp3, &big_locals->temp2, &big_locals->temp1);
        /* last 32 bits must be zero anyway, check */
        zero_masked_bits(&big_locals->temp1, &big_locals->extract_mask);
        divide_shift_n(&big_locals->temp1, 32);
        double_copy(&big_locals->temp1, &big_locals->a_copy);
    }
    
    /* shift last remaining bits */
    double_null(&big_locals->extract_mask);
    moving_one = 0x0001;
    /* enter as many bits in the mask */
    for (i =0; i< num_bits_divide; i++){
        big_locals->extract_mask.e[DOUBLE_WORD] |= moving_one;
        moving_one = moving_one << 1;
    }
    
    extract_masked_bits(&big_locals->a_copy, &big_locals->extract_mask, &big_locals->temp_masked);
    double_copy(&big_locals->temp_masked, &big_locals->temp1);
    double_copy(&big_locals->temp_masked, &big_locals->temp2);
    
    multiply_shift_n(&big_locals->temp1, 233);
    multiply_shift_n(&big_locals->temp2, 74);
    
    double_add(&big_locals->a_copy, &big_locals->temp1, &big_locals->temp3);
    double_add(&big_locals->temp3, &big_locals->temp2, &big_locals->temp1);
    
    /* last num_bits_divide bits must be zero anyway, check */
    zero_masked_bits(&big_locals->temp1, &big_locals->extract_mask);
    divide_shift_n(&big_locals->temp1, num_bits_divide);
    double_copy(&big_locals->temp1, &big_locals->a_copy);
    double_to_single(&big_locals->a_copy, b);

#ifdef __KERNEL__
    kfree(big_locals);
#endif
}  


#if 0
/* returns true if a < b */
void 
is_less_than(field_2n *a, field_2n *b, field_boolean *result)
{
    int i;
    for ( i = 0; i < MAX_LONG; i++){
        if (a->e[i] == b->e[i]) continue;
        else{
            if (a->e[i] < b->e[i]){
                *result =  CSL_TRUE;
                return ;
            }
            else {
                *result = CSL_FALSE;
                return ;
            }
        }
    }

}
#endif

/* poly_inv severely optimised, to see the reference implementation
 * check the OLD_IMP below
 */

typedef struct {
    field_2n	f, b, c, g;
} poly_inv_big_locals;

void
poly_inv(field_2n *a, field_2n *dest)
{
    int	i, j, m, n, f_top, c_top;
    element bits;
    unsigned int longword = (NUM_BITS + 1)/WORD_SIZE;
    poly_inv_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (big_locals == NULL)
        return;  // currently has no way to tell caller of error
#else
    poly_inv_big_locals _big_locals;
    big_locals = &_big_locals;
#endif
    
    /* Set c to 0, b to 1, and n to 0 */
    poly_null(&big_locals->c);
    poly_null(&big_locals->b);
    poly_copy(a, &big_locals->f);
    poly_copy(&poly_prime, &big_locals->g);
    big_locals->b.e[longword] = 1;
    
    n = 0;
    
    
    /* Now find a polynomial b, such that a*b = u^n */
    
    /* f and g decrease and b and c increase, 
       c_top and f_top point to the edges 
    */
    c_top = longword;
    f_top = 0;
    
    /* now c is zero, so no need to shift it left, implement
       that part first */
    
    do {
        i = shift_by[big_locals->f.e[longword] & 0xff];
        n += i;
        
        if (i ==0 ) break;
        /* Shift f right i (divide by u^i) */
        m = 0;
        for ( j=f_top; j<=longword; j++ ) {
            bits = big_locals->f.e[j];
            big_locals->f.e[j] = (bits>>i) | ((element)m << (WORD_SIZE-i));
            m = bits;
        }
    } while ( i == 8 && (big_locals->f.e[longword] & 1) == 0 );
    /* while you keep shifting whole words */
    
    /* find the first significant word */
    for ( j=0; j<longword; j++ )
        if ( big_locals->f.e[j] ) break;
    if ( j<longword || big_locals->f.e[longword] != 1 ) {
        /* implement two loops, to exchange variables go to the other
         * loop 
         */	  
        do {
            /* Shorten f and g when possible */
            while ( big_locals->f.e[f_top] == 0 && big_locals->g.e[f_top] == 0 ) f_top++;
            /* f needs to be bigger - if not, exchange f with g and b with c.
             * instead of exchanging jump to the other loop
             */	
            if ( big_locals->f.e[f_top] < big_locals->g.e[f_top] ) {
                goto loop2;
            }
            
        loop1:
            /* f = f+g */
            for ( i=f_top; i<=longword; i++ )
                big_locals->f.e[i] ^= big_locals->g.e[i];
            
            /* b = b+c */
            for ( i=c_top; i<=longword; i++ )
                big_locals->b.e[i] ^= big_locals->c.e[i];
            do {
                i = shift_by[big_locals->f.e[longword] & 0xff];
                n+=i;
                /* Shift c left i (multiply by u^i)*/
                m = 0;
                for ( j=longword; j>=c_top; j-- ) {
                    bits = big_locals->c.e[j];
                    big_locals->c.e[j] = (bits<<i) | m;
                    m = bits >> (WORD_SIZE-i);
                }
                if ( m ) big_locals->c.e[c_top=j] = m;
                
                /* Shift f right i (divide by u^i) */
                m = 0;
                for ( j=f_top; j<=longword; j++ ) {
                    bits = big_locals->f.e[j];
                    big_locals->f.e[j] = (bits>>i) | ((element)m << (WORD_SIZE-i));
                    m = bits;
                }
            } while ( i == 8 && (big_locals->f.e[longword] & 1) == 0 );
            /* Check if we are done (f=1) */
            for ( j=f_top; j<longword; j++ )
                if ( big_locals->f.e[j] ) break;
        } while ( j<longword || big_locals->f.e[longword] != 1 );
        
        if ( j>0 ) 
            goto done;
        do { 
            /* do the same drill with variables reversed */
            /* Shorten f and g when possible */
            while ( big_locals->g.e[f_top] == 0 && big_locals->f.e[f_top] == 0 ) f_top++;
            
            if ( big_locals->g.e[f_top] < big_locals->f.e[f_top] ) goto loop1;
        loop2:
            
            /* g = f+g, making g divisible by u */
            for ( i=f_top; i<=longword; i++ )
                big_locals->g.e[i] ^= big_locals->f.e[i];
            /* c = b+c */
            for ( i=c_top; i<=longword; i++ )
                big_locals->c.e[i] ^= big_locals->b.e[i];
            
            do {
                i = shift_by[big_locals->g.e[longword] & 0xff];
                n+=i;
                /* Shift b left i (multiply by u^i)*/
                m = 0;
                for ( j=longword; j>=c_top; j-- ) {
                    bits = big_locals->b.e[j];
                    big_locals->b.e[j] = (bits<<i) | m;
                    m = bits >> (WORD_SIZE-i);
                }
                if ( m ) big_locals->b.e[c_top=j] = m;
		
                /* Shift g right i (divide by u^i) */
                m = 0;
                for ( j=f_top; j<=longword; j++ ) {
                    bits = big_locals->g.e[j];
                    big_locals->g.e[j] = (bits>>i) | ((element)m << (WORD_SIZE-i));
                    m = bits;
                }
            } while ( i == 8 && (big_locals->g.e[longword] & 1) == 0 );
            /* Check if we are done (g=1) */
            for ( j=f_top; j<longword; j++ )
                if ( big_locals->g.e[j] ) break;
        } while ( j<longword || big_locals->g.e[longword] != 1 );
        poly_copy(&big_locals->c, &big_locals->b);
    }
    
    /* Now b is a polynomial such that a*b = u^n, so multiply b by 
       u^(-n) */
    
done: cus_times_u_to_n(&big_locals->b, n, dest);  

#ifdef __KERNEL__
    kfree(big_locals);
#endif
} 

/*
 * void 
 * poly_rot_right(field_2n *a)
 * Purpose : polynomial rotate right once
 * input: pointers to field_2n variable
 * 
 */


void
poly_rot_right(field_2n *a)
{
    int i;
    element bit, temp;
    bit = (a->e[NUM_WORD] & 1)? UPR_BIT : 0L;
    for (i = 0; i< MAX_LONG; i++){
        temp = (a->e[i] >> 1) | bit;
        bit = (a->e[i] & 1) ? MSB : 0L;
        a->e[i] = temp;
    }
    a->e[0] &= UPR_MASK;
}
