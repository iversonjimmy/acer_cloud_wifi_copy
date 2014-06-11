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
 * elliptic_math.h
 * elliptic curve operations
 * only deals with polynomial basis and support for 
 * DH and ECDSA
 * equation: y^2 + xy = x^3 + a x^2 + b
 * no point compression
*/



#ifndef __ELLIPTIC_MATH_H__
#define __ELLIPTIC_MATH_H__


#include "poly_math.h"

typedef struct {
	short int form;
	field_2n a2;
	field_2n a6;
}curve;


typedef struct {
	field_2n x;
	field_2n y;
}point;

void poly_elliptic_init_233_bit(void);
void copy_point(point *p1, point *p2);
void print_field(char *string, field_2n *field);
void print_point(char *string, point *point);

void poly_elliptic_sum(point *p1, point *p2, point *p3, curve *curv);
void poly_elliptic_double(point *p1, point *p3, curve *curv);
void poly_elliptic_sub(point *p1, point *p2, point *p3, curve *curv);
/* this is generic, uses window = 2, caches precomputed values */
void poly_elliptic_mul(field_2n *k, point *p, point *r, curve *curv);
void do_precompute_four(point *p, point *precompute, curve *curv);
void poly_elliptic_mul_precomputed(field_2n *k, point *precompute, point *r, 
curve *curv);
/* fast version, use only if the point is base point. window = 4 */
void poly_elliptic_mul_four(field_2n *k, point *p, point *r, curve *curv);
/* generic version without precomputation, binary NAF */
void poly_elliptic_mul_slow(field_2n *k, point *p, point *r, curve *curv);

#define CSL_UNUSED(var) ((void)&var)

#endif /* __ELLIPTIC_MATH_H__ */
