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


 
void I2OSP (unsigned char *a, int len, bigint_digit *b, int digits)
{
    bigint_digit t;
    int j;
    int i, u;
    
    for (i = 0, j = len - 1; i < digits && j >= 0; i++) {
        t = b[i];
        for (u = 0; j >= 0 && u < BIGINT_DIGIT_BITS; j--, u += 8)
            a[j] = (unsigned char)(t >> u);
    }
    
    for (; j >= 0; j--)
        a[j] = 0;
}

/* need to add this */
void OS2IP (bigint_digit *a, int digits, unsigned char *b, int len){
    bigint_digit t;
    int i, j, u;
    
    for(i =0, j=len-1; i < digits && j >=0; i++){
        t = 0;
        for(u=0; j >= 0 && u < BIGINT_DIGIT_BITS; j--, u+=8)
            t |= ((bigint_digit)b[j] ) << u;
        a[i] = t;
    }
    for(; i < digits; i++)
        a[i] = 0;
}

