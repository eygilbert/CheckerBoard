#include <windows.h>
#include <stdio.h>
#include "board.h"
#include "move_api.h"
#include "cb_interface.h"
#include "..\min_movegen.h"
#include "min_movegen_ital.h"
#include "reverse_rows.h"

#define SAVE_CAPTURE_PATH 0
#include "MoveItalian.h"


inline int cb_to_kr_color(int cbcolor)
{
	if (cbcolor == CB_BLACK)
		return(BLACK);
	else
		return(WHITE);
}


int makecapturelist_italian(struct pos *p, struct move movelist[MAXMOVES], int cbcolor)
{
	int i;
	BOARD board, *kb;
	MOVELIST krmovelist;

	/* Convert a cb position to a kr board. */
	board.black = reverse_rows(p->bm | p->bk);
	board.white = reverse_rows(p->wm | p->wk);
	board.king = reverse_rows(p->bk | p->wk);

	/* Generate the kr italian movelist for captures. */
	krmovelist.count = build_jump_list(&board, &krmovelist, cb_to_kr_color(cbcolor));

	/* Convert the kr movelist to a cb movelist. */
	for (i = 0; i < krmovelist.count; ++i) {
		kb = krmovelist.board + i;
		movelist[i].bm = reverse_rows((board.black & ~board.king) ^ (kb->black & ~kb->king));
		movelist[i].bk = reverse_rows((board.black & board.king) ^ (kb->black & kb->king));
		movelist[i].wm = reverse_rows((board.white & ~board.king) ^ (kb->white & ~kb->king));
		movelist[i].wk = reverse_rows((board.white & board.king) ^ (kb->white & kb->king));
	}
	return(krmovelist.count);
}


int makemovelist_italian(struct pos *p, struct move movelist[MAXMOVES], int cbcolor)
{
	int i;
	BOARD board, *kb;
	MOVELIST krmovelist;

	/* Convert a cb position to a kr board. */
	board.black = reverse_rows(p->bm | p->bk);
	board.white = reverse_rows(p->wm | p->wk);
	board.king = reverse_rows(p->bk | p->wk);

	/* Generate the kr italian movelist for non-captures. */
	krmovelist.count = build_nonjump_list(&board, &krmovelist, cb_to_kr_color(cbcolor));

	/* Convert the kr movelist to a cb movelist. */
	for (i = 0; i < krmovelist.count; ++i) {
		kb = krmovelist.board + i;
		movelist[i].bm = reverse_rows((board.black & ~board.king) ^ (kb->black & ~kb->king));
		movelist[i].bk = reverse_rows((board.black & board.king) ^ (kb->black & kb->king));
		movelist[i].wm = reverse_rows((board.white & ~board.king) ^ (kb->white & ~kb->king));
		movelist[i].wk = reverse_rows((board.white & board.king) ^ (kb->white & kb->king));
	}
	return(krmovelist.count);
}
