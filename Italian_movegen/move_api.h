#define CLEAR_BOARD(b) (b->black = b->white = b->king = 0)


#define LF_EVEN_MOVE_SHIFT 5
#define LF_EVEN 0x07070707	/* Even rows 0, 2, 4, 6 */

#define RF_EVEN_MOVE_SHIFT 4
#define RF_EVEN 0x0f0f0f0f

#define LF_ODD_MOVE_SHIFT 4
#define LF_ODD 0x00f0f0f0	/* Odd rows 1, 3, 5, 7 */

#define RF_ODD_MOVE_SHIFT 3
#define RF_ODD 0x00e0e0e0

#define LB_ODD_MOVE_SHIFT 4
#define LB_ODD 0xf0f0f0f0

#define RB_ODD_MOVE_SHIFT 5
#define RB_ODD 0xe0e0e0e0

#define LB_EVEN_MOVE_SHIFT 3
#define LB_EVEN 0x07070700

#define RB_EVEN_MOVE_SHIFT 4
#define RB_EVEN 0x0f0f0f00

#define LF_EVEN_JUMP_SHIFT 4
#define LFJ_EVEN 0x00070707

#define RF_EVEN_JUMP_SHIFT 3
#define RFJ_EVEN 0x000e0e0e

#define LF_ODD_JUMP_SHIFT 5
#define LFJ_ODD 0x00707070

#define RF_ODD_JUMP_SHIFT 4
#define RFJ_ODD 0x00e0e0e0

#define LB_ODD_JUMP_SHIFT 3
#define LBJ_ODD 0x70707000

#define RB_ODD_JUMP_SHIFT 4
#define RBJ_ODD 0xe0e0e000

#define LB_EVEN_JUMP_SHIFT 4
#define LBJ_EVEN 0x07070700

#define RB_EVEN_JUMP_SHIFT 5
#define RBJ_EVEN 0x0e0e0e00

/*
 *   Bit positions        Square numbers
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

#define LF3_EVEN_MOVE_SHIFT 5
#define LF3_EVEN 0x00000303	/* Even rows 0, 2, 4, 6 */

#define RF3_EVEN_MOVE_SHIFT 4
#define RF3_EVEN 0x00000e0e

#define LF3_ODD_MOVE_SHIFT 4
#define LF3_ODD 0x00007070	/* Odd rows 1, 3, 5, 7 */

#define RF3_ODD_MOVE_SHIFT 3
#define RF3_ODD 0x0000c0c0

#define LB3_ODD_MOVE_SHIFT 4
#define LB3_ODD 0x70700000

#define RB3_ODD_MOVE_SHIFT 5
#define RB3_ODD 0xc0c00000

#define LB3_EVEN_MOVE_SHIFT 3
#define LB3_EVEN 0x03030000

#define RB3_EVEN_MOVE_SHIFT 4
#define RB3_EVEN 0x0e0e0000


typedef struct {
	int count;					/* The number of moves in this list. */
	BOARD board[KRMAXMOVES];	/* How the board looks after each move. */
} MOVELIST;

typedef struct {
	int color;
	BOARD board;
} GAMEPOS;

typedef struct {
	char capture_count;
	char king_capturing;
	char king_capture_count;
	short int earliest_king;
	BOARD path[MAXPIECES_JUMPED];
} CAPTURE_INFO;


int build_jump_list(BOARD *board, MOVELIST *movelist, int color);
int build_nonjump_list(BOARD *board, MOVELIST *movelist, int color);
int canjump(BOARD *board, int color);
int testital();
int build_jump_list_path(BOARD *board, MOVELIST *movelist, int color, CAPTURE_INFO ital_attr[KRMAXMOVES]);
void moveto_squares_ital(BOARD *board, unsigned int *bm_moveto,
						 unsigned int *bk_moveto,
						 unsigned int *wm_moveto,
						 unsigned int *wk_moveto);

inline void get_fromto_bitmasks(BOARD *fromboard, BOARD *toboard, int color, BITBOARD *frombb, BITBOARD *tobb)
{
	*frombb = fromboard->pieces[color] & ~toboard->pieces[color];
	*tobb = ~fromboard->pieces[color] & toboard->pieces[color];
}


inline void get_fromto_bitnums(BITBOARD fromboard, BITBOARD toboard, int *frombitnum, int *tobitnum)
{
	BITBOARD from, to;

	from = fromboard & ~toboard;
	*frombitnum = LSB(from);
	to = ~fromboard & toboard;
	*tobitnum = LSB(to);
}


inline void get_fromto_square0(BOARD *fromboard, BOARD *toboard, int color, int *fromsq, int *tosq)
{
	*fromsq = bitmask_to_square0(fromboard->pieces[color] & ~toboard->pieces[color]);
	*tosq = bitmask_to_square0(~fromboard->pieces[color] & toboard->pieces[color]);
}


