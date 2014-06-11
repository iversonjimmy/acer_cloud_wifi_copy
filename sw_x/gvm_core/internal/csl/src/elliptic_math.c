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
 * elliptic_math.c 
 * common operations for elliptic curve, polynomial basis
 * only, all thats needed for elliptic multiplication
*/

#include "elliptic_math.h"

#include "csl_impl.h"

#include "poly_math.h"


/* constants for FIPS curve for 233 bits 
 */


curve named_curve;
point named_point;
point precomputed_bp[16];

point precomputed_signer={{{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}},
			  {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff,0xffffffff, 0xffffffff, 0xffffffff}}};
/* caches public key precomputation for one signer */
point precomputed_pk[4]={{{{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}},
			  {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff,0xffffffff, 0xffffffff, 0xffffffff}}},
			 {{{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}},
			  {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff,0xffffffff, 0xffffffff, 0xffffffff}}},
			 {{{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}},
			  {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff,0xffffffff, 0xffffffff, 0xffffffff}}},
			 {{{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}},
			  {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			    0xffffffff,0xffffffff, 0xffffffff, 0xffffffff}}}};

			      
void 
poly_elliptic_init_233_bit(void)
{
    named_curve.form = 1;
  
    named_curve.a6.e[0] = 0x66;
    named_curve.a6.e[1] = 0x647ede6c;
    named_curve.a6.e[2] = 0x332c7f8c;
    named_curve.a6.e[3] = 0x0923bb58;
    named_curve.a6.e[4] = 0x213b333b;
    named_curve.a6.e[5] = 0x20e9ce42;
    named_curve.a6.e[6] = 0x81fe115f;
    named_curve.a6.e[7] = 0x7d8f90ad;
    
    named_curve.a2.e[0] = 0x0;
    named_curve.a2.e[1] = 0x0;
    named_curve.a2.e[2] = 0x0;
    named_curve.a2.e[3] = 0x0;
    named_curve.a2.e[4] = 0x0;
    named_curve.a2.e[5] = 0x0;
    named_curve.a2.e[6] = 0x0;
    named_curve.a2.e[7] = 0x1;
    
  
    named_point.x.e[0] = 0x0fa;
    named_point.x.e[1] = 0xc9dfcbac;
    named_point.x.e[2] = 0x8313bb21;
    named_point.x.e[3] = 0x39f1bb75;
    named_point.x.e[4] = 0x5fef65bc;
    named_point.x.e[5] = 0x391f8b36;
    named_point.x.e[6] = 0xf8f8eb73;
    named_point.x.e[7] = 0x71fd558b;
    
    named_point.y.e[0] = 0x100;
    named_point.y.e[1] = 0x6a08a419;
    named_point.y.e[2] = 0x03350678;
    named_point.y.e[3] = 0xe58528be;
    named_point.y.e[4] = 0xbf8a0bef;
    named_point.y.e[5] = 0xf867a7ca;
    named_point.y.e[6] = 0x36716f7e;
    named_point.y.e[7] = 0x01f81052;


  /* precomputed table lookup values for base point for making 
     public key calculation faster 
  */
    precomputed_bp[0].x.e[0] = 0x00000000;
    precomputed_bp[0].x.e[1] = 0x00000000;
    precomputed_bp[0].x.e[2] = 0x00000000;
    precomputed_bp[0].x.e[3] = 0x00000000;
    precomputed_bp[0].x.e[4] = 0x00000000;
    precomputed_bp[0].x.e[5] = 0x00000000;
    precomputed_bp[0].x.e[6] = 0x00000000;
    precomputed_bp[0].x.e[7] = 0x00000000;
    
    precomputed_bp[0].y.e[0] = 0x00000000;
    precomputed_bp[0].y.e[1] = 0x00000000;
    precomputed_bp[0].y.e[2] = 0x00000000;
    precomputed_bp[0].y.e[3] = 0x00000000;
    precomputed_bp[0].y.e[4] = 0x00000000;
    precomputed_bp[0].y.e[5] = 0x00000000;
    precomputed_bp[0].y.e[6] = 0x00000000;
    precomputed_bp[0].y.e[7] = 0x00000000;
 
    precomputed_bp[1].x.e[0] = 0x000000fa;
    precomputed_bp[1].x.e[1] = 0xc9dfcbac;
    precomputed_bp[1].x.e[2] = 0x8313bb21;
    precomputed_bp[1].x.e[3] = 0x39f1bb75;
    precomputed_bp[1].x.e[4] = 0x5fef65bc;
    precomputed_bp[1].x.e[5] = 0x391f8b36;
    precomputed_bp[1].x.e[6] = 0xf8f8eb73;
    precomputed_bp[1].x.e[7] = 0x71fd558b;
    
    precomputed_bp[1].y.e[0] = 0x00000100;
    precomputed_bp[1].y.e[1] = 0x6a08a419;
    precomputed_bp[1].y.e[2] = 0x03350678;
    precomputed_bp[1].y.e[3] = 0xe58528be;
    precomputed_bp[1].y.e[4] = 0xbf8a0bef;
    precomputed_bp[1].y.e[5] = 0xf867a7ca;
    precomputed_bp[1].y.e[6] = 0x36716f7e;
    precomputed_bp[1].y.e[7] = 0x01f81052;
    
    precomputed_bp[2].x.e[0] = 0x00000123;
    precomputed_bp[2].x.e[1] = 0xd731a036;
    precomputed_bp[2].x.e[2] = 0x6b9ed354;
    precomputed_bp[2].x.e[3] = 0x2323a649;
    precomputed_bp[2].x.e[4] = 0x7cf718ab;
    precomputed_bp[2].x.e[5] = 0x012fe27b;
    precomputed_bp[2].x.e[6] = 0x5cb0f320;
    precomputed_bp[2].x.e[7] = 0xf7b10aed;
    
    precomputed_bp[2].y.e[0] = 0x00000163;
    precomputed_bp[2].y.e[1] = 0x6760ce8f;
    precomputed_bp[2].y.e[2] = 0x4eaae9ba;
    precomputed_bp[2].y.e[3] = 0xf12a1514;
    precomputed_bp[2].y.e[4] = 0x67a7b3f4;
    precomputed_bp[2].y.e[5] = 0xb92a0e61;
    precomputed_bp[2].y.e[6] = 0xfa190593;
    precomputed_bp[2].y.e[7] = 0x00100a80;
 
    precomputed_bp[3].x.e[0] = 0x000001c6;
    precomputed_bp[3].x.e[1] = 0x40b411fe;
    precomputed_bp[3].x.e[2] = 0xa77c6cdc;
    precomputed_bp[3].x.e[3] = 0x4a587fc3;
    precomputed_bp[3].x.e[4] = 0x46a40358;
    precomputed_bp[3].x.e[5] = 0x7856edb0;
    precomputed_bp[3].x.e[6] = 0x193ea71f;
    precomputed_bp[3].x.e[7] = 0xa7524b93;
    
    precomputed_bp[3].y.e[0] = 0x00000104;
    precomputed_bp[3].y.e[1] = 0xa659102f;
    precomputed_bp[3].y.e[2] = 0x034b0bb8;
    precomputed_bp[3].y.e[3] = 0xafb5afa6;
    precomputed_bp[3].y.e[4] = 0x1b8adf8c;
    precomputed_bp[3].y.e[5] = 0xb6cca538;
    precomputed_bp[3].y.e[6] = 0x631bf941;
    precomputed_bp[3].y.e[7] = 0x3c5d1bfa;
 
    precomputed_bp[4].x.e[0] = 0x00000084;
    precomputed_bp[4].x.e[1] = 0xc44003bb;
    precomputed_bp[4].x.e[2] = 0x3da842d2;
    precomputed_bp[4].x.e[3] = 0x7022ef1c;
    precomputed_bp[4].x.e[4] = 0x3264a00e;
    precomputed_bp[4].x.e[5] = 0x70ed2f1a;
    precomputed_bp[4].x.e[6] = 0x42027637;
    precomputed_bp[4].x.e[7] = 0xe48a3a6d;
 
    precomputed_bp[4].y.e[0] = 0x0000007b;
    precomputed_bp[4].y.e[1] = 0x91e07dd6;
    precomputed_bp[4].y.e[2] = 0x9ef7f6a2;
    precomputed_bp[4].y.e[3] = 0xe1f72622;
    precomputed_bp[4].y.e[4] = 0x135f0d1d;
    precomputed_bp[4].y.e[5] = 0xbbcc886e;
    precomputed_bp[4].y.e[6] = 0x85ccd0f7;
    precomputed_bp[4].y.e[7] = 0xda1f6d98;
 
    precomputed_bp[5].x.e[0] = 0x00000096;
    precomputed_bp[5].x.e[1] = 0x15d88a99;
    precomputed_bp[5].x.e[2] = 0xceb0b782;
    precomputed_bp[5].x.e[3] = 0xc4794801;
    precomputed_bp[5].x.e[4] = 0xe0378bfb;
    precomputed_bp[5].x.e[5] = 0x96b102be;
    precomputed_bp[5].x.e[6] = 0x4d8bc62b;
    precomputed_bp[5].x.e[7] = 0xf640fd06;
    
    precomputed_bp[5].y.e[0] = 0x000001d2;
    precomputed_bp[5].y.e[1] = 0xa79fa10b;
    precomputed_bp[5].y.e[2] = 0x9e8d0426;
    precomputed_bp[5].y.e[3] = 0xb22d2db7;
    precomputed_bp[5].y.e[4] = 0x410004c8;
    precomputed_bp[5].y.e[5] = 0xc5ef7bca;
    precomputed_bp[5].y.e[6] = 0x4bd020a2;
    precomputed_bp[5].y.e[7] = 0x7bc255c3;
    
    precomputed_bp[6].x.e[0] = 0x000000f8;
    precomputed_bp[6].x.e[1] = 0x9335641a;
    precomputed_bp[6].x.e[2] = 0x6d52d1b7;
    precomputed_bp[6].x.e[3] = 0x1fe5286a;
    precomputed_bp[6].x.e[4] = 0xb5d18eb1;
    precomputed_bp[6].x.e[5] = 0xb2a6bf0a;
    precomputed_bp[6].x.e[6] = 0x47e4a3fb;
    precomputed_bp[6].x.e[7] = 0x95b18299;
 
    precomputed_bp[6].y.e[0] = 0x00000114;
    precomputed_bp[6].y.e[1] = 0x011918fc;
    precomputed_bp[6].y.e[2] = 0xe2afb564;
    precomputed_bp[6].y.e[3] = 0x384e6a3f;
    precomputed_bp[6].y.e[4] = 0x4fa20321;
    precomputed_bp[6].y.e[5] = 0xf7dce7e2;
    precomputed_bp[6].y.e[6] = 0x316149dd;
    precomputed_bp[6].y.e[7] = 0x83816459;
 
    precomputed_bp[7].x.e[0] = 0x0000000d;
    precomputed_bp[7].x.e[1] = 0xb61258ca;
    precomputed_bp[7].x.e[2] = 0x728a0565;
    precomputed_bp[7].x.e[3] = 0x128ea0f8;
    precomputed_bp[7].x.e[4] = 0x3a67e3e7;
    precomputed_bp[7].x.e[5] = 0xcb752032;
    precomputed_bp[7].x.e[6] = 0x1d1b0144;
    precomputed_bp[7].x.e[7] = 0x2e390e0f;
 
    precomputed_bp[7].y.e[0] = 0x0000008a;
    precomputed_bp[7].y.e[1] = 0x90174ec0;
    precomputed_bp[7].y.e[2] = 0x8bcaef7c;
    precomputed_bp[7].y.e[3] = 0xafedb681;
    precomputed_bp[7].y.e[4] = 0x3817f429;
    precomputed_bp[7].y.e[5] = 0x9167bb6d;
    precomputed_bp[7].y.e[6] = 0x5345180f;
    precomputed_bp[7].y.e[7] = 0x9f17791d;
    
    precomputed_bp[8].x.e[0] = 0x00000041;
    precomputed_bp[8].x.e[1] = 0xa7ab0b40;
    precomputed_bp[8].x.e[2] = 0x47234421;
    precomputed_bp[8].x.e[3] = 0x54a87fc5;
    precomputed_bp[8].x.e[4] = 0x8278556d;
    precomputed_bp[8].x.e[5] = 0x83618e53;
    precomputed_bp[8].x.e[6] = 0x1f9c4c7c;
    precomputed_bp[8].x.e[7] = 0x3816c879;
    
    precomputed_bp[8].y.e[0] = 0x000000ba;
    precomputed_bp[8].y.e[1] = 0x7a211a5d;
    precomputed_bp[8].y.e[2] = 0xbc8de196;
    precomputed_bp[8].y.e[3] = 0x48a65332;
    precomputed_bp[8].y.e[4] = 0x907318e2;
    precomputed_bp[8].y.e[5] = 0xd9205a76;
    precomputed_bp[8].y.e[6] = 0xc64a2da1;
    precomputed_bp[8].y.e[7] = 0x9f6972f2;
    
    precomputed_bp[9].x.e[0] = 0x00000122;
    precomputed_bp[9].x.e[1] = 0x9a00ca04;
    precomputed_bp[9].x.e[2] = 0x6dc401be;
    precomputed_bp[9].x.e[3] = 0x2335bb37;
    precomputed_bp[9].x.e[4] = 0x59c7c243;
    precomputed_bp[9].x.e[5] = 0xf884dea8;
    precomputed_bp[9].x.e[6] = 0xd835dfed;
    precomputed_bp[9].x.e[7] = 0x00eabd47;
    
    precomputed_bp[9].y.e[0] = 0x00000082;
    precomputed_bp[9].y.e[1] = 0xfbe8c10b;
    precomputed_bp[9].y.e[2] = 0x7532ac2b;
    precomputed_bp[9].y.e[3] = 0x6be6db88;
    precomputed_bp[9].y.e[4] = 0xebcc7739;
    precomputed_bp[9].y.e[5] = 0x2b267e5e;
    precomputed_bp[9].y.e[6] = 0xa23a88ea;
    precomputed_bp[9].y.e[7] = 0x69da2f73;
    
    precomputed_bp[10].x.e[0] = 0x00000100;
    precomputed_bp[10].x.e[1] = 0x85e45b07;
    precomputed_bp[10].x.e[2] = 0xe6ef1d6e;
    precomputed_bp[10].x.e[3] = 0x87d6d582;
    precomputed_bp[10].x.e[4] = 0x623db5d0;
    precomputed_bp[10].x.e[5] = 0x65182781;
    precomputed_bp[10].x.e[6] = 0xe5c3f825;
    precomputed_bp[10].x.e[7] = 0x138ad7e8;
    
    precomputed_bp[10].y.e[0] = 0x0000019f;
    precomputed_bp[10].y.e[1] = 0xf9609cd0;
    precomputed_bp[10].y.e[2] = 0x4c8fa4c5;
    precomputed_bp[10].y.e[3] = 0x9862e9cb;
    precomputed_bp[10].y.e[4] = 0x864f72d4;
    precomputed_bp[10].y.e[5] = 0x362665dd;
    precomputed_bp[10].y.e[6] = 0xc30643db;
    precomputed_bp[10].y.e[7] = 0x59e4b08a;
    
    precomputed_bp[11].x.e[0] = 0x0000017f;
    precomputed_bp[11].x.e[1] = 0xcd50381e;
    precomputed_bp[11].x.e[2] = 0xa66df765;
    precomputed_bp[11].x.e[3] = 0x5cf43cf7;
    precomputed_bp[11].x.e[4] = 0x3d0788a6;
    precomputed_bp[11].x.e[5] = 0x8938b3e5;
    precomputed_bp[11].x.e[6] = 0x5012df2b;
    precomputed_bp[11].x.e[7] = 0x8c1589e5;
    
    precomputed_bp[11].y.e[0] = 0x00000017;
    precomputed_bp[11].y.e[1] = 0xf5719186;
    precomputed_bp[11].y.e[2] = 0xd0eefc0c;
    precomputed_bp[11].y.e[3] = 0x4aac1fd7;
    precomputed_bp[11].y.e[4] = 0x95ce8eda;
    precomputed_bp[11].y.e[5] = 0x81d8b45a;
    precomputed_bp[11].y.e[6] = 0x3b132ee2;
    precomputed_bp[11].y.e[7] = 0x770851d2;
    
    precomputed_bp[12].x.e[0] = 0x000000c1;
    precomputed_bp[12].x.e[1] = 0x4b5349c9;
    precomputed_bp[12].x.e[2] = 0x4c748262;
    precomputed_bp[12].x.e[3] = 0xe39e7051;
    precomputed_bp[12].x.e[4] = 0x71e5a2e8;
    precomputed_bp[12].x.e[5] = 0xaf258bc9;
    precomputed_bp[12].x.e[6] = 0x7efa83eb;
    precomputed_bp[12].x.e[7] = 0x8ed8777e;
    
    precomputed_bp[12].y.e[0] = 0x0000019e;
    precomputed_bp[12].y.e[1] = 0x923caf64;
    precomputed_bp[12].y.e[2] = 0x5601af9b;
    precomputed_bp[12].y.e[3] = 0x0f5d1c6c;
    precomputed_bp[12].y.e[4] = 0x48de1ee6;
    precomputed_bp[12].y.e[5] = 0xf438e699;
    precomputed_bp[12].y.e[6] = 0x0e0a7651;
    precomputed_bp[12].y.e[7] = 0xf834cfd1;
    
    precomputed_bp[13].x.e[0] = 0x00000156;
    precomputed_bp[13].x.e[1] = 0x0a419bc4;
    precomputed_bp[13].x.e[2] = 0x84828ae2;
    precomputed_bp[13].x.e[3] = 0xd0114081;
    precomputed_bp[13].x.e[4] = 0x00697d40;
    precomputed_bp[13].x.e[5] = 0xc2bb840a;
    precomputed_bp[13].x.e[6] = 0xb626775f;
    precomputed_bp[13].x.e[7] = 0x40955dde;
    
    precomputed_bp[13].y.e[0] = 0x000001e0;
    precomputed_bp[13].y.e[1] = 0xafcb6b6a;
    precomputed_bp[13].y.e[2] = 0x2efb3ae6;
    precomputed_bp[13].y.e[3] = 0x68ec3fac;
    precomputed_bp[13].y.e[4] = 0x4d9524b9;
    precomputed_bp[13].y.e[5] = 0x8503e4fe;
    precomputed_bp[13].y.e[6] = 0x3d0c3039;
    precomputed_bp[13].y.e[7] = 0xbf9410ac;
    
    precomputed_bp[14].x.e[0] = 0x00000149;
    precomputed_bp[14].x.e[1] = 0x9459c5b9;
    precomputed_bp[14].x.e[2] = 0x935f6c3b;
    precomputed_bp[14].x.e[3] = 0x223fe1c0;
    precomputed_bp[14].x.e[4] = 0x76ccb5e9;
    precomputed_bp[14].x.e[5] = 0x9d810fbb;
    precomputed_bp[14].x.e[6] = 0x8d051e14;
    precomputed_bp[14].x.e[7] = 0x3fc6ad47;
    
    precomputed_bp[14].y.e[0] = 0x00000119;
    precomputed_bp[14].y.e[1] = 0x7d175b61;
    precomputed_bp[14].y.e[2] = 0x4da9494c;
    precomputed_bp[14].y.e[3] = 0x3fe040b9;
    precomputed_bp[14].y.e[4] = 0x43017773;
    precomputed_bp[14].y.e[5] = 0xd2061ea2;
    precomputed_bp[14].y.e[6] = 0xe9c9356c;
    precomputed_bp[14].y.e[7] = 0xff0f3d83;
    
    precomputed_bp[15].x.e[0] = 0x0000010b;
    precomputed_bp[15].x.e[1] = 0x47a49b1f;
    precomputed_bp[15].x.e[2] = 0xf7c75c45;
    precomputed_bp[15].x.e[3] = 0xa9d578f2;
    precomputed_bp[15].x.e[4] = 0xb07cc152;
    precomputed_bp[15].x.e[5] = 0x80b18075;
    precomputed_bp[15].x.e[6] = 0x9ae882b1;
    precomputed_bp[15].x.e[7] = 0x8bc66c86;
    
    precomputed_bp[15].y.e[0] = 0x000001b4;
    precomputed_bp[15].y.e[1] = 0xd626657d;
    precomputed_bp[15].y.e[2] = 0x510fdcf0;
    precomputed_bp[15].y.e[3] = 0xc0cff138;
    precomputed_bp[15].y.e[4] = 0x75653e50;
    precomputed_bp[15].y.e[5] = 0xd1465cc5;
    precomputed_bp[15].y.e[6] = 0x49571cda;
    precomputed_bp[15].y.e[7] = 0x0ec46b60;
    
 
}

/* 
 * CSL_error 
 * copy_point() 
 * Purpose: to copy one big integer to another: copy p1 into p2
 * inputs: pointer to allocated points
 */

void
copy_point(point *p1, point *p2)
{
    poly_copy(&p1->x, &p2->x); /*field_2n copy*/
    poly_copy(&p1->y, &p2->y); 
    return;
}
  

/* 
 * void 
 * poly_elliptic_sum() 
 * polynomial elliptic sum from schroeppel's paper
 * point at infinity is represented by (0,0) 
 * purpose: p3 = p1 + p2
 */


void 
poly_elliptic_sum(point *p1, point *p2, point *p3, curve *curv)
{
    short int i;
    field_2n x1, y1, lambda, onex, lambda2;
    element check;

    /*check if either point is at infinity*/
    check = 0;
    for(i = 0; i < MAX_LONG; i++){
      check |= p1->x.e[i] | p1->y.e[i];
    }
  /*point x is at infinity?*/
    if(!check){ 
        copy_point(p2, p3);
        return;
    }
    check =0;
    for(i = 0; i < MAX_LONG; i++){
        check |= p2->x.e[i] | p2->y.e[i];
    }
    /*point y is at infinity? */
    if(!check){
        copy_point(p1, p3);
        return;
    }
    
    /* check if we are doubling! special equations*/
    check =0;
    for(i = 0; i < MAX_LONG; i++){
        if((p1->x.e[i] != p2->x.e[i])||(p1->y.e[i] != p2->y.e[i])){
            check =1;
        }
    }

    if (check != 1){ /* all equal: call doubling*/
        poly_elliptic_double(p1, p3, curv);
        return;
    }


    /* compute lambda, this is not doubling, so if 
     * denominator is zero, exit now with point at infinity
     * to avoid inverting zero
     */
    poly_null(&x1);
    poly_null(&y1);
    check =0;
    for(i=0; i< MAX_LONG; i++){
        x1.e[i] = p1->x.e[i] ^ p2->x.e[i];
        y1.e[i] = p1->y.e[i] ^ p2->y.e[i];
        /*anyway compute x + y*/
        check |= x1.e[i];
    }
    if(!check) {
        poly_null(&p3->x);
        poly_null(&p3->y);
        return;
    }

    poly_inv(&x1, &onex);
    poly_mul(&onex, &y1, &lambda);
    poly_mul(&lambda, &lambda, &lambda2);

    /* compute x3*/
    if(curv->form){ /*x^3 + ax^2 + b*/
        for(i=0; i< MAX_LONG; i++){
            p3->x.e[i]  = lambda.e[i] ^ lambda2.e[i] ^ x1.e[i] ^ curv->a2.e[i];
        }
    }
    else{
      /* we do not support other forms, so quit instead of causing 
       * unexpected computations
       */
#if 0
        for(i=0; i< MAX_LONG; i++){
            p3->x.e[i]  = lambda.e[i] ^ lambda2.e[i] ^ x1.e[i]; /* no a*/
        }
#endif
	return;
    }
  
  /* y3*/
    for(i=0; i< MAX_LONG; i++){
        x1.e[i] = p1->x.e[i] ^ p3->x.e[i];
    }
    poly_mul(&x1, &lambda, &lambda2);
    for(i=0; i< MAX_LONG; i++){
        p3->y.e[i] = lambda2.e[i] ^ p3->x.e[i] ^ p1->y.e[i];
    }
    return;
}


/* 
 * void 
 * poly_elliptic_double() 
 * polynomial elliptic double from schroeppel's paper
 * point at infinity is represented by (0,0) 
 * purpose: p3 = p1 + p1
 */

void 
poly_elliptic_double(point *p1, point *p3, curve *curv)
{
    field_2n x1, y1, lambda, lambda2, t1;
    short int i;
    element check;

    check =0; 
    /* check if input is zero*/
    for(i=0; i< MAX_LONG; i++){
        check |= p1->x.e[i];
    }
    if(!check){
        poly_null(&p3->x);
        poly_null(&p3->y);
        return;
    }
  
  
    /*lambda = x + y/x*/
    poly_inv(&p1->x, &x1);
    poly_mul(&x1, &p1->y, &y1);
  

    /*add to x*/
    for(i =0; i< MAX_LONG; i++){
        lambda.e[i] = p1->x.e[i] ^ y1.e[i];
    }
  
    /* x3*/
    poly_mul(&lambda, &lambda, &lambda2);
    if(curv->form){
      for(i = 0; i< MAX_LONG; i++){
	p3->x.e[i] = lambda.e[i] ^ lambda2.e[i] ^ curv->a2.e[i];
      }
    }
    else{
      /* again, other forms are not supported. exit instead
       * of causing unexpected computations
       */
#if 0
      for(i = 0; i< MAX_LONG; i++){
	p3->x.e[i] = lambda.e[i] ^ lambda2.e[i];
      }
#endif
      return;
    }
    /*y3*/
    lambda.e[NUM_WORD] ^= 1; /*add 1*/
    poly_mul(&lambda, &p3->x, &t1);
    poly_mul(&p1->x, &p1->x, &x1);
    for(i =0; i< MAX_LONG; i++){
        p3->y.e[i] = x1.e[i] ^ t1.e[i];
    }
    return;
}



/* 
 * void 
 * poly_elliptic_sub() 
 * 
 * point at infinity is represented by (0,0) 
 * purpose: p3 = p1 - p2
 * subtracting two points by negate and add
 * rule for negating: -1(x, y) = (x, x+y)
 */

void 
poly_elliptic_sub(point  *p1, point *p2, point *p3, curve *curv){
    point temp;
    short int i;
    poly_copy(&p2->x, &temp.x);
    poly_null(&temp.y);
    for(i=0; i < MAX_LONG; i++){
        temp.y.e[i] = p2->x.e[i] ^ p2->y.e[i];
    }
    poly_elliptic_sum(p1, &temp, p3, curv);
    return;
}




/* 
 * void 
 * poly_elliptic_mul_slow() 
 * 
 * point at infinity is represented by (0,0) 
 * purpose: p3 = k * p2 where k is an integer
 * multiplication using balanced representation: check Solinas
 * paper: an improved algorithm for arithmetic on a family of 
 * elliptic curves

 * uses binary NAF, no precomputation, generic type.
 */
typedef struct {
    signed char balanced[NUM_BITS + 1];
    field_2n number;
    point temp;
} poly_elliptic_mul_slow_big_locals;

void 
poly_elliptic_mul_slow(field_2n *k, point *p, point *r, curve *curv)
{
    short int i, bit_count;
    element notzero;

    poly_elliptic_mul_slow_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return;  // currently no way to tell caller of error
#else
    poly_elliptic_mul_slow_big_locals _big_locals;
    big_locals = &_big_locals;
#endif
  
    /* check for multiplier zero, if so return point at inf */
    poly_copy (k, &big_locals->number);
    notzero =0;
    for(i=0; i< MAX_LONG; i++){
      notzero |= big_locals->number.e[i];
    }
    if(!notzero){
        poly_null(&r->x);
        poly_null(&r->y);
        goto end;
    }

    /* find balanced representation: need reference */

    bit_count =0;
    while(notzero){
      /* if number odd, create 1 or -1 from last two bits */
        if(big_locals->number.e[NUM_WORD] & 1){
            big_locals->balanced[bit_count] = 2  - (big_locals->number.e[NUM_WORD] & 3);
            /* if -1, add 1 and propogate carry bit */
            if(big_locals->balanced[bit_count] < 0){
                for(i= NUM_WORD; i >= 0; i--){
                    big_locals->number.e[i]++;
                    if(big_locals->number.e[i]) break;
                }
            }
        }
        else{
            big_locals->balanced[bit_count] = 0;
        }
        big_locals->number.e[NUM_WORD] &= (element) (~0 << 1);
        poly_rot_right(&big_locals->number);
        bit_count++;
        notzero =0; 
        for(i =0; i< MAX_LONG; i++){
            notzero |= big_locals->number.e[i];
        }
    }

    /* now compute kP*/
    bit_count--;
    copy_point(p, r);
  
    while(bit_count >0){
    
        poly_elliptic_double(r, &big_locals->temp, curv);
    
        bit_count--;
        switch(big_locals->balanced[bit_count]){
        case 1:   
            poly_elliptic_sum(p, &big_locals->temp, r, curv); 
            break;
        case -1: 
            poly_elliptic_sub(&big_locals->temp, p, r, curv); 
            break;
        case 0: copy_point(&big_locals->temp, r);       
       }
    }

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif

    return;
}
/* code to use precomputed form of public key for ECDH as well */

/* precompute 16 values for generic point */
void
do_precompute_four(point *p, point *precompute, curve *curv)
{
 
    int i; 
    field_2n temp;
    
    /* do precomputation of all 4 bit words * P */
    for(i =0; i < 16; i++){
        /* put i in the field suitably: ! */
        poly_null(&temp);
        temp.e[1] = (i & 0x8)>>3; /* 4th bit from right */
        temp.e[3] = (i & 0x4)>>2; /* 3rd bit from right */
        temp.e[5] = (i & 0x2)>>1; /* 2nd bit from right */
        temp.e[7] = (i & 0x1); /* right most bit */
        poly_elliptic_mul_slow(&temp, p, precompute + i, curv); /*use ith index*/
    }
    return;
}

typedef struct {
    element columns[64];
    point temppoint;
    field_2n number;
} poly_elliptic_mul_precomputed_big_locals;

void 
poly_elliptic_mul_precomputed(field_2n *k, point *precompute, point *r, curve *curv)
{
    int i, j;
    element mask;
    element notzero;
    int bit_num;

    poly_elliptic_mul_precomputed_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return;  // currently no way to tell caller of error
#else
    poly_elliptic_mul_precomputed_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

/*
  point precompute[16];
  do_precompute_four(p, precompute, curv);   
*/
    /* check for multiplier zero, if so return point at inf */
    poly_copy (k, &big_locals->number);

    notzero =0;
    for(i=0; i< MAX_LONG; i++){
        notzero |= big_locals->number.e[i];
    }
    
    if(!notzero){
        poly_null(&r->x);
        poly_null(&r->y);
        goto end;
    }
    
    /* 
     * convert rows to columns, read off columns of [k0 k1, k2 k3, k4 k5, k6 k7]
     */
    
    mask = 0x80000000;
    for(i=0; i< 32; i++){
        big_locals->columns[i] =0;
        bit_num = 3;
        for(j =0; j < 8; j=j+2){
            big_locals->columns[i] |= ((k->e[j] & mask)>>(32-i-1))<<bit_num;
            bit_num--;
        }
        mask = mask >>1;
    }
    mask = 0x80000000;
    for(i=32; i< 64; i++){
        big_locals->columns[i] =0;
        bit_num = 3;
        for(j =1; j < 8; j=j+2){
            big_locals->columns[i] |= ((k->e[j] & mask)>>(32-(i-32)-1))<<bit_num;
            bit_num--;
        }
        mask = mask >>1;
    }
    
    poly_null(&(r->x));
    poly_null(&(r->y));
    for(i=0; i<64; i++){
        poly_elliptic_double(r, &big_locals->temppoint, curv);
        poly_elliptic_sum(&big_locals->temppoint, (precompute + ((int)big_locals->columns[i])), r, curv);
	/* precompute is array of points, index into column[i]th element */
    }

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif

    return;
}

typedef struct {
    element columns[64];
    point temppoint;
    field_2n number;
} poly_elliptic_mul_four_big_locals;

void 
poly_elliptic_mul_four(field_2n *k, point *p, point *r, curve *curv)
{
    int i, j;
    element mask;
    element notzero;
    int bit_num;

    poly_elliptic_mul_four_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return;  // currently no way to tell caller of error
#else
    poly_elliptic_mul_four_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    CSL_UNUSED(p);
    
    /* check for multiplier zero, if so return point at inf */
    poly_copy (k, &big_locals->number);
    
    notzero =0;
    for(i=0; i< MAX_LONG; i++){
        notzero |= big_locals->number.e[i];
    }
    
    if(!notzero){
        poly_null(&r->x);
        poly_null(&r->y);
        goto end;
    }
    
    /* 
     * convert rows to columns, read off columns of [k0 k1, k2 k3, k4 k5, k6 k7]
     */
  
    mask = 0x80000000;
    for(i=0; i< 32; i++){
        big_locals->columns[i] =0;
        bit_num = 3;
        for(j =0; j < 8; j=j+2){
            big_locals->columns[i] |= ((k->e[j] & mask)>>(32-i-1))<<bit_num;
            bit_num--;
        }
        mask = mask >>1;
    }
    mask = 0x80000000;
    for(i=32; i< 64; i++){
        big_locals->columns[i] =0;
        bit_num = 3;
        for(j =1; j < 8; j=j+2){
            big_locals->columns[i] |= ((k->e[j] & mask)>>(32-(i-32)-1))<<bit_num;
            bit_num--;
        }
        mask = mask >>1;
    }
    
    poly_null(&(r->x));
    poly_null(&(r->y));
    for(i=0; i<64; i++){
        poly_elliptic_double(r, &big_locals->temppoint, curv);
        poly_elliptic_sum(&big_locals->temppoint, &(precomputed_bp[((int)big_locals->columns[i])]), r, curv);
    }

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif    

    return;
}

static void
do_precompute_two(point *p, point *precompute, curve *curv)
{
    
    int i; 
    field_2n temp;
    
    /* do precomputation of all 4 bit words * P */
    for(i =0; i < 4; i++){
        /* put i in the field suitably: ! */
        poly_null(&temp);
        temp.e[3] = (i & 0x2)>>1; /* 2nd bit from right */
        temp.e[7] = (i & 0x1); /* right most bit */
        poly_elliptic_mul_slow(&temp, p, &(precompute[i]), curv);
    }
    return;
}


/* window = 2. first time initialise the precomputed values,
 * then use them 
 * generic type, but slower than binary NAF. so use only if expected to
 * call again with same signer.
 */

#ifdef __KERNEL__
struct mul_bigvars {
    point precompute[4];
    element columns[128];
    point temppoint;
    field_2n number;
};
#endif

void 
poly_elliptic_mul(field_2n *k, point *p, point *r, curve *curv)
{
#ifdef __KERNEL__
    struct mul_bigvars *_bigvars = kmalloc(sizeof(struct mul_bigvars), GFP_KERNEL);
    point *precompute = _bigvars->precompute;
    element *columns = _bigvars->columns;
#define temppoint _bigvars->temppoint
#define number _bigvars->number
#else
    point precompute[4];
    element columns[128];
    point temppoint;
    field_2n number;
#endif
    int i, j, diff, wordpos;
    element mask;
    element notzero;
    int bit_num;
    /* check for multiplier zero, if so return point at inf */
    poly_copy (k, &number);
    
    notzero =0;
    for(i=0; i< MAX_LONG; i++){
        notzero |= number.e[i];
    }
    
    if(!notzero){
        poly_null(&r->x);
        poly_null(&r->y);
        goto end;
    }
    
    /* check if the signer is cached */
    diff = 0;
    for(i=0; i< MAX_LONG; i++){
        if(p->x.e[i] != precomputed_signer.x.e[i]){
            diff = 1;
            break;
        }
        if(p->y.e[i] != precomputed_signer.y.e[i]){
            diff = 1;
            break;
        }
    }
    if(diff ==1){
        do_precompute_two(p, precompute, curv);
        /* copy for future */
        copy_point(p, &precomputed_signer);
        for(i=0; i< 4; i++){
            copy_point(&(precompute[i]), &(precomputed_pk[i]));
        }
    }
    else{
        /* take the precomputed values */
        for(i=0; i< 4; i++){
            copy_point(&(precomputed_pk[i]), &(precompute[i]));
        }
    }
    
    
    
    /* 
     * convert rows to columns, read off columns of [k0 k1 k2 k3, k4 k5 k6 k7]
     */
    
    for(wordpos = 0; wordpos < 4; wordpos++){
        mask = 0x80000000;
        for(i = wordpos*32; i< (wordpos+1)*32 ; i++){
            columns[i] = 0;
            bit_num = 1;
            for(j = wordpos; j < 8; j=j+4){
                columns[i] |= ((k->e[j] & mask)>>(32-(i-(wordpos*32))-1))<<bit_num;
                bit_num--;
            }
            mask = mask >> 1;
        }
    }
    
    poly_null(&(r->x));
    poly_null(&(r->y));
    for(i=0; i<128; i++){
        poly_elliptic_double(r, &temppoint, curv);
        poly_elliptic_sum(&temppoint, &(precompute[((int)columns[i])]), r, curv);
    }

 end:
#ifdef __KERNEL__
    kfree(_bigvars);
#endif

    return;

#ifdef __KERNEL__
#undef temppoint
#undef number
#endif
}
