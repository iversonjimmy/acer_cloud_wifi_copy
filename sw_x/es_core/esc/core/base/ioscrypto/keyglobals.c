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
 * The keystore is an array of elements, each element containing key material
 * of fixed length (eg, 128 bits), and associated data. Longer key material is 
 * split over many elements. Elements corresponding to one key need not be 
 * contiguous, the element contains the pointer to the next element.
 * The key handle is the array index of the first element that contains 
 * material of that key.
 */

#include "keystore.h"

keyMetaElem keyMeta[KEY_META_MAX_SIZE];
keyStoreElem keyStore[KEY_STORE_MAX_SIZE];

