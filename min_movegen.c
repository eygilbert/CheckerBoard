// min_movegen.c			
//
// part of checkerboard
//
// is used as a fast move generator for pdn parsing/searching operations
//
// ugly - CB has two move generators!

/* movegen.c is the movegenerator for the bitboard checkers engine */

/* the move structure consists of 4 uint32_t's to toggle the position with */

/* and one uint32_t info with additional information about the move */

/* info & 0x0000FFFF contains the ordering value of the move */
#include <windows.h>
#include <stdint.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "pdnfind.h"

// disable the signed/unsigned warning that gets thrown in this file dozens of times
#pragma warning(disable : 4146)
int makemovelist(pos *p, move movelist[MAXMOVES], int color)
{
	uint32_t n = 0, free;
	uint32_t m, tmp;

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
	free = ~(p->bm | p->bk | p->wm | p->wk);
	if (color == CB_BLACK) {
		if (p->bk) {

			/* moves left forwards */

			/* I: columns 1357 */
			m = ((p->bk & LF1) << 3) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);					/* least significant bit of m */
				tmp = tmp | (tmp >> 3);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);				/* clears least significant bit of m */
			}

			/* II: columns 2468 */
			m = ((p->bk & LF2) << 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 4);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}

			/* moves right forwards */

			/* I: columns 1357 */
			m = ((p->bk & RF1) << 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 4);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}

			/* II: columns 2468 */
			m = ((p->bk & RF2) << 5) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 5);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}

			/* moves left backwards */

			/* I: columns 1357 */
			m = ((p->bk & LB1) >> 5) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);					/* least significant bit of m */
				tmp = tmp | (tmp << 5);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);				/* clears least significant bit of m */
			}

			/* II: columns 2468 */
			m = ((p->bk & LB2) >> 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 4);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}

			/* moves right backwards */

			/* I: columns 1357 */
			m = ((p->bk & RB1) >> 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 4);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}

			/* II: columns 2468 */
			m = ((p->bk & RB2) >> 3) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 3);
				movelist[n].bm = 0;
				movelist[n].bk = tmp;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}
		}

		/* moves with black stones:*/
		if (p->bm) {

			/* moves left forwards */

			/* I: columns 1357: just moves */
			m = ((p->bm & LF1) << 3) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);					/* least significant bit of m */
				tmp = tmp | (tmp >> 3);			/* square where man came from */
				movelist[n].bm = tmp & NWBR;	/* NWBR: not white back rank */
				movelist[n].bk = tmp & WBR;		/*if stone moves to WBR (white back rank) it's a king*/
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);				/* clears least significant bit of m */
			}

			/* II: columns 2468 */
			m = ((p->bm & LF2) << 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 4);
				movelist[n].bm = tmp;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}

			/* moves right forwards */

			/* I: columns 1357 :just moves*/
			m = ((p->bm & RF1) << 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 4);
				movelist[n].bm = tmp & NWBR;
				movelist[n].bk = tmp & WBR;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}

			/* II: columns 2468 */
			m = ((p->bm & RF2) << 5) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 5);
				movelist[n].bm = tmp;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}
		}

		return n;
	}

	/* ****************************************************************/
	else {

		/* color is CB_WHITE */

	/******************************************************************/
		/* moves with white kings:*/
		if (p->wk) {

			/* moves left forwards */

			/* I: columns 1357 */
			m = ((p->wk & LF1) << 3) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);					/* least significant bit of m */
				tmp = tmp | (tmp >> 3);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);				/* clears least significant bit of m */
			}

			/* II: columns 2468 */
			m = ((p->wk & LF2) << 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 4);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);
			}

			/* moves right forwards */

			/* I: columns 1357 */
			m = ((p->wk & RF1) << 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 4);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);
			}

			/* II: columns 2468 */
			m = ((p->wk & RF2) << 5) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp >> 5);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);
			}

			/* moves left backwards */

			/* I: columns 1357 */
			m = ((p->wk & LB1) >> 5) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);					/* least significant bit of m */
				tmp = tmp | (tmp << 5);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);				/* clears least significant bit of m */
			}

			/* II: columns 2468 */
			m = ((p->wk & LB2) >> 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 4);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);
			}

			/* moves right backwards */

			/* I: columns 1357 */
			m = ((p->wk & RB1) >> 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 4);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);
			}

			/* II: columns 2468 */
			m = ((p->wk & RB2) >> 3) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 3);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = 0;
				movelist[n].wk = tmp;
				n++;
				m = m & (m - 1);
			}
		}

		/* moves with white stones:*/
		if (p->wm) {

			/* moves left backwards */

			/* II: columns 2468 ;just moves*/
			m = ((p->wm & LB2) >> 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 4);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = tmp & NBBR;
				movelist[n].wk = tmp & BBR;
				n++;
				m = m & (m - 1);
			}

			/* I: columns 1357 */
			m = ((p->wm & LB1) >> 5) & free;

			/* now m contains a bit for every free square where a white man can move*/
			while (m) {
				tmp = (m & -m);					/* least significant bit of m */
				tmp = tmp | (tmp << 5);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = tmp;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);				/* clears least significant bit of m */
			}

			/* moves right backwards */

			/* II: columns 2468 : just the moves*/
			m = ((p->wm & RB2) >> 3) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 3);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = tmp & NBBR;
				movelist[n].wk = tmp & BBR;
				n++;
				m = m & (m - 1);
			}

			/* I: columns 1357 */
			m = ((p->wm & RB1) >> 4) & free;
			while (m) {
				tmp = (m & -m);
				tmp = tmp | (tmp << 4);
				movelist[n].bm = 0;
				movelist[n].bk = 0;
				movelist[n].wm = tmp;
				movelist[n].wk = 0;
				n++;
				m = m & (m - 1);
			}
		}

		return n;
	}
}

/******************************************************************************/

/* capture list */

/******************************************************************************/

/* used to be captgen.c */

/* generates the capture moves */
int makecapturelist(pos *p, move movelist[MAXMOVES], int color)
{
	uint32_t free, free2, m, tmp, white, black, white2, black2;
	int n = 0;
	move partial;
	pos q;

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
	free = ~(p->bm | p->bk | p->wm | p->wk);
	if (color == CB_BLACK) {
		if (p->bm) {

			/* captures with black men! */
			white = p->wm | p->wk;

			/* jumps left forwards with men*/
			m = ((((p->bm & LFJ2) << 4) & white) << 3) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {

				/* find a move */
				tmp = (m & -m); /* least significant bit of m */
				partial.bm = (tmp | (tmp >> 7)) & NWBR; /* NWBR: not white back rank */
				partial.bk = (tmp | (tmp >> 7)) & WBR;	/*if stone moves to WBR (white back rank) it's a king*/
				partial.wm = (tmp >> 3) & p->wm;
				partial.wk = (tmp >> 3) & p->wk;

				/* toggle it */
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				/* recursion */

				/* only if black has another capture move! */
				white2 = p->wm | p->wk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LFJ2) << 4) & white2) << 3) & free2) | (((((tmp & RFJ2) << 5) & white2) << 4) & free2))
					blackmancapture2(&q, movelist, &n, &partial, tmp);
				else {

					// save move
					movelist[n] = partial;
					n++;
				}

				/* clears least significant bit of m, associated with that move. */
				m = m & (m - 1);
			}

			m = ((((p->bm & LFJ1) << 3) & white) << 4) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = (tmp | (tmp >> 7));
				partial.bk = 0;
				partial.wm = (tmp >> 4) & p->wm;
				partial.wk = (tmp >> 4) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				white2 = p->wm | p->wk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LFJ1) << 3) & white2) << 4) & free2) | (((((tmp & RFJ1) << 4) & white2) << 5) & free2))
					blackmancapture1(&q, movelist, &n, &partial, tmp);
				else {

					/* save move */
					movelist[n] = partial;
					n++;
				}

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps right forwards with men*/
			m = ((((p->bm & RFJ2) << 5) & white) << 4) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = (tmp | (tmp >> 9)) & NWBR;
				partial.bk = (tmp | (tmp >> 9)) & WBR;
				partial.wm = (tmp >> 4) & p->wm;
				partial.wk = (tmp >> 4) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				white2 = p->wm | p->wk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LFJ2) << 4) & white2) << 3) & free2) | (((((tmp & RFJ2) << 5) & white2) << 4) & free2))
					blackmancapture2(&q, movelist, &n, &partial, tmp);
				else {

					/* save move */
					movelist[n] = partial;
					n++;
				}

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->bm & RFJ1) << 4) & white) << 5) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = tmp | (tmp >> 9);
				partial.bk = 0;
				partial.wm = (tmp >> 5) & p->wm;
				partial.wk = (tmp >> 5) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				white2 = p->wm | p->wk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LFJ1) << 3) & white2) << 4) & free2) | (((((tmp & RFJ1) << 4) & white2) << 5) & free2))
					blackmancapture1(&q, movelist, &n, &partial, tmp);
				else {
					movelist[n] = partial;
					n++;
				}

				m = m & (m - 1);	/* clears least significant bit of m */
			}
		}

		if (p->bk) {
			white = p->wm | p->wk;

			/* jumps left forwards with black kings*/
			m = ((((p->bk & LFJ1) << 3) & white) << 4) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = (tmp | (tmp >> 7));
				partial.wm = (tmp >> 4) & p->wm;
				partial.wk = (tmp >> 4) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				blackkingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->bk & LFJ2) << 4) & white) << 3) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = (tmp | (tmp >> 7));
				partial.wm = (tmp >> 3) & p->wm;
				partial.wk = (tmp >> 3) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				blackkingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps right forwards with black kings*/
			m = ((((p->bk & RFJ1) << 4) & white) << 5) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = tmp | (tmp >> 9);
				partial.wm = (tmp >> 5) & p->wm;
				partial.wk = (tmp >> 5) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				blackkingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->bk & RFJ2) << 5) & white) << 4) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = (tmp | (tmp >> 9));
				partial.wm = (tmp >> 4) & p->wm;
				partial.wk = (tmp >> 4) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				blackkingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps left backwards with black kings*/
			m = ((((p->bk & LBJ1) >> 5) & white) >> 4) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = (tmp | (tmp << 9));
				partial.wm = (tmp << 4) & p->wm;
				partial.wk = (tmp << 4) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				blackkingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->bk & LBJ2) >> 4) & white) >> 5) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = (tmp | (tmp << 9));
				partial.wm = (tmp << 5) & p->wm;
				partial.wk = (tmp << 5) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				blackkingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps right backwards with black kings*/
			m = ((((p->bk & RBJ1) >> 4) & white) >> 3) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = tmp | (tmp << 7);
				partial.wm = (tmp << 3) & p->wm;
				partial.wk = (tmp << 3) & p->wk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				blackkingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->bk & RBJ2) >> 3) & white) >> 4) & free;

			/* now m contains a bit for every free square where a black king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.bm = 0;
				partial.bk = (tmp | (tmp << 7));
				partial.wm = (tmp << 4) & p->wm;
				partial.wk = (tmp << 4) & p->wk;

				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				blackkingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}
		}

		return n;
	}
	else {

		/*******************COLOR IS CB_WHITE *********************************/
		if (p->wm) {
			black = p->bm | p->bk;

			/* jumps left backwards with men*/
			m = ((((p->wm & LBJ1) >> 5) & black) >> 4) & free;

			/* now m contains a bit for every free square where a white man can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = (tmp | (tmp << 9)) & NBBR;
				partial.wk = (tmp | (tmp << 9)) & BBR;
				partial.bm = (tmp << 4) & p->bm;
				partial.bk = (tmp << 4) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;

				/* only if white has another capture move! */
				black2 = p->bm | p->bk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LBJ1) >> 5) & black2) >> 4) & free2) | (((((tmp & RBJ1) >> 4) & black2) >> 3) & free2))
					whitemancapture1(&q, movelist, &n, &partial, tmp);
				else {

					/* save move */
					movelist[n] = partial;
					n++;
				}

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->wm & LBJ2) >> 4) & black) >> 5) & free;

			/* now m contains a bit for every free square where a white man can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = (tmp | (tmp << 9));
				partial.wk = 0;
				partial.bm = (tmp << 5) & p->bm;
				partial.bk = (tmp << 5) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				black2 = p->bm | p->bk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LBJ2) >> 4) & black2) >> 5) & free2) | (((((tmp & RBJ2) >> 3) & black2) >> 4) & free2))
					whitemancapture2(&q, movelist, &n, &partial, tmp);
				else {

					/* save move */
					movelist[n] = partial;
					n++;
				}

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps right backwards with men*/
			m = ((((p->wm & RBJ1) >> 4) & black) >> 3) & free;

			/* now m contains a bit for every free square where a white man can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = (tmp | (tmp << 7)) & NBBR;
				partial.wk = (tmp | (tmp << 7)) & BBR;
				partial.bm = (tmp << 3) & p->bm;
				partial.bk = (tmp << 3) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				black2 = p->bm | p->bk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LBJ1) >> 5) & black2) >> 4) & free2) | (((((tmp & RBJ1) >> 4) & black2) >> 3) & free2))
					whitemancapture1(&q, movelist, &n, &partial, tmp);
				else {

					/* save move */
					movelist[n] = partial;
					n++;
				}

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->wm & RBJ2) >> 3) & black) >> 4) & free;

			/* now m contains a bit for every free square where a black man can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = (tmp | (tmp << 7));
				partial.wk = 0;
				partial.bm = (tmp << 4) & p->bm;
				partial.bk = (tmp << 4) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				black2 = p->bm | p->bk;
				free2 = ~(p->wm | p->wk | p->bm | p->bk);
				if ((((((tmp & LBJ2) >> 4) & black2) >> 5) & free2) | (((((tmp & RBJ2) >> 3) & black2) >> 4) & free2))
					whitemancapture2(&q, movelist, &n, &partial, tmp);
				else {

					/* save move */
					movelist[n] = partial;
					n++;
				}

				m = m & (m - 1);	/* clears least significant bit of m */
			}
		}

		if (p->wk) {
			black = p->bm | p->bk;

			/* jumps left forwards with white kings*/
			m = ((((p->wk & LFJ1) << 3) & black) << 4) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = (tmp | (tmp >> 7));
				partial.bm = (tmp >> 4) & p->bm;
				partial.bk = (tmp >> 4) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->wk & LFJ2) << 4) & black) << 3) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = (tmp | (tmp >> 7));
				partial.bm = (tmp >> 3) & p->bm;
				partial.bk = (tmp >> 3) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps right forwards with white kings*/
			m = ((((p->wk & RFJ1) << 4) & black) << 5) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = tmp | (tmp >> 9);
				partial.bm = (tmp >> 5) & p->bm;
				partial.bk = (tmp >> 5) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->wk & RFJ2) << 5) & black) << 4) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = (tmp | (tmp >> 9));
				partial.bm = (tmp >> 4) & p->bm;
				partial.bk = (tmp >> 4) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps left backwards with white kings*/
			m = ((((p->wk & LBJ1) >> 5) & black) >> 4) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = (tmp | (tmp << 9));
				partial.bm = (tmp << 4) & p->bm;
				partial.bk = (tmp << 4) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->wk & LBJ2) >> 4) & black) >> 5) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = (tmp | (tmp << 9));
				partial.bm = (tmp << 5) & p->bm;
				partial.bk = (tmp << 5) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			/* jumps right backwards with white kings*/
			m = ((((p->wk & RBJ1) >> 4) & black) >> 3) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = tmp | (tmp << 7);
				partial.bm = (tmp << 3) & p->bm;
				partial.bk = (tmp << 3) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture1(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}

			m = ((((p->wk & RBJ2) >> 3) & black) >> 4) & free;

			/* now m contains a bit for every free square where a white king can move*/
			while (m) {
				tmp = (m & -m);		/* least significant bit of m */
				partial.wm = 0;
				partial.wk = (tmp | (tmp << 7));
				partial.bm = (tmp << 4) & p->bm;
				partial.bk = (tmp << 4) & p->bk;
				q.bm = p->bm ^ partial.bm;
				q.bk = p->bk ^ partial.bk;
				q.wm = p->wm ^ partial.wm;
				q.wk = p->wk ^ partial.wk;
				whitekingcapture2(&q, movelist, &n, &partial, tmp);

				m = m & (m - 1);	/* clears least significant bit of m */
			}
		}

		return n;
	}
}

static void blackmancapture1
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LFJ1 and RFJ1 */
	uint32_t m, free, white;
	int found = 0;
	move next_partial, whole_partial;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	white = p->wm | p->wk;

	/* left forward jump */
	m = ((((square & LFJ1) << 3) & white) << 4) & free;
	if (m) {
		next_partial.bm = (m | (m >> 7));
		next_partial.bk = 0;
		next_partial.wm = (m >> 4) & p->wm;
		next_partial.wk = (m >> 4) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;
		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackmancapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right forward jump */
	m = ((((square & RFJ1) << 4) & white) << 5) & free;
	if (m) {
		next_partial.bm = (m | (m >> 9));
		next_partial.bk = 0;
		next_partial.wm = (m >> 5) & p->wm;
		next_partial.wk = (m >> 5) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackmancapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */

		/* set info tag in move */
		movelist[*n] = *partial;
		(*n)++;
	}
}

static void blackmancapture2
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LFJ2 and RFJ2 */

	/* additional complication: black stone might crown here */
	uint32_t m, free, white;
	move next_partial, whole_partial;
	int found = 0;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	white = p->wm | p->wk;

	/* left forward jump */
	m = ((((square & LFJ2) << 4) & white) << 3) & free;
	if (m) {
		next_partial.bm = (m | (m >> 7)) & NWBR;
		next_partial.bk = (m | (m >> 7)) & WBR;
		next_partial.wm = (m >> 3) & p->wm;
		next_partial.wk = (m >> 3) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackmancapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right forward jump */
	m = ((((square & RFJ2) << 5) & white) << 4) & free;
	if (m) {
		next_partial.bm = (m | (m >> 9)) & NWBR;
		next_partial.bk = (m | (m >> 9)) & WBR;
		next_partial.wm = (m >> 4) & p->wm;
		next_partial.wk = (m >> 4) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackmancapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */
		movelist[*n] = *partial;
		(*n)++;
	}
}

static void blackkingcapture1
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LFJ1 RFJ1 LBJ1 RBJ1*/
	uint32_t m, free, white;
	move next_partial, whole_partial;
	int found = 0;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	white = p->wm | p->wk;

	/* left forward jump */
	m = ((((square & LFJ1) << 3) & white) << 4) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m >> 7));
		next_partial.wm = (m >> 4) & p->wm;
		next_partial.wk = (m >> 4) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right forward jump */
	m = ((((square & RFJ1) << 4) & white) << 5) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m >> 9));
		next_partial.wm = (m >> 5) & p->wm;
		next_partial.wk = (m >> 5) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* left backward jump */
	m = ((((square & LBJ1) >> 5) & white) >> 4) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m << 9));
		next_partial.wm = (m << 4) & p->wm;
		next_partial.wk = (m << 4) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right backward jump */
	m = ((((square & RBJ1) >> 4) & white) >> 3) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m << 7));
		next_partial.wm = (m << 3) & p->wm;
		next_partial.wk = (m << 3) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */
		movelist[*n] = *partial;
		(*n)++;
	}
}

static void blackkingcapture2
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LFJ1 RFJ1 LBJ1 RBJ1*/
	uint32_t m, free, white;
	move next_partial, whole_partial;
	int found = 0;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	white = p->wm | p->wk;

	/* left forward jump */
	m = ((((square & LFJ2) << 4) & white) << 3) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m >> 7));
		next_partial.wm = (m >> 3) & p->wm;
		next_partial.wk = (m >> 3) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right forward jump */
	m = ((((square & RFJ2) << 5) & white) << 4) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m >> 9));
		next_partial.wm = (m >> 4) & p->wm;
		next_partial.wk = (m >> 4) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* left backward jump */
	m = ((((square & LBJ2) >> 4) & white) >> 5) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m << 9));
		next_partial.wm = (m << 5) & p->wm;
		next_partial.wk = (m << 5) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right backward jump */
	m = ((((square & RBJ2) >> 3) & white) >> 4) & free;
	if (m) {
		next_partial.bm = 0;
		next_partial.bk = (m | (m << 7));
		next_partial.wm = (m << 4) & p->wm;
		next_partial.wk = (m << 4) & p->wk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		blackkingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */
		movelist[*n] = *partial;
		(*n)++;
	}
}

static void whitemancapture1
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LBJ1 and RBJ1 */
	uint32_t m, free, black;
	move next_partial, whole_partial;
	int found = 0;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	black = p->bm | p->bk;

	/* left backward jump */
	m = ((((square & LBJ1) >> 5) & black) >> 4) & free;
	if (m) {
		next_partial.wm = (m | (m << 9)) & NBBR;
		next_partial.wk = (m | (m << 9)) & BBR;
		next_partial.bm = (m << 4) & p->bm;
		next_partial.bk = (m << 4) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitemancapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right backward jump */
	m = ((((square & RBJ1) >> 4) & black) >> 3) & free;
	if (m) {
		next_partial.wm = (m | (m << 7)) & NBBR;
		next_partial.wk = (m | (m << 7)) & BBR;
		next_partial.bm = (m << 3) & p->bm;
		next_partial.bk = (m << 3) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitemancapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */
		movelist[*n] = *partial;
		(*n)++;
	}
}

static void whitemancapture2
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LBJ1 and RBJ1 */
	uint32_t m, free, black;
	move next_partial, whole_partial;
	int found = 0;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	black = p->bm | p->bk;

	/* left backward jump */
	m = ((((square & LBJ2) >> 4) & black) >> 5) & free;
	if (m) {
		next_partial.wm = (m | (m << 9));
		next_partial.wk = 0;
		next_partial.bm = (m << 5) & p->bm;
		next_partial.bk = (m << 5) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitemancapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right backward jump */
	m = ((((square & RBJ2) >> 3) & black) >> 4) & free;
	if (m) {
		next_partial.wm = (m | (m << 7));
		next_partial.wk = 0;
		next_partial.bm = (m << 4) & p->bm;
		next_partial.bk = (m << 4) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitemancapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */
		movelist[*n] = *partial;
		(*n)++;
	}
}

static void whitekingcapture1
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LFJ1 RFJ1 LBJ1 RBJ1*/
	uint32_t m, free, black;
	move next_partial, whole_partial;
	int found = 0;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	black = p->bm | p->bk;

	/* left forward jump */
	m = ((((square & LFJ1) << 3) & black) << 4) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m >> 7));
		next_partial.bm = (m >> 4) & p->bm;
		next_partial.bk = (m >> 4) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right forward jump */
	m = ((((square & RFJ1) << 4) & black) << 5) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m >> 9));
		next_partial.bm = (m >> 5) & p->bm;
		next_partial.bk = (m >> 5) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* left backward jump */
	m = ((((square & LBJ1) >> 5) & black) >> 4) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m << 9));
		next_partial.bm = (m << 4) & p->bm;
		next_partial.bk = (m << 4) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right backward jump */
	m = ((((square & RBJ1) >> 4) & black) >> 3) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m << 7));
		next_partial.bm = (m << 3) & p->bm;
		next_partial.bk = (m << 3) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture1(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */
		movelist[*n] = *partial;
		(*n)++;
	}
}

static void whitekingcapture2
			(
				pos *p,
				move movelist[MAXMOVES],
				int *n,
				move *partial,
				uint32_t square
			)
{
	/* partial move has already been executed. seek LFJ1 RFJ1 LBJ1 RBJ1*/
	uint32_t m, free, black;
	move next_partial, whole_partial;
	int found = 0;
	pos q;

	free = ~(p->bm | p->bk | p->wm | p->wk);
	black = p->bm | p->bk;

	/* left forward jump */
	m = ((((square & LFJ2) << 4) & black) << 3) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m >> 7));
		next_partial.bm = (m >> 3) & p->bm;
		next_partial.bk = (m >> 3) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right forward jump */
	m = ((((square & RFJ2) << 5) & black) << 4) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m >> 9));
		next_partial.bm = (m >> 4) & p->bm;
		next_partial.bk = (m >> 4) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* left backward jump */
	m = ((((square & LBJ2) >> 4) & black) >> 5) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m << 9));
		next_partial.bm = (m << 5) & p->bm;
		next_partial.bk = (m << 5) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	/* right backward jump */
	m = ((((square & RBJ2) >> 3) & black) >> 4) & free;
	if (m) {
		next_partial.wm = 0;
		next_partial.wk = (m | (m << 7));
		next_partial.bm = (m << 4) & p->bm;
		next_partial.bk = (m << 4) & p->bk;
		q.bm = p->bm ^ next_partial.bm;
		q.bk = p->bk ^ next_partial.bk;
		q.wm = p->wm ^ next_partial.wm;
		q.wk = p->wk ^ next_partial.wk;

		whole_partial.bm = partial->bm ^ next_partial.bm;
		whole_partial.bk = partial->bk ^ next_partial.bk;
		whole_partial.wm = partial->wm ^ next_partial.wm;
		whole_partial.wk = partial->wk ^ next_partial.wk;
		whitekingcapture2(&q, movelist, n, &whole_partial, m);

		found = 1;
	}

	if (!found) {

		/* no continuing jumps - save the move in the movelist */
		movelist[*n] = *partial;
		(*n)++;
	}
}
