// utility.c
//
// part of checkerboard
//
// implements various utility functions

#include <windows.h>
#include <stdio.h>
#include <time.h>
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

int three[174][4]=
	{{0,6,0,0},{0,6,1,0},{0,6,2,2},{0,4,0,0},{0,5,2,3},{0,5,3,1},{0,5,4,3},{0,5,5,3},{0,5,6,1},{0,5,7,3},
	{0,2,1,0},{0,2,2,3},{0,2,3,2},{0,2,4,0},{0,2,5,3},{0,2,6,1},{0,2,7,0},{0,3,1,0},{0,3,2,0},{0,3,3,0},
	{0,3,4,1},{0,3,6,0},{0,0,1,0},{0,0,2,0},{0,0,3,3},{0,0,4,2},{0,0,5,0},{0,0,6,0},{0,1,1,0},{0,1,2,0},
	{0,1,3,0},{0,1,4,0},{0,1,5,0},{0,1,6,1},{0,1,7,2},{1,4,1,0},{1,4,2,3},{1,4,4,0},{1,4,5,0},{1,5,1,0},
	{1,5,3,0},{1,5,4,0},{1,5,5,0},{1,5,6,2},{1,5,0,2},{1,2,0,3},{1,3,2,0},{1,3,4,2},{1,3,6,0},{1,3,1,3},

	{1,0,2,0},{1,0,4,2},{1,0,5,0},{1,0,6,0},{1,1,2,0},{1,1,4,0},{1,1,5,0},{1,1,6,3},{2,4,2,3},{2,4,3,1},
	{2,4,4,1},{2,4,5,1},{2,4,0,3},{2,5,1,3},{2,5,2,3},{2,5,4,0},{2,5,5,0},{2,5,6,3},{2,2,0,0},{2,3,2,3},
	{2,3,3,3},{2,3,5,3},{2,3,6,0},{2,3,1,3},{2,0,2,3},{2,0,3,0},{2,0,5,2},{2,0,6,0},{2,0,1,3},{2,1,2,0},
	{2,1,3,0},{2,1,5,0},{2,1,6,0},{2,1,1,3},{3,6,2,3},{3,6,3,3},{3,6,4,3},{3,6,5,2},{3,6,6,3},{3,6,0,0},
	{3,4,2,3},{3,4,3,0},{3,4,4,3},{3,4,5,2},{3,4,6,0},{3,4,1,3},{3,5,0,0},{3,2,1,3},{3,2,2,0},{3,2,4,0},

	{3,2,5,0},{3,2,6,3},{3,3,1,0},{3,3,2,0},{3,3,5,1},{3,0,0,0},{3,1,2,0},{3,1,3,0},{3,1,6,2},{3,1,1,3},
	{4,6,3,0},{4,6,4,0},{4,6,5,0},{4,6,6,2},{4,6,1,3},{4,4,3,0},{4,4,4,0},{4,4,0,0},{4,4,1,0},{4,5,0,0},
	{4,2,2,0},{4,2,4,0},{4,2,5,0},{4,2,6,3},{4,2,0,3},{4,3,2,0},{4,3,3,0},{4,3,4,0},{4,0,0,0},{4,1,3,0},
	{4,1,7,3},{4,1,0,0},{5,6,2,3},{5,6,3,0},{5,6,4,3},{5,6,5,0},{5,6,6,2},{5,6,1,0},{5,4,2,0},{5,4,3,0},
	{5,4,4,1},{5,4,1,0},{5,5,2,3},{5,5,3,0},{5,5,7,2},{5,5,0,0},{5,5,1,0},{5,2,2,3},{5,2,3,0},{5,2,5,0},

	{5,2,6,0},{5,2,1,0},{5,3,0,1},{5,0,1,3},{5,0,2,0},{5,0,6,2},{5,0,0,0},{5,1,1,3},{5,1,0,0},{6,6,3,3},
	{6,6,4,3},{6,6,0,3},{6,6,1,0},{6,4,0,0},{6,4,1,0},{6,5,0,3},{6,5,1,3},{6,2,4,2},{6,2,0,3},{6,2,1,0},
	{6,3,0,2},{6,0,0,0},{6,1,1,0},{6,1,5,1}};


extern char g_app_instance_suffix[10];


int initcolorstruct(HWND hwnd, CHOOSECOLOR *ccs, int index)
	{
	COLORREF dCustomColors[16];
	extern struct CBoptions gCBoptions;
	ccs->lStructSize = (DWORD) sizeof(CHOOSECOLOR);
	ccs->hwndOwner = (HWND) hwnd;
	ccs->hInstance = (HWND) NULL;
	ccs->lpCustColors = dCustomColors;
	ccs->Flags = CC_RGBINIT|CC_FULLOPEN;
	ccs->lCustData = 0L;
	ccs->lpfnHook = NULL;
	ccs->lpTemplateName = (LPSTR) NULL;
	ccs->rgbResult = gCBoptions.colors[index];
	return 1;
	}


int FENtoclipboard(HWND hwnd, int board8[8][8], int color, int gametype)
	{
	char *FENstring;

	FENstring = (char *) malloc(GAMEBUFSIZE);
	board8toFEN(board8, FENstring, color, gametype);
	MessageBox(hwnd, FENstring,"printout is",MB_OK);
	texttoclipboard(FENstring);
	free(FENstring);
	return 1;
	}

int PDNtoclipboard(HWND hwnd, struct PDNgame *game)
	{
	char *gamestring;

	// allocate memory for game, print game to memory, call texttoclipboard to 
	// place it on clipboard.
	gamestring = (char *) malloc(GAMEBUFSIZE);
	PDNgametoPDNstring(game,gamestring, "\r\n");
	MessageBox(hwnd,gamestring,"printout is",MB_OK);
	texttoclipboard(gamestring);
	free(gamestring);
	return 1;
	}


int logtofile(char *filename, char *str, char *mode)
	{
	// appends the text <str> to the file <filename>
	FILE *fp;
	int closed = 0;

	fp = fopen(filename, mode);
	if(fp == NULL)
		{
		closed = _fcloseall();
		fp = fopen(filename, mode);
		if(fp == NULL)
			return 0;
		}

	fprintf(fp, "\n%s", str);
	fclose(fp);

	return 1;
	}


int texttoclipboard(char *text)
	{
	// generic text-to-clipboard: pass a text, and it gets put
	// on the clipboard
	HGLOBAL hOut;
	char *gamestring;

	// allocate memory for the game string
	hOut = GlobalAlloc(GHND|GMEM_DDESHARE, (DWORD) GAMEBUFSIZE);	 
	// and lock it
	gamestring = (char *) GlobalLock(hOut);	

	sprintf(gamestring,"%s",text);

	GlobalUnlock(hOut);
	if(OpenClipboard(NULL))
		{
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
	int loadok = 0;
	char *gamestring = NULL;
	char *p;
	HGLOBAL hIn;
	int i;

	gamestring = (char *) malloc(GAMEBUFSIZE);
	if(gamestring == NULL)
		{
		sprintf(str,"clipboard open failed");
		return 0;
		}

	if(OpenClipboard(hwnd))
		{
		hIn = GetClipboardData(CF_TEXT);
		if(hIn != NULL)
			{
			p =  (char *) GlobalLock(hIn);
			if(p == NULL)
				{
				sprintf(str,"globalloc failed");
				GlobalUnlock(hIn);
				free(gamestring);
				gamestring = NULL;
				CloseClipboard();
				return gamestring;
				}

			// copy data from clipboard 
			for(i=0;i<GAMEBUFSIZE-1;i++)
				{
				gamestring[i]=*p;
				if(*p==0)
					break;
				*p++;
				}
			*p=0;
			gamestring[GAMEBUFSIZE-1] = 0;
			loadok = 1;
			GlobalUnlock(hIn);
			}
		else
			{
			sprintf(str,"no valid clipboard data");
			}
		CloseClipboard();
		}
	return gamestring;
	}

int fileispresent(char *filename)
	{
	// returns 1 if a file with name "filename" is present, 0 otherwise

	FILE *fp;
	
	fp = fopen(filename,"r");
	if(fp != NULL)
		{
		fclose(fp);
		return 1;
		}
	else
		return 0;
	}

int checklevelmenu(HMENU hmenu,int item, struct CBoptions *CBoptions)
	{
	int increment;

	CheckMenuItem(hmenu,LEVELINSTANT,MF_UNCHECKED);
	CheckMenuItem(hmenu, LEVEL01S, MF_UNCHECKED);
	CheckMenuItem(hmenu, LEVEL02S, MF_UNCHECKED);
	CheckMenuItem(hmenu, LEVEL05S, MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL1S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL2S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL5S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL10S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL15S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL30S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL1M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL2M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL5M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL15M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL30M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVELINFINITE,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVELINCREMENT,MF_UNCHECKED);
	CheckMenuItem(hmenu,item,MF_CHECKED);
	
	if(CBoptions->level==14) 
		increment=1;
	else 
		increment=0;
	
	return increment;
	}

void setmenuchecks(struct CBoptions *CBoptions, HMENU hmenu)
	{
	// set menu checks 
	if(CBoptions->priority)
		CheckMenuItem(hmenu,OPTIONSPRIORITY,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSPRIORITY,MF_UNCHECKED);
	
	if(CBoptions->highlight)
     	CheckMenuItem(hmenu,OPTIONSHIGHLIGHT,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSHIGHLIGHT,MF_UNCHECKED);
   
	if(CBoptions->sound)
		CheckMenuItem(hmenu,OPTIONSSOUND,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSSOUND,MF_UNCHECKED);
	
	if(CBoptions->invert)
		CheckMenuItem(hmenu,DISPLAYINVERT,MF_CHECKED);
	else
		CheckMenuItem(hmenu,DISPLAYINVERT,MF_UNCHECKED);
	
	if(CBoptions->mirror)
		CheckMenuItem(hmenu,DISPLAYMIRROR,MF_CHECKED);
	else
		CheckMenuItem(hmenu,DISPLAYMIRROR,MF_UNCHECKED);
	
	if(CBoptions->exact)
		CheckMenuItem(hmenu,LEVELEXACT,MF_CHECKED);
	else
		CheckMenuItem(hmenu,LEVELEXACT,MF_UNCHECKED);
	
	if(CBoptions->numbers)
		CheckMenuItem(hmenu,DISPLAYNUMBERS,MF_CHECKED);
	else
		CheckMenuItem(hmenu,DISPLAYNUMBERS,MF_UNCHECKED);

	if(CBoptions->userbook)
		CheckMenuItem(hmenu,OPTIONSUSERBOOK,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSUSERBOOK,MF_UNCHECKED);
	}


void CBlog(char *str)
{
	FILE *cblogfile;
	static TCHAR path[MAX_PATH];

	// open a log file on startup
	if (path[0] == 0) {
		sprintf(path, "%s\\CBlog%s.txt", CBdocuments, g_app_instance_suffix);
		cblogfile = fopen(path, "w");
		fclose(cblogfile);
		cblogfile = fopen(path, "a");
	}

	if (str == NULL)
		return;

	cblogfile = fopen(path, "a");
	if (cblogfile == NULL)
		return;

	fprintf(cblogfile, "%s\n", str);
	fclose(cblogfile);
}


int getopening(struct CBoptions *CBoptions)
	/* chooses a 3-move opening at random. */
	{
	int op=0;
	int ok=0;
	
	srand( (unsigned)time( NULL ) );

	while(!ok)
		{
		op=random(174);
		if(three[op][3] == OP_BOARD)         
			{
			if(CBoptions->op_crossboard) 
				ok=1;
			}
		if(three[op][3] == OP_MAILPLAY)
			{
			if(CBoptions->op_mailplay) 
				ok=1;
			}
		if(three[op][3] == OP_BARRED)
			{
			if(CBoptions->op_barred) 
				ok=1;
			}
		}
	return op;
	}

int getthreeopening(int n, struct CBoptions *CBoptions)
	{
	/* n is the number of the game in the engine match. 
		getthreeopening returns the number of the opening that should
		be played in game number n depending on which subset of
		the 3-move-deck is active */
	int i;
	int m;

	/* play every opening twice */
	n = n/2;

	/* makes gamenumber: 1 2 3 4 5 6 7 8
				         n: 0 0 1 1 2 2 3 3 */
	m=-1;
	for(i=0;i<174;i++)
		{
		/* if the opening is part of the current set, increment m */
		// normal if((three[i][3]==op_crossboard) && op_crossboard) m++;
		if((three[i][3]==OP_CTD || three[i][3]==OP_BOARD) && CBoptions->op_crossboard) 
			m++;
		if((three[i][3]==OP_BARRED) && CBoptions->op_barred) 
			m++;
		if((three[i][3]==OP_MAILPLAY) && CBoptions->op_mailplay) 
			m++;
		
		/* after having found N eligible openings, our counter m is set to N-1, so
			that it runs from 0...173 at most */

		if(m==n) return i;
		}
	return -1;
	}
		
void toggle(int *x)
	{
	if(*x==0) 
		*x=1;
	else *x=0;
	}
   
int builtingametype(void)
	{
	return GT_ENGLISH;
	}


char *piecestr(int piece)
{
	if (piece == 0)
		return(".");
	if (piece & CB_BLACK)
		if (piece & CB_KING)
			return("bk");
		else
			return("bm");
	if (piece & CB_WHITE)
		if (piece & CB_KING)
			return("wk");
		else
			return("wm");
	return(".");
}


void log_fen(char *msg, int board[8][8], int color)
{
	char buf[150];

	sprintf(buf, "%s: ", msg);
	board8toFEN(board, buf + strlen(buf), color, gametype());
	CBlog(buf);
}


void log_bitboard(char *msg, int32 black, int32 white, int32 king)
{
	char buf[150];

	sprintf(buf, "%s: bwk(%x, %x, %x)", msg, black, white, king);
	CBlog(buf);
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

/*
void builddb(char *str)
	{
	// call the db generator if there is enough free disk space
	HINSTANCE hinst;
	int error;
	ULARGE_INTEGER FreeBytesAvailable,TotalNumberOfBytes,TotalNumberOfFreeBytes;
	__int64 freebytes;

	GetDiskFreeSpaceEx(NULL,&FreeBytesAvailable,&TotalNumberOfBytes,&TotalNumberOfFreeBytes);
	
	freebytes = (__int64)(FreeBytesAvailable.LowPart) + (((__int64)FreeBytesAvailable.HighPart)<<32);

	if(freebytes < (DWORD64)220000000)
		{
		(str,"not enough free disk space for this operation");
		return;
		}
	else
		sprintf(str,"There is enough free disk space (about %I64i MB) - building database...", freebytes/1024/1024);

	hinst = ShellExecute(NULL,"open","db\\dbgen.bat",NULL,NULL,SW_SHOW);
	error = PtrToLong(hinst);
	if (error <= 32)
		sprintf(str,"error: %i", error);
	
	return;	
	}
	*/