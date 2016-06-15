#include "reverse_rows.h"


/*
 * Reverse bits in each row of a bitboard.
 */
unsigned int reverse_rows(unsigned int bb)
{
	int i;
	unsigned int result, nib;
	static char xlat[] = {
		0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15
	};

	result = 0;
	for (i = 0; i < 8; ++i) {
		nib = xlat[(bb >> (4 * i)) & 0xf];
		nib <<= (4 * i);
		result |= nib;
	}

	return(result);
}


