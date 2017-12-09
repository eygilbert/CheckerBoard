// utility.c
//
// part of checkerboard
//
// implements various utility functions
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <cstdarg>
#include "standardheader.h"
#include "cb_interface.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "CheckerBoard.h"
#include "coordinates.h"
#include "utility.h"
#include "fen.h"


/* The ACF list of 3-move ballots.
 * The ACF opening number is one greater than the index into the table.
 */
Three_move three_move_table[174] = {
	{"9-13 21-17 5-9", OP_CROSSBOARD, nullptr},						/* ACF #1 */
	{"9-13 21-17 6-9", OP_CROSSBOARD, nullptr},						/* ACF #2 */
	{"9-13 21-17 10-14", OP_BARRED, nullptr},						/* ACF #3 */
	{"9-13 22-17 13x22", OP_CROSSBOARD, nullptr},					/* ACF #4 */
	{"9-13 22-18 6-9", OP_CROSSBOARD | OP_CTD, "Dreaded Edinburgh"},	/* ACF #5 */
	{"9-13 22-18 10-14", OP_MAILPLAY, "Inferno"},					/* ACF #6 */
	{"9-13 22-18 10-15", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #7 */
	{"9-13 22-18 11-15", OP_CROSSBOARD | OP_CTD, "Edinburgh Single"},	/* ACF #8 */
	{"9-13 22-18 11-16", OP_MAILPLAY, "Wilderness I"},				/* ACF #9 */
	{"9-13 22-18 12-16", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #10 */
	{"9-13 23-18 5-9", OP_CROSSBOARD, nullptr},						/* ACF #11 */
	{"9-13 23-18 6-9", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #12 */
	{"9-13 23-18 10-14", OP_BARRED, nullptr},						/* ACF #13 */
	{"9-13 23-18 10-15", OP_CROSSBOARD, nullptr},					/* ACF #14 */
	{"9-13 23-18 11-15", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #15 */
	{"9-13 23-18 11-16", OP_MAILPLAY, "Wilderness II"},				/* ACF #16 */
	{"9-13 23-18 12-16", OP_CROSSBOARD, nullptr},					/* ACF #17 */
	{"9-13 23-19 5-9", OP_CROSSBOARD, nullptr},						/* ACF #18 */
	{"9-13 23-19 6-9", OP_CROSSBOARD, nullptr},						/* ACF #19 */
	{"9-13 23-19 10-14", OP_CROSSBOARD, nullptr},					/* ACF #20 */
	{"9-13 23-19 10-15", OP_MAILPLAY, nullptr},						/* ACF #21 */
	{"9-13 23-19 11-16", OP_CROSSBOARD, nullptr},					/* ACF #22 */
	{"9-13 24-19 5-9", OP_CROSSBOARD, nullptr},						/* ACF #23 */
	{"9-13 24-19 6-9", OP_CROSSBOARD, nullptr},						/* ACF #24 */
	{"9-13 24-19 10-14", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #25 */
	{"9-13 24-19 10-15", OP_BARRED, nullptr},						/* ACF #26 */
	{"9-13 24-19 11-15", OP_CROSSBOARD, nullptr},					/* ACF #27 */
	{"9-13 24-19 11-16", OP_CROSSBOARD, nullptr},					/* ACF #28 */
	{"9-13 24-20 5-9", OP_CROSSBOARD, nullptr},						/* ACF #29 */
	{"9-13 24-20 6-9", OP_CROSSBOARD, nullptr},						/* ACF #30 */
	{"9-13 24-20 10-14", OP_CROSSBOARD, nullptr},					/* ACF #31 */
	{"9-13 24-20 10-15", OP_CROSSBOARD, nullptr},					/* ACF #32 */
	{"9-13 24-20 11-15", OP_CROSSBOARD, nullptr},					/* ACF #33 */
	{"9-13 24-20 11-16", OP_MAILPLAY, "Twilight Zone"},				/* ACF #34 */
	{"9-13 24-20 12-16", OP_BARRED, "Dundee Barred"},				/* ACF #35 */
	{"9-14 22-17 5-9", OP_CROSSBOARD, nullptr},						/* ACF #36 */
	{"9-14 22-17 6-9", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #37 */
	{"9-14 22-17 11-15", OP_CROSSBOARD, nullptr},					/* ACF #38 */
	{"9-14 22-17 11-16", OP_CROSSBOARD, nullptr},					/* ACF #39 */
	{"9-14 22-18 5-9", OP_CROSSBOARD, nullptr},						/* ACF #40 */
	{"9-14 22-18 10-15", OP_CROSSBOARD, nullptr},					/* ACF #41 */
	{"9-14 22-18 11-15", OP_CROSSBOARD, nullptr},					/* ACF #42 */
	{"9-14 22-18 11-16", OP_CROSSBOARD, nullptr},					/* ACF #43 */
	{"9-14 22-18 12-16", OP_BARRED, nullptr},						/* ACF #44 */
	{"9-14 22-18 14-17", OP_BARRED, nullptr},						/* ACF #45 */
	{"9-14 23-18 14x23", OP_CROSSBOARD | OP_CTD, "Double Cross"},	/* ACF #46 */
	{"9-14 23-19 5-9", OP_CROSSBOARD, nullptr},						/* ACF #47 */
	{"9-14 23-19 10-15", OP_BARRED, "Rattlesnake"},					/* ACF #48 */
	{"9-14 23-19 11-16", OP_CROSSBOARD, nullptr},					/* ACF #49 */
	{"9-14 23-19 14-18", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #50 */
	{"9-14 24-19 5-9", OP_CROSSBOARD, nullptr},						/* ACF #51 */
	{"9-14 24-19 10-15", OP_BARRED, nullptr},						/* ACF #52 */
	{"9-14 24-19 11-15", OP_CROSSBOARD, nullptr},					/* ACF #53 */
	{"9-14 24-19 11-16", OP_CROSSBOARD, nullptr},					/* ACF #54 */
	{"9-14 24-20 5-9", OP_CROSSBOARD, nullptr},						/* ACF #55 */
	{"9-14 24-20 10-15", OP_CROSSBOARD, nullptr},					/* ACF #56 */
	{"9-14 24-20 11-15", OP_CROSSBOARD, nullptr},					/* ACF #57 */
	{"9-14 24-20 11-16", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #58 */
	{"10-14 22-17 7-10", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #59 */
	{"10-14 22-17 9-13", OP_MAILPLAY | OP_CTD, "Black Hole"},		/* ACF #60 */
	{"10-14 22-17 11-15", OP_MAILPLAY, nullptr},					/* ACF #61 */
	{"10-14 22-17 11-16", OP_MAILPLAY, "Gemini I"},					/* ACF #62 */
	{"10-14 22-17 14-18", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #63 */
	{"10-14 22-18 6-10", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #64 */
	{"10-14 22-18 7-10", OP_CROSSBOARD | OP_CTD, "Fraser's Inferno"},	/* ACF #65 */
	{"10-14 22-18 11-15", OP_CROSSBOARD, nullptr},					/* ACF #66 */
	{"10-14 22-18 11-16", OP_CROSSBOARD, nullptr},					/* ACF #67 */
	{"10-14 22-18 12-16", OP_CROSSBOARD | OP_CTD, "White Doctor"},	/* ACF #68 */
	{"10-14 23-18 14x23", OP_CROSSBOARD, nullptr},					/* ACF #69 */
	{"10-14 23-19 6-10", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #70 */
	{"10-14 23-19 7-10", OP_CROSSBOARD | OP_CTD, "Diabolical Denny"},	/* ACF #71 */
	{"10-14 23-19 11-15", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #72 */
	{"10-14 23-19 11-16", OP_CROSSBOARD, nullptr},					/* ACF #73 */
	{"10-14 23-19 14-18", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #74 */
	{"10-14 24-19 6-10", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #75 */
	{"10-14 24-19 7-10", OP_CROSSBOARD, nullptr},					/* ACF #76 */
	{"10-14 24-19 11-15", OP_BARRED, nullptr},						/* ACF #77 */
	{"10-14 24-19 11-16", OP_CROSSBOARD, nullptr},					/* ACF #78 */
	{"10-14 24-19 14-18", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #79 */
	{"10-14 24-20 6-10", OP_CROSSBOARD, nullptr},					/* ACF #80 */
	{"10-14 24-20 7-10", OP_CROSSBOARD, nullptr},					/* ACF #81 */
	{"10-14 24-20 11-15", OP_CROSSBOARD, nullptr},					/* ACF #82 */
	{"10-14 24-20 11-16", OP_CROSSBOARD, nullptr},					/* ACF #83 */
	{"10-14 24-20 14-18", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #84 */
	{"10-15 21-17 6-10", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #85 */
	{"10-15 21-17 7-10", OP_CROSSBOARD | OP_CTD, "Octopus"},		/* ACF #86 */
	{"10-15 21-17 9-13", OP_CROSSBOARD | OP_CTD, "Tyne"},			/* ACF #87 */
	{"10-15 21-17 9-14", OP_BARRED, nullptr},						/* ACF #88 */
	{"10-15 21-17 11-16", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #89 */
	{"10-15 21-17 15-18", OP_CROSSBOARD, nullptr},					/* ACF #90 */
	{"10-15 22-17 6-10", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #91 */
	{"10-15 22-17 7-10", OP_CROSSBOARD, nullptr},					/* ACF #92 */
	{"10-15 22-17 9-13", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #93 */
	{"10-15 22-17 9-14", OP_BARRED, "Nutcracker"},					/* ACF #94 */
	{"10-15 22-17 11-16", OP_CROSSBOARD, nullptr},					/* ACF #95 */
	{"10-15 22-17 15-19", OP_CROSSBOARD | OP_CTD, "Skull-Cracker"},	/* ACF #96 */
	{"10-15 22-18 15x22", OP_CROSSBOARD, nullptr},					/* ACF #97 */
	{"10-15 23-18 6-10", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #98 */
	{"10-15 23-18 7-10", OP_CROSSBOARD, nullptr},					/* ACF #99 */
	{"10-15 23-18 9-14", OP_CROSSBOARD, nullptr},					/* ACF #100 */
	{"10-15 23-18 11-16", OP_CROSSBOARD, nullptr},					/* ACF #101 */
	{"10-15 23-18 12-16", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #102 */
	{"10-15 23-19 6-10", OP_CROSSBOARD, nullptr},					/* ACF #103 */
	{"10-15 23-19 7-10", OP_CROSSBOARD, nullptr},					/* ACF #104 */
	{"10-15 23-19 11-16", OP_MAILPLAY, "Gemini II"},				/* ACF #105 */
	{"10-15 24-19 15x24", OP_CROSSBOARD, nullptr},					/* ACF #106 */
	{"10-15 24-20 6-10", OP_CROSSBOARD, nullptr},					/* ACF #107 */
	{"10-15 24-20 7-10", OP_CROSSBOARD, nullptr},					/* ACF #108 */
	{"10-15 24-20 11-16", OP_BARRED, nullptr},						/* ACF #109 */
	{"10-15 24-20 15-19", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #110 */
	{"11-15 21-17 8-11", OP_CROSSBOARD, nullptr},					/* ACF #111 */
	{"11-15 21-17 9-13", OP_CROSSBOARD, nullptr},					/* ACF #112 */
	{"11-15 21-17 9-14", OP_CROSSBOARD, nullptr},					/* ACF #113 */
	{"11-15 21-17 10-14", OP_BARRED, nullptr},						/* ACF #114 */
	{"11-15 21-17 15-19", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #115 */
	{"11-15 22-17 8-11", OP_CROSSBOARD, nullptr},					/* ACF #116 */
	{"11-15 22-17 9-13", OP_CROSSBOARD, nullptr},					/* ACF #117 */
	{"11-15 22-17 15-18", OP_CROSSBOARD, nullptr},					/* ACF #118 */
	{"11-15 22-17 15-19", OP_CROSSBOARD, nullptr},					/* ACF #119 */
	{"11-15 22-18 15x22", OP_CROSSBOARD, nullptr},					/* ACF #120 */
	{"11-15 23-18 8-11", OP_CROSSBOARD, nullptr},					/* ACF #121 */
	{"11-15 23-18 9-14", OP_CROSSBOARD, nullptr},					/* ACF #122 */
	{"11-15 23-18 10-14", OP_CROSSBOARD, nullptr},					/* ACF #123 */
	{"11-15 23-18 12-16", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #124 */
	{"11-15 23-18 15-19", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #125 */
	{"11-15 23-19 8-11", OP_CROSSBOARD, nullptr},					/* ACF #126 */
	{"11-15 23-19 9-13", OP_CROSSBOARD, nullptr},					/* ACF #127 */
	{"11-15 23-19 9-14", OP_CROSSBOARD, nullptr},					/* ACF #128 */
	{"11-15 24-19 15x24", OP_CROSSBOARD, nullptr},					/* ACF #129 */
	{"11-15 24-20 8-11", OP_CROSSBOARD, nullptr},					/* ACF #130 */
	{"11-15 24-20 12-16", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #131 */
	{"11-15 24-20 15-18", OP_CROSSBOARD, nullptr},					/* ACF #132 */
	{"11-16 21-17 7-11", OP_CROSSBOARD | OP_CTD, "Octopus"},		/* ACF #133 */
	{"11-16 21-17 8-11", OP_CROSSBOARD, nullptr},					/* ACF #134 */
	{"11-16 21-17 9-13", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #135 */
	{"11-16 21-17 9-14", OP_CROSSBOARD, nullptr},					/* ACF #136 */
	{"11-16 21-17 10-14", OP_BARRED, "Shark"},						/* ACF #137 */
	{"11-16 21-17 16-20", OP_CROSSBOARD, nullptr},					/* ACF #138 */
	{"11-16 22-17 7-11", OP_CROSSBOARD, nullptr},					/* ACF #139 */
	{"11-16 22-17 8-11", OP_CROSSBOARD, nullptr},					/* ACF #140 */
	{"11-16 22-17 9-13", OP_MAILPLAY, nullptr},						/* ACF #141 */
	{"11-16 22-17 16-20", OP_CROSSBOARD, nullptr},					/* ACF #142 */
	{"11-16 22-18 7-11", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #143 */
	{"11-16 22-18 8-11", OP_CROSSBOARD, nullptr},					/* ACF #144 */
	{"11-16 22-18 10-15", OP_BARRED, "Cheetah"},					/* ACF #145 */
	{"11-16 22-18 16-19", OP_CROSSBOARD, nullptr},					/* ACF #146 */
	{"11-16 22-18 16-20", OP_CROSSBOARD, nullptr},					/* ACF #147 */
	{"11-16 23-18 7-11", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #148 */
	{"11-16 23-18 8-11", OP_CROSSBOARD, nullptr},					/* ACF #149 */
	{"11-16 23-18 9-14", OP_CROSSBOARD, nullptr},					/* ACF #150 */
	{"11-16 23-18 10-14", OP_CROSSBOARD, nullptr},					/* ACF #151 */
	{"11-16 23-18 16-20", OP_CROSSBOARD, nullptr},					/* ACF #152 */
	{"11-16 23-19 16x23", OP_MAILPLAY, "Black Widow"},				/* ACF #153 */
	{"11-16 24-19 7-11", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #154 */
	{"11-16 24-19 8-11", OP_CROSSBOARD, nullptr},					/* ACF #155 */
	{"11-16 24-19 10-15", OP_BARRED, nullptr},						/* ACF #156 */
	{"11-16 24-19 16-20", OP_CROSSBOARD, nullptr},					/* ACF #157 */
	{"11-16 24-20 7-11", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #158 */
	{"11-16 24-20 16-19", OP_CROSSBOARD, nullptr},					/* ACF #159 */
	{"12-16 21-17 9-13", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #160 */
	{"12-16 21-17 9-14", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #161 */
	{"12-16 21-17 16-19", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #162 */
	{"12-16 21-17 16-20", OP_CROSSBOARD, nullptr},					/* ACF #163 */
	{"12-16 22-17 16-19", OP_CROSSBOARD, nullptr},					/* ACF #164 */
	{"12-16 22-17 16-20", OP_CROSSBOARD, nullptr},					/* ACF #165 */
	{"12-16 22-18 16-19", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #166 */
	{"12-16 22-18 16-20", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #167 */
	{"12-16 23-18 9-14", OP_BARRED, nullptr},						/* ACF #168 */
	{"12-16 23-18 16-19", OP_CROSSBOARD | OP_CTD, nullptr},			/* ACF #169 */
	{"12-16 23-18 16-20", OP_CROSSBOARD, nullptr},					/* ACF #170 */
	{"12-16 23-19 16x23", OP_BARRED, nullptr},						/* ACF #171 */
	{"12-16 24-19 16-20", OP_CROSSBOARD, nullptr},					/* ACF #172 */
	{"12-16 24-20 8-12", OP_CROSSBOARD, nullptr},					/* ACF #173 */
	{"12-16 24-20 10-15", OP_MAILPLAY, "Skunk"},					/* ACF #174 */
};

timemap time_table[] =
{
	{ 1, LEVELINSTANT, 0.01 },
	{ 2, LEVEL01S, 0.1 },
	{ 3, LEVEL02S, 0.2 },
	{ 4, LEVEL05S, 0.5 },
	{ 5, LEVEL1S, 1 },
	{ 6, LEVEL2S, 2 },
	{ 7, LEVEL5S, 5 },
	{ 8, LEVEL10S, 10 },
	{ 9, LEVEL15S, 15 },
	{ 10, LEVEL30S, 30 },
	{ 11, LEVEL1M, 60 },
	{ 12, LEVEL2M, 120 },
	{ 13, LEVEL5M, 300 },
	{ 14, LEVEL15M, 900 },
	{ 15, LEVEL30M, 1800 },
	{ 16, LEVELINFINITE, 8600000 },
};

static char cblogfile_path[MAX_PATH];

extern char g_app_instance_suffix[10];

int initcolorstruct(HWND hwnd, CHOOSECOLOR *ccs, int index)
{
	COLORREF dCustomColors[16];
	extern CBoptions cboptions;
	ccs->lStructSize = (DWORD) sizeof(CHOOSECOLOR);
	ccs->hwndOwner = (HWND) hwnd;
	ccs->hInstance = (HWND) NULL;
	ccs->lpCustColors = dCustomColors;
	ccs->Flags = CC_RGBINIT | CC_FULLOPEN;
	ccs->lCustData = 0L;
	ccs->lpfnHook = NULL;
	ccs->lpTemplateName = (LPSTR) NULL;
	ccs->rgbResult = cboptions.colors[index];
	return 1;
}

int FENtoclipboard(HWND hwnd, Board8x8 board8, int color, int gametype)
{
	char FENstring[1000];

	board8toFEN(board8, FENstring, color, gametype);
	MessageBox(hwnd, FENstring, "printout is", MB_OK);
	texttoclipboard(FENstring);
	return 1;
}

int PDNtoclipboard(HWND hwnd, PDNgame &game)
{
	std::string gamestring;

	PDNgametoPDNstring(game, gamestring, "\r\n");
	MessageBox(hwnd, gamestring.c_str(), "printout is", MB_OK);
	texttoclipboard(gamestring.c_str());
	return 1;
}

int logtofile(char *filename, char *str, char *mode)
{
	// appends the text <str> to the file <filename>
	FILE *fp;
	int closed = 0;

	fp = fopen(filename, mode);
	if (fp == NULL) {
		closed = _fcloseall();
		fp = fopen(filename, mode);
		if (fp == NULL)
			return 0;
	}

	fprintf(fp, "\n%s", str);
	fclose(fp);

	return 1;
}

int writefile(char *filename, char *mode, char *fmt, ...)
{
	FILE *fp;
	va_list args;
	va_start(args, fmt);

	fp = fopen(filename, mode);
	if (!fp)
		return(1);

	vfprintf(fp, fmt, args);
	fclose(fp);
	return(0);
}

int texttoclipboard(const char *text)
{
	// generic text-to-clipboard: pass a text, and it gets put
	// on the clipboard
	size_t size;
	HGLOBAL hOut;
	char *gamestring;

	// allocate memory for the game string
	size = 1 + strlen(text);
	hOut = GlobalAlloc(GHND | GMEM_DDESHARE, (DWORD)size);

	// and lock it
	gamestring = (char *)GlobalLock(hOut);

	sprintf(gamestring, "%s", text);

	GlobalUnlock(hOut);
	if (OpenClipboard(NULL)) {
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hOut);
		CloseClipboard();
	}

	return 1;
}

char *textfromclipboard(HWND hwnd, char *str)
{
	// read a text from the clipboard; return it if it is valid text, NULL otherwise
	// prints an error message in *str if an error occurs
	char *gamestring;
	char *p;
	HGLOBAL hIn;
	size_t length;

	gamestring = NULL;
	if (OpenClipboard(hwnd)) {
		hIn = GetClipboardData(CF_TEXT);
		if (hIn != NULL) {
			p = (char *)GlobalLock(hIn);
			if (p == NULL) {
				sprintf(str, "globalloc failed");
				GlobalUnlock(hIn);
				CloseClipboard();
				return NULL;
			}

			length = strlen(p);
			gamestring = (char *)malloc(length + 1);
			if (!gamestring) {
				sprintf(str, "malloc failed");
				GlobalUnlock(hIn);
				CloseClipboard();
				return NULL;
			}

			strcpy(gamestring, p);
			GlobalUnlock(hIn);
		}
		else {
			sprintf(str, "no valid clipboard data");
		}

		CloseClipboard();
	}

	return gamestring;
}

int fileispresent(char *filename)
{
	// returns 1 if a file with name "filename" is present, 0 otherwise
	FILE *fp;

	fp = fopen(filename, "r");
	if (fp != NULL) {
		fclose(fp);
		return 1;
	}
	else
		return 0;
}

double timelevel_to_time(int level)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(time_table); ++i)
		if (time_table[i].level == level)
			return(time_table[i].time);

	/* Shouldn't get here. */
	assert(0);
	return(1.0);
}

int timelevel_to_token(int level)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(time_table); ++i)
		if (time_table[i].level == level)
			return(time_table[i].token);

	/* Shouldn't get here. */
	assert(0);
	return(LEVEL1S);
}

int timetoken_to_level(int token)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(time_table); ++i)
		if (time_table[i].token == token)
			return(time_table[i].level);

	/* Shouldn't get here. */
	assert(0);
	return(5);
}

double timetoken_to_time(int token)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(time_table); ++i)
		if (time_table[i].token == token)
			return(time_table[i].time);

	/* Shouldn't get here. */
	assert(0);
	return(1.0);
}

void checklevelmenu(CBoptions *options, HMENU hmenu, int resource)
{
	int i;

	/* Uncheck everything first. */
	for (i = 0; i < ARRAY_SIZE(time_table); ++i)
		CheckMenuItem(hmenu, time_table[i].token, MF_UNCHECKED);

	/* Check the selected item. */
	if (options->use_incremental_time)
		CheckMenuItem(hmenu, LEVELINCREMENT, MF_CHECKED);
	else {
		CheckMenuItem(hmenu, LEVELINCREMENT, MF_UNCHECKED);
		CheckMenuItem(hmenu, resource, MF_CHECKED);
	}
}

void setmenuchecks(CBoptions *CBoptions, HMENU hmenu)
{
	// set menu checks
	if (CBoptions->priority)
		CheckMenuItem(hmenu, OPTIONSPRIORITY, MF_CHECKED);
	else
		CheckMenuItem(hmenu, OPTIONSPRIORITY, MF_UNCHECKED);

	if (CBoptions->highlight)
		CheckMenuItem(hmenu, OPTIONSHIGHLIGHT, MF_CHECKED);
	else
		CheckMenuItem(hmenu, OPTIONSHIGHLIGHT, MF_UNCHECKED);

	if (CBoptions->sound)
		CheckMenuItem(hmenu, OPTIONSSOUND, MF_CHECKED);
	else
		CheckMenuItem(hmenu, OPTIONSSOUND, MF_UNCHECKED);

	if (CBoptions->invert)
		CheckMenuItem(hmenu, DISPLAYINVERT, MF_CHECKED);
	else
		CheckMenuItem(hmenu, DISPLAYINVERT, MF_UNCHECKED);

	if (CBoptions->mirror)
		CheckMenuItem(hmenu, DISPLAYMIRROR, MF_CHECKED);
	else
		CheckMenuItem(hmenu, DISPLAYMIRROR, MF_UNCHECKED);

	if (CBoptions->exact_time)
		CheckMenuItem(hmenu, LEVELEXACT, MF_CHECKED);
	else
		CheckMenuItem(hmenu, LEVELEXACT, MF_UNCHECKED);

	if (CBoptions->numbers)
		CheckMenuItem(hmenu, DISPLAYNUMBERS, MF_CHECKED);
	else
		CheckMenuItem(hmenu, DISPLAYNUMBERS, MF_UNCHECKED);

	if (CBoptions->userbook)
		CheckMenuItem(hmenu, OPTIONSUSERBOOK, MF_CHECKED);
	else
		CheckMenuItem(hmenu, OPTIONSUSERBOOK, MF_UNCHECKED);
}


void cblog_init()
{
	FILE *cblogfile;

	// open a log file on startup
	if (cblogfile_path[0] == 0) {
		sprintf(cblogfile_path, "%s\\CBlog%s.txt", CBdocuments, g_app_instance_suffix);
		cblogfile = fopen(cblogfile_path, "w");
		fclose(cblogfile);
	}
}

void CBlog(char *str)
{
	FILE *cblogfile;

	cblog_init();
	if (str == NULL)
		return;

	cblogfile = fopen(cblogfile_path, "a");
	if (cblogfile == NULL)
		return;

	fprintf(cblogfile, "%s\n", str);
	fclose(cblogfile);
}

void cblog(const char *fmt, ...)
{
	FILE *fp;
	va_list args;
	va_start(args, fmt);

	cblog_init();
	if (fmt == NULL)
		return;

	fp = fopen(cblogfile_path, "a");
	if (fp == NULL)
		return;

	vfprintf(fp, fmt, args);
	fclose(fp);
}

int getopening(CBoptions *CBoptions)
/* chooses a 3-move opening at random. */
{
	int op = 0;
	int ok = 0;

	srand((unsigned)time(NULL));

	while (!ok) {
		op = random(174);
		if (three_move_table[op].attributes & OP_CROSSBOARD) {
			if (CBoptions->op_crossboard)
				ok = 1;
		}

		if (three_move_table[op].attributes & OP_MAILPLAY) {
			if (CBoptions->op_mailplay)
				ok = 1;
		}

		if (three_move_table[op].attributes & OP_BARRED) {
			if (CBoptions->op_barred)
				ok = 1;
		}
	}

	return op;
}

/*
 * Return the number of 3-move ballots that will be played based 
 * on the current settings for normal, mail, and lost ballots.
 */
int num_3move_ballots(CBoptions *options)
{
	int i, count;

	for (i = 0, count = 0; i < ARRAY_SIZE(three_move_table); ++i) {
		if (three_move_table[i].attributes & OP_CROSSBOARD) {
			if (options->op_crossboard)
				++count;
		}
		else if (three_move_table[i].attributes & OP_MAILPLAY) {
			if (options->op_mailplay)
				++count;
		}
		else if (three_move_table[i].attributes & OP_BARRED) {
			if (options->op_barred)
				++count;
		}
	}
	return(count);
}

/*
 * Given a 0-based ballot number between 0 .. num_3move_ballots() - 1, return an
 * index into three_move_table[].
 * num_3move_ballots() returns the number of ballots that will be played depending
 * on which subset of the 3-move deck is active.
 */
int get_3move_index(int ballotnum, CBoptions *CBoptions)
{
	int i, count;

	count = 0;
	for (i = 0; i < ARRAY_SIZE(three_move_table); i++) {
		if ((three_move_table[i].attributes & OP_CROSSBOARD) && CBoptions->op_crossboard)
			++count;
		else if ((three_move_table[i].attributes & OP_MAILPLAY) && CBoptions->op_mailplay)
			++count;
		else if ((three_move_table[i].attributes & OP_BARRED) && CBoptions->op_barred)
			++count;

		if ((count - 1) == ballotnum)
			return i;
	}

	/* Should never get here. */
	assert(false);
	return(0);
}

void toggle(int *x)
{
	if (*x == 0)
		*x = 1;
	else
		*x = 0;
}

int builtingametype(void)
{
	return GT_ENGLISH;
}

/*
 * Separate the filename from the path.
 * Return 0 on success, 1 if no separators are found.
 */
int extract_path(char *name, char *path)
{
	int i, len;

	len = (int)strlen(name);
	for (i = len - 1; i >= 0; --i) {
		if (name[i] == '\\' || name[i] == '/')
			break;
	}

	if (i <= 0)
		return(1);
	strncpy(path, name, i);
	path[i] = 0;
	return(0);
}

uint32_t filesize(char *filename)
{
	uint32_t size;
	HANDLE fp;

	fp = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	size = GetFileSize(fp, NULL);
	CloseHandle(fp);
	return(size);
}

/* malloc a buffer large enough to hold the contents of the text file,
 * and read the file into the buffer.
 * Return a pointer to the buffer, or nullptr if the file could not be opened or read.
 */
char *read_text_file(char *filename, READ_TEXT_FILE_ERROR_TYPE &etype)
{
	uint32_t size;
	size_t bytesread;
	char *buf;
	FILE *fp;

	size = filesize(filename);
	if (size == 0 || size == INVALID_FILE_SIZE) {
		etype = RTF_FILE_ERROR;
		return(nullptr);
	}

	fp = fopen(filename, "r");
	if (!fp) {
		etype = RTF_FILE_ERROR;
		return(nullptr);
	}
	
	buf = (char *)malloc(size + 1);		/* Leave room for null terminator. */
	if (!buf) {
		fclose(fp);
		etype = RTF_MALLOC_ERROR;
		return(nullptr);
	}

	bytesread = fread(buf, 1, size, fp);
	buf[bytesread] = 0;
	fclose(fp);
	etype = RTF_NO_ERROR;
	return(buf);
}
