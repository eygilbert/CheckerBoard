#pragma once
#include <vector>
#include "cb_interface.h"
#include <time.h>
#include "bitboard.h"

#define COMMENTLENGTH 1024
#define MAXNAME 256

enum PDN_RESULT {
	UNKNOWN_RES, WHITE_WIN_RES, BLACK_WIN_RES, DRAW_RES
};

enum EM_START_POSITIONS {
	START_POS_3MOVE, START_POS_FROM_FILE
};

struct CBoptions {
	// holds all options of CB.
	// the point is that it is much easier to store one struct in the registry
	// than to save every value separately.
	unsigned int crc;					/* The crc is calculated on the whole struct using the sizeof(CBoptions) in the crc field. */
	char userdirectory[256];
	char matchdirectory[256];
	char EGTBdirectory[256];
	char primaryenginestring[64];
	char secondaryenginestring[64];
	char start_pos_filename[MAX_PATH];
	COLORREF colors[5];
	int userbook;
	int sound;
	int invert;
	int mirror;
	int numbers;
	int highlight;
	int priority;
	bool exact_time;
	bool use_incremental_time;
	bool early_game_adjudication;
	EM_START_POSITIONS em_start_positions;
	int level;
	double initial_time;		/* incremental time control settings. */
	double time_increment;
	int match_repeat_count;
	int op_crossboard;
	int op_mailplay;
	int op_barred;
	int window_x;
	int window_y;
	int window_width;
	int window_height;
	int addoffset;
	int language;
	int piecesetindex;
};

struct BALLOT_INFO {
	int color;
	int board8[8][8];
	std::string event;
};

struct RESULT_COUNTS {
	int black_wins;
	int white_wins;
	int draws;
	int unknowns;
};

/* A game move with associated move text, comments, and analysis text. */
struct gamebody_entry {
	CBmove move;						// move
	char PDN[64];						// PDN of move, e.g. 8-11 or 8x15
	char comment[COMMENTLENGTH];		// user comment
	char analysis[COMMENTLENGTH];		// engine analysis comment - separate from above so they can coexist
};

struct PDNgame {
	// structure for a PDN game
	// standard 7-tag-roster
	char event[MAXNAME];
	char site[MAXNAME];
	char date[MAXNAME];
	char round[MAXNAME];
	char black[MAXNAME];
	char white[MAXNAME];
	char resultstring[MAXNAME];
	char FEN[MAXNAME];
	PDN_RESULT result;
	int gametype;
	int movesindex;							/* Current index in moves[]. */
	std::vector<gamebody_entry> moves;		/* Moves and comments in the game body. */
};

/* This type is used to display game previews in the game select dialog. */
struct gamepreview {
	int game_index;		/* index of game into the current pdn database. */
	char black[64];
	char white[64];
	char result[10];
	char event[128];
	char date[32];
	char PDN[256];
};

struct userbookentry {
	pos position;
	CBmove move;
};

/* A mapping between different time constants. */
struct timemap {
	int level;		/* cboptions setting. */
	int token;		/* resource token of Windows control. */
	double time;	/* search time. */
};

struct time_ctrl_t {
	bool clock_paused;
	clock_t starttime;
	double black_time_remaining;
	double white_time_remaining;
	double cumulative_time_used[3];		/* Indexed by engine number, 1 or 2. */
	int searchcount;
};

struct emstats_t {
	int wins;
	int draws;
	int losses;
	int unknowns;
	int blackwins;
	int blacklosses;
	int games;
	int opening_index;	/* index into 3-move table, 1 less than the ACF ballot number. */

	inline bool is_odd(int n) {return((n & 1) == 1);}

	/*
	 * Given an engine match game number (1..N), return true if engine 1 plays black for that game.
	 * Engine 1 plays black in odd numbered games.
	 */
	inline bool engine1_plays_black(int gamenum) {return(is_odd(gamenum));}

	/*
	 * Given an engine match game number (1..N), and a side-to-move color (CB_BLACK or CB_WHITE), 
	 * return the engine number (1 or 2) that should select the next move.
	 * Engine 1 plays black in odd numbered games.
	 */
	inline int get_enginenum(int gamenum, int color) {
		if (is_odd(gamenum + color))
			return(1);
		else
			return(2);
	}
};

class Squarelist {
public:
	Squarelist(void) {clear();}
	void clear(void) {m_size = 0;}
	int first(void) {return(squares[0]);}
	int last(void) {return(squares[m_size - 1]);}
	int size(void) {return(m_size);}
	int read(int index) {return(squares[index]);}
	void append(int square) {
		if (m_size < sizeof(squares) / sizeof(squares[0])) {
			squares[m_size] = square;
			++m_size;
		}
	}
	int frequency(int square) {
		int count = 0;
		for (int i = 0; i < m_size; ++i)
			if (squares[i] == square)
				++count;
		return(count);
	}
	void reverse_color(void) {
		for (int i = 0; i < m_size; ++i)
			squares[i] = 33 - squares[i];
	}
	void reverse_rows(void) {
		for (int i = 0; i < m_size; ++i) {
			int sq0 = squares[i] - 1;
			int row = sq0 / 4;
			squares[i] = 1 + (4 * row) + (3 - (sq0 & 3));
		}
	}

private:
	char m_size;
	char squares[15];
};
