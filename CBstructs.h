#pragma once
#include <vector>
#include "cb_interface.h"
#include "min_movegen.h"
#include <time.h>

#define COMMENTLENGTH 1024
#define MAXNAME 256

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

struct RESULT {
	int win;
	int loss;
	int draw;
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
	int result;								/* internal conversion to integers */
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
};

struct squarelist {
	char size;
	char squares[13];
};
