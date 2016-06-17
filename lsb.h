#pragma once
#include <intrin.h>

inline int LSB(unsigned int x)
{
	unsigned long bitpos;

	if (_BitScanForward(&bitpos, x))
		return(bitpos);
	else
		return(0);
}

