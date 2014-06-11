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
#include "conversions.h"
#include "csl_impl.h"

/* conversions between integers, field elements, elliptic points and 
   octet strings as per ieee 1363 */

/*
 * encodes character string a[len] to number b[digits]
 */


unsigned int byteswap(unsigned int input)
{
    unsigned int output;
    output = ((input & 0xff) << 24 | ((input & 0xff00) << 8) | ((input & 0xff0000) >> 8) | ((input & 0xff000000) >> 24));
    return output;
}
 

/* encodes field element to integer */

void FE2IP(field_2n *a, bigint_digit *b, int digits)
{
    short int i;
    
    for(i= 0; i < digits; i++){
        b[digits-1-i] = a->e[i];    
    }
}

/* encodes integer to field element : somehow not defined in ieee 1363 */

void I2FEP(bigint_digit *a, field_2n *b, int digits)
{
    short int i;
    poly_null(b);
    for(i= 0; i < digits; i++){
        b->e[i] = a[digits-1-i];
    }
}

/* encodes octet string to elliptic curve point */

void OS2ECP(unsigned char *a, int len, point *p){
    bigint_digit x_int[MAX_ECC_DIGITS], y_int[MAX_ECC_DIGITS];
    OS2IP (x_int, MAX_ECC_DIGITS, a, len/2);
    OS2IP (y_int, MAX_ECC_DIGITS, a + (len/2), len/2);
    I2FEP(x_int, &(p->x), MAX_ECC_DIGITS);
    I2FEP(y_int, &(p->y), MAX_ECC_DIGITS);
}

/* encodes elliptic point as octet string */
void EC2OSP(point *p, unsigned char *a, int len){
    bigint_digit x_int[MAX_ECC_DIGITS], y_int[MAX_ECC_DIGITS];
    FE2IP(&(p->x), x_int, MAX_ECC_DIGITS);
    FE2IP(&(p->y), y_int, MAX_ECC_DIGITS);
    I2OSP (a, len/2, x_int, MAX_ECC_DIGITS);
    I2OSP (a + len/2, len/2, y_int, MAX_ECC_DIGITS);
}

/* encodes field element to octet string: length is OCTET_STRING_LEN */
void FE2OSP(unsigned char *a, field_2n *b){
    unsigned int outputstring[MAX_LONG];
    int i;
    unsigned int mask, shiftsize; 
    mask = OCTET_STRING_MASK;
    shiftsize = OCTET_STRING_SHIFT;
    for(i=0; i< MAX_LONG-1; i++){
        outputstring[i]  = ((b->e[i] & mask)<< (WORD_SIZE - shiftsize) |((b->e[i+1] & ~mask) >> shiftsize));
    }
    outputstring[7] = ((b->e[7] & mask)<< (WORD_SIZE - shiftsize)); 
    
#ifdef HOST_IS_LITTLE_ENDIAN
    for(i = 0; i < MAX_LONG; i++){
        outputstring[i] = byteswap(outputstring[i]);
    }
#endif
    memcpy(a, (unsigned char *) outputstring, OCTET_STRING_LEN);
}
        
/* encodes octet string to field element: length is OCTET_STRING_LEN */
void OS2FEP(unsigned char *a, field_2n *b){
    unsigned int outputfield[MAX_LONG];
    int i, j;
    unsigned int shiftsize;
    unsigned int leftnibble, rightnibble;
    shiftsize = OCTET_STRING_SHIFT;
    outputfield[0] = (unsigned int) (a[0] << BYTE_SHIFT)  | a[1] ;
    /* mask the bits */
    outputfield[0] &= UPR_MASK;
    j = 2;
    for (i = 1; i < MAX_LONG; i++) {
        leftnibble = ((unsigned int)( a[j] << BYTE_SHIFT) | a[j+1])<< (WORD_SIZE - shiftsize);
        rightnibble = ((unsigned int) (a[j+2] << BYTE_SHIFT)| a[j+3]);
        outputfield[i] = leftnibble | rightnibble;
        j += 4;
    }
    
    memcpy(b->e, outputfield, MAX_LONG*sizeof(int));
}
