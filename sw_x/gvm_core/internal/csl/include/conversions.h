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

#ifndef __CONVERSIONS_H__
#define __CONVERSIONS_H__

#include "poly_math.h"
#include "integer_math.h"
#include "elliptic_math.h"

/* conversions between integers, field elements, elliptic points and 
   octet strings as per ieee 1363 */


unsigned int byteswap(unsigned int input);
void I2OSP (unsigned char *a, int len, bigint_digit *b, int digits);
void OS2IP (bigint_digit *a, int digits, unsigned char *b, int len);
void FE2IP(field_2n *a, bigint_digit *b, int digits);
void I2FEP(bigint_digit *a, field_2n *b, int digits);
void OS2ECP(unsigned char *a, int len, point *p);
void EC2OSP(point *p, unsigned char *a, int len);
void FE2OSP(unsigned char *a, field_2n *b);
void OS2FEP(unsigned char *a, field_2n *b);

#endif /* __CONVERSIONS_H__ */
