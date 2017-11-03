// cb_movegen.c: generates a list of legal moves
// 	getmovelist()
//	is the only function which cb_movegen.c exports. it takes b[8][8] as
//  board with the following representation, color to move, and returns a
//  list of CBmoves.

/* INCLUDES */
#include <memory.h>
#include <windows.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"

/* exported functions */
int getmovelist(int color, CBmove movelist[MAXMOVES], int b[8][8], int *isjump);

/* internal functions */
static int makemovelist(int color, CBmove movelist[MAXMOVES], int b[12][12], int *isjump);
static void board8toboard12(int board8[8][8], int board12[12][12]);
static void whitecapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d);
static void blackcapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d);
static void whitekingcapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d);
static void blackkingcapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d);

static int n;

static inline int cbcolor_to_getmovelistcolor(int cbcolor)
{
	if (cbcolor == CB_BLACK)
		return(1);
	else
		return(-1);
}

int getmovelist(int color, CBmove m[MAXMOVES], int b[8][8], int *isjump)
{
	int i, j;
	int n;
	int board12[12][12];

	board8toboard12(b, board12);
	assert(color == CB_BLACK || color == CB_WHITE);
	n = makemovelist(cbcolor_to_getmovelistcolor(color), m, board12, isjump);

	// now: do something to the coordinates, so that the moves are in a 8x8-format
	for (i = 0; i < n; i++) {
		m[i].from.x -= 2;
		m[i].to.x -= 2;
		m[i].from.y -= 2;
		m[i].to.y -= 2;
		for (j = 0; j < 11; j++) {
			m[i].path[j].x -= 2;
			m[i].path[j].y -= 2;
			m[i].del[j].x -= 2;
			m[i].del[j].y -= 2;
		}
	}

	// and set the pieces to CB-format
	for (i = 0; i < n; i++) {
		switch (m[i].oldpiece) {
		case -2:
			m[i].oldpiece = (CB_WHITE | CB_KING);
			break;

		case -1:
			m[i].oldpiece = (CB_WHITE | CB_MAN);
			break;

		case 1:
			m[i].oldpiece = (CB_BLACK | CB_MAN);
			break;

		case 2:
			m[i].oldpiece = (CB_BLACK | CB_KING);
			break;
		}

		switch (m[i].newpiece) {
		case -2:
			m[i].newpiece = (CB_WHITE | CB_KING);
			break;

		case -1:
			m[i].newpiece = (CB_WHITE | CB_MAN);
			break;

		case 1:
			m[i].newpiece = (CB_BLACK | CB_MAN);
			break;

		case 2:
			m[i].newpiece = (CB_BLACK | CB_KING);
			break;
		}

		for (j = 0; j < m[i].jumps; j++) {
			switch (m[i].delpiece[j]) {
			case -2:
				m[i].delpiece[j] = (CB_WHITE | CB_KING);
				break;

			case -1:
				m[i].delpiece[j] = (CB_WHITE | CB_MAN);
				break;

			case 1:
				m[i].delpiece[j] = (CB_BLACK | CB_MAN);
				break;

			case 2:
				m[i].delpiece[j] = (CB_BLACK | CB_KING);
				break;
			}
		}
	}

	return n;
}

void board8toboard12(int board8[8][8], int board12[12][12])
{
	// checkerboard uses a 8x8 board representation, the move generator a 12x12
	// this routine converts a 8x8 to a 12x12 board
	int i, j;
	for (i = 0; i <= 11; i++) {
		for (j = 0; j <= 11; j++)
			board12[i][j] = 10;
	}

	for (i = 2; i <= 9; i++) {
		for (j = 2; j <= 9; j++)
			board12[i][j] = 0;
	}

	for (i = 0; i <= 7; i++) {
		for (j = 0; j <= 7; j++) {
			if (board8[i][j] == (CB_BLACK | CB_MAN))
				board12[i + 2][j + 2] = 1;
			if (board8[i][j] == (CB_BLACK | CB_KING))
				board12[i + 2][j + 2] = 2;
			if (board8[i][j] == (CB_WHITE | CB_MAN))
				board12[i + 2][j + 2] = -1;
			if (board8[i][j] == (CB_WHITE | CB_KING))
				board12[i + 2][j + 2] = -2;
		}
	}
}

int makemovelist(int color, CBmove movelist[MAXMOVES], int board[12][12], int *isjump)
{
	// produces a movelist for color to move on board
	coor wk[12], bk[12], ws[12], bs[12];
	int nwk = 0, nbk = 0, nws = 0, nbs = 0;
	int i, j;
	int x, y;
	CBmove m;
	*isjump = 0;

	for (i = 0; i < MAXMOVES; i++)
		movelist[i].jumps = 0;
	n = 0;

	// initialize list of stones
	for (j = 2; j <= 8; j += 2) {
		for (i = 2; i <= 8; i += 2) {
			if (board[i][j] == 0)
				continue;
			if (board[i][j] == 1) {
				ws[nws].x = i;
				ws[nws].y = j;
				nws++;
				continue;
			}

			if (board[i][j] == 2) {
				wk[nwk].x = i;
				wk[nwk].y = j;
				nwk++;
				continue;
			}

			if (board[i][j] == -1) {
				bs[nbs].x = i;
				bs[nbs].y = j;
				nbs++;
				continue;
			}

			if (board[i][j] == -2) {
				bk[nbk].x = i;
				bk[nbk].y = j;
				nbk++;
				continue;
			}
		}
	}

	for (j = 3; j <= 9; j += 2) {
		for (i = 3; i <= 9; i += 2) {
			if (board[i][j] == 0)
				continue;
			if (board[i][j] == 1) {
				ws[nws].x = i;
				ws[nws].y = j;
				nws++;
				continue;
			}

			if (board[i][j] == 2) {
				wk[nwk].x = i;
				wk[nwk].y = j;
				nwk++;
				continue;
			}

			if (board[i][j] == -1) {
				bs[nbs].x = i;
				bs[nbs].y = j;
				nbs++;
				continue;
			}

			if (board[i][j] == -2) {
				bk[nbk].x = i;
				bk[nbk].y = j;
				nbk++;
				continue;
			}
		}
	}

	if (color == CB_WHITE) {

		/* search for captures with white kings*/
		if (nwk > 0) {
			for (i = 0; i < nwk; i++) {
				x = wk[i].x;
				y = wk[i].y;
				if
				(
					(board[x + 1][y + 1] < 0 && board[x + 2][y + 2] == 0) ||
					(board[x - 1][y + 1] < 0 && board[x - 2][y + 2] == 0) ||
					(board[x + 1][y - 1] < 0 && board[x + 2][y - 2] == 0) ||
					(board[x - 1][y - 1] < 0 && board[x - 2][y - 2] == 0)
				) {
					m.from.x = x;
					m.from.y = y;
					m.path[0].x = x;
					m.path[0].y = y;
					whitekingcapture(board, movelist, m, x, y, 0);
				}
			}
		}

		/* search for captures with white stones */
		if (nws > 0) {
			for (i = 0; i < nws; i++) {
				x = ws[i].x;
				y = ws[i].y;
				if
				(
					(board[x + 1][y + 1] < 0 && board[x + 2][y + 2] == 0) ||
					(board[x - 1][y + 1] < 0 && board[x - 2][y + 2] == 0)
				) {
					m.from.x = x;
					m.from.y = y;
					m.path[0].x = x;
					m.path[0].y = y;
					whitecapture(board, movelist, m, x, y, 0);
				}
			}
		}

		/* if there are capture moves return. */
		if (n > 0) {
			*isjump = 1;
			return(n);
		}

		/* search for moves with white kings */
		if (nwk > 0) {
			for (i = 0; i < nwk; i++) {
				x = wk[i].x;
				y = wk[i].y;
				if (board[x + 1][y + 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x + 1;
					movelist[n].to.y = y + 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x + 1;
					movelist[n].path[1].y = y + 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = 2;
					movelist[n].oldpiece = 2;
					n++;
				}

				if (board[x + 1][y - 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x + 1;
					movelist[n].to.y = y - 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x + 1;
					movelist[n].path[1].y = y - 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = 2;
					movelist[n].oldpiece = 2;
					n++;
				}

				if (board[x - 1][y + 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x - 1;
					movelist[n].to.y = y + 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x - 1;
					movelist[n].path[1].y = y + 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = 2;
					movelist[n].oldpiece = 2;
					n++;
				}

				if (board[x - 1][y - 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x - 1;
					movelist[n].to.y = y - 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x - 1;
					movelist[n].path[1].y = y - 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = 2;
					movelist[n].oldpiece = 2;
					n++;
				}
			}
		}

		/* search for moves with white stones */
		if (nws > 0) {
			for (i = nws - 1; i >= 0; i--) {
				x = ws[i].x;
				y = ws[i].y;
				if (board[x + 1][y + 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x + 1;
					movelist[n].to.y = y + 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x + 1;
					movelist[n].path[1].y = y + 1;
					movelist[n].del[0].x = -1;
					if (y == 8) {
						movelist[n].newpiece = 2;
					}
					else
						movelist[n].newpiece = 1;
					movelist[n].oldpiece = 1;
					n++;
				}

				if (board[x - 1][y + 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x - 1;
					movelist[n].to.y = y + 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x - 1;
					movelist[n].path[1].y = y + 1;
					movelist[n].del[0].x = -1;
					if (y == 8) {
						movelist[n].newpiece = 2;
					}
					else
						movelist[n].newpiece = 1;
					movelist[n].oldpiece = 1;
					n++;
				}
			}
		}

		if (n > 0)
			return(n);
	}
	else {

		/* search for captures with black kings*/
		n = 0;
		if (nbk > 0) {
			for (i = 0; i < nbk; i++) {
				x = bk[i].x;
				y = bk[i].y;
				if
				(
					((board[x + 1][y + 1] > 0) && (board[x + 1][y + 1] < 3) && (board[x + 2][y + 2] == 0)) ||
					((board[x - 1][y + 1] > 0) && (board[x - 1][y + 1] < 3) && (board[x - 2][y + 2] == 0)) ||
					((board[x + 1][y - 1] > 0) && (board[x + 1][y - 1] < 3) && (board[x + 2][y - 2] == 0)) ||
					((board[x - 1][y - 1] > 0) && (board[x - 1][y - 1] < 3) && (board[x - 2][y - 2] == 0))
				) {
					m.from.x = x;
					m.from.y = y;
					m.path[0].x = x;
					m.path[0].y = y;
					blackkingcapture(board, movelist, m, x, y, 0);
				}
			}
		}

		/* search for captures with black stones */
		if (nbs > 0) {
			for (i = nbs - 1; i >= 0; i--) {
				x = bs[i].x;
				y = bs[i].y;
				if
				(
					(board[x + 1][y - 1] > 0 && board[x + 2][y - 2] == 0) ||
					(board[x - 1][y - 1] > 0 && board[x - 2][y - 2] == 0)
				) {
					m.from.x = x;
					m.from.y = y;
					m.path[0].x = x;
					m.path[0].y = y;
					blackcapture(board, movelist, m, x, y, 0);
				}
			}
		}

		/* search for moves with black kings */
		if (n > 0) {
			*isjump = 1;
			return(n);
		}

		if (nbk > 0) {
			for (i = 0; i < nbk; i++) {
				x = bk[i].x;
				y = bk[i].y;
				if (board[x + 1][y + 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x + 1;
					movelist[n].to.y = y + 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x + 1;
					movelist[n].path[1].y = y + 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = -2;
					movelist[n].oldpiece = -2;
					n++;
				}

				if (board[x + 1][y - 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x + 1;
					movelist[n].to.y = y - 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x + 1;
					movelist[n].path[1].y = y - 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = -2;
					movelist[n].oldpiece = -2;
					n++;
				}

				if (board[x - 1][y + 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x - 1;
					movelist[n].to.y = y + 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x - 1;
					movelist[n].path[1].y = y + 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = -2;
					movelist[n].oldpiece = -2;
					n++;
				}

				if (board[x - 1][y - 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x - 1;
					movelist[n].to.y = y - 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x - 1;
					movelist[n].path[1].y = y - 1;
					movelist[n].del[0].x = -1;
					movelist[n].newpiece = -2;
					movelist[n].oldpiece = -2;
					n++;
				}
			}
		}

		/* search for moves with black stones */
		if (nbs > 0) {
			for (i = 0; i < nbs; i++) {
				x = bs[i].x;
				y = bs[i].y;
				if (board[x + 1][y - 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x + 1;
					movelist[n].to.y = y - 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x + 1;
					movelist[n].path[1].y = y - 1;
					movelist[n].del[0].x = -1;
					if (y == 3) {
						movelist[n].newpiece = -2;
					}
					else
						movelist[n].newpiece = -1;
					movelist[n].oldpiece = -1;
					n++;
				}

				if (board[x - 1][y - 1] == 0) {
					movelist[n].jumps = 0;
					movelist[n].from.x = x;
					movelist[n].from.y = y;
					movelist[n].to.x = x - 1;
					movelist[n].to.y = y - 1;
					movelist[n].path[0].x = x;
					movelist[n].path[0].y = y;
					movelist[n].path[1].x = x - 1;
					movelist[n].path[1].y = y - 1;
					movelist[n].del[0].x = -1;
					if (y == 3) {
						movelist[n].newpiece = -2;
					}
					else
						movelist[n].newpiece = -1;
					movelist[n].oldpiece = -1;
					n++;
				}
			}
		}
	}

	return(n);
}

void whitecapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d)
{
	int b[12][12];
	CBmove mm;
	int end = 1;

	mm = m;
	if (y < 8) {
		if (board[x + 1][y + 1] < 0 && board[x + 2][y + 2] == 0) {
			memcpy(b, board, 144 * sizeof(int));
			b[x][y] = 0;
			b[x + 1][y + 1] = 0;
			b[x + 2][y + 2] = 1;
			mm.to.x = x + 2;
			mm.to.y = y + 2;
			mm.path[d + 1].x = x + 2;
			mm.path[d + 1].y = y + 2;
			mm.del[d].x = x + 1;
			mm.del[d].y = y + 1;
			mm.delpiece[d] = board[x + 1][y + 1];
			mm.del[d + 1].x = -1;
			if (y == 7)
				mm.newpiece = 2;
			else
				mm.newpiece = 1;

			whitecapture(b, movelist, mm, x + 2, y + 2, d + 1);
			end = 0;
		}

		mm = m;
		if (board[x - 1][y + 1] < 0 && board[x - 2][y + 2] == 0) {
			memcpy(b, board, 144 * sizeof(int));
			b[x][y] = 0;
			b[x - 1][y + 1] = 0;
			b[x - 2][y + 2] = 1;
			mm.to.x = x - 2;
			mm.to.y = y + 2;
			mm.path[d + 1].x = x - 2;
			mm.path[d + 1].y = y + 2;
			mm.del[d].x = x - 1;
			mm.del[d].y = y + 1;
			mm.del[d + 1].x = -1;
			mm.delpiece[d] = board[x - 1][y + 1];
			if (y == 7)
				mm.newpiece = 2;
			else
				mm.newpiece = 1;

			whitecapture(b, movelist, mm, x - 2, y + 2, d + 1);
			end = 0;
		}
	}

	if (end) {
		m.jumps = d;
		movelist[n] = m;
		movelist[n].oldpiece = 1;
		n++;
	}
}

void whitekingcapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d)
{
	int b[12][12];
	CBmove mm;
	int end = 1;

	mm = m;
	if (board[x + 1][y + 1] < 0 && board[x + 2][y + 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x + 1][y + 1] = 0;
		b[x + 2][y + 2] = 2;
		mm.to.x = x + 2;
		mm.to.y = y + 2;
		mm.path[d + 1].x = x + 2;
		mm.path[d + 1].y = y + 2;
		mm.del[d].x = x + 1;
		mm.del[d].y = y + 1;
		mm.del[d + 1].x = -1;
		mm.delpiece[d] = board[x + 1][y + 1];
		mm.newpiece = 2;

		whitekingcapture(b, movelist, mm, x + 2, y + 2, d + 1);
		end = 0;
	}

	mm = m;
	if (board[x - 1][y + 1] < 0 && board[x - 2][y + 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x - 1][y + 1] = 0;
		b[x - 2][y + 2] = 2;
		mm.to.x = x - 2;
		mm.to.y = y + 2;
		mm.path[d + 1].x = x - 2;
		mm.path[d + 1].y = y + 2;
		mm.del[d].x = x - 1;
		mm.del[d].y = y + 1;
		mm.delpiece[d] = board[x - 1][y + 1];
		mm.del[d + 1].x = -1;
		mm.newpiece = 2;

		whitekingcapture(b, movelist, mm, x - 2, y + 2, d + 1);
		end = 0;
	}

	if (board[x + 1][y - 1] < 0 && board[x + 2][y - 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x + 1][y - 1] = 0;
		b[x + 2][y - 2] = 2;
		mm.to.x = x + 2;
		mm.to.y = y - 2;
		mm.path[d + 1].x = x + 2;
		mm.path[d + 1].y = y - 2;
		mm.del[d].x = x + 1;
		mm.del[d].y = y - 1;
		mm.delpiece[d] = board[x + 1][y - 1];
		mm.del[d + 1].x = -1;
		mm.newpiece = 2;

		whitekingcapture(b, movelist, mm, x + 2, y - 2, d + 1);
		end = 0;
	}

	mm = m;
	if (board[x - 1][y - 1] < 0 && board[x - 2][y - 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x - 1][y - 1] = 0;
		b[x - 2][y - 2] = 2;
		mm.to.x = x - 2;
		mm.to.y = y - 2;
		mm.path[d + 1].x = x - 2;
		mm.path[d + 1].y = y - 2;
		mm.del[d].x = x - 1;
		mm.del[d].y = y - 1;
		mm.delpiece[d] = board[x - 1][y - 1];
		mm.del[d + 1].x = -1;
		mm.newpiece = 2;

		whitekingcapture(b, movelist, mm, x - 2, y - 2, d + 1);
		end = 0;
	}

	if (end) {
		m.jumps = d;
		movelist[n] = m;
		movelist[n].oldpiece = 2;
		n++;
	}
}

void blackcapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d)
{
	int b[12][12];
	CBmove mm;
	int end = 1;

	mm = m;
	if (y > 3) {
		if (board[x + 1][y - 1] > 0 && board[x + 2][y - 2] == 0) {
			memcpy(b, board, 144 * sizeof(int));
			b[x][y] = 0;
			b[x + 1][y - 1] = 0;
			b[x + 2][y - 2] = -1;
			mm.to.x = x + 2;
			mm.to.y = y - 2;
			mm.path[d + 1].x = x + 2;
			mm.path[d + 1].y = y - 2;
			mm.del[d].x = x + 1;
			mm.del[d].y = y - 1;
			mm.delpiece[d] = board[x + 1][y - 1];
			mm.del[d + 1].x = -1;
			if (y == 4)
				mm.newpiece = -2;
			else
				mm.newpiece = -1;

			blackcapture(b, movelist, mm, x + 2, y - 2, d + 1);
			end = 0;
		}

		mm = m;
		if (board[x - 1][y - 1] > 0 && board[x - 2][y - 2] == 0) {
			memcpy(b, board, 144 * sizeof(int));
			b[x][y] = 0;
			b[x - 1][y - 1] = 0;
			b[x - 2][y - 2] = -1;
			mm.to.x = x - 2;
			mm.to.y = y - 2;
			mm.path[d + 1].x = x - 2;
			mm.path[d + 1].y = y - 2;
			mm.del[d].x = x - 1;
			mm.del[d].y = y - 1;
			mm.delpiece[d] = board[x - 1][y - 1];
			mm.del[d + 1].x = -1;
			if (y == 4)
				mm.newpiece = -2;
			else
				mm.newpiece = -1;

			blackcapture(b, movelist, mm, x - 2, y - 2, d + 1);
			end = 0;
		}
	}

	if (end) {
		m.jumps = d;
		movelist[n] = m;
		movelist[n].oldpiece = -1;
		n++;
	}
}

void blackkingcapture(int board[12][12], CBmove movelist[MAXMOVES], CBmove m, int x, int y, int d)
{
	int b[12][12];
	CBmove mm;
	int end = 1;

	mm = m;
	if (board[x + 1][y + 1] > 0 && board[x + 2][y + 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x + 1][y + 1] = 0;
		b[x + 2][y + 2] = 2;
		mm.to.x = x + 2;
		mm.to.y = y + 2;
		mm.path[d + 1].x = x + 2;
		mm.path[d + 1].y = y + 2;
		mm.del[d].x = x + 1;
		mm.del[d].y = y + 1;
		mm.delpiece[d] = board[x + 1][y + 1];
		mm.del[d + 1].x = -1;
		mm.newpiece = -2;

		blackkingcapture(b, movelist, mm, x + 2, y + 2, d + 1);
		end = 0;
	}

	mm = m;
	if (board[x - 1][y + 1] > 0 && board[x - 2][y + 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x - 1][y + 1] = 0;
		b[x - 2][y + 2] = 2;
		mm.to.x = x - 2;
		mm.to.y = y + 2;
		mm.path[d + 1].x = x - 2;
		mm.path[d + 1].y = y + 2;
		mm.del[d].x = x - 1;
		mm.del[d].y = y + 1;
		mm.delpiece[d] = board[x - 1][y + 1];
		mm.del[d + 1].x = -1;
		mm.newpiece = -2;

		blackkingcapture(b, movelist, mm, x - 2, y + 2, d + 1);
		end = 0;
	}

	if (board[x + 1][y - 1] > 0 && board[x + 2][y - 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x + 1][y - 1] = 0;
		b[x + 2][y - 2] = 2;
		mm.to.x = x + 2;
		mm.to.y = y - 2;
		mm.path[d + 1].x = x + 2;
		mm.path[d + 1].y = y - 2;
		mm.del[d].x = x + 1;
		mm.del[d].y = y - 1;
		mm.delpiece[d] = board[x + 1][y - 1];
		mm.del[d + 1].x = -1;
		mm.newpiece = -2;

		blackkingcapture(b, movelist, mm, x + 2, y - 2, d + 1);
		end = 0;
	}

	mm = m;
	if (board[x - 1][y - 1] > 0 && board[x - 2][y - 2] == 0) {
		memcpy(b, board, 144 * sizeof(int));
		b[x][y] = 0;
		b[x - 1][y - 1] = 0;
		b[x - 2][y - 2] = 2;
		mm.to.x = x - 2;
		mm.to.y = y - 2;
		mm.path[d + 1].x = x - 2;
		mm.path[d + 1].y = y - 2;
		mm.del[d].x = x - 1;
		mm.del[d].y = y - 1;
		mm.delpiece[d] = board[x - 1][y - 1];
		mm.del[d + 1].x = -1;
		mm.newpiece = -2;

		blackkingcapture(b, movelist, mm, x - 2, y - 2, d + 1);
		end = 0;
	}

	if (end) {
		m.jumps = d;
		movelist[n] = m;
		movelist[n].oldpiece = -2;
		n++;
	}
}
