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
 *
 * algorithms.h
 * algorithm prototypes.
 *
 */

#ifndef __ALGORITHMS_H__
#define __ALGORITHMS_H__


#include "csl.h"
#include "poly_math.h"
#include "elliptic_math.h"


typedef struct {
  field_2n c;
  field_2n d;
}ec_signature;


typedef struct {
  field_2n private_key;
  point public_key;
}ec_keypair;

typedef struct {
  curve par_curve;
  point par_point;
  field_2n point_order;
  field_2n cofactor;
}ec_parameter;
  

void alg_init_233_bit_ECDSA(ec_parameter *base, int num_bits);

CSL_error alg_poly_ECDSA_signature(char *messagedigest, unsigned long length, ec_parameter *public_curve, field_2n *private_key, ec_signature *signature, field_2n  *rand_input);

CSL_error alg_poly_ECDSA_verify(char *messagedigest, unsigned long length, ec_parameter *public_curve, point *signer_point, ec_signature *signature, field_boolean *result);

CSL_error alg_generate_public_key(point *base_point, curve *E, field_2n *my_private, point *my_public);


CSL_error alg_generate_shared_key(point *base_point, curve *E, point *recipient_public, field_2n *my_private, field_2n *shared_secret);

CSL_error alg_do_precompute_four(point * public_key, point *precomputed_public_key, curve *curv);

CSL_error alg_generate_shared_key_pre(point *base_point, curve *E, point *recipient_public, field_2n *my_private, field_2n *shared_secret);
#endif /* __ALGORITHMS_H__ */
