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
#include "min_movegen.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "CheckerBoard.h"
#include "coordinates.h"
#include "utility.h"
#include "fen.h"

// the following array describes the ACF three-move-deck. three[n][0-1-2] are
// the three move numbers which have to be executed after generating the movelist
// in all three positions. n+1 is the number used in the ACF-deck.
// three[n][4] is the qualifier of the opening, 0,1,2,3 for normal, mailplay and probably lost, 3
// is also normal, but in CTD
int three[174][4] =
{
	{ 0, 6, 0, 0 },
	{ 0, 6, 1, 0 },
	{ 0, 6, 2, 2 },
	{ 0, 4, 0, 0 },
	{ 0, 5, 2, 3 },
	{ 0, 5, 3, 1 },
	{ 0, 5, 4, 3 },
	{ 0, 5, 5, 3 },
	{ 0, 5, 6, 1 },
	{ 0, 5, 7, 3 },
	{ 0, 2, 1, 0 },
	{ 0, 2, 2, 3 },
	{ 0, 2, 3, 2 },
	{ 0, 2, 4, 0 },
	{ 0, 2, 5, 3 },
	{ 0, 2, 6, 1 },
	{ 0, 2, 7, 0 },
	{ 0, 3, 1, 0 },
	{ 0, 3, 2, 0 },
	{ 0, 3, 3, 0 },
	{ 0, 3, 4, 1 },
	{ 0, 3, 6, 0 },
	{ 0, 0, 1, 0 },
	{ 0, 0, 2, 0 },
	{ 0, 0, 3, 3 },
	{ 0, 0, 4, 2 },
	{ 0, 0, 5, 0 },
	{ 0, 0, 6, 0 },
	{ 0, 1, 1, 0 },
	{ 0, 1, 2, 0 },
	{ 0, 1, 3, 0 },
	{ 0, 1, 4, 0 },
	{ 0, 1, 5, 0 },
	{ 0, 1, 6, 1 },
	{ 0, 1, 7, 2 },
	{ 1, 4, 1, 0 },
	{ 1, 4, 2, 3 },
	{ 1, 4, 4, 0 },
	{ 1, 4, 5, 0 },
	{ 1, 5, 1, 0 },
	{ 1, 5, 3, 0 },
	{ 1, 5, 4, 0 },
	{ 1, 5, 5, 0 },
	{ 1, 5, 6, 2 },
	{ 1, 5, 0, 2 },
	{ 1, 2, 0, 3 },
	{ 1, 3, 2, 0 },
	{ 1, 3, 4, 2 },
	{ 1, 3, 6, 0 },
	{ 1, 3, 1, 3 },
	{ 1, 0, 2, 0 },
	{ 1, 0, 4, 2 },
	{ 1, 0, 5, 0 },
	{ 1, 0, 6, 0 },
	{ 1, 1, 2, 0 },
	{ 1, 1, 4, 0 },
	{ 1, 1, 5, 0 },
	{ 1, 1, 6, 3 },
	{ 2, 4, 2, 3 },
	{ 2, 4, 3, 1 },
	{ 2, 4, 4, 1 },
	{ 2, 4, 5, 1 },
	{ 2, 4, 0, 3 },
	{ 2, 5, 1, 3 },
	{ 2, 5, 2, 3 },
	{ 2, 5, 4, 0 },
	{ 2, 5, 5, 0 },
	{ 2, 5, 6, 3 },
	{ 2, 2, 0, 0 },
	{ 2, 3, 2, 3 },
	{ 2, 3, 3, 3 },
	{ 2, 3, 5, 3 },
	{ 2, 3, 6, 0 },
	{ 2, 3, 1, 3 },
	{ 2, 0, 2, 3 },
	{ 2, 0, 3, 0 },
	{ 2, 0, 5, 2 },
	{ 2, 0, 6, 0 },
	{ 2, 0, 1, 3 },
	{ 2, 1, 2, 0 },
	{ 2, 1, 3, 0 },
	{ 2, 1, 5, 0 },
	{ 2, 1, 6, 0 },
	{ 2, 1, 1, 3 },
	{ 3, 6, 2, 3 },
	{ 3, 6, 3, 3 },
	{ 3, 6, 4, 3 },
	{ 3, 6, 5, 2 },
	{ 3, 6, 6, 3 },
	{ 3, 6, 0, 0 },
	{ 3, 4, 2, 3 },
	{ 3, 4, 3, 0 },
	{ 3, 4, 4, 3 },
	{ 3, 4, 5, 2 },
	{ 3, 4, 6, 0 },
	{ 3, 4, 1, 3 },
	{ 3, 5, 0, 0 },
	{ 3, 2, 1, 3 },
	{ 3, 2, 2, 0 },
	{ 3, 2, 4, 0 },
	{ 3, 2, 5, 0 },
	{ 3, 2, 6, 3 },
	{ 3, 3, 1, 0 },
	{ 3, 3, 2, 0 },
	{ 3, 3, 5, 1 },
	{ 3, 0, 0, 0 },
	{ 3, 1, 2, 0 },
	{ 3, 1, 3, 0 },
	{ 3, 1, 6, 2 },
	{ 3, 1, 1, 3 },
	{ 4, 6, 3, 0 },
	{ 4, 6, 4, 0 },
	{ 4, 6, 5, 0 },
	{ 4, 6, 6, 2 },
	{ 4, 6, 1, 3 },
	{ 4, 4, 3, 0 },
	{ 4, 4, 4, 0 },
	{ 4, 4, 0, 0 },
	{ 4, 4, 1, 0 },
	{ 4, 5, 0, 0 },
	{ 4, 2, 2, 0 },
	{ 4, 2, 4, 0 },
	{ 4, 2, 5, 0 },
	{ 4, 2, 6, 3 },
	{ 4, 2, 0, 3 },
	{ 4, 3, 2, 0 },
	{ 4, 3, 3, 0 },
	{ 4, 3, 4, 0 },
	{ 4, 0, 0, 0 },
	{ 4, 1, 3, 0 },
	{ 4, 1, 7, 3 },
	{ 4, 1, 0, 0 },
	{ 5, 6, 2, 3 },
	{ 5, 6, 3, 0 },
	{ 5, 6, 4, 3 },
	{ 5, 6, 5, 0 },
	{ 5, 6, 6, 2 },
	{ 5, 6, 1, 0 },
	{ 5, 4, 2, 0 },
	{ 5, 4, 3, 0 },
	{ 5, 4, 4, 1 },
	{ 5, 4, 1, 0 },
	{ 5, 5, 2, 3 },
	{ 5, 5, 3, 0 },
	{ 5, 5, 7, 2 },
	{ 5, 5, 0, 0 },
	{ 5, 5, 1, 0 },
	{ 5, 2, 2, 3 },
	{ 5, 2, 3, 0 },
	{ 5, 2, 5, 0 },
	{ 5, 2, 6, 0 },
	{ 5, 2, 1, 0 },
	{ 5, 3, 0, 1 },
	{ 5, 0, 1, 3 },
	{ 5, 0, 2, 0 },
	{ 5, 0, 6, 2 },
	{ 5, 0, 0, 0 },
	{ 5, 1, 1, 3 },
	{ 5, 1, 0, 0 },
	{ 6, 6, 3, 3 },
	{ 6, 6, 4, 3 },
	{ 6, 6, 0, 3 },
	{ 6, 6, 1, 0 },
	{ 6, 4, 0, 0 },
	{ 6, 4, 1, 0 },
	{ 6, 5, 0, 3 },
	{ 6, 5, 1, 3 },
	{ 6, 2, 4, 2 },
	{ 6, 2, 0, 3 },
	{ 6, 2, 1, 0 },
	{ 6, 3, 0, 2 },
	{ 6, 0, 0, 0 },
	{ 6, 1, 1, 0 },
	{ 6, 1, 5, 1 }
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

int FENtoclipboard(HWND hwnd, int board8[8][8], int color, int gametype)
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
		if (three[op][3] == OP_BOARD) {
			if (CBoptions->op_crossboard)
				ok = 1;
		}

		if (three[op][3] == OP_MAILPLAY) {
			if (CBoptions->op_mailplay)
				ok = 1;
		}

		if (three[op][3] == OP_BARRED) {
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

	for (i = 0, count = 0; i < ARRAY_SIZE(three); ++i) {
		if (three[i][3] == OP_BOARD || three[i][3] == OP_CTD) {
			if (options->op_crossboard)
				++count;
		}
		else if (three[i][3] == OP_MAILPLAY) {
			if (options->op_mailplay)
				++count;
		}
		else if (three[i][3] == OP_BARRED) {
			if (options->op_barred)
				++count;
		}
	}
	return(count);
}

int getthreeopening(int n, CBoptions *CBoptions)
{
	/* n is the number of the game in the engine match, 0 through numopenings - 1. 
		getthreeopening returns the number of the opening that should
		be played in game number n depending on which subset of
		the 3-move-deck is active */
	int i;
	int m;

	/* play every opening twice */
	n = n / 2;

	/* makes gamenumber: 1 2 3 4 5 6 7 8
				         n: 0 0 1 1 2 2 3 3 */
	m = -1;
	for (i = 0; i < 174; i++) {

		/* if the opening is part of the current set, increment m */

		// normal if((three[i][3]==op_crossboard) && op_crossboard) m++;
		if ((three[i][3] == OP_CTD || three[i][3] == OP_BOARD) && CBoptions->op_crossboard)
			m++;
		if ((three[i][3] == OP_BARRED) && CBoptions->op_barred)
			m++;
		if ((three[i][3] == OP_MAILPLAY) && CBoptions->op_mailplay)
			m++;

		/* after having found N eligible openings, our counter m is set to N-1, so
			that it runs from 0...173 at most */
		if (m == n)
			return i;
	}

	return -1;
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
