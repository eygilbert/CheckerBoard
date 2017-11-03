#include <windows.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "CBconsts.h"

// bitboard.c contains routines to convert from 8x8-array-representation to bitboard and back
// this is used because CB has an internal 8x8 board representation, but for PDN database
// searches, it uses a bitboard representation + move generator to speed things up. of course,

// the 8x8 board representation could/should be dropped at some point but that means many changes!
void bitboardtoboard8(pos *p, int b[8][8])
{
	// converts from a bitboard structure to an 8x8 board
	int i, board[32];

	for (i = 0; i < 32; i++) {
		board[i] = 0;
		if (p->bm & 1 << i)
			board[i] = CB_BLACK | CB_MAN;
		if (p->bk & 1 << i)
			board[i] = CB_BLACK | CB_KING;
		if (p->wm & 1 << i)
			board[i] = CB_WHITE | CB_MAN;
		if (p->wk & 1 << i)
			board[i] = CB_WHITE | CB_KING;
	}

	b[0][0] = board[0];
	b[2][0] = board[1];
	b[4][0] = board[2];
	b[6][0] = board[3];
	b[1][1] = board[4];
	b[3][1] = board[5];
	b[5][1] = board[6];
	b[7][1] = board[7];
	b[0][2] = board[8];
	b[2][2] = board[9];
	b[4][2] = board[10];
	b[6][2] = board[11];
	b[1][3] = board[12];
	b[3][3] = board[13];
	b[5][3] = board[14];
	b[7][3] = board[15];
	b[0][4] = board[16];
	b[2][4] = board[17];
	b[4][4] = board[18];
	b[6][4] = board[19];
	b[1][5] = board[20];
	b[3][5] = board[21];
	b[5][5] = board[22];
	b[7][5] = board[23];
	b[0][6] = board[24];
	b[2][6] = board[25];
	b[4][6] = board[26];
	b[6][6] = board[27];
	b[1][7] = board[28];
	b[3][7] = board[29];
	b[5][7] = board[30];
	b[7][7] = board[31];
}

void boardtocrbitboard(int b[8][8], pos *position)
{
	// get reversed position to *position
	// this is used for database search with reversed colors.
	// initialize bitboard
	int i, board[32];

	/*
		  CB_WHITE
   	   28  29  30  31
	 24  25  26  27
	   20  21  22  23
	 16  17  18  19
	   12  13  14  15
	  8   9  10  11
	    4   5   6   7
	  0   1   2   3
	      CB_BLACK
	*/
	board[0] = b[0][0];
	board[1] = b[2][0];
	board[2] = b[4][0];
	board[3] = b[6][0];
	board[4] = b[1][1];
	board[5] = b[3][1];
	board[6] = b[5][1];
	board[7] = b[7][1];
	board[8] = b[0][2];
	board[9] = b[2][2];
	board[10] = b[4][2];
	board[11] = b[6][2];
	board[12] = b[1][3];
	board[13] = b[3][3];
	board[14] = b[5][3];
	board[15] = b[7][3];
	board[16] = b[0][4];
	board[17] = b[2][4];
	board[18] = b[4][4];
	board[19] = b[6][4];
	board[20] = b[1][5];
	board[21] = b[3][5];
	board[22] = b[5][5];
	board[23] = b[7][5];
	board[24] = b[0][6];
	board[25] = b[2][6];
	board[26] = b[4][6];
	board[27] = b[6][6];
	board[28] = b[1][7];
	board[29] = b[3][7];
	board[30] = b[5][7];
	board[31] = b[7][7];

	position->bm = 0;
	position->bk = 0;
	position->wm = 0;
	position->wk = 0;

	for (i = 0; i < 32; i++) {
		switch (board[i]) {
		case CB_BLACK | CB_MAN:
			position->wm |= (1 << (31 - i));
			break;

		case CB_BLACK | CB_KING:
			position->wk |= (1 << (31 - i));
			break;

		case CB_WHITE | CB_MAN:
			position->bm |= (1 << (31 - i));
			break;

		case CB_WHITE | CB_KING:
			position->bk |= (1 << (31 - i));
			break;
		}
	}
}

void boardtobitboard(int b[8][8], pos *position)
{
	// converts an 8x8 board to a bitboard representation
	// initialize bitboard
	int i, board[32];

	/*
		  CB_WHITE
   	   28  29  30  31
	 24  25  26  27
	   20  21  22  23
	 16  17  18  19
	   12  13  14  15
	  8   9  10  11
	    4   5   6   7
	  0   1   2   3
	      CB_BLACK
	*/
	board[0] = b[0][0];
	board[1] = b[2][0];
	board[2] = b[4][0];
	board[3] = b[6][0];
	board[4] = b[1][1];
	board[5] = b[3][1];
	board[6] = b[5][1];
	board[7] = b[7][1];
	board[8] = b[0][2];
	board[9] = b[2][2];
	board[10] = b[4][2];
	board[11] = b[6][2];
	board[12] = b[1][3];
	board[13] = b[3][3];
	board[14] = b[5][3];
	board[15] = b[7][3];
	board[16] = b[0][4];
	board[17] = b[2][4];
	board[18] = b[4][4];
	board[19] = b[6][4];
	board[20] = b[1][5];
	board[21] = b[3][5];
	board[22] = b[5][5];
	board[23] = b[7][5];
	board[24] = b[0][6];
	board[25] = b[2][6];
	board[26] = b[4][6];
	board[27] = b[6][6];
	board[28] = b[1][7];
	board[29] = b[3][7];
	board[30] = b[5][7];
	board[31] = b[7][7];

	position->bm = 0;
	position->bk = 0;
	position->wm = 0;
	position->wk = 0;

	for (i = 0; i < 32; i++) {
		switch (board[i]) {
		case CB_BLACK | CB_MAN:
			position->bm = position->bm | (1 << i);
			break;

		case CB_BLACK | CB_KING:
			position->bk = position->bk | (1 << i);
			break;

		case CB_WHITE | CB_MAN:
			position->wm = position->wm | (1 << i);
			break;

		case CB_WHITE | CB_KING:
			position->wk = position->wk | (1 << i);
			break;
		}
	}
}
