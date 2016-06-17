/* The declarations in this file allow the Simplech and Dama move generators
 * to be included in other programs (such as Perft).
 */

#define MAXMOVES 28

struct move2 {
	short n;		/* number of entries in move array m[]. */
	int m[12];		/* bits 0 - 7: affected square (index into b[46]). */
					/* bits 8 - 15: piece on square before the move. */
					/* bits 16 - 23: piece on square after the move. */
					/* m[0] has the 'from' square info. */
					/* m[1] has the 'to' square info. */
					/* m[2] and up have the jumped squares info. */
};

int generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color);
int generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color);
void domove(int b[46], struct move2 &move);
void undomove(int b[46], struct move2 &move);
