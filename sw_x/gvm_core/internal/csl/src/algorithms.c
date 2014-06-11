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
 * Implementation of algorithms for RSA and ECC
 *
 */

#include "algorithms.h"

#include "elliptic_math.h"
#include "sha1.h"
#include "integer_math.h"
#include "conversions.h"
#include "csl_impl.h"

extern curve named_curve;
extern point named_point;

/* generate public key */

typedef struct {
    field_2n yyxy, yy, xy, xx, xxx;
    field_2n xxxa6;
    field_2n x, y;
} alg_generate_public_key_big_locals;

CSL_error 
alg_generate_public_key(point *base_point, curve *E, 
                        field_2n *my_private, point *my_public)
{
    int validate = 1;
    int i;
    element notzero;
    field_boolean isequal;
    CSL_error err = CSL_OK;
#ifndef __KERNEL__
    alg_generate_public_key_big_locals _big_locals;
#endif
    alg_generate_public_key_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(alg_generate_public_key_big_locals), GFP_KERNEL);
    if (big_locals == NULL)
        return CSL_NO_MEMORY;
#else
    big_locals = &_big_locals;
#endif
    
    /* use window = 4 only for multiply by base point */
    poly_elliptic_mul_four(my_private, base_point, my_public, E);
    
    /* do validation here */
    if (validate == 1){
        notzero = 0x0;
        for(i = 0; i < MAX_LONG; i++){
            notzero |= (my_public->x).e[i];
        }
        
        for(i = 0; i < MAX_LONG; i++){
            notzero |= (my_public->y).e[i];
        }
        
        if (notzero ==0x0){
            err = CSL_BAD_KEY;
            goto end;
        }
        
        /* check if y^2 + xy = x^3 + x^2 + a6 */
        for(i =0; i < MAX_LONG; i++){
            big_locals->x.e[i] = (my_public->x).e[i];
            big_locals->y.e[i] = (my_public->y).e[i];
        }
        
        
        poly_mul(&big_locals->x, &big_locals->y, &big_locals->xy);
        poly_mul(&big_locals->y, &big_locals->y, &big_locals->yy);
        
        /* add */
        for(i =0; i < MAX_LONG; i++){
            big_locals->yyxy.e[i] = big_locals->xy.e[i] ^ big_locals->yy.e[i];
        }
        
        poly_mul(&big_locals->x, &big_locals->x, &big_locals->xx);
        poly_mul(&big_locals->xx, &big_locals->x, &big_locals->xxx);
        
        /* add */
        for(i = 0; i< MAX_LONG; i++){
            big_locals->xxxa6.e[i] = big_locals->xxx.e[i] ^ E->a6.e[i];
            big_locals->xxxa6.e[i] = big_locals->xxxa6.e[i] ^ big_locals->xx.e[i];
        }


        /* test if equal, then point is on curve */
        isequal = CSL_TRUE;
        for(i =0; i < MAX_LONG; i++){
            if(big_locals->xxxa6.e[i] != big_locals->yyxy.e[i]){
                isequal = CSL_FALSE;
            }
        }
        if(isequal == CSL_FALSE){
            err = CSL_BAD_KEY;
            goto end;
        }
    }

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#else
    ;  // empty statement for the label
#endif

    return err;
}

/* 
 * Generate ECDH shared key from private and public keys
 */
CSL_error 
alg_generate_shared_key(point *base_point, curve *E, 
                        point *recipient_public, field_2n *my_private, 
                        field_2n *shared_secret)
{
    point tmp;
    CSL_UNUSED(base_point);
    
    poly_elliptic_mul_slow(my_private, recipient_public, &tmp, E);
    poly_copy(&tmp.x, shared_secret);
    
    return CSL_OK;
}

/* 
 * Generate ECDH shared key from precomputed public key form 
 */
CSL_error
alg_do_precompute_four(point * public_key, point *precomputed_public_key, 
                       curve *curv){
    do_precompute_four(public_key, precomputed_public_key, curv);
    return CSL_OK;
}


CSL_error 
alg_generate_shared_key_pre(point *base_point, curve *E, 
                            point *recipient_public, field_2n *my_private, 
                            field_2n *shared_secret)
{
    point tmp;
    CSL_UNUSED(base_point);
    
    poly_elliptic_mul_precomputed(my_private, recipient_public, &tmp, E);
    poly_copy(&tmp.x, shared_secret);

    return CSL_OK;
}

// check if integer is in range [1,n-1]
// n being the point order;
// returns 0 if out of range, 1 in range;

static int
loc_int_po_range(bigint_digit *i, bigint_digit *po, int ndigits)
{
	if(bigint_iszero(i, ndigits))
		return(0);
	if(bigint_cmp(i, po, ndigits) >= 0)
		return(0);
	return(1);
}

/* 
 * generate random key pair, needed for ECDSA
 * inputs random number: replace with one from 
 * hardware
 */
static CSL_error 
loc_poly_key_gen_primitive(ec_parameter *Base, ec_keypair *key, 
                           field_2n *random_input){
    bigint_digit key_num[MAX_ECC_DIGITS], point_order[MAX_ECC_DIGITS];
    bigint_digit quotient[MAX_ECC_DIGITS], remainder[MAX_ECC_DIGITS];
    int point_order_digits, key_num_digits;        
    
    bigint_zero(key_num, MAX_ECC_DIGITS);
    bigint_zero(point_order, MAX_ECC_DIGITS);
    bigint_zero(quotient, MAX_ECC_DIGITS);
    bigint_zero(remainder, MAX_ECC_DIGITS);
    
	// we need to check that the random input is not 0;
	// no need to check random>pointorder-1, because of mod op below,
	// which wraps the random instead of exiting to generate new one;

    FE2IP(random_input, key_num, MAX_ECC_DIGITS);        
    FE2IP(&Base->point_order, point_order, MAX_ECC_DIGITS);
	if(bigint_iszero(key_num, MAX_ECC_DIGITS))
		return(CSL_VERIFY_ERROR);
    key_num_digits = bigint_digits(key_num, MAX_ECC_DIGITS);
    point_order_digits = bigint_digits(point_order, MAX_ECC_DIGITS);
    bigint_div(quotient, remainder, key_num, key_num_digits, point_order, point_order_digits);
    I2FEP(remainder, &key->private_key, MAX_ECC_DIGITS);
    /* use window = 4 only for multiply with base point */
    poly_elliptic_mul_four(&key->private_key, &Base->par_point, &key->public_key, &Base->par_curve);
    
    return CSL_OK;

}

/*
 * Treat a byte string in memory as a bit array with the bits in each
 * byte arranged big endian (i.e. bit offset 0 corresponds to the
 * high order bit of the byte).
 */
static int
bitIsSet(u8 *sp, int bitOffset)
{
    return ((*(sp + (bitOffset >> 3))) & (1 << (7 - (bitOffset & 7)))) != 0;
}

/*
 * OR in a bit to a field_2n ECC big integer
 *
 * Note that bitNum is the number rather than the offset of the bit,
 * meaning that the valid values are 1 to 233.
 */
static void
setFEBit(field_2n *fe, int bitNum)
{
    int digIndex;
    int bitIndex = bitNum - 1;
    
    digIndex = bitIndex / WORD_SIZE;

    /*
     * In field_2n representation, the BIG_INT digits are ordered
     * high to low in memory
     */
    fe->e[MAX_LONG - 1 - digIndex] |= 1 << (bitIndex % WORD_SIZE);
}

/* 
 * Convert hash to a field integer, truncating or padding as required
 *
 * This routine is agnostic with respect to the digest algorithm, but the
 * higher level code uses either SHA1 or SHA256.
 */
static CSL_error 
loc_hash_to_integer(char *messagedigest, unsigned long length, 
                    bigint_digit *hash_value, unsigned int num_bits)
{
    field_2n tmp;
    int num_word;
#ifdef HOST_IS_LITTLE_ENDIAN
    int i;
#endif

    num_word = num_bits/WORD_SIZE;	
    poly_null(&tmp);

    /*
     * If the message digest has fewer bits than a field element,
     * copy and left pad with zeros.
     *
     * If the digest is longer, then take the leftmost num_bits.
     */
    if ((length << 3) <= num_bits) {
        u8 *bytep = (u8 *) &tmp;
        memcpy(bytep + (sizeof(tmp) - length), messagedigest, length);
#ifdef HOST_IS_LITTLE_ENDIAN
        for(i = 0; i <= num_word; i++){
            tmp.e[i] = byteswap(tmp.e[i]);
        }
#endif
    } else {
        int bitOffset;

        /*
         * There's got to be a more efficient way to write this,
         * but this is at least arguably correct (XXX)
         */
        for (bitOffset = 0; bitOffset < num_bits; bitOffset++) {
            if (bitIsSet((u8 *)messagedigest, bitOffset)) {
                setFEBit(&tmp, num_bits - bitOffset);
            }
        }
    }
    
    /*
     * Convert from field_2n representation to bigint representation
     */
    FE2IP(&tmp, hash_value, MAX_ECC_DIGITS);
    return CSL_OK;
}

// init ECC global state;

void 
alg_init_233_bit_ECDSA(ec_parameter *base, int num_bits)
{
    int num_word;
       
    /*
     * Init point order 
     * ascii "69017463467905637874347558622770255558
     * 39812737345013555379383634485463"
     * hardcode that parameter: we are not supporting other point orders 
     */
    bigint_digit tmpbigint[MAX_ECC_DIGITS] = {0x03cfe0d7, 0x22031d26, 
                                              0xe72f8a69, 0x0013e974, 
                                              0x00000000, 0x00000000, 
                                              0x00000000, 0x00000100}; 
    num_word = num_bits/WORD_SIZE;
    /* Init curve and base point */
    poly_elliptic_init_233_bit();

    I2FEP(tmpbigint, &(base->point_order), MAX_ECC_DIGITS);
    /* Init cofactor */
    poly_null(&(base->cofactor));
    base->cofactor.e[num_word] = 2;
    /* Init base point */
    copy_point(&named_point, &(base->par_point));
    /* Init curve */
    poly_copy(&named_curve.a2, &((base->par_curve).a2));
    poly_copy(&named_curve.a6, &((base->par_curve).a6));
    (base->par_curve).form = named_curve.form;
}

/* This is algorithm for ECDSA, enter with message pointer, size,
 * private key, and get signature: two points 
 */

typedef struct {
    bigint_digit hash_value[MAX_ECC_DIGITS], x_value[MAX_ECC_DIGITS];
    bigint_digit k_value[MAX_ECC_DIGITS], sig_value[MAX_ECC_DIGITS];
    bigint_digit c_value[MAX_ECC_DIGITS], temp[MAX_ECC_DIGITS*2];
    bigint_digit quotient[2*MAX_ECC_DIGITS];
    bigint_digit key_value[MAX_ECC_DIGITS], point_order[MAX_ECC_DIGITS];
    bigint_digit u_value[MAX_ECC_DIGITS];
    ec_keypair random_key;
} alg_poly_ECDSA_signature_big_locals;

CSL_error 
alg_poly_ECDSA_signature(char *messagedigest, unsigned long length, 
                         ec_parameter *public_curve, field_2n *private_key, 
                         ec_signature *signature, field_2n  *rand_input)
{
    int x_value_digits, point_order_digits, temp_digits;
    int not_zero;
    int i;
    int num_bits = NUM_BITS;
    CSL_error err = CSL_OK;
#ifndef __KERNEL__
    alg_poly_ECDSA_signature_big_locals _big_locals;
#endif
    alg_poly_ECDSA_signature_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(alg_poly_ECDSA_signature_big_locals), GFP_KERNEL);
    if (big_locals == NULL)
        return CSL_NO_MEMORY;
#else
    big_locals = &_big_locals;
#endif
    
    bigint_zero(big_locals->hash_value, MAX_ECC_DIGITS);
    bigint_zero(big_locals->x_value, MAX_ECC_DIGITS);
    bigint_zero(big_locals->k_value, MAX_ECC_DIGITS);
    bigint_zero(big_locals->temp, MAX_ECC_DIGITS*2);
    bigint_zero(big_locals->quotient, MAX_ECC_DIGITS*2);
    bigint_zero(big_locals->sig_value, MAX_ECC_DIGITS);
    bigint_zero(big_locals->c_value, MAX_ECC_DIGITS);
    bigint_zero(big_locals->point_order, MAX_ECC_DIGITS);
    bigint_zero(big_locals->u_value, MAX_ECC_DIGITS);
    bigint_zero(big_locals->key_value, MAX_ECC_DIGITS);
    
    loc_hash_to_integer(messagedigest, length, big_locals->hash_value, num_bits);
    
    err = loc_poly_key_gen_primitive(public_curve, &big_locals->random_key, rand_input);
    if (err != CSL_OK)
        goto end;
    FE2IP(&public_curve->point_order,big_locals->point_order,MAX_ECC_DIGITS);
    FE2IP(&big_locals->random_key.public_key.x, big_locals->x_value, MAX_ECC_DIGITS);
    
    x_value_digits = bigint_digits(big_locals->x_value, MAX_ECC_DIGITS);
    point_order_digits = bigint_digits(big_locals->point_order, MAX_ECC_DIGITS);
    bigint_div(big_locals->quotient, big_locals->c_value, big_locals->x_value, x_value_digits, big_locals->point_order, point_order_digits);
    
    I2FEP(big_locals->c_value, &signature->c, MAX_ECC_DIGITS);
    /* 
     * check if component calculated is zero. check for attack
     * described in ECDSA-johnstone, menezes, vanstone
     */
    not_zero = 0;
    for (i = 0; i < MAX_LONG; i++) {
        not_zero |= (signature->c).e[i];
    }
    if (not_zero == 0) {
        err = CSL_DIVIDE_BY_ZERO;
        goto end;
    }
    
    /* hash + priv key *c_value */
    FE2IP(private_key, big_locals->key_value, MAX_ECC_DIGITS);
    
    bigint_mult(big_locals->temp, big_locals->key_value, big_locals->c_value, MAX_ECC_DIGITS);
    bigint_add(big_locals->temp, big_locals->hash_value, big_locals->temp, MAX_ECC_DIGITS);
    
    temp_digits = bigint_digits(big_locals->temp, MAX_ECC_DIGITS*2);
    bigint_div(big_locals->quotient, big_locals->k_value, big_locals->temp, temp_digits, big_locals->point_order, point_order_digits);
    
    /* multiply by inv of random key mod order of base point */
    FE2IP(&big_locals->random_key.private_key, big_locals->temp, MAX_ECC_DIGITS);
    bigint_mod_inv(big_locals->u_value, big_locals->temp, big_locals->point_order, MAX_ECC_DIGITS);
    
    bigint_mult(big_locals->temp, big_locals->u_value, big_locals->k_value, MAX_ECC_DIGITS);
    temp_digits = bigint_digits(big_locals->temp, MAX_ECC_DIGITS*2);
    bigint_div(big_locals->quotient, big_locals->sig_value, big_locals->temp, temp_digits, big_locals->point_order, point_order_digits);
    
    I2FEP(big_locals->sig_value, &signature->d, MAX_ECC_DIGITS);
    not_zero =0;
    for(i=0; i< MAX_LONG; i++){
        not_zero |= (signature->d).e[i];
    }
    if(not_zero ==0){
        err = CSL_DIVIDE_BY_ZERO;
        goto end;
    }

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#else
    ;  // empty statement for the label
#endif

    return err;
}


struct verify_bigvars {
    bigint_digit hash_value[MAX_ECC_DIGITS], c_value[MAX_ECC_DIGITS];
    bigint_digit d_value[MAX_ECC_DIGITS];
    bigint_digit temp[2*MAX_ECC_DIGITS], quotient[2*MAX_ECC_DIGITS];
    bigint_digit h1[MAX_ECC_DIGITS], h2[MAX_ECC_DIGITS];
    bigint_digit check_value[MAX_ECC_DIGITS], point_order[MAX_ECC_DIGITS];
    point Temp1, Temp2, Verify;
    field_2n h1_field, h2_field;
};

/*
 * Call to verify ECDSA 1:verifies, 0: not verifies
 */
CSL_error 
alg_poly_ECDSA_verify(char *messagedigest, unsigned long length, 
                      ec_parameter *public_curve, point *signer_point, 
                      ec_signature *signature, field_boolean *result){
#ifdef __KERNEL__
    struct verify_bigvars *_bigvars = kmalloc(sizeof(struct verify_bigvars), GFP_KERNEL);
    bigint_digit *hash_value = _bigvars->hash_value;
    bigint_digit *c_value = _bigvars->c_value;
    bigint_digit *d_value = _bigvars->d_value;
    bigint_digit *temp = _bigvars->temp;
    bigint_digit *quotient = _bigvars->quotient;
    bigint_digit *h1 = _bigvars->h1;
    bigint_digit *h2 = _bigvars->h2;
    bigint_digit *check_value = _bigvars->check_value;
    bigint_digit *point_order = _bigvars->point_order;
#define Temp1 _bigvars->Temp1
#define Temp2 _bigvars->Temp2
#define Verify _bigvars->Verify
#define h1_field _bigvars->h1_field
#define h2_field _bigvars->h2_field
#else
    bigint_digit hash_value[MAX_ECC_DIGITS], c_value[MAX_ECC_DIGITS];
    bigint_digit d_value[MAX_ECC_DIGITS];
    bigint_digit temp[2*MAX_ECC_DIGITS], quotient[2*MAX_ECC_DIGITS];
    bigint_digit h1[MAX_ECC_DIGITS], h2[MAX_ECC_DIGITS];
    bigint_digit check_value[MAX_ECC_DIGITS], point_order[MAX_ECC_DIGITS];
    point Temp1, Temp2, Verify;
    field_2n h1_field, h2_field;
#endif    
    short int i;
    int num_bits = NUM_BITS;
    int temp_digits, point_order_digits;
    CSL_error err = CSL_OK;

    bigint_zero(hash_value, MAX_ECC_DIGITS);
    bigint_zero(c_value,MAX_ECC_DIGITS );
    bigint_zero(d_value, MAX_ECC_DIGITS);
    bigint_zero(temp, 2*MAX_ECC_DIGITS);
    bigint_zero(quotient, 2*MAX_ECC_DIGITS);
    bigint_zero(h1, MAX_ECC_DIGITS);
    bigint_zero(h2, MAX_ECC_DIGITS);
    bigint_zero(check_value, MAX_ECC_DIGITS);
    bigint_zero(point_order, MAX_ECC_DIGITS);
    
	// check c/d values in range [1,point_order-1];
    /* compute inv of second value */
    
    *result = CSL_FALSE;
    FE2IP(&public_curve->point_order, point_order, MAX_ECC_DIGITS);
    FE2IP(&signature->c, c_value, MAX_ECC_DIGITS);
    FE2IP(&signature->d, temp, MAX_ECC_DIGITS);
    if( !loc_int_po_range(c_value, point_order, MAX_ECC_DIGITS)) {
        err = CSL_VERIFY_ERROR;
        goto end;
    }
    if( !loc_int_po_range(temp, point_order, MAX_ECC_DIGITS)) {
        err = CSL_VERIFY_ERROR;
        goto end;
    }
    bigint_mod_inv(d_value, temp, point_order, MAX_ECC_DIGITS);
    
    /* generate hash */
    
    loc_hash_to_integer(messagedigest,length, hash_value, num_bits);
    
    /* compute h1, h2*/
    bigint_mult(temp, hash_value, d_value, MAX_ECC_DIGITS);
    temp_digits = bigint_digits(temp, MAX_ECC_DIGITS*2);
    point_order_digits = bigint_digits(point_order, MAX_ECC_DIGITS);
    bigint_div(quotient, h1, temp, temp_digits, point_order, point_order_digits);
    I2FEP(h1, &h1_field, MAX_ECC_DIGITS);
    
    bigint_mult(temp, d_value, c_value, MAX_ECC_DIGITS);
    
    temp_digits = bigint_digits(temp, MAX_ECC_DIGITS*2);
    bigint_div(quotient, h2, temp, temp_digits, point_order, point_order_digits);
    
    I2FEP(h2, &h2_field, MAX_ECC_DIGITS);
    /* use window =4 only for multiply with base point */
    poly_elliptic_mul_four(&h1_field, &public_curve->par_point, &Temp1, &public_curve->par_curve);
    
    
    /* find hidden point from public data */
    
    poly_elliptic_mul_slow(&h2_field, signer_point, &Temp2, 
                      &public_curve->par_curve);
    poly_elliptic_sum(&Temp1, &Temp2, &Verify, 
                      &public_curve->par_curve);
    
    /* if point is zero, return false */
    *result = CSL_FALSE;
    for(i= 0; i< MAX_LONG; i++){
        if((Verify.x.e[i] !=0x0)||(Verify.y.e[i] != 0x0)){
            *result = CSL_TRUE;
        }
    }
    if(*result == CSL_FALSE){
        err = CSL_DIVIDE_BY_ZERO;
        goto end;
    }
    
    
    /*convert x value */
    FE2IP(&Verify.x, temp, MAX_ECC_DIGITS);
    temp_digits = bigint_digits(temp, MAX_ECC_DIGITS);
    bigint_div(quotient, check_value, temp, temp_digits, point_order, point_order_digits);
    
    bigint_sub(temp, c_value, check_value, MAX_ECC_DIGITS);
    /* Need to check this: whether to wrap around point order
     */
    /*
      while(temp.half_word[0] & 0x8000)
      bigint_add(&point_order, &temp, &temp, int_max);
    */
    
    *result = CSL_TRUE;
    for(i= 0; i< MAX_ECC_DIGITS; i++){
        if(temp[i]) *result = CSL_FALSE;
    }

 end:
#ifdef __KERNEL__
    kfree(_bigvars);
#endif

    return err;

#ifdef __KERNEL__
#undef Temp1
#undef Temp2
#undef Verify
#undef h1_field
#undef h2_field
#endif
}
