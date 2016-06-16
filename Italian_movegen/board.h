#pragma once
#include "lsb.h"

#define BLACK 0
#define WHITE 1
#define OTHER_COLOR(color) ((color) ^ 1)

#define KRMAXMOVES 30
#define MAXPIECES_JUMPED 12
#define NUMSQUARES 32
#define ROWSIZE 4
#define NUM_BITBOARD_BITS 32

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/*
 *   Bit positions       English squares
 *
 *       white                white
 *
 *    31  30  29  28      32  31  30  29
 *  27  26  25  24      28  27  26  25
 *    23  22  21  20      24  23  22  21
 *  19  18  17  16      20  19  18  17
 *    15  14  13  12      16  15  14  13
 *  11  10  09  08      12  11  10  09
 *    07  06  05  04      08  07  06  05
 *  03  02  01  00      04  03  02  01
 *
 *       black                black
 */

#define S1 1
#define S2 2
#define S3 4
#define S4 8
#define S5 0x10
#define S6 0x20
#define S7 0x40
#define S8 0x80
#define S9 0x100
#define S10 0x200
#define S11 0x400
#define S12 0x800
#define S13 0x1000
#define S14 0x2000
#define S15 0x4000
#define S16 0x8000
#define S17 0x10000
#define S18 0x20000
#define S19 0x40000
#define S20 0x80000
#define S21 0x100000 
#define S22 0x200000
#define S23 0x400000
#define S24 0x800000
#define S25 0x1000000
#define S26 0x2000000
#define S27 0x4000000
#define S28 0x8000000
#define S29 0x10000000
#define S30 0x20000000
#define S31 0x40000000
#define S32 0x80000000

#define COLUMN0 (S5 | S13 | S21 | S29)
#define COLUMN1 (S1 | S9 | S17 | S25)
#define COLUMN2 (S6 | S14 | S22 | S30)
#define COLUMN3 (S2 | S10 | S18 | S26)
#define COLUMN4 (S7 | S15 | S23 | S31)
#define COLUMN5 (S3 | S11 | S19 | S27)
#define COLUMN6 (S8 | S16 | S24 | S32)
#define COLUMN7 (S4 | S12 | S20 | S28)
#define ROW0 (S1 | S2 | S3 | S4)
#define ROW1 (S5 | S6 | S7 | S8)
#define ROW2 (S9 | S10 | S11 | S12)
#define ROW3 (S13 | S14 | S15 | S16)
#define ROW4 (S17 | S18 | S19 | S20)
#define ROW5 (S21 | S22 | S23 | S24)
#define ROW6 (S25 | S26 | S27 | S28)
#define ROW7 (S29 | S30 | S31 | S32)

#define BLACK_KING_RANK_MASK ROW7
#define WHITE_KING_RANK_MASK ROW0


typedef unsigned int BITBOARD;
typedef int SIGNED_BITBOARD;

typedef union {
	struct {
		BITBOARD pieces[2];		/* Indexed by BLACK or WHITE. */
		BITBOARD king;
	};
	struct {
		BITBOARD black;
		BITBOARD white;
		BITBOARD king;
	};
} BOARD;


inline int bitmask_to_square0(BITBOARD bitmask)
{
	int bitnum;

	bitnum = LSB(bitmask);
#ifdef ITALIAN_RULES
	return(4 * (bitnum / 4) + 3 - (bitnum & 3));
#else
	return(bitnum);
#endif
}


inline BITBOARD square0_to_bitmask(int sq0)
{
#ifdef ITALIAN_RULES
	return(1 << (4 * ((sq0) / 4) + 3 - ((sq0) & 3)));
#else
	return(1 << (sq0));
#endif
}

