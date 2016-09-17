//  checkerboard.c
//
// 	version 1.0 was written by Martin Fierz	on															
// 	15th february 2000	
//  (c) 2000-2011 by Martin Fierz - all rights reserved.
//  contributions by Ed Gilbert are gratefully acknowledged
// 																			
// 	checkerboard is a graphical front-end for checkers engines. it checks	
// 	user moves for correctness. you can save and load games. you can change 
// 	the board and piece colors. you can change the window size. 			
// 																						
// 	interfacing to checkers engines: 													
// 	if you want your checkers engine to use checkerboard as a front-end, 	
// 	you must compile your engine as a dll, and provide the following 2
// 	functions:		
//  int WINAPI getmove(int board[8][8], int color, double maxtime, char str[1024], int *playnow, int info, int moreinfo, struct CBmove *move);
//  int WINAPI enginecommand(char command[256], char reply[1024]); 


// TODO: bug report: if you hit takeback while CB is animating a move, you get an undefined state

/******************************************************************************/

// CB uses multithreading, up to 4 threads:
//	-> main thread for the window
//	-> checkers engine runs in 'Thread'
//	-> animation runs in 'AniThread'
//	-> game analysis & engine match are driven by 'AutoThread'


#define STRICT


// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
#include <commctrl.h>
#include <stdio.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <time.h>
#include <io.h>
#include <intrin.h>

#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "PDNparser.h"
#include "dialogs.h"
#include "pdnfind.h"
#include "checkerboard.h"
#include "bmp.h"
#include "coordinates.h"
#include "bitboard.h"
#include "utility.h"
#include "fen.h"
#include "saveashtml.h"
#include "graphics.h"
#include "registry.h"
#include "app_instance.h"

#ifdef _WIN64
#pragma message("_WIN64 is defined.")
#endif

//---------------------------------------------------------------------
// globals - should be identified in code by g_varname but aren't all...

struct PDNgame GPDNgame;
struct CBmove GCBmove;

// all checkerboard options are collected in CBoptions; like this, they can be saved 
// as one struct in the registry, instead of using lots of commands.
struct CBoptions gCBoptions;

int g_app_instance;					/* 0, 1, 2, ... */
char g_app_instance_suffix[10];		/* "", "[1]", "[2]", ... */
DWORD g_ThreadId,g_AniThreadId,AutoThreadId;
HANDLE hThread, hAniThread,hAutoThread;
int enginethreadpriority = THREAD_PRIORITY_NORMAL; /* default priority setting*/
int usersetpriority = THREAD_PRIORITY_NORMAL; /* default priority setting*/
HICON hIcon;  /* CB icon for the window */
TBBUTTON tbButtons[NUMBUTTONS]; /* for the toolbar */

/* these globals are used to synchronize threads */
int abortcalculation = 0;		// used to tell the threadfunc that the calculation has been aborted
static BOOL enginebusy = FALSE;		/* true while engine thread is busy */
static BOOL animationbusy = FALSE;		/* true while animation thread is busy */
static BOOL enginestarting = FALSE;		// true when a play command is issued to the engine but engine has 
										// not started yet
BOOL gameover = FALSE; 			/* true when autoplay or engine match game is finished */
BOOL startmatch = TRUE;			/* startmatch is only true before engine match was started */
BOOL forceexact = FALSE;		/* forceexact is true -> CB-autothread checks time limit */
BOOL newposition = TRUE;		/* is true when position has changed. used in analysis mode to
								restart search and then reset */
BOOL startengine = FALSE; 		/* is true if engine is expected to start */
int result;
clock_t starttime,currenttime;

int toolbarheight = 30;			//30;
int statusbarheight = 20;		//20;
int menuheight = 16;			//16;
int titlebarheight = 12;		//12;
int offset = 40;				//40;
int upperoffset = 20;			//20;

char szWinName[] = "CheckerBoard";	/* name of window class */
int board8[8][8];					/* global which holds the board */
int setup = 0;						/* 1 if in setup mode */
int increment = 0;					// 1 if in an incremental time level
static int addcomment = 0;
int handicap = 0;
int testset_number = 0;
int color = CB_BLACK;					/* color is the side to move next */
int playnow = 0; 						/* playnow is passed to the checkers engines, it is set to nonzero if the user chooses 'play' */
int analyze = 0;						/* is set to 1 if the computer is analyzing the game - obsolete?*/
int reset = 0;
int gameindex = 0;					/* game to load/replace from/in a database*/
int gamenumber = 0;					/* number of games in the database */

/* dll globals */
/* CB uses function pointers to access the dll.
enginename, engineabout, engineoptions, enginehelp, getmove point to the currently used functions
...1 and ...2 are the pointers to dll1 and dll2 as read from engines.ini. */

/* library instances for primary, secondary and analysis engines */
HINSTANCE hinstLib=0,hinstLib1=0,hinstLib2=0;

/* function pointers for the engine functions */
CB_GETMOVE getmove=0,getmove1=0,getmove2=0;
CB_ENGINECOMMAND enginecommandtmp=0,enginecommand1=0, enginecommand2=0;

// multi-version support 
CB_ISLEGAL islegal=0,islegal1=0,islegal2=0;
CB_GETSTRING enginename1=0,enginename2=0;
CB_GETGAMETYPE CBgametype=0; // built in gametype and islegal functions 
CB_ISLEGAL CBislegal=0;

int enginename(char Lstr[256]);
BOOL fFreeResult;

// instance and window handles 
HINSTANCE g_hInst;		//instance of checkerboard
HWND hwnd;				// main window 
HWND hStatusWnd;		// status window 
static HWND tbwnd; 		// toolbar window 
HWND hHeadWnd;			// window of header control for game load 
HWND hDlgSelectgame;

struct gamedatabase data[MAXGAMES];
int gamelist[MAXGAMES]; 

// str holds the output string shown in the status bar - it is updated by WM_TIMER messages
char str[1024]="";
char playername[256];				// name of the player we are searching games of
char eventname[256];				// event we're searching for
char datename[256];					// date we're searching for
char commentname[256];				// comment we're searching for
int	searchwithposition = 0;			// search with position?
char string[256];
HMENU hmenu;								// menu handle 
double o,xmetric,ymetric;					//gives the size of the board8: one square is xmetric*ymetric 
int dummy,x1=-1,x2=-1,y1=-1,y2=-1;
struct CBmove m[28]; 						// movelist 
double maxtime, incrementtime=60.0, initialtime=1200.0;				//time limit - is set by setlevel() 
char reply[ENGINECOMMAND_REPLY_SIZE];		// holds reply of engine to command requests 
char CBdirectory[256]="";			// holds the directory from where CB is started:
char CBdocuments[MAX_PATH];			// CheckerBoard directory under My Documents
char database[256]="";				// current PDN database 
char userbookname[256];	// current userbook

// the game is stored in a doubly linked list. the following 3 pointers are always
//	valid: head, tail, current.  tail.next is always NULL, as is head.previous of course.

struct listentry *head,*tail,*newlistentry,*current,*tmplistentry;

struct CBmove move;
char filename[255]="";
char engine1[255]="";
char engine2[255]="";
int currentengine=1; 	 // 1=primary, 2=secondary 
int goneforward;
int op=0;
int togglemode = 0;		// 1-2-player toggle state
int togglebook = 0;		// engine book state (0/1/2/3)
int toggleengine = 1;   // primary/secondary engine (1/2)
struct pos currentposition;

// keep a small user book
struct userbookentry userbook[MAXUSERBOOK];
size_t userbooknum = 0;
size_t userbookcur = 0;
static CHOOSECOLOR ccs;

// reindex tells whether we have to reindex a database when searching.
// reindex is set to 1 if a game is saved, a game is replaced, or the
// database changed. and initialized to 1.
int reindex = 1;
RESULT r;
int re_search_ok = 0;
char piecesetname[MAXPIECESET][256];
int maxpieceset=0;
CRITICAL_SECTION ani_criticalsection, engine_criticalsection;
int	handletooltiprequest(LPTOOLTIPTEXT TTtext); 
void reset_current_game_pdn();
void forward_to_game_end(void);


// checkerboard goes finite-state: it can be in one of the modes above.
//	normal:	after the user enters a move, checkerboard starts to calculate
//				with engine.
//	autoplay: checkerboard plays engine-engine
//	enginematch: checkerboard plays engine-engine2
//	analyzegame: checkerboard moves through a game and comments on every move
//	entergame: checkerboard does nothing while the user enters a game
//	observegame: checkerboard calculates while the user enters a game
enum state {NORMAL, AUTOPLAY, ENGINEMATCH, ENGINEGAME, ANALYZEGAME, OBSERVEGAME, 
ENTERGAME, BOOKVIEW, BOOKADD, RUNTESTSET, ANALYZEPDN} CBstate=NORMAL;


int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst,LPSTR lpszArgs, int nWinMode)
	{
	// the main function which runs all the time, processing messages and sending them on
	// to windowfunc() which is the heart of CB
	MSG msg;
	WNDCLASS wcl;
	HACCEL hAccel;
	INITCOMMONCONTROLSEX iccex;
	RECT rect;

	// Define a window class. 
	wcl.lpszMenuName = NULL;			 
	wcl.hInstance = hThisInst;					// handle to this instance 
	wcl.lpszClassName = szWinName;				// window class name 
	wcl.lpfnWndProc = WindowFunc;				// window function 
	wcl.style = 0;								// default style 
	wcl.hIcon=LoadIcon(hThisInst,"icon1");		// load CB icon
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);	// cursor style 
	wcl.cbClsExtra =0;							// no extra 
	wcl.cbWndExtra =0;							// information needed 
	wcl.hbrBackground = (HBRUSH)GetSysColorBrush(GetSysColor(COLOR_MENU));

	// register the window class 
	if(!RegisterClass(&wcl)) 
		return 0;

	// create the window 
	hwnd = CreateWindow(
		szWinName,								// name of window class 
		"CheckerBoard:",						// title 
		WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ,	// window style - normal 
		CW_USEDEFAULT,							// x coordinate - let windows decide 
		CW_USEDEFAULT,							// y coordinate - let windows decide 
		480,									// width 
		560,									// height 
		HWND_DESKTOP,							// no parent window 
		NULL,									// no menu 
		hThisInst,								// handle of this instance of the program
		NULL									// no additional arguments 
		);

	// load settings from the registry
	createcheckerboard(hwnd);

	// display the window 
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// set database filename in case of shell-doubleclick on a *.pdn file
	sprintf(filename,lpszArgs);

	// initialize common controls - toolbar and status bar need this 
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_COOL_CLASSES|ICC_BAR_CLASSES;
	 InitCommonControlsEx(&iccex);
	
	// save the instance in a global variable - the dialog boxes in dialogs.c need this
	g_hInst = hThisInst;

	// load the keyboard accelerator table 
	hAccel = LoadAccelerators(hThisInst, "MENUENGLISH");

	// Initialize the Toolbar 
	tbwnd = CreateAToolBar(hwnd);

	// get toolbar height
	GetWindowRect(tbwnd, &rect);
	toolbarheight = rect.bottom - rect.top;
	
	// initialize status bar
	InitStatus(hwnd);

	// get status bar height
	GetWindowRect(hStatusWnd, &rect);
	statusbarheight = rect.bottom - rect.top;

	// get menu and title bar height:
	menuheight = GetSystemMetrics(SM_CYMENU);
	titlebarheight = GetSystemMetrics(SM_CXSIZE);

	// get offsets before the board is printed for the first time
	offset = toolbarheight + statusbarheight - 1;
	upperoffset = toolbarheight - 1;
	setoffsets(offset, upperoffset);

	// start a timer @ 10Hz: every time this timer goes off, handletimer() is called
	// this updates the status bar and the toolbar
	SetTimer(hwnd, 1, 100, NULL);

	// create the message loop 
	while (GetMessage(&msg, NULL, 0, 0))
		{	
		if(!TranslateAccelerator(hwnd, hAccel, &msg)) 
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		}
	return (int) msg.wParam; 
	}


LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message,WPARAM wParam, LPARAM lParam)
	{
	// this is the main function of checkerboard. it receives messages from winmain(), and
	// then acts appropriately
	FILE *fp;
	LPRECT lprec;
	int i;
	int k,l,x,y;
	char str2[256],Lstr[256];
	char str1024[1024];
	char *gamestring; 
	static enum state laststate;
	static int oldengine;
	RECT windowrect;
	RECT WinDim;
	static int cxClient,cyClient;
	MENUBARINFO mbi;
	HINSTANCE hinst;

	
	switch(message) 
		{
		case WM_CREATE:
			InitializeCriticalSection(&ani_criticalsection);
			InitializeCriticalSection(&engine_criticalsection);
			PostMessage(hwnd, WM_COMMAND, LOADENGINES, 0);
			break;

		case WM_NOTIFY: 
			// respond to tooltip request //
			// lParam contains (LPTOOLTIPTEXT) - send it on to handletooltiprequest function
			handletooltiprequest((LPTOOLTIPTEXT) lParam);
			break;

		case WM_DROPFILES:
			DragQueryFile((HDROP) wParam,0,database,sizeof(database));
			DragFinish((HDROP) wParam);
			PostMessage(hwnd,WM_COMMAND,GAMELOAD,0);
			break;

		case WM_TIMER:
			// timer goes off, telling us to update status bar, toolbar
			// icons. handletimer does this, only if it's necessary.
			handletimer();
			break;

		case WM_RBUTTONDOWN:
			x = (int)(LOWORD(lParam)/xmetric);
			y = (int)(8-(HIWORD(lParam)-toolbarheight)/ymetric);
			handle_rbuttondown(x,y);
			break;

		case WM_LBUTTONDOWN:
			x = (int)(LOWORD(lParam)/xmetric);
			y = (int)(8-(HIWORD(lParam)-toolbarheight)/ymetric);
			handle_lbuttondown(x,y);
			break;

		case WM_PAINT: 	// repaint window 
			updategraphics(hwnd);
			break;

		case WM_SIZING:  // keep window quadratic 
			lprec=(LPRECT)lParam;
			mbi.cbSize = sizeof(MENUBARINFO);
			GetMenuBarInfo(hwnd,OBJID_MENU,0,&mbi);
			menuheight = mbi.rcBar.bottom - mbi.rcBar.top;
			offset = toolbarheight + statusbarheight - 1;
			upperoffset = toolbarheight - 1;
			setoffsets(offset, upperoffset);
			cxClient = lprec->right - lprec->left;
			cxClient -= cxClient%8;
			cxClient += 2*(GetSystemMetrics(SM_CXSIZEFRAME)-4);
			cyClient = cxClient;
			lprec->right = lprec->left + cxClient;
			lprec->bottom = lprec->top + cyClient + offset + menuheight + titlebarheight + 2; //+ gCBoptions.addoffset;
			break;

		case WM_SIZE:	 
			// window size has changed 
			cxClient = LOWORD(lParam);
			cyClient = HIWORD(lParam);
			// check menu height
			mbi.cbSize = sizeof(MENUBARINFO);
			GetMenuBarInfo(hwnd,OBJID_MENU,0,&mbi);
			menuheight = mbi.rcBar.bottom - mbi.rcBar.top;
			offset = toolbarheight + statusbarheight-1;
			upperoffset = toolbarheight-1;
			setoffsets(offset, upperoffset);

			// get window size, set xmetric and ymetric which CB needs to know where user clicks
			GetClientRect(hwnd, &WinDim);
			xmetric = WinDim.right/8.0;
			ymetric = (WinDim.bottom - offset)/8.0;

			// get error:
			GetWindowRect(hwnd,&WinDim);
			// make window quadratic
			if((xmetric - ymetric)*8 != 0)
				MoveWindow(hwnd,WinDim.left, WinDim.top, WinDim.right-WinDim.left, WinDim.bottom-WinDim.top + (int)((xmetric-ymetric)*8),1);
		
			// update stretched stones etc
			resizegraphics(hwnd);
			updateboardgraphics(hwnd);
			SendMessage(hStatusWnd, WM_SIZE, wParam, lParam);
			SendMessage(tbwnd, WM_SIZE,wParam,lParam);
			break;

		case WM_COMMAND:
			// the following case structure handles user command (and also internal commands
			// that CB may generate itself
			switch(LOWORD(wParam)) 
				{
				case LOADENGINES:
					hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)initengines,(HWND) 0,0,&g_ThreadId);
					break;

				case GAMENEW:
					PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
					newgame();
					break;

				case GAMEANALYZE:
					if(CBstate == BOOKVIEW || CBstate == BOOKADD)
						break;
					changeCBstate(CBstate,ANALYZEGAME);
					startmatch=TRUE;
					// the rest is taken care of in the AutoThreadFunc section
					break;

				case GAMEANALYZEPDN:
					if(CBstate == BOOKVIEW || CBstate == BOOKADD)
						break;
					changeCBstate(CBstate,ANALYZEPDN);
					startmatch = TRUE;
					// the rest is taken care of in the AutoThreadFunc section
					break;

				case GAME3MOVE:
					PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
					if( gametype() == GT_ENGLISH)
						{
						if(gCBoptions.op_crossboard || gCBoptions.op_barred || gCBoptions.op_mailplay)
							{
							op=getopening(&gCBoptions);
							PostMessage(hwnd,WM_COMMAND,START3MOVE,0);
							}
						else
							MessageBox(hwnd,"nothing selected in the 3-move deck!", "Error",MB_OK);
						}
					else
						MessageBox(hwnd,"This option is only for engines\nwhich play the english/american\nversion of checkers.", "Error",MB_OK);
					break;

				case START3MOVE:
					start3move();
					break;

				case GAMEREPLACE:
					// replace a game in the pdn database 
					// assumption: you have loaded a game, so now "database" holds the db filename  
					// and gameindex is the index of the game in the file 
					handlegamereplace(gameindex, database);
					break;

				case GAMESAVE: 
					// show save game dialog. if OK, call 'dosave' to do the work 
					SetCurrentDirectory(gCBoptions.userdirectory);
					if (DialogBox(g_hInst, "IDD_SAVEGAME", hwnd, (DLGPROC)DialogFuncSavegame))
						{
						if(getfilename(filename,OF_SAVEGAME))
							{
							SendMessage(hwnd,WM_COMMAND,DOSAVE,0);
							}
						}
					SetCurrentDirectory(CBdirectory);
					break;

				case GAMESAVEASHTML:
					// show save game dialog. if OK is selected, call 'savehtml' to do the work 
					if (DialogBox(g_hInst, "IDD_SAVEGAME", hwnd, (DLGPROC)DialogFuncSavegame)) {
						if (getfilename(filename, OF_SAVEASHTML)) {
							saveashtml(filename, &GPDNgame);
							sprintf(str, "game saved as HTML!");
						}
					}
					break;

				case DOSAVE:
					// saves the game stored in GPDNgame 
					fp = fopen(filename, "at+");
					// file with filename opened	- we append to that file
					// filename was set by save game 
					if(fp != NULL)
						{
						gamestring = (char *) malloc(GAMEBUFSIZE);	
						if(gamestring!=NULL) {
							PDNgametoPDNstring(&GPDNgame,gamestring, "\n");
							fprintf(fp,"%s",gamestring);
							free(gamestring);
							}
						fclose(fp);
						}
					// set reindex flag
					reindex = 1;
					break;

				case GAMEDATABASE:
					// set working database 
					sprintf(database,"%s",gCBoptions.userdirectory);
					getfilename(database,OF_LOADGAME);
					// set reindex flag
					reindex = 1;
					break;

				case SELECTUSERBOOK:
					// set user book.
					sprintf(userbookname,"%s",CBdocuments);
					if(getfilename(userbookname,OF_USERBOOK))
						{
						// load user book
						fp = fopen(userbookname,"rb");
						if(fp != NULL)
							{
							userbooknum = fread(userbook,  sizeof(struct userbookentry),MAXUSERBOOK,fp);
							fclose(fp);
							}
						sprintf(str,"found %zi positions in user book", userbooknum);
						}
					break;

				case GAMELOAD:
					// call selectgame with GAMELOAD to let the user select from all games
					selectgame(GAMELOAD);
					break;

				case GAMEINFO:
					// display a box with information on the game
					//cpuid(str);
					sprintf(str1024,"Black: %s\nWhite: %s\nEvent: %s\nResult: %s",GPDNgame.black,GPDNgame.white,GPDNgame.event,GPDNgame.resultstring);
					MessageBox(hwnd,str1024,"Game information",MB_OK);
					sprintf(str,"");
					break;

				case SEARCHMASK:
					// call selectgame with SEARCHMASK to let the user
					// select from games of a certain player/event/date
					selectgame(SEARCHMASK);
					break;

				case RE_SEARCH:
					selectgame(RE_SEARCH);
					break;

				case GAMEFIND:
					// find a game with the current position in the current database
					// index the database
					selectgame(GAMEFIND);
					break;

				case GAMEFINDCR:
					// find games with current position color-reversed
					selectgame(GAMEFINDCR);
					break;

				case GAMEFINDTHEME:
					// find a game with the current position in the current database
					// index the database
					selectgame(GAMEFINDTHEME);
					break;

				case LOADNEXT:
					sprintf(str,"load next game");
					loadnextgame();
					break;

				case LOADPREVIOUS:
					sprintf(str,"load previous game");
					loadpreviousgame();
					break;

				case GAMEEXIT:
					PostMessage(hwnd,WM_DESTROY,0,0);
					break;

				case DIAGRAM:
					diagramtoclipboard(hwnd);
					break;

				case SAMPLEDIAGRAM:
					samplediagramtoclipboard(hwnd);
					break;

				case GAME_FENTOCLIPBOARD:
					if(setup)
						MessageBox(hwnd, "Cannot copy position in setup mode.\nLeave the setup mode first if you\nwant to copy this position.","Error", MB_OK);
					else
						FENtoclipboard(hwnd, board8, color, GPDNgame.gametype);
					break;

				case GAME_FENFROMCLIPBOARD:
					// first, get the stuff that is in the clipboard
					gamestring = textfromclipboard(hwnd, str);
					// now if we have something, do something with it
					if(gamestring != NULL)
						{
						PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);

						if(FENtoboard8(board8, gamestring, &color, GPDNgame.gametype))
							{
							updateboardgraphics(hwnd);
							reset = 1;
							newposition = TRUE;
							sprintf(str,"position copied");
							PostMessage(hwnd,WM_COMMAND,GAMEINFO,0);
							sprintf(GPDNgame.setup,"1");
							sprintf(GPDNgame.FEN,gamestring);
							}
						else
							sprintf(str,"no valid FEN position in clipboard!");
						free(gamestring);
						}
					break;

				case GAMECOPY:
					if(setup)
						MessageBox(hwnd, "Cannot copy game in setup mode.\nLeave the setup mode first if you\nwant to copy this game.","Error", MB_OK);
					else
						PDNtoclipboard(hwnd, &GPDNgame);
					break;

				case GAMEPASTE:
					// copy game or fen string from the clipboard...
					gamestring = textfromclipboard(hwnd, str);

					// now that the game is in gamestring doload() on it 
					if (gamestring != NULL) {
						PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);

						/* Detect fen or game, load it in either case. */
						if (is_fen(gamestring)) {
							if (!FENtoboard8(board8, gamestring, &color, GPDNgame.gametype)) {
								doload(&GPDNgame, gamestring, &color, board8);
								sprintf(str,"game copied");
							}
							else {
								reset_current_game_pdn();
								sprintf(str,"position copied");
							}
						}
						else {
							doload(&GPDNgame, gamestring, &color, board8);
							sprintf(str,"position copied");
						}
						free(gamestring);

						// game is fully loaded, clean up 
						updateboardgraphics(hwnd);
						reset=1;
						newposition=TRUE;
						PostMessage(hwnd,WM_COMMAND,GAMEINFO,0);
					}
					else
						sprintf(str,"clipboard open failed");
					break;

				case MOVESPLAY:		
					// force the engine to either play now, or to start calculating 
					// this is the only place where the engine is started
					if(!getenginebusy() && !getanimationbusy())
						{
						// TODO think about synchronization issues here!
						setenginebusy(TRUE);
						setenginestarting(FALSE);
						CloseHandle(hThread);
						hThread = CreateThread(NULL, 100000, (LPTHREAD_START_ROUTINE) ThreadFunc,(LPVOID) 0, 0, &g_ThreadId);
						}
					else
						SendMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
					x1 = -1;
					break;

				case INTERRUPTENGINE:
					// tell engine to stop thinking and play a move
					if(getenginebusy())
						playnow=1;
					break;

				case ABORTENGINE:
					// tell engine to stop thinking and not play a move
					if(getenginebusy())
						{
						abortcalculation = 1;
						playnow = 1;
						}
					break;

				case MOVESBACK:	
					// take back a move 
					abortengine();
					if(CBstate == BOOKVIEW && userbooknum != 0)
						{
						if(userbookcur>0)
							userbookcur--;
						userbookcur %= userbooknum;
						sprintf(str,"position %zi of %zi: %i-%i", userbookcur+1,userbooknum,coortonumber(userbook[userbookcur].move.from, GPDNgame.gametype), coortonumber(userbook[userbookcur].move.to,GPDNgame.gametype));

						// set up position
						if(userbookcur < userbooknum) // only if there are any positions
							{
							bitboardtoboard8(&(userbook[userbookcur].position), board8);
							updateboardgraphics(hwnd);
							}
						break;
						}

					if (current->last == NULL && (CBstate == ANALYZEGAME || CBstate == ANALYZEPDN) )
						gameover = TRUE;

					if (current->last != NULL)
						{
						current = current->last;
						undomove(current->move, board8);
						updateboardgraphics(hwnd);
						// shouldnt this color thing be handled in undomove?
						color = CB_CHANGECOLOR(color);
						sprintf(str,"takeback: ");
						// and print move number and move into the status bar
						// get move number:
						if(current != NULL)
							{
							i = getmovenumber(current);
							if(i%2)
								sprintf(Lstr,"%i... %s",i/2,current->PDN);
							else
								sprintf(Lstr,"%i. %s",i/2,current->PDN);
							strcat(str,Lstr);

							if(strcmp(current->comment,"")!=0)
								{
								sprintf(Lstr," %s",current->comment);
								strcat(str,Lstr);
								}
							}
						if(CBstate == OBSERVEGAME)
							PostMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
						else
							abortengine();
							}
					else 
						sprintf(str,"Takeback not possible: you are at the start of the game!");

					newposition=TRUE;
					reset=1;
					break;

				case MOVESFORWARD:	
					// go forward one move 
					// stop the engine if it is still running
					abortengine();

					// if in user book mode, move to the next position in user book
					if(CBstate == BOOKVIEW && userbooknum!=0)
						{
						if(userbookcur<userbooknum-1)
							userbookcur++;
						userbookcur %= userbooknum;
						sprintf(str,"position %zi of %zi: %i-%i", userbookcur+1,userbooknum,coortonumber(userbook[userbookcur].move.from,GPDNgame.gametype), coortonumber(userbook[userbookcur].move.to,GPDNgame.gametype));
						// set up position
						if(userbookcur < userbooknum) // only if there are any positions
							{
							bitboardtoboard8(&(userbook[userbookcur].position), board8);
							updateboardgraphics(hwnd);
							}
						break;
						}

					// normal case - move forward one move
					if( current->next!=NULL )
						{
						domove(current->move, board8);
						updateboardgraphics(hwnd);
						color = CB_CHANGECOLOR(color);
						// get move number:
						i = getmovenumber(current);

						// and print move number and move into the status bar
						if(i%2)
							sprintf(Lstr,"%i... %s",i/2,current->PDN);
						else
							sprintf(Lstr,"%i. %s",i/2,current->PDN);
						sprintf(str,"%s ",Lstr);

						if(strcmp(current->comment,"")!=0)
							{
							sprintf(Lstr,"%s",current->comment);
							strcat(str,Lstr);
							}
						current = current->next;

						goneforward = 1;

						if (CBstate == OBSERVEGAME)
							PostMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
						newposition=TRUE;
						reset=1;
						}
					else
						{
						sprintf(str,"Forward not possible: End of game");
						}
					break;

				case MOVESBACKALL:		
					// take back all moves 
					if(CBstate == BOOKVIEW || CBstate == BOOKADD)
						break;
					PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
					while ( current->last != NULL )
						{
						current = current->last;
						undomove( current->move, board8);
						color = CB_CHANGECOLOR(color);
						}
					if(CBstate == OBSERVEGAME)
						PostMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
					updateboardgraphics(hwnd);
					sprintf(str,"you are now at the start of the game");
					newposition=TRUE;
					reset=1;
					break;

				case MOVESFORWARDALL:	
					// go forward all moves 
					if(CBstate == BOOKVIEW || CBstate == BOOKADD)
						break;
					PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
					forward_to_game_end();
					if(CBstate == OBSERVEGAME)
						PostMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
					updateboardgraphics(hwnd);
					sprintf(str,"you are now at the end of the game");
					newposition=TRUE;
					reset=1;
					break;

				case MOVESCOMMENT: 
					// add a comment to the last move 
					DialogBox(g_hInst, "IDD_COMMENT", hwnd, (DLGPROC)DialogFuncAddcomment);
					break;

				case LEVELEXACT:
					if(gCBoptions.exact==TRUE)
						gCBoptions.exact=FALSE;
					else gCBoptions.exact=TRUE;
					if(gCBoptions.exact==TRUE)
						CheckMenuItem(hmenu,LEVELEXACT,MF_CHECKED);
					else
						CheckMenuItem(hmenu,LEVELEXACT,MF_UNCHECKED);
					break;
				case LEVELINSTANT:
					maxtime=0.01;
					gCBoptions.level=1;
					checklevelmenu(hmenu,LEVELINSTANT, &gCBoptions);
					break;
				case LEVEL01S:
					maxtime = 0.1;
					gCBoptions.level = 2;
					checklevelmenu(hmenu, LEVEL01S, &gCBoptions);
					break;
				case LEVEL02S:
					maxtime = 0.2;
					gCBoptions.level = 3;
					checklevelmenu(hmenu, LEVEL02S, &gCBoptions);
					break;
				case LEVEL05S:
					maxtime = 0.5;
					gCBoptions.level = 4;
					checklevelmenu(hmenu, LEVEL05S, &gCBoptions);
					break;
				case LEVEL1S:
					maxtime = 1;
					gCBoptions.level = 5;
					checklevelmenu(hmenu, LEVEL1S, &gCBoptions);
					break;
				case LEVEL2S:
					maxtime=2;
					gCBoptions.level=6;
					checklevelmenu(hmenu,LEVEL2S, &gCBoptions);
					break;
				case LEVEL5S:
					maxtime=5;
					gCBoptions.level=7;
					checklevelmenu(hmenu,LEVEL5S, &gCBoptions);
					break;
				case LEVEL10S:
					maxtime=10;
					gCBoptions.level=8;
					checklevelmenu(hmenu,LEVEL10S, &gCBoptions);
					break;
				case LEVEL15S:
					maxtime=15;
					gCBoptions.level=9;
					checklevelmenu(hmenu,LEVEL15S, &gCBoptions);
					break;
				case LEVEL30S:
					maxtime=30;
					gCBoptions.level=10;
					checklevelmenu(hmenu,LEVEL30S, &gCBoptions);
					break;
				case LEVEL1M:
					maxtime=60;
					gCBoptions.level=11;
					checklevelmenu(hmenu,LEVEL1M, &gCBoptions);
					break;
				case LEVEL2M:
					maxtime=120;
					gCBoptions.level=12;
					checklevelmenu(hmenu,LEVEL2M, &gCBoptions);
					break;
				case LEVEL5M:
					maxtime=300;
					gCBoptions.level=13;
					checklevelmenu(hmenu,LEVEL5M, &gCBoptions);
					break;
				case LEVEL15M:
					maxtime=900;
					gCBoptions.level=14;
					checklevelmenu(hmenu,LEVEL15M, &gCBoptions);
					break;
				case LEVEL30M:
					maxtime=1800;
					gCBoptions.level=15;
					checklevelmenu(hmenu,LEVEL30M, &gCBoptions);
					break;
				case LEVELINFINITE:
					maxtime=8600000;
					gCBoptions.level=16;
					checklevelmenu(hmenu,LEVELINFINITE, &gCBoptions);
					break;
				case LEVELINCREMENT:
					// set clock to two minutes
					maxtime=initialtime;
					gCBoptions.level=17;
					checklevelmenu(hmenu,LEVELINCREMENT, &gCBoptions);
					sprintf(str,"increment level set: initial time %.0f, increment time %.0f (seconds)",initialtime,incrementtime);
					break;
				case LEVELADDTIME:
					// add  seconds when '+' is pressed
					if(gCBoptions.level == 17)
						{
						maxtime+=1.0;
						sprintf(str,"remaining time: %.1f",maxtime);
						}
					else
						sprintf(str,"error: not in increment mode!");
					break;

				case LEVELSUBTRACTTIME:
					// subtract 1 seconds when '-' is pressed
					if(gCBoptions.level==17)
						{
						maxtime-=1.0;
						sprintf(str,"remaining time: %.1f",maxtime);
						}
					else
						sprintf(str,"error: not in increment mode!");
					break;

				// piece sets 
				case PIECESET:
				case PIECESET+1:
				case PIECESET+2:
				case PIECESET+3:
				case PIECESET+4:
				case PIECESET+5:
				case PIECESET+6:
				case PIECESET+7:
				case PIECESET+8:
				case PIECESET+9:
				case PIECESET+10:
				case PIECESET+11:
				case PIECESET+12:
				case PIECESET+13:
				case PIECESET+14:
				case PIECESET+15:
					gCBoptions.piecesetindex = LOWORD(wParam)-PIECESET;
					sprintf(str,"piece set %i: %s", gCBoptions.piecesetindex, piecesetname[gCBoptions.piecesetindex]);
					SetCurrentDirectory(CBdirectory);
					SetCurrentDirectory("bmp");
					initbmp(hwnd,piecesetname[gCBoptions.piecesetindex]);
					resizegraphics(hwnd);
					updateboardgraphics(hwnd);
					InvalidateRect(hwnd, NULL, 1);
					SetCurrentDirectory(CBdirectory);
					break;


				//  set highlighting color to draw frame around selected stone square
				case COLORHIGHLIGHT: 	
					initcolorstruct(hwnd, &ccs, 0);
					if(ChooseColor(&ccs))
						{
						gCBoptions.colors[0]=(COLORREF) ccs.rgbResult;
						sprintf(str,"new highlighting color");
						}
					else
						sprintf(str,"no new colors! error %i",CommDlgExtendedError());
					updateboardgraphics(hwnd);
					break;

				//  set color for board numbers
				case COLORBOARDNUMBERS: 
					initcolorstruct(hwnd, &ccs,1);
					if(ChooseColor(&ccs))
						{
						gCBoptions.colors[1]=(COLORREF) ccs.rgbResult;
						sprintf(str,"new board number color");
						}
					else
						sprintf(str,"no new colors! error %i",CommDlgExtendedError());
					updateboardgraphics(hwnd);
					break;

				case OPTIONS3MOVE:
					DialogBox(g_hInst,"IDD_3MOVE",hwnd,(DLGPROC)ThreeMoveDialogFunc);
					break;

				case OPTIONSDIRECTORIES:
					DialogBox(g_hInst,"IDD_DIRECTORIES",hwnd,(DLGPROC)DirectoryDialogFunc);
					break;

				case OPTIONSUSERBOOK:
					toggle(&(gCBoptions.userbook));
					setmenuchecks(&gCBoptions, hmenu);
					break;

				case OPTIONSLANGUAGEENGLISH:
					SetMenuLanguage(OPTIONSLANGUAGEENGLISH);
					break;

				case OPTIONSLANGUAGEDEUTSCH:
					SetMenuLanguage(OPTIONSLANGUAGEDEUTSCH);
					break;

				case OPTIONSLANGUAGEESPANOL:
					SetMenuLanguage(OPTIONSLANGUAGEESPANOL);
					break;

				case OPTIONSLANGUAGEITALIANO:
					SetMenuLanguage(OPTIONSLANGUAGEITALIANO);
					break;
				
				case OPTIONSLANGUAGEFRANCAIS:
					SetMenuLanguage(OPTIONSLANGUAGEFRANCAIS);
					break;

				case OPTIONSPRIORITY:
					toggle(&(gCBoptions.priority));
					if(gCBoptions.priority) // low priority mode
						usersetpriority=THREAD_PRIORITY_BELOW_NORMAL;
					else
						usersetpriority=THREAD_PRIORITY_NORMAL;
					setmenuchecks(&gCBoptions, hmenu);
					break;

				case OPTIONSHIGHLIGHT:
					if(gCBoptions.highlight==TRUE) 
						gCBoptions.highlight=FALSE;
					else 
						gCBoptions.highlight=TRUE;
					if(gCBoptions.highlight==TRUE)
						CheckMenuItem(hmenu,OPTIONSHIGHLIGHT,MF_CHECKED);
					else
						CheckMenuItem(hmenu,OPTIONSHIGHLIGHT,MF_UNCHECKED);
					break;

				case OPTIONSSOUND:
					if(gCBoptions.sound==TRUE) gCBoptions.sound=FALSE;
					else gCBoptions.sound=TRUE;
					if(gCBoptions.sound==TRUE)
						CheckMenuItem(hmenu,OPTIONSSOUND,MF_CHECKED);
					else
						CheckMenuItem(hmenu,OPTIONSSOUND,MF_UNCHECKED);
					break;


				case BOOKMODE_VIEW:
					// go in view book mode
					if(userbooknum == 0)
						{
						sprintf(str,"no moves in user book");
						break;
						}
					if(CBstate == BOOKVIEW)
						changeCBstate(CBstate,NORMAL);
					else
						{
						changeCBstate(CBstate,BOOKVIEW);
						// now display the first user book position
						userbookcur = 0;
						sprintf(str,"position %zi of %zi: %i-%i", userbookcur+1,userbooknum,coortonumber(userbook[userbookcur].move.from,GPDNgame.gametype), coortonumber(userbook[userbookcur].move.to,GPDNgame.gametype));
						// set up position
						if(userbookcur < userbooknum) // only if there are any positions
							{
							bitboardtoboard8(&(userbook[userbookcur].position), board8);
							updateboardgraphics(hwnd);
							}
						}
					break;

				case BOOKMODE_ADD:	
					// go in add/edit book mode
					if(CBstate == BOOKADD)
						changeCBstate(CBstate,NORMAL);
					else
						changeCBstate(CBstate,BOOKADD);
					break;

				case BOOKMODE_DELETE:
					// remove current user book position from book
					if(CBstate == BOOKVIEW && userbooknum!=0)
						{
						// want to delete book move here:
						for(i=userbookcur;i<(int)userbooknum-1;i++)
							userbook[i] = userbook[i+1];
						userbooknum--;
						// if we deleted last position, move to new last position.
						if(userbookcur==userbooknum)
							userbookcur--;
						// display what position we have:
						sprintf(str,"position %zi of %zi: %i-%i", userbookcur+1,userbooknum,coortonumber(userbook[userbookcur].move.from,GPDNgame.gametype), coortonumber(userbook[userbookcur].move.to,GPDNgame.gametype));
						if(userbooknum==0)
							sprintf(str,"no moves in user book");
						// set up position
						if(userbookcur < userbooknum && userbooknum!=0) // only if there are any positions
							{
							bitboardtoboard8(&(userbook[userbookcur].position), board8);
							updateboardgraphics(hwnd);
							}
						// save user book
						fp = fopen(userbookname,"wb");
						if(fp != NULL)
							{
							fwrite(userbook,sizeof(struct userbookentry),userbooknum,fp);
							fclose(fp);
							}
						else
							sprintf(str,"unable to write to user book");
						}
					else
						{
						if(CBstate == BOOKVIEW)
							sprintf(str,"no moves in user book");
						else
							sprintf(str,"You must be in 'view user book' mode to delete moves!");
						}
					break;

				case CM_NORMAL:	  
					// go to normal play mode 
					if(getenginebusy())
						playnow = 1;		  
					// stop engine 
					PostMessage(hwnd,WM_COMMAND,GOTONORMAL,0);
					break;

				case CM_AUTOPLAY:
					if(CBstate == BOOKVIEW || CBstate == BOOKADD)
						break;
					changeCBstate(CBstate,AUTOPLAY);
					//setenginebusy(FALSE); // what is this here for?
					break;

				case CM_ENGINEMATCH:
					if(CBstate == BOOKVIEW || CBstate == BOOKADD)
						break;
					changeCBstate(CBstate,ENGINEMATCH);
					break;

				case ENGINEVSENGINE:
					startmatch = TRUE;
					changeCBstate(CBstate, ENGINEGAME);
					break;

				case CM_ANALYSIS:  
					// go to analysis mode 
					if(CBstate == BOOKVIEW || CBstate == BOOKADD)
						break;
					changeCBstate(CBstate,OBSERVEGAME);
					break;

				case CM_2PLAYER:
					SendMessage(hwnd, WM_COMMAND, TOGGLEMODE, 0);
					break;

				case GOTONORMAL:
					// the following is weird, posts the same message again, why?
					if(getenginebusy())
						{
						PostMessage(hwnd,WM_COMMAND,GOTONORMAL,0);
						Sleep(10);
						}
					else
						{
						changeCBstate(CBstate,NORMAL);
						//setenginebusy(FALSE);
						//setanimationbusy(FALSE);
						}
					break;

				case ENGINESELECT:	
					// select engines
					DialogBox(g_hInst,"IDD_DIALOGENGINES",hwnd,(DLGPROC)EngineDialogFunc);
					break;

				case ENGINEOPTIONS:
					// select engine options
					oldengine = currentengine;
					DialogBox(g_hInst,"IDD_ENGINEOPTIONS",hwnd,(DLGPROC)EngineOptionsFunc);
					setcurrentengine(oldengine);
					enginecommand("get book",Lstr);
					togglebook = atoi(Lstr);
					break;

				case ENGINEEVAL:
					// static eval of the current positions
					board8toFEN(board8,str2,color,GPDNgame.gametype);
					sprintf(Lstr,"staticevaluation %s",str2);
					if(enginecommand(Lstr,reply))
						MessageBox(hwnd,reply,"Static Evaluation",MB_OK);
					else
						MessageBox(hwnd,"This engine does not support\nstatic evaluation","About Engine",MB_OK);
					break;

				case ENGINEABOUT: 	
					// tell engine to display information about itself 
					if(enginecommand("about",reply))
						MessageBox(hwnd,reply,"About Engine",MB_OK);
					break;

				case ENGINEHELP:		
					// get a help-filename from engine and display file with default viewer
					if(enginecommand("help",str2))
						{
						if(strcmp(str2,""))
							{
							SetCurrentDirectory(CBdirectory);
							SetCurrentDirectory("engines");
							showfile(str2);
							SetCurrentDirectory(CBdirectory);
							}
						else
							MessageBox(hwnd,"This engine has no help file","CheckerBoard says:",MB_OK);
						}
					else
						MessageBox(hwnd,"This engine has no help file","CheckerBoard says:",MB_OK);
					break;

				case DISPLAYINVERT:		
					// toggle: invert the board yes/no
					toggle(&(gCBoptions.invert));
					if(gCBoptions.invert) 
						CheckMenuItem(hmenu,DISPLAYINVERT,MF_CHECKED);
					else 
						CheckMenuItem(hmenu,DISPLAYINVERT,MF_UNCHECKED);
					updateboardgraphics(hwnd);
					break;

				case DISPLAYMIRROR:		
					// toggle: mirror the board yes/no
					// this is a trick to make checkers variants like italian display properly.
					toggle(&(gCBoptions.mirror));
					if(gCBoptions.mirror) 
						CheckMenuItem(hmenu,DISPLAYMIRROR,MF_CHECKED);
					else 
						CheckMenuItem(hmenu,DISPLAYMIRROR,MF_UNCHECKED);
					updateboardgraphics(hwnd);
					break;

				case DISPLAYNUMBERS:   
					// toggle display numbers on / off 
					toggle(&(gCBoptions.numbers));
					if(gCBoptions.numbers) CheckMenuItem(hmenu,DISPLAYNUMBERS,MF_CHECKED);
					else CheckMenuItem(hmenu,DISPLAYNUMBERS,MF_UNCHECKED);
					updateboardgraphics(hwnd);
					break;

				case SETUPMODE:		 
					// toggle from play to setup mode 
					PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
					toggle(&setup);
					if(setup)
						{
						// entering setup mode
						CheckMenuItem(hmenu,SETUPMODE,MF_CHECKED);
						}
					else
						{
						// leaving setup mode;
						CheckMenuItem(hmenu,SETUPMODE,MF_UNCHECKED);
						reset=1;
						x1=-1;
						}
					if(setup) 
						sprintf(str, "Setup mode...");
					if(!setup)
						{
						sprintf(str, "Setup done");
						// get FEN string
						reset_current_game_pdn();
						board8toFEN(board8, GPDNgame.FEN, color, GPDNgame.gametype);
						sprintf(GPDNgame.setup, "1");
						}
					break;

				case SETUPCLEAR:      
					// clear board 
					for(k=0;k<=7;k++) 
						{
						for(l=0;l<=7;l++) 
							board8[k][l]=0;
						}
					updateboardgraphics(hwnd);
					break;

				case TOGGLEMODE:
					if(getenginebusy() || getanimationbusy())
						break;
					toggle(&togglemode);
					if(togglemode)
						changeCBstate(CBstate,ENTERGAME);
					else
						changeCBstate(CBstate,NORMAL);
					break;

				case TOGGLEBOOK:
					if(getenginebusy() || getanimationbusy())
						break;
					togglebook++;
					togglebook %= 4;
					// set opening book on/off
					sprintf(Lstr, "set book %i", togglebook);
					enginecommand(Lstr, str);
					break;

				case TOGGLEENGINE:
					if(getenginebusy() || getanimationbusy())
						break;
					toggleengine++;
					if(toggleengine>2)
						toggleengine = 1;

					setcurrentengine(toggleengine);

					// reset game if an engine of different game type was selected!
					if (gametype() != GPDNgame.gametype) {
						PostMessage(hwnd, (UINT)WM_COMMAND, (WPARAM)GAMENEW, (LPARAM)0);
						PostMessage(hwnd, (UINT)WM_SIZE, (WPARAM)0, (LPARAM)0);
					}
					break;

				case SETUPCC:
					handlesetupcc(&color);
					break;

				case HELPHELP:      
					// open the help.htm file with default .htm viewer 
					SetCurrentDirectory(CBdirectory);
					switch(gCBoptions.language)
						{
						case ESPANOL:
							showfile("helpspanish.htm");
							break;
						case DEUTSCH:
							if(fileispresent("helpdeutsch.htm"))
								showfile("helpdeutsch.htm");
							else
								showfile("help.htm");
							break;
						case ITALIANO:
							if(fileispresent("helpitaliano.htm"))
								showfile("helpitaliano.htm");
							else
								showfile("help.htm");
							break;
						case FRANCAIS:
							if(fileispresent("helpfrancais.htm"))
								showfile("helpfrancais.htm");
							else
								showfile("help.htm");
							break;
						case ENGLISH:
							showfile("help.htm");
							break;
						}
					break;

				case HELPCHECKERSINANUTSHELL:      
					// open the richard pask's tutorial with default .htm viewer 
					showfile("nutshell.htm");
					break;

				case HELPABOUT:    
					// display a an about box
					DialogBox(g_hInst,"IDD_CBABOUT",hwnd,(DLGPROC)AboutDialogFunc);
					break;

				case HELPHOMEPAGE:
					// open the checkerboard homepage with default htm viewer
					hinst = ShellExecute(NULL,"open","www.fierz.ch/checkers.htm",NULL,NULL,SW_SHOW);
					break;

				case CM_ENGINECOMMAND:
					DialogBox(g_hInst,"IDD_ENGINECOMMAND",hwnd,(DLGPROC)DialogFuncEnginecommand);
					break;

				case CM_ADDCOMMENT:
					toggle(&addcomment);
					if(addcomment) 
						CheckMenuItem(hmenu,CM_ADDCOMMENT,MF_CHECKED);
					else 
						CheckMenuItem(hmenu,CM_ADDCOMMENT,MF_UNCHECKED);
					break;

				case CM_HANDICAP:
					toggle(&handicap);
					if(handicap == TRUE)
						CheckMenuItem(hmenu, CM_HANDICAP, MF_CHECKED);
					else
						CheckMenuItem(hmenu, CM_HANDICAP, MF_UNCHECKED);
					break;

				case CM_RUNTESTSET:
					// let CB run over a set of test positions in the current pdn database
					testset_number = 0;
					changeCBstate(CBstate,RUNTESTSET);
					break;

				/*case CM_BUILDEGDB:
					// build six-piece db with dbgen.bat
					if(MessageBox(hwnd,"Building the 6-piece database will\ntake a while (4 hours on a fast computer).\nDo you wish to coninue?","Confirm database build",MB_OKCANCEL|MB_ICONQUESTION) == IDOK)
						builddb(str);
					break;*/
				}
			break;

		case WM_DESTROY: 
			// terminate the program 
			// save window size:
			GetWindowRect(hwnd,&windowrect);
			gCBoptions.window_x = windowrect.left;
			gCBoptions.window_y = windowrect.top;
			gCBoptions.window_width = windowrect.right - windowrect.left;
			gCBoptions.window_height = windowrect.bottom - windowrect.top;

			// save settings 
			savesettings(&gCBoptions);

			//Shell_NotifyIcon(NIM_DELETE,&pnid); // remove tray icon 
		

			// unload engines
			fFreeResult = FreeLibrary(hinstLib1);
			fFreeResult = FreeLibrary(hinstLib2);

			PostQuitMessage(0);
			break;

		default:
			// Let Windows process any messages not specified
			//	in the preceding switch statement 
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	return 0;
	}


void reset_current_game_pdn()
		{
		initlinkedlist();
		sprintf(GPDNgame.black, "");
		sprintf(GPDNgame.white, "");
		sprintf(GPDNgame.resultstring, "*");
		sprintf(GPDNgame.event, "");
		sprintf(GPDNgame.date, "");
		sprintf(GPDNgame.FEN, "");
		sprintf(GPDNgame.round, "");
		sprintf(GPDNgame.setup, "");
		sprintf(GPDNgame.site, "");
		GPDNgame.result = CB_UNKNOWN;
		GPDNgame.head = head;
		GPDNgame.gametype = gametype();
		}


int SetMenuLanguage(int language)
	{
	// load the proper menu
	DestroyMenu(hmenu);
	switch(language)
		{
		case ENGLISH:
		case OPTIONSLANGUAGEENGLISH:
			hmenu = LoadMenu(g_hInst, "MENUENGLISH");
			gCBoptions.language = ENGLISH;
			SetMenu(hwnd, hmenu);
			break;
		case DEUTSCH:
		case OPTIONSLANGUAGEDEUTSCH:
			hmenu = LoadMenu(g_hInst, "MENUDEUTSCH");
			gCBoptions.language = DEUTSCH;
			SetMenu(hwnd, hmenu);
			break;
		case ESPANOL:
		case OPTIONSLANGUAGEESPANOL:
			hmenu = LoadMenu(g_hInst, "MENUESPANOL");
			gCBoptions.language = ESPANOL;
			SetMenu(hwnd, hmenu);
			break;
		case ITALIANO:
		case OPTIONSLANGUAGEITALIANO:
			hmenu = LoadMenu(g_hInst, "MENUITALIANO");
			gCBoptions.language = ITALIANO;
			SetMenu(hwnd, hmenu);
			break;
		case FRANCAIS:
		case OPTIONSLANGUAGEFRANCAIS:
			hmenu = LoadMenu(g_hInst, "MENUFRANCAIS");
			gCBoptions.language = FRANCAIS;
			SetMenu(hwnd, hmenu);
			break;
		}

	// delete stuff we don't need
	SetCurrentDirectory(CBdirectory);
	if(fileispresent("db\\db6.cpr"))
		DeleteMenu(hmenu, 8, MF_BYPOSITION);

	DeleteMenu(hmenu,LEVELINCREMENT,MF_BYCOMMAND);
	DeleteMenu(hmenu,LEVELADDTIME,MF_BYCOMMAND);
	DeleteMenu(hmenu,LEVELSUBTRACTTIME,MF_BYCOMMAND);

	// now insert stuff we do need: piece set choice depending on what is installed
	add_piecesets_to_menu(hmenu);

	DrawMenuBar(hwnd);
	return 1;
	}

int get_startcolor(int gametype)
{
	int color = CB_BLACK;

	if (gametype == GT_ITALIAN) 
		color=CB_WHITE;
	else if (gametype == GT_SPANISH)
		color = CB_WHITE;
	else if (gametype == GT_RUSSIAN) 
		color = CB_WHITE;
	else if (gametype == GT_CZECH)
		color = CB_WHITE;

	return(color);
}


int is_mirror_gametype(int gametype)
{
	if (gametype == GT_ITALIAN) 
		return(1);
	if (gametype == GT_SPANISH) 
		return(1);

	return(0);
}


int handlesetupcc(int *color)
// handle change color request
	{
	char str2[256];

	if(*color==CB_BLACK)
		*color=CB_WHITE;
	else
		*color=CB_BLACK;

	reset_current_game_pdn();
	gCBoptions.mirror = is_mirror_gametype(GPDNgame.gametype);

	// and the setup codes 
	sprintf(GPDNgame.setup,"1");
	board8toFEN(board8,str2,*color,GPDNgame.gametype);
	sprintf(GPDNgame.FEN,str2);
	return 1;
	}

int handle_rbuttondown(int x, int y)
	{
	if(setup)
		{
		coorstocoors(&x,&y, gCBoptions.invert, gCBoptions.mirror);
		if( (x+y+1)%2)
			{
			switch(board8[x][y])
				{
				case CB_WHITE|CB_MAN: 
					board8[x][y] = CB_WHITE|CB_KING;
					break;
				case CB_WHITE|CB_KING: 
					board8[x][y] = 0;
					break;
				default: 
					board8[x][y] = CB_WHITE|CB_MAN;
					break;
				}
			}
		updateboardgraphics(hwnd);
		}
	return 1;
	}


int handle_lbuttondown(int x, int y)
	{
	int i, legal, legalmovenumber;
	int from, to; 
	// if we are in setup mode, add a black piece.
	if(setup)
		{
		coorstocoors(&x,&y,gCBoptions.invert, gCBoptions.mirror);
		if( (x+y+1)%2)
			{
			switch(board8[x][y])
				{
				case CB_BLACK|CB_MAN: 
					board8[x][y]=CB_BLACK|CB_KING;
					break;
				case CB_BLACK|CB_KING: 
					board8[x][y]=0;
					break;
				default: 
					board8[x][y]=CB_BLACK|CB_MAN;
					break;
				}
			}
		updateboardgraphics(hwnd);
		return 1;
		}

	// if the engine is calculating we don't accept any input - except if
	//	we are in "enter game" mode 
	if((getenginebusy() || getanimationbusy()) && (CBstate != OBSERVEGAME))
		return 0;

	if(x1==-1) //then its the first click
		{
		x1 = x;
		y1 = y;
		coorstocoors(&x1,&y1,gCBoptions.invert, gCBoptions.mirror);

		// if there is only one move with this piece, then do it!
		if(islegal != NULL)
			{
			legal = 0;
			legalmovenumber = 0;
			for(i=1; i<=32; i++)
				{
				if(islegal(board8, color, coorstonumber(x1,y1,GPDNgame.gametype), i, &GCBmove) != 0)
					{
					legal++;
					legalmovenumber = i;
					from = coorstonumber(x1,y1,GPDNgame.gametype);
					to = i; 
					}
				}

			// look for a single move possible to an empty square
			if(legal == 0) {
				for(i=1; i<=32; i++)
				{
				if(islegal(board8, color, i, coorstonumber(x1,y1,GPDNgame.gametype), &GCBmove) != 0)
					{
					legal++;
					legalmovenumber = i;
					from = i; 
					to = coorstonumber(x1,y1,GPDNgame.gametype); 
					}
				}
				if(legal != 1)
					legal = 0; 
			}

			// remove the output that islegal generated, it's disturbing ("1-32 illegal move")
			sprintf(str,"");
			if(legal == 1)
				{
				// is it the only legal move?
				// if yes, do it! 
				// if we are in user book mode, add it to user book!
				//if(islegal((int *)board8,color,coorstonumber(x1,y1,GPDNgame.gametype),legalmovenumber,&GCBmove)!=0)
				if(islegal(board8,color,from,to,&GCBmove)!=0)
					// a legal move!
					{
					// insert move in the linked list 
					appendmovetolist(GCBmove);
					// animate the move: 
					move = GCBmove;
					
					// if we are in userbook mode, we save the move
					if(CBstate == BOOKADD)
						addmovetouserbook(board8, &GCBmove);

					// call animation function which will also execute the move
					CloseHandle(hAniThread);
					setanimationbusy(TRUE);
					hAniThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AnimationThreadFunc,hwnd,0,&g_AniThreadId);
					x1 = -1;
					// if we are in enter game mode: tell engine to stop 
					if(CBstate == OBSERVEGAME)
						SendMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
					newposition = TRUE;
					if(CBstate == NORMAL) 
						startengine = TRUE;
					// startengine = TRUE tells the autothread to start the engine 
					// if we are in add moves to book mode, add this move to the book
					}
				}
			}
		// if the stone is the color of the side to move, allow it to be selected
		if((color==CB_BLACK && board8[x1][y1]&CB_BLACK) || (color==CB_WHITE && board8[x1][y1]&CB_WHITE))
			{
			// re-print board to overwrite last selection if there was one
			updateboardgraphics(hwnd);
			// and then select stone
			selectstone(x1, y1, hwnd, board8);
			}
		// else, reset the click count to 0.
		else
			x1=-1;
		}
	else			 
		//then its the second click
		{
		x2 = x;
		y2 = y;
		coorstocoors(&x2,&y2,gCBoptions.invert, gCBoptions.mirror);
		if(! ((x2+y2+1)%2))
			return 0;

		// now, perhaps the user selected another stone; i.e. the second
		// click is ALSO on a stone of the user. then we assume he has changed
		// his mind and now wants to move this stone. 
		// if the stone is the color of the side to move, allow it to be selected
		// !! there is one exception to this: a round-trip move such as
		// here [FEN "W:WK14:B19,18,11,10."]
		// however, with the new one-click-move input, this will work fine now!

		if((color==CB_BLACK && board8[x2][y2]&CB_BLACK) || (color==CB_WHITE && board8[x2][y2]&CB_WHITE))
			{
			// re-print board to overwrite last selection if there was one
			updateboardgraphics(hwnd);
			// and then select stone
			selectstone(x2, y2, hwnd, board8);
			// set second click to first click
			x1 = x2;
			y1 = y2;

			// check whether this is an only move
			legal = 0;
			legalmovenumber = 0;
			if(islegal!=NULL)
				{
				legalmovenumber = 0;
				for(i=1; i<=32; i++)
					{
					if(islegal(board8, color, coorstonumber(x1,y1,GPDNgame.gametype), i, &GCBmove)!=0)
						{
						legal++;
						legalmovenumber = i;
						}
					}
				}
			sprintf(str,"");
			if(legal == 1) // only one legal move
				{
				if(islegal(board8,color,coorstonumber(x1,y1,GPDNgame.gametype),legalmovenumber,&GCBmove)!=0)
					// a legal move!
					{
					// insert move in the linked list 
					appendmovetolist(GCBmove);
					// animate the move: 
					move = GCBmove;
					CloseHandle(hAniThread);

					// if we are in userbook mode, we save the move
					if(CBstate == BOOKADD)
						addmovetouserbook(board8, &GCBmove);

					// call animation function which will also execute the move
					setanimationbusy(TRUE);
					hAniThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AnimationThreadFunc,(HWND) hwnd,0,&g_AniThreadId);

					// if we are in enter game mode: tell engine to stop 
					if(CBstate==OBSERVEGAME)
						SendMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
					newposition=TRUE;
					if(CBstate==NORMAL) 
						startengine=TRUE;
					// startengine = TRUE tells the autothread to start the engine 
					// if we are in add moves to book mode, add this move to the book
					}
				}
			else
				// and break so as not to execute the rest of this clause, because
				// that is for actually making a move.
				return 0;
			}

		// check move and if ok
		if(islegal!=NULL)
			{
			if(islegal(board8,color,coorstonumber(x1,y1,GPDNgame.gametype),coorstonumber(x2,y2, GPDNgame.gametype),&GCBmove)!=0)
				// a legal move!
				{
				// insert move in the linked list 
				appendmovetolist(GCBmove);
				// animate the move: 
				move=GCBmove;
				CloseHandle(hAniThread);

				// if we are in userbook mode, we save the move
				if(CBstate == BOOKADD)
					addmovetouserbook(board8, &GCBmove);

				// call animation function which will also execute the move
				setanimationbusy(TRUE);
				hAniThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AnimationThreadFunc,(HWND) hwnd,0,&g_AniThreadId);
				// if we are in enter game mode: tell engine to stop 
				if(CBstate==OBSERVEGAME)
					SendMessage(hwnd,WM_COMMAND,INTERRUPTENGINE,0);
				newposition=TRUE;
				if(CBstate==NORMAL) 
					startengine=TRUE;
				// startengine = TRUE tells the autothread to start the engine 
				// if we are in add moves to book mode, add this move to the book
				}
			}
		x1=-1;
		}
	//updateboardgraphics(hwnd);
	return 1;
	}

int handletimer(void)
	{
	// timer goes off all 1/10th of a second. this function polls some things and updates
	// them if necessary:
	// icons in the toolbar (color, two-player, engine, book mode).
	// generates pseudo-logfile for engines that don't do this themselves.
	static char oldstr[1024];
	char filename[MAX_PATH];
	static int oldcolor;
	static int oldtogglemode;
	static int oldtogglebook;
	static int oldtoggleengine;
	static int engineIcon; 
	FILE *Lfp;
	int  ch = '=';


	if(strcmp(oldstr,str)!=0)
		{
		SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) str);
		sprintf(oldstr,"%s",str);
		// if we're running a test set, create a pseudolog-file
		if(CBstate == RUNTESTSET)
			{
			if(strchr(str,ch) != NULL)
				{
				strcpy(filename, CBdocuments);
				PathAppend(filename, "testlog.txt");
				Lfp = fopen(filename, "a");
				if(Lfp != NULL)
					{
					fprintf(Lfp,"%s\n",str);
					fclose(Lfp);
					}
				}
			}
		}

	// TODO: update toolbar to display engine thinking
	/*if(enginebusy) {
		engineIcon = (engineIcon++ % 8);
		SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)..., MAKELPARAM(x,0)); 
	}
	else {
	}*/

	// update toolbar to display whose turn it is 
	if(oldcolor!=color)
		{
		if(color==CB_BLACK)
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)SETUPCC,MAKELPARAM(10,0));
		else
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)SETUPCC,MAKELPARAM(9,0));
		oldcolor=color;
		InvalidateRect(hwnd,NULL,0);
		}

	// update toolbar to display what mode (normal/2player) we're in 
	if(oldtogglemode!=togglemode)
		{
		if(togglemode==0)
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEMODE,MAKELPARAM(17,0));
		else
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEMODE,MAKELPARAM(18,0));
		oldtogglemode = togglemode;
		InvalidateRect(hwnd,NULL,0);
		}

	// update toolbar to display book mode (on/off) we're in 
	if(oldtogglebook!=togglebook)
		{
		switch(togglebook)
			{
			case 0:
				SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEBOOK,MAKELPARAM(3,0));
				break;
			case 1:
				SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEBOOK,MAKELPARAM(6,0));
				break;
			case 2:
				SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEBOOK,MAKELPARAM(5,0));
				break;
			case 3:
				SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEBOOK,MAKELPARAM(4,0));
				break;

			}
		oldtogglebook = togglebook;
		InvalidateRect(hwnd,NULL,0);
		}

	// update toolbar to display active engine (primary/secondary)
	if(oldtoggleengine!=toggleengine)
		{
		if(toggleengine==1)
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEENGINE,MAKELPARAM(0,0));
		else
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)TOGGLEENGINE,MAKELPARAM(1,0));
		oldtoggleengine = toggleengine;
		InvalidateRect(hwnd,NULL,0);
		}

	return 1;
	}


int addmovetouserbook(int b[8][8], struct CBmove *move)
	{
	size_t i, n;
	FILE *fp;
	struct pos userbookpos;

	// if we have too many moves, stop!
	// userbooknum is a global, as it is also used in removing stuff from book
	if(userbooknum >= MAXUSERBOOK)
		{
		sprintf(str,"user book size limit reached!");
		return 0;
		}

	boardtobitboard(b, &userbookpos);
	// check if we already have this position:
	n = userbooknum;
	for(i=0;i<userbooknum;i++)
		{
		if(userbook[i].position.bm == userbookpos.bm && userbook[i].position.bk == userbookpos.bk && userbook[i].position.wm == userbookpos.wm && userbook[i].position.wk == userbookpos.wk)
			{
			// we already have this position!
			// in this case, we overwrite the old entry!
			n=i;
			break;
			}
		}

	userbook[n].position = userbookpos;
	userbook[n].move = *move;
	if(n == userbooknum)
		{
		(userbooknum)++;
		sprintf(str,"added move to userbook (%zi moves)",userbooknum);
		}
	else
		sprintf(str,"replaced move in userbook (%zi moves)",userbooknum);
	// save user book
	fp = fopen(userbookname,"wb");
	if(fp != NULL)
		{
		fwrite(userbook,sizeof(struct userbookentry),userbooknum,fp);
		fclose(fp);
		}
	else
		sprintf(str,"unable to write to user book");
	
	return 1;
	}


int handlegamereplace(int replaceindex, char *databasename)
	{
	FILE *fp;
	char *gamestring, *dbstring, *p;
	size_t bytesread;
	int i;
	int filesize = getfilesize(databasename);
	// give the user a chance to save new results / names 
	if(DialogBox(g_hInst,"IDD_SAVEGAME",hwnd,(DLGPROC)DialogFuncSavegame))
		{
		// if the user gives his ok, replace: 

		// set reindex flag
		reindex = 1;

		// read database into memory */
		dbstring = (char *) malloc(filesize);
		if(dbstring == NULL)
			{
			sprintf(str,"malloc error");
			return 0;
			}	

		fp = fopen(databasename,"r");

		if(fp == NULL)
			{
			sprintf(str,"invalid filename");
			free(dbstring);
			return 0;
			}							

		bytesread = fread(dbstring,1,filesize,fp);
		dbstring[bytesread] = 0;
		fclose(fp);

		// allocate gamestring for pdnstring 
		gamestring = (char *) malloc(GAMEBUFSIZE);
		if(gamestring == NULL) 
			{
			sprintf(str,"malloc error");
			free(dbstring);
			return 0;
			}	

		// rewrite file 
		fp = fopen(databasename,"w");

		// get all games up to gameindex and write them into file 
		p=dbstring;
		for(i=0; i<replaceindex; i++)
			{
			PDNparseGetnextgame(&p,gamestring);
			fprintf(fp,"%s",gamestring);
			}

		// skip current game 
		PDNparseGetnextgame(&p,gamestring);

		// write replaced game 
		PDNgametoPDNstring(&GPDNgame,gamestring,"\n");
		if(gameindex!=0)
			fprintf(fp,"\n\n\n\n%s",gamestring);
		else
			fprintf(fp,"%s",gamestring);

		// and read the rest of the file 
		while(PDNparseGetnextgame(&p,gamestring))
			fprintf(fp,"%s",gamestring);

		fclose(fp);
		if(gamestring != NULL)
			free(gamestring);
		if(dbstring != NULL)
			free(dbstring);
		return 1;
		}
	return 0;
	}


int loadnextgame(void)
	{
	// load the next game of the last search.
	char *dbstring;
	int i;

	for(i=0;i<MAXGAMES;i++)
		if(gameindex == gamelist[i])
			break;

	if(gameindex != gamelist[i])
		{
		sprintf(str,"error while looking for next game...");
		return 0;
		}

	if(i+1 >= gamenumber)
		{
		sprintf(str,"reached last game in list");
		return 0;
		}

	gameindex = gamelist[i+1];

	// ok, if we arrive here, we have a valid game index for the game to load.
	sprintf(str,"should load game %i",gameindex);
	// load the database into memory
	dbstring = loadPDNdbstring(database);
	// extract game from database
	loadgamefromPDNstring(gameindex, dbstring);
	// free up database memory
	free(dbstring);
	sprintf(str,"loaded game %i of %i", i+2, gamenumber);

	// return the number of the game we loaded
	return i+2;
	}

int loadpreviousgame(void)
	{
	// load the previous game of the last search.
	char *dbstring;
	int i;

	for(i=0;i<MAXGAMES;i++)
		if(gameindex == gamelist[i])
			break;

	if(gameindex != gamelist[i])
		{
		sprintf(str,"error while looking for next game...");
		return 0;
		}

	if(i == 0)
		{
		sprintf(str,"reached first game in list");
		return 0;
		}

	gameindex = gamelist[i-1];

	sprintf(str,"should load game %i",gameindex);
	dbstring = loadPDNdbstring(database);
	loadgamefromPDNstring(gameindex, dbstring);
	free(dbstring);
	sprintf(str,"loaded game %i of %i", i, gamenumber);

	return 0;
	}

char *loadPDNdbstring(char *dbname)
	{
	// attempts to load the file <dbname> into the
	// string dbstring - checks for existence of that
	// file, allocates enough memory for the file, and loads it.
	FILE *fp;
	int filesize;
	int bytesread;
	char *dbstring;

	filesize = getfilesize(dbname);

	dbstring = (char *) malloc(filesize);
	if(dbstring == NULL )
		{
		MessageBox(hwnd,"not enough memory for this operation","Error",MB_OK);
		SetCurrentDirectory(CBdirectory);
		return 0;
		}
	fp = fopen(dbname,"r");
	if(fp == NULL)
		{
		MessageBox(hwnd,"not a valid database!\nuse game->select database\nto select a valid database","Error",MB_OK);
		SetCurrentDirectory(CBdirectory);
		return 0;		
		}

	bytesread = fread(dbstring,1,filesize,fp);
	dbstring[bytesread] = 0;
	fclose(fp);

	return dbstring;
	}

int selectgame(int how)
// lets the user select a game from a PDN database in a dialog box.
// how describes which games are displayed: 
//		GAMELOAD:		all games 
//		SEARCHMASK:		only games of a specific player/event/date/with comment
//						new: also with optional "searchwithposition" enabled!
//		GAMEFIND:		only games with the current board position
//		GAMEFINDCR:		only games with current board position color-reversed
//		FINDTHEME:		only games with the current board position as "theme"
//		LASTSEARCH:		re-display results of the last search

// i should rewrite this code - it was originally designed for a simple search,
// now, CB has lots of different search options and the code is a bit unreadable!

	{
	int i, j;
	static int oldgameindex;
	int entry;
	char *dbstring = NULL;
	char *gamestring = NULL;
	char *p, *start, *tag;
	char header[256];
	char headername[256],headervalue[256];
	char token[1024];
	int searchhit;

	// stop engine
	PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);

	if(how == RE_SEARCH)
		{
		// the easiest: re-display the result of the last search.
		// only possible if there is a last search!
		if(re_search_ok == 0)
			{
			sprintf(str,"no old search to re-search!");
			return 0;
			}

		sprintf(str,"re-searching! gamenumber is %i", gamenumber);
		// load database into dbstring:
		dbstring = loadPDNdbstring(database);

		gamestring = (char *) malloc(GAMEBUFSIZE);
		if(gamestring == NULL)
			{
			MessageBox(hwnd,"not enough memory for this operation","Error",MB_OK);
			SetCurrentDirectory(CBdirectory);
			return 0;			
			}
		}

	else
		{
		r.win = 0;
		r.draw = 0;
		r.loss = 0;
		for(i=0;i<MAXGAMES;i++)
			gamelist[i] = 0;

		// if we're looking for a player name, get it
		if(how == SEARCHMASK)
			{
			// this dialog box sets the variables 
			// <playername>, <eventname> and <datename>
			if(DialogBox(g_hInst,"IDD_SEARCHMASK",hwnd,(DLGPROC)DialogSearchMask) == 0)
				return 0;
			}


		// set directory to games directory
		SetCurrentDirectory(gCBoptions.userdirectory);
		// get a valid database filename. if we already have one, we reuse it,
		// else we prompt the user to select a PDN database
		if(strcmp(database,"")==0)
			// no valid database name
			{ // display a dialog box with the available databases
			sprintf(database,"%s",gCBoptions.userdirectory);
			result = getfilename(database,OF_LOADGAME); // 1 on ok, 0 on cancel
			if(!result)
				sprintf(database,"");
			}
		else
			{
			sprintf(str,"database is '%s'",database);
			result = 1;
			}

		if(strcmp(database,"")!=0 && result)
			{
			sprintf(str,"loading...");
			// get number of games
			i=PDNparseGetnumberofgames(database);
			sprintf(str,"%i games in database",i);


			if(how == GAMEFIND || how == GAMEFINDTHEME || how == GAMEFINDCR ||
				(how == SEARCHMASK && searchwithposition == 1))
				// search for a position: this is done by calling pdnopen to index
				// the pdn file, pdnfind to return a list of games with the current position
				{
				// reset pdn find module
				if(reindex)
					{
					pdnfindreset();
					sprintf(str,"indexing database...");
					//if(how == SEARCHMASK && searchwithposition == 1)
					//	sprintf(str,"searching with position");
					SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) str);
					// index database with pdnopen
					pdnopen(database, GPDNgame.gametype);
					reindex = 0;
					}

				// search for games with current position
				// transform the current position into a bitboard:
				boardtobitboard(board8, &currentposition);
				if(how == GAMEFINDCR)
					boardtocrbitboard(board8, &currentposition);

				sprintf(str,"searching database...");
				SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) str);
				if(how == GAMEFIND || how == SEARCHMASK)
					gamenumber = pdnfind(&currentposition, color, gamelist, &r);
				if(how == GAMEFINDCR)
					gamenumber = pdnfind(&currentposition, CB_CHANGECOLOR(color), gamelist, &r);
				if(how == GAMEFINDTHEME)
					gamenumber = pdnfindtheme(&currentposition, gamelist);

				// gamelist now contains a list of games with the current position
				if(gamenumber == 0)
					{
					sprintf(str,"no games with the current position found");
					re_search_ok = 0;
					SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM)0, (LPARAM)str);
					return 0;
					}
				else
					{
					sprintf(str,"%i games with the current position found",gamenumber);
					re_search_ok = 1;
					}
				}
			
			SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM)0, (LPARAM)str);

			// read database file into buffer 'dbstring' 
			dbstring = loadPDNdbstring(database);

			// fill the struct 'data' with the PDN headers 
			gamestring = (char *) malloc(GAMEBUFSIZE);
			if(gamestring == NULL)
				{
				MessageBox(hwnd,"not enough memory for this operation","Error",MB_OK);
				SetCurrentDirectory(CBdirectory);
				return 0;			
				}
			p=dbstring;
			entry=0;
			i=0;
			if(how == GAMEFIND || how == GAMEFINDTHEME || how == GAMEFINDCR)
				i=-1;

			while(PDNparseGetnextgame(&p,gamestring) && entry<MAXGAMES && i<MAXGAMES)
				{
				if(how == GAMEFIND || how == GAMEFINDTHEME || how == GAMEFINDCR)
					{
					// we already know what should go in the list, no point parsing
					i++;
					if(i != gamelist[entry])
						continue;
					}

				// get headers and moves for this game
				sprintf(data[entry].white,"");
				sprintf(data[entry].black,"");
				sprintf(data[entry].event,"");
				sprintf(data[entry].result,"");
				sprintf(data[entry].date,"");


				start=gamestring;

				// parse headers
				while(PDNparseGetnextheader(&start,header))
					{
					tag=header;
					PDNparseGetnexttoken(&tag,headername);
					PDNparseGetnexttag(&tag,headervalue);
					if(strcmp(headername,"Event")==0)
						sprintf(data[entry].event,"%s",headervalue);
					if(strcmp(headername,"White")==0)
						sprintf(data[entry].white,"%s",headervalue);
					if(strcmp(headername,"Black")==0)
						sprintf(data[entry].black,"%s",headervalue);
					if(strcmp(headername,"Result")==0)
						sprintf(data[entry].result,"%s",headervalue);
					if(strcmp(headername,"Date")==0)
						sprintf(data[entry].date,"%s",headervalue);
					}
				// headers parsed
				// add the first few moves to the data structure to display them
				// when the user selects a game.
				//sprintf(data[entry].white,"game %i",i);

				sprintf(data[entry].PDN,"");
				for(j=0;j<48;j++)
					{
					if(!PDNparseGetnextPDNtoken(&start, token))
						break;
					if(strlen(data[entry].PDN) + strlen(token) < 255)
						{
						strcat(data[entry].PDN,token);
						strcat(data[entry].PDN," ");
						}
					else
						break;
					}

				// now, depending on what we are doing, we add this game to the list of
				// games to display
				// remember: entry is our running variable, from 0...numberofgames in db
				// i is 
				switch(how)
					{
					case GAMEFIND:
						entry++;
						break;
					case GAMEFINDCR:
						entry++;
						break;
					case GAMEFINDTHEME:
						entry++;
						break;
					case GAMELOAD:
						//	remember what game number this has
						gamelist[entry] = entry;
						// increment entry number
						entry++;
						break;
					case SEARCHMASK:
						// add the entry to the list
						// only if the name matches one of the players
						searchhit = 1;
						if(searchwithposition)
							{
							searchhit = 0;
							for(j=0;j<gamenumber;j++)
								{
								if(i == gamelist[j])
									searchhit = 1;
								}
							}

						// if a player name to search is set, search for that name
						if(strcmp(playername,"") != 0)
							{
							if (strstr(data[entry].black,playername) || 
								strstr(data[entry].white,playername))
								searchhit &= 1;
							else
								searchhit = 0;
							}

						// if an event name to search is set, search for that event
						if(strcmp(eventname,"") != 0)
							{
							if(strstr(data[entry].event, eventname))
								searchhit &=1;
							else
								searchhit = 0;
							}

						// if a date to search is set, search for that date
						if(strcmp(datename,"") != 0)
							{
							if(strstr(data[entry].date, datename))
								searchhit &= 1;
							else
								searchhit = 0;
							}

						// if a comment is defined, search for that comment
						if(strcmp(commentname,"") != 0)
							{
							if(strstr(gamestring, commentname))
								searchhit &= 1;
							else
								searchhit = 0;
							}

						if(searchhit == 1)
							{
							// remember what entry in the list corresponds 
							// to which game
							gamelist[entry] = i;
							entry++;
							}
						i++;
						break;
					}
				}
			sprintf(str,"%i games found",entry);

			if(how == SEARCHMASK && searchwithposition)
				{
				// the result is wrong, because it comes from pdnfind without 
				// filtering for names!
				r.draw = 0;
				r.win = 0;
				r.loss = 0;
				for(j=0;j<entry;j++)
					{
					if(strcmp(data[j].result,"1-0") == 0)
						r.win++;
					if(strcmp(data[j].result,"1/2-1/2") == 0)
						r.draw++;
					if(strcmp(data[j].result,"0-1") == 0)
						r.loss++;
					}
				}

			// total number of games is saved in <gamenumber>
			gamenumber = entry; 

			// save old game index
			oldgameindex = gameindex;

			// default game index 
			gameindex = 0;  
			}
		}			

	// headers loaded into 'data', display load game dialog 
	if (gamenumber) {
		if(DialogBox(g_hInst,"IDD_SELECTGAME",hwnd,(DLGPROC)DialogFuncSelectgame))
			{
			// a game was selected; with index <gameindex> in the dialog box 
			sprintf(str,"gameindex is %i",gameindex);
			// transform dialog box index to game index in database
			gameindex = gamelist[gameindex];
			// load game with index 'gameindex' 
			loadgamefromPDNstring(gameindex, dbstring);
			}
		else
			{
			// dialog box was cancelled 
			gameindex = oldgameindex;
			}
	}

	// free up memory
	if(gamestring != NULL)
		free(gamestring);
	if(dbstring != NULL)
		free(dbstring);

	SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM)0, (LPARAM)str);
	return 1;
	}


int loadgamefromPDNstring(int gameindex, char *dbstring)
	{
	char *p;
	int i;	
	char *gamestring = (char *) malloc(GAMEBUFSIZE);

	p = dbstring;
	i = gameindex+1;
	while (i)
		{
		if(!PDNparseGetnextgame(&p,gamestring))
			break;
		i--;
		}
	// now the game is in gamestring. use pdnparser routines to convert
	//	it into a GPDNgame
	doload(&GPDNgame, gamestring, &color, board8);
	free(gamestring);

	// game is fully loaded, clean up 
	updateboardgraphics(hwnd);
	reset = 1;
	newposition = TRUE;
	sprintf(str,"game loaded");
	SetCurrentDirectory(CBdirectory);
	//  only display info if not in analyzepdnmode
	if(CBstate != ANALYZEPDN)
		PostMessage(hwnd,WM_COMMAND,GAMEINFO,0);
	return 1;
	}

int handletooltiprequest(LPTOOLTIPTEXT TTtext)
	{
	if(TTtext->hdr.code != TTN_NEEDTEXT)
		return 0;

	switch(TTtext->hdr.idFrom)
		{
		// set tooltips 
		case TOGGLEBOOK: TTtext->lpszText="Change engine book setting";
			break;
		case TOGGLEMODE: TTtext->lpszText="Switch from normal to 2-player mode and back";
			break;
		case TOGGLEENGINE: TTtext->lpszText="Switch from primary to secondary engine and back";
			break;
		case GAMEFIND: TTtext->lpszText = "Find Game";
			break;

		case GAMENEW: TTtext->lpszText="New Game";
			break;
		case MOVESBACK: TTtext->lpszText="Take Back";
			break;
		case MOVESFORWARD: TTtext->lpszText="Forward";
			break;
		case MOVESPLAY: TTtext->lpszText="Play";
			break;
		case HELPHELP: TTtext->lpszText="Help";
			break;
		case GAMELOAD: TTtext->lpszText="Load Game";
			break;
		case GAMESAVE: TTtext->lpszText="Save Game";
			break;
		case MOVESBACKALL: TTtext->lpszText="Go to the start of the game";
			break;
		case MOVESFORWARDALL: TTtext->lpszText="Go to the end of the game";
			break;
		case DISPLAYINVERT: TTtext->lpszText="Invert Board";
			break;
		case SETUPCC:
			if(color==CB_BLACK)
				TTtext->lpszText="Red to move";
			else
				TTtext->lpszText="White to move";
			break;
		case BOOKMODE_VIEW: TTtext->lpszText="View User Book";
			break;
		case BOOKMODE_ADD: TTtext->lpszText="Add Moves to User Book";
			break;
		case BOOKMODE_DELETE: TTtext->lpszText="Delete Position from User Book";
			break;

		case HELPHOMEPAGE: TTtext->lpszText = "CheckerBoard Homepage";
			break;
		}
	return 1;
	}

void add_piecesets_to_menu(HMENU hmenu)
	{
	// this is a bit ugly; hard-coded that piece sets appear at
	// a certain position in the menu. 
	HMENU hpop, hsubpop;
	HANDLE hfind;
	WIN32_FIND_DATA FileData;
	int i=0;

	hpop = GetSubMenu(hmenu,4);
	hsubpop = GetSubMenu(hpop,3);

	// first, we need to find the subdirectories in the bmp dir

	SetCurrentDirectory(CBdirectory);
	SetCurrentDirectory("bmp");

	FileData.dwFileAttributes = 0;

	hfind = FindFirstFile("*", &FileData);

	DeleteMenu(hsubpop,0,MF_BYPOSITION);
	while(i<MAXPIECESET)
		{
		if(hfind == INVALID_HANDLE_VALUE)
			return;


		if(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
			if(strlen(FileData.cFileName)>3 && FileData.cFileName[0] != '.')	/* Exclude ".svn" directory. */
				{
				AppendMenu(hsubpop,MF_STRING,PIECESET+i,FileData.cFileName);
				sprintf(piecesetname[i],"%s",FileData.cFileName);
				i++;
				maxpieceset = 1;
				}
			}

		if(FindNextFile(hfind, &FileData) == 0)
			return;
		}
	}

int createcheckerboard(HWND hwnd)
	{
	FILE *fp;
	COLORREF dCustomColors[16];
	char windowtitle[256], str2[256];
	RECT rect;

	/* To support running multiple instances of CB, use suffixes in logfilenames. */
	get_app_instance(szWinName, &g_app_instance);
	if (g_app_instance > 0)
		sprintf(g_app_instance_suffix, "[%d]", g_app_instance);

	// load settings from registry: &gCBoptions is one key, CBdirectory another.
	loadsettings(&gCBoptions, CBdirectory);
	SetMenuLanguage(gCBoptions.language);

	// set appropriate pieceset - load bitmaps for the board
	SendMessage(hwnd,WM_COMMAND,PIECESET+gCBoptions.piecesetindex,0);

	// resize window to last window size:
	if(gCBoptions.window_x !=0) // check if we have stored something
		MoveWindow(hwnd,gCBoptions.window_x,gCBoptions.window_y, gCBoptions.window_width, gCBoptions.window_height,1);
	GetClientRect(hwnd, &rect);
	PostMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rect.right-rect.left, rect.bottom-rect.top));

	initgraphics(hwnd);
	
	//initialize linked list which stores the game
	initlinkedlist();

	// initialize colors 
	ccs.lStructSize=(DWORD) sizeof(CHOOSECOLOR);
	ccs.hwndOwner=(HWND) hwnd;
	ccs.hInstance=(HWND) NULL;
	ccs.rgbResult=RGB(255,0,0);
	ccs.lpCustColors=dCustomColors;
	ccs.Flags=CC_RGBINIT|CC_FULLOPEN;
	ccs.lCustData=0L;
	ccs.lpfnHook=NULL;
	ccs.lpTemplateName=(LPSTR) NULL;

	// load user book
	strcpy(userbookname, CBdocuments);
	PathAppend(userbookname, "userbook.bin");
	fp = fopen(userbookname,"rb");
	if(fp != 0)
		{
		userbooknum = fread(userbook,  sizeof(struct userbookentry),MAXUSERBOOK,fp);
		fclose(fp);
		}

	setmenuchecks(&gCBoptions, hmenu);
	// set the level 
	// which has been retrieved by loadsettings 
	switch(gCBoptions.level)
		{
		case 1:
			PostMessage(hwnd,WM_COMMAND,LEVELINSTANT,0);
			break;
		case 2:
			PostMessage(hwnd, WM_COMMAND, LEVEL01S, 0);
			break;
		case 3:
			PostMessage(hwnd, WM_COMMAND, LEVEL02S, 0);
			break;
		case 4:
			PostMessage(hwnd, WM_COMMAND, LEVEL05S, 0);
			break;
		case 5:
			PostMessage(hwnd,WM_COMMAND,LEVEL1S,0);
			break;
		case 6:
			PostMessage(hwnd,WM_COMMAND,LEVEL2S,0);
			break;
		case 7:
			PostMessage(hwnd,WM_COMMAND,LEVEL5S,0);
			break;
		case 8: 
			PostMessage(hwnd,WM_COMMAND,LEVEL10S,0);
			break;
		case 9:
			PostMessage(hwnd,WM_COMMAND,LEVEL15S,0);
			break;
		case 10:
			PostMessage(hwnd,WM_COMMAND,LEVEL30S,0);
			break;
		case 11:
			PostMessage(hwnd,WM_COMMAND,LEVEL1M,0);
			break;
		case 12:
			PostMessage(hwnd,WM_COMMAND,LEVEL2M,0);
			break;
		case 13:
			PostMessage(hwnd,WM_COMMAND,LEVEL5M,0);
			break;
		case 14:
			PostMessage(hwnd,WM_COMMAND,LEVEL15M,0);
			break;
		case 15:
			PostMessage(hwnd,WM_COMMAND,LEVEL30M,0);
			break;
		case 16:
			PostMessage(hwnd,WM_COMMAND,LEVELINFINITE,0);
			break;
		case 17:
			PostMessage(hwnd,WM_COMMAND,LEVELINCREMENT,0);
			break;

		default:
			PostMessage(hwnd,WM_COMMAND,LEVEL1S,0);
		}

	// in case of shell double click 
	if(strcmp(filename,"")!=0)
		{
		sprintf(database,"%s",filename);
		PostMessage(hwnd,WM_COMMAND,GAMELOAD,0);
		}

	// support drag and drop
	DragAcceptFiles(hwnd,1);

	// start autothread 
	hAutoThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AutoThreadFunc,(HWND) 0,0,&AutoThreadId);
	if(hAutoThread==0)
		CBlog("failed to start auto thread");

	// and issue a newgame command 
	PostMessage(hwnd,WM_COMMAND,GAMENEW,0);

	// display the engine name in the window title 
	sprintf(windowtitle,"loading engine - please wait...");
	sprintf(str2,"CheckerBoard%s: %s",windowtitle,g_app_instance_suffix);
	SetWindowText(hwnd,str2);

	// initialize the board which is stored in board8 
	InitCheckerBoard(board8);

	// print version number in status bar
	sprintf(str, "This is CheckerBoard %s, %s", VERSION, PLACE);
	return 1;
	}


int showfile(char *filename)
	{
	// opens a file with the default viewer, e.g. a html help file
	int error;
	HINSTANCE hinst;


	hinst = ShellExecute(NULL,"open",filename,NULL,NULL,SW_SHOW);
	error = PtrToLong(hinst);

	sprintf(str,"CheckerBoard could not open\nthe file %s\nError code %i",filename, error);
	if(error<32)
		{
		if(error==ERROR_FILE_NOT_FOUND) strcat(str,": File not found");
		if(error==SE_ERR_NOASSOC) strcat(str,": no .htm viewer configured");
		MessageBox(hwnd,str,"Error !",MB_OK);
		}
	else sprintf(str,"opened %s", filename);
	return 1;
	}

int start11man(int number)
	{
	// start a new 11-move game:
	// read FEN for this 11 man from file
	// returns 1 if a game with gamenumber could be started, 0 if there is no FEN left in the file.
	int i=0;
	FILE *fp; 
	char str[256];

	// set directory to CB directory
	SetCurrentDirectory(CBdirectory);

	// don't play entire match as it takes way too much time!
	//if(number >= 500)
	//	return 0;

	fp = fopen("11man_FEN.txt","r");
	if(fp == NULL)
		return 0;

	while (!feof(fp) && i<=number)
		{
		// read a line
		fgets(str, 255, fp);
		i++;
		}

	if(feof(fp))
		{
		fclose(fp);
		return 0;
		}

	fclose(fp);

	// now we have the right FEN in str
	FENtoboard8(board8, str, &color, GT_ENGLISH);

	// set linked list 
	initlinkedlist();
	GPDNgame.head = head;
	board8toFEN(board8, str, color, GT_ENGLISH);
	sprintf(GPDNgame.FEN,"%s",str);
	sprintf(GPDNgame.event,"11-man #%i",number+1);
	sprintf(GPDNgame.setup,"1");

	updateboardgraphics(hwnd);
	InvalidateRect(hwnd,NULL,0);
	sprintf(str,"11 man opening number %i",number+1);
	newposition = TRUE;
	reset = 1;

	return 1;
	}


/*
 * Translate the current game into pdn of its colors-reversed mirror.
 */
void game_to_colors_reversed_pdn(char *pdn)
{
	listentry *p;
	int from, to;

	pdn[0] = 0;
	for (p = head; p->next != NULL; p = p->next) {
		PDNparseTokentonumbers(p->PDN, &from, &to);
		sprintf(pdn + strlen(pdn), "%d-%d ", 33 - from, 33 - to);
	}
}


/*
 * Move to the end of the current game.
 */
void forward_to_game_end(void)
{
	while (current->next != NULL) {
		domove(current->move, board8);
		color = CB_CHANGECOLOR(color);
		current = current->next;
	}
}

int start3move(void)
	{
	// start a new 3-move game: 
	// this function executes the 3 first moves of 3moveopening #(op), op
	// is a global which is set by random if the user chooses
	// 3-move, or it can be set controlled by engine match 

	extern int three[174][4]; // describes 3-move-openings

	InitCheckerBoard(board8);
	InvalidateRect(hwnd,NULL,0);
	color=CB_BLACK;
	// set linked list 
	initlinkedlist();
	GPDNgame.head = head;

	getmovelist(1, m,board8,&dummy);
	domove(m[three[op][0]],board8);
	appendmovetolist(m[three[op][0]]);

	color = CB_CHANGECOLOR(color);
	getmovelist(-1, m,board8,&dummy);
	domove(m[three[op][1]],board8);
	appendmovetolist(m[three[op][1]]);

	color = CB_CHANGECOLOR(color);
	getmovelist(1, m,board8,&dummy);
	domove(m[three[op][2]],board8);
	appendmovetolist(m[three[op][2]]);

	color = CB_CHANGECOLOR(color);

	if (gametype() == GT_ITALIAN) {
		char pdn[80];

		game_to_colors_reversed_pdn(pdn);
		doload(&GPDNgame, pdn, &color, board8);
		forward_to_game_end();
	}

	updateboardgraphics(hwnd);	
	sprintf(str,"ACF opening number %i",op+1);
	newposition=TRUE;

	// new march 2005, jon kreuzer told me this was missing.
	reset = 1;

	return 1;
	}	


DWORD ThreadFunc(LPVOID param)
// Threadfunc calls the checkers engine to find a move
// it also logs the return string of the checkers engine
// to a file if CB is in either ANALYZEGAME or ENGINEMATCH mode 
	{
	size_t i, n;
	int original8board[8][8],b8copy[8][8],originalcopy[8][8];
	struct CBmove m[MAXMOVES];

	char PDN[40];
	int found=0;
	int c;
	int dummy;
	FILE *Lfp;
	char Lstr[1024];
	struct CBmove LCBmove;
	struct pos userbookpos;
	int founduserbookmove = 0;

	starttime=clock();
	abortcalculation = 0;		// if this remains 0, we will execute the move - else not


	// test if there is a move at all: if not, return and set state to NORMAL
	if(color==CB_BLACK) c=1;
	else
		c=-1;
	n = getmovelist(c, m,board8,&dummy);
	if(n==0)
		{
		sprintf(str,"there is no move in this position");
		// if this happens in autoplay or in an enginematch, set mode back to normal 
		if (CBstate == AUTOPLAY) 
			{gameover=TRUE;sprintf(str,"game over");}
		if (CBstate==ENGINEMATCH || CBstate==ENGINEGAME) 
			{gameover=TRUE;sprintf(str,"game over");}
		//setanimationbusy(FALSE); // is this necessary??
		setenginebusy(FALSE);
		return 1;
		}

	// check if this position is in the userbook
	if(gCBoptions.userbook)
		{
		boardtobitboard(board8,&userbookpos);
		for(i=0;i<userbooknum;i++)
			{
			if(userbookpos.bm == userbook[i].position.bm &&
				userbookpos.bk == userbook[i].position.bk &&
				userbookpos.wm == userbook[i].position.wm &&
				userbookpos.wk == userbook[i].position.wk)
				{
				// we have this position in the userbook!
				move = userbook[i].move;
				founduserbookmove = 1;
				found = 1;
				sprintf(str,"found move in user book");
				}
			}
		}


	if(!founduserbookmove)
		// we did not find a move in our user book, so continue
		{
		//board8 is a global [8][8] int which holds the board
		//get 3 copies of the global board8
		memcpy(b8copy,board8,64*sizeof(int));
		memcpy(original8board,board8,64*sizeof(int));
		memcpy(originalcopy,board8,64*sizeof(int));

		// set thread priority 
		// next lower ist '_LOWEST', higher '_NORMAL' 
		enginethreadpriority = usersetpriority;
		SetThreadPriority(hThread,enginethreadpriority);  

		// set directory to CB directory
		SetCurrentDirectory(CBdirectory);

		//--------------------------------------------------------------//
		//						do a search								//
		//--------------------------------------------------------------//
		if(getmove != NULL)
			{
			/* Display the Play! bitmap with red foreground when the engine is searching. */
			PostMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)MOVESPLAY, MAKELPARAM(19, 0));

			// if in engine match handicap mode, give primary engine half the time of secondary engine.
			if(CBstate == ENGINEMATCH && handicap && currentengine == 1)
				result=(getmove)(originalcopy,color,maxtime/2,str,&playnow,reset+2*gCBoptions.exact+4*increment,0,&LCBmove);
			else
				result=(getmove)(originalcopy,color,maxtime,str,&playnow,reset+2*gCBoptions.exact+4*increment,0,&LCBmove);

			/* Display the Play! bitmap with black foreground when the engine is not searching. */
			PostMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)MOVESPLAY, MAKELPARAM(2, 0));

			if(increment)
				{
				maxtime += incrementtime;
				maxtime -= (clock()-starttime)/(double)CLK_TCK;
				sprintf(Lstr," time remaining:%.1fs  ",maxtime);
				strcat(Lstr,str);
				sprintf(str,"%s",Lstr);
				}
			}
		else
			sprintf(str,"error: no engine defined!");
		// reset playnow immediately 
		playnow=0;

		// save engine string as comment if it's an engine match 
		// actually, always save if add comment is on
		if(addcomment)
			{
			if(current != NULL)
				{
				if(strlen(str)<COMMENTLENGTH)
					sprintf(current->comment,"%s",str); //%255 to make sure it fits in the string
				else
					strncpy(current->comment,str,COMMENTLENGTH-2);
				}
			}
		// now, we execute the move on the board, but only if we are not in observe or analyze mode 
		// in observemode, the user will provide all moves, in analyse mode the autothread drives the
		// game forward 
		if(CBstate!=OBSERVEGAME && CBstate!=ANALYZEGAME && CBstate!=ANALYZEPDN &&
			!abortcalculation)
			memcpy(board8,originalcopy,64*sizeof(int));

		// if we are in engine match mode and one of the engines claims a win
		// or a loss or a draw we stop 
		if(result!=CB_UNKNOWN && (CBstate==ENGINEMATCH))
			{
			sprintf(current->comment,"%s : gameover claimed",str);
			gameover=TRUE;
			}
		//got board8 & a copy before move was made

		if(CBstate!=OBSERVEGAME && CBstate!=ANALYZEGAME && CBstate!=ANALYZEPDN && !abortcalculation)
			{
			// determine the move that was made: we only do this if gametype is 21,
			//	 else the engine must return the appropriate information in LCBmove
			if(gametype()==GT_ENGLISH) /* should be 'if gametype==21' */
				{
				if(color==CB_BLACK)
					n=getmovelist(1,m,b8copy,&dummy);
				else
					n=getmovelist(-1,m,b8copy,&dummy);
				move=m[0];
				for(i=0;i<n;i++)
					{//put original board8 in b8copy, execute move and compare with returned board8...
					memcpy(b8copy,original8board,64*sizeof(int));
					domove(m[i],b8copy);
					if(memcmp(board8,b8copy,64*sizeof(int))==0)
						{
						move4tonotation(m[i],PDN);
						move=m[i];
						found=1;
						break;
						}
					}

				if(found == 0)
					memcpy(board8,original8board,64*sizeof(int));
				}
			else // gametype not 21, not regular checkers, use the move of the engine 
				{
				move = LCBmove;
				memcpy(board8,original8board,64*sizeof(int));
				found = 1;
				}
			}
		// ************* finished determining what move was made....
		}
	// now we execute the move, but only if we are not in the mode
	// ANALYZEGAME or OBSERVEGAME 
	if((CBstate!=OBSERVEGAME) && (CBstate!=ANALYZEGAME) && (CBstate!=ANALYZEPDN) && (found) && !abortcalculation)
		{
		appendmovetolist(move);
		// if sound is on we make a beep 
		if(gCBoptions.sound) 
			Beep(440,5);
		CloseHandle(hAniThread);
		setanimationbusy(TRUE);  // this was missing in CB 1.65 which was the reason for the bug...
		hAniThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AnimationThreadFunc,(HWND) hwnd,0,&g_AniThreadId);
		}
	// if CBstate is ANALYZEGAME, we have to print the analysis to a logfile,
	// make the move played in the game & also print it into the logfile 

	sprintf(current->analysis,"");

	switch (CBstate)
		{
		case ANALYZEPDN: // drop through to analyzegame

		case ANALYZEGAME:
			// don't add analysis if there is only one move
			if(n==1)
				break;
			sprintf(current->analysis, "%s", str);
			break;

		case ENGINEMATCH:
			{
				filename[MAX_PATH];

				sprintf(filename, "%s\\matchlog%s.txt", gCBoptions.matchdirectory, g_app_instance_suffix);
				Lfp=fopen(filename,"a");
				enginename(Lstr);
				if(Lfp != NULL)
					{
					fprintf(Lfp,"\n%s played %s",Lstr,PDN);
					fprintf(Lfp,"\nanalysis: %s",str);
					fclose(Lfp);
					}
			}
			break;
		}

	reset = 0;
	setenginebusy(FALSE);
	setenginestarting(FALSE);
	return 1;
	}

int changeCBstate(int oldstate, int newstate)
	{
	// changes the state of Checkerboard from old state to new state
	// does whatever is necessary to do this - checks/unchecks menu buttons
	// changes state of buttons in toolbar etc.

	// when we change the state, we first of all make sure that the
	// engine is not running
	PostMessage(hwnd,WM_COMMAND,ABORTENGINE,0);

	// toolbar buttons
	if(oldstate == BOOKVIEW)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM)BOOKMODE_VIEW,MAKELONG(0,0));
	if(oldstate == BOOKADD)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM)BOOKMODE_ADD,MAKELONG(0,0));

	CBstate = (enum state) newstate;
//	CBstate = newstate;

	// toolbar buttons
	if(CBstate == BOOKVIEW)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM)BOOKMODE_VIEW,MAKELONG(1,0));
	if(CBstate == BOOKADD)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM)BOOKMODE_ADD,MAKELONG(1,0));

	// mode menu checks are taken care of in autothread function
	// set menu 
	CheckMenuItem(hmenu,CM_NORMAL,MF_UNCHECKED);
	CheckMenuItem(hmenu,CM_ANALYSIS,MF_UNCHECKED);
	CheckMenuItem(hmenu,CM_AUTOPLAY,MF_UNCHECKED);
	CheckMenuItem(hmenu,CM_ENGINEMATCH,MF_UNCHECKED);
	CheckMenuItem(hmenu,CM_2PLAYER,MF_UNCHECKED);
	CheckMenuItem(hmenu,BOOKMODE_VIEW,MF_UNCHECKED);
	CheckMenuItem(hmenu,BOOKMODE_ADD,MF_UNCHECKED);

	/* Update animation state. */
	if (CBstate == ENGINEMATCH && maxtime <= 1)
		set_animation(false);
	else
		set_animation(true);

	switch (CBstate)
		{
		case NORMAL:
			CheckMenuItem(hmenu,CM_NORMAL,MF_CHECKED);
			break;
		case OBSERVEGAME: CheckMenuItem(hmenu,CM_ANALYSIS,MF_CHECKED);
			break;
		case AUTOPLAY: CheckMenuItem(hmenu,CM_AUTOPLAY,MF_CHECKED);
			break;
		case ENGINEMATCH: CheckMenuItem(hmenu,CM_ENGINEMATCH,MF_CHECKED);
			break;
		case ENTERGAME: CheckMenuItem(hmenu,CM_2PLAYER,MF_CHECKED);
			break;
		case BOOKVIEW: CheckMenuItem(hmenu,BOOKMODE_VIEW,MF_CHECKED);
			break;
		case BOOKADD: CheckMenuItem(hmenu,BOOKMODE_ADD,MF_CHECKED);
			break;
		}
	// clear status bar
	sprintf(str,"");
	return 1;
	}

DWORD AutoThreadFunc(LPVOID param)
	{
	//	this thread drives autoplay,analyze game and engine match.
	//	it looks in what state CB is currently and
	//  then does the appropriate thing and sets CBstate to the new state.
	//  CBstate only changes inside this function or on the menu commands
	//  'analyze game', 'engine match', 'autoplay' and 'play (->normal)'
	//  all automatic changes are in here!
	//
	//  it uses the booleans enginebusy and animationbusy to
	//  detect if it is allowed to do anything 

	char Lstr[256];
	char windowtitle[256];
	char analysisfilename[256];
	char testlogname[MAX_PATH];
	char testsetname[MAX_PATH];
	char statsfilename[MAX_PATH];
	static char matchlogstring[65536];	// large string which holds the output which we write to match_progress.txt
	static int oldengine;
	FILE *Lfp;
	static int gamenumber;
	static int movecount;
	int i;
	const int maxmovecount = 200;
	static int wins,draws,losses,unknowns;
	static int blackwins, blacklosses; //wins as black of primary engine
	static char FEN[256];
	char engine1[256], engine2[256];	// holds engine names
	int matchcontinues = 0;
	static int iselevenman = 0;

	// autothread is started at startup, and keeps running until the program
	// terminates, that's what for(;;) is here for. 

	for(;;)
		{
		// thread sleeps for AUTOSLEEPTIME (10ms) so that this loop runs at
		// approximately 100x per second
		Sleep(AUTOSLEEPTIME);

		// if CB is doing something else, wait for it to finish by dropping back to sleep command above
		if(getanimationbusy() || getenginebusy() || getenginestarting())
			continue;

		switch (CBstate)
			{
			case NORMAL:
				if(startengine)
					{
					/* after determining the user move startengine flag is set and
					the move is animated. */
					PostMessage(hwnd,(UINT)WM_COMMAND,(WPARAM)MOVESPLAY,(LPARAM)0);
					setenginestarting(TRUE);
					startengine = FALSE;
					}
				break;

			case RUNTESTSET:
				// sleep for 0.2 seconds to allow handletimer() to update the testlog file
				Sleep(200);
				// create or update testlog file
				strcpy(testlogname, CBdocuments);
				PathAppend(testlogname, "testlog.txt");
				if(testset_number == 0)
					{
					// testset start: clear file testlog.txt
					Lfp=fopen(testlogname,"w");
					fclose(Lfp);
					}
				else
					// write analysis
					logtofile(testlogname, "\n\n", "a");

				// load the next position from the test set
				strcpy(testsetname, CBdocuments);
				PathAppend(testsetname, "testset.txt");
				Lfp = fopen(testsetname, "r");
				if(Lfp == NULL)
					{
					sprintf(str,"could not find %s", testsetname);
					break;
					}

				for(i=0;i<=testset_number;i++)
					{
					fgets(FEN,255,Lfp);
					if(feof(Lfp))
						{
						changeCBstate(RUNTESTSET, NORMAL);
						}
					}
				fclose(Lfp);
				testset_number++;

				// write FEN in testlog
				sprintf(str,"#%i: %s", testset_number, FEN);
				logtofile(testlogname, str, "a");

				// convert position to internal board
				FENtoboard8(board8, FEN, &color,GPDNgame.gametype);
				updateboardgraphics(hwnd);
				reset = 1;
				newposition = TRUE;
				PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);
				setenginestarting(TRUE);
				Sleep(SLEEPTIME);
				break;

			case AUTOPLAY:
				// check if game is over, if yes, go from autoplay to normal state
				if(gameover == TRUE)
					{
					gameover = FALSE; 
					changeCBstate(CBstate,NORMAL);
					sprintf(str,"game over");
					}
				// else continue game by sending a play command
				else
					{
					if(getenginestarting() || getanimationbusy() || getenginebusy())
						break;
					PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
					setenginestarting(TRUE);
					// sleep a bit to allow engine to start
					Sleep(SLEEPTIME);
					}

				break;

			case ENGINEGAME:
				if(gameover==TRUE)
					{
					gameover=FALSE; 
					changeCBstate(CBstate,NORMAL);
					setcurrentengine(1);
					break;
					}

				if(startmatch == TRUE)
					startmatch = FALSE;
				else
					{
					currentengine^=3;
					setcurrentengine(currentengine);
					enginename(Lstr);
					sprintf(str,"CheckerBoard%s: ", g_app_instance_suffix);
					strcat(str,Lstr);
					SetWindowText(hwnd,str);
					}
				PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
				setenginestarting(TRUE);

				break;

			case ANALYZEGAME:
				if(gameover == TRUE)
					{
					gameover=FALSE;
					changeCBstate(CBstate,NORMAL);
					sprintf(str,"Game analysis finished!");
					strcpy(analysisfilename, CBdocuments);
					PathAppend(analysisfilename, "analysis");
					PathAppend(analysisfilename, "analysis.htm");
					makeanalysisfile(analysisfilename);
					break;
					}
				if(currentengine!=1) 
					setcurrentengine(1);
				PostMessage(hwnd,WM_COMMAND,MOVESFORWARDALL,0);

				Sleep(SLEEPTIME);

				// start analysis logfile - overwrite anything old 
				strcpy(analysisfilename, CBdocuments);
				PathAppend(analysisfilename, "analysis.txt");
				Lfp = fopen(analysisfilename,"w");
				fclose(Lfp);
				sprintf(str,"played in game: 1. %s",current->PDN);
				logtofile(analysisfilename, str, "a");

				PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
				setenginestarting(TRUE);

				// go into a for loop until the game is completely analyzed
				for(;;)
					{
					Sleep(SLEEPTIME);
					if((CBstate != ANALYZEGAME) || (gameover == TRUE)) 
						break;
					if(!getenginebusy() && !getanimationbusy())
						{
						PostMessage(hwnd,WM_COMMAND,MOVESBACK,0);
						if(CBstate==ANALYZEGAME && gameover==FALSE)
							{
							PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
							setenginestarting(TRUE);
							}
						}
					}
				break;

			case ANALYZEPDN:
				if(startmatch == TRUE)
					// this is the case when the user chooses analyzepdn in the menu;
					// at this point it is true for the first and only time in the
					// analyzepdn mode.
					{
					gamenumber = 1;
					startmatch = FALSE;
					strcpy(analysisfilename, CBdocuments);
					PathAppend(analysisfilename, "analysis");
					PathAppend(analysisfilename, "analysis1.htm");
					}

				if(gameover == TRUE)
					{
					gameover = FALSE;
					makeanalysisfile(analysisfilename);
					// get number of next game; loadnextgame returns 0 if 
					// there is no further game.
					gamenumber = loadnextgame(); 
					sprintf(analysisfilename, "%s\\analysis\\analysis%i.htm", CBdocuments, gamenumber);
					}

				if(gamenumber == 0) 
					// we're done with the file
					{
					changeCBstate(CBstate,NORMAL);
					sprintf(str,"PDN analysis finished!");
					break;
					}

				// this is the signal that we are at the start of the analysis of a game
				if(currentengine != 1) 
					setcurrentengine(1);
				PostMessage(hwnd,WM_COMMAND,MOVESFORWARDALL,0);
				PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
				setenginestarting(TRUE);
				// analyze entire game in this for loop
				for(;;)
					{
					Sleep(SLEEPTIME);
					if((CBstate != ANALYZEPDN) || (gameover == TRUE)) 
						break;
					if(!getenginebusy() && !getanimationbusy())
						{
						PostMessage(hwnd,WM_COMMAND,MOVESBACK,0);

						// give the main thread some time to stop analysis if
						//we are at the end of the game 
						if(CBstate==ANALYZEPDN && gameover==FALSE)
							{
							PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
							setenginestarting(TRUE);
							}
						}
					}
				break;

			case OBSERVEGAME:
				// select primary engine if this is not the case
				if(currentengine != 1)
					setcurrentengine(1);

				// start engine if we have a new position. 
				if(newposition)
					{
					playnow=0;
					PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
					setenginestarting(TRUE);
					newposition=0;
					}
				break;

			case ENGINEMATCH:
				if(startmatch)
					{
					// a new match has been started, do some initializations
					sprintf(matchlogstring,"");
					gamenumber=0;
					wins=0;
					losses=0;
					draws=0;
					unknowns=0;
					blackwins=0;
					blacklosses=0;
					// check to see if a stats.txt file is here, and if yes, continue the match 
					sprintf(statsfilename, "%s\\stats%s.txt", gCBoptions.matchdirectory, g_app_instance_suffix);
					Lfp = fopen(statsfilename,"r");
					if(Lfp != NULL)
						{
						// stats.txt exists 
						// read first line just to read second line, which holds the actual stats
						fgets(Lstr,255,Lfp);
						fgets(Lstr,255,Lfp);
						sscanf(Lstr,"+:%i =:%i -:%i unknown:%i +B:%i -B:%i",&wins,&draws,&losses,&unknowns,&blackwins,&blacklosses);
						gamenumber = wins+losses+draws+unknowns;
						sprintf(str,"resuming match at game #%i, (+:%i -:%i =:%i unknown:%i)",gamenumber,wins,losses,draws,unknowns);
						fclose(Lfp);

						// read match-progress file 	// TODO: this should be superfluous, write directly to file...
						sprintf(statsfilename, "%s\\match_progress%s.txt", gCBoptions.matchdirectory, g_app_instance_suffix);
						Lfp = fopen(statsfilename,"r");
						if(Lfp!=NULL)
							{
							while(!feof(Lfp))
								{
								fgets(Lstr,255,Lfp);
								strcat(matchlogstring,Lstr);
								}
							fclose(Lfp);
							}
						}

					// finally, display stats in window title
					enginecommand1("name", engine1);
					enginecommand2("name", engine2);
					sprintf(windowtitle,"%s - %s", engine1, engine2);
					sprintf(Lstr,": W-L-D:%i-%i-%i",wins,losses,draws+unknowns);
					strcat(windowtitle,Lstr);
					SetWindowText(hwnd,windowtitle);

					// ask user whether this is regular match or 11-man-match
					iselevenman = MessageBox(hwnd, "Play 3-move openings? Choose Yes for 3-move, No for 11-man", "Choose Match Type",MB_ICONQUESTION|MB_YESNO);
					if(iselevenman == IDYES)
						iselevenman = 0;
					else
						iselevenman = 1;
					}	// end if startmatch

				// stuff below is for regular games
				// stop games which have been going for too long 
				if(movecount > maxmovecount)
					gameover = TRUE;

				// when a game is terminated, save result and save game 
				if((gameover || startmatch==TRUE))
					{
					if(gameover)
						{
						// set white and black players 
						if(gamenumber % 2)
							{
							sprintf(GPDNgame.black,"%s",engine1);
							sprintf(GPDNgame.white,"%s",engine2);
							}
						else
							{
							sprintf(GPDNgame.black,"%s",engine2);
							sprintf(GPDNgame.white,"%s",engine1);
							}
						sprintf(GPDNgame.resultstring,"?");
						if(!((gamenumber-1) % 20))
							{
							if(gamenumber != 1)
								strcat(matchlogstring,"\n");
							}
						if(gamenumber % 2) 
							{
							if(iselevenman == 1)
								sprintf(Lstr,"%4i:",gamenumber/2+1);
							else
								sprintf(Lstr,"%3i:",op+1);
							strcat(matchlogstring,Lstr);
							}

						// check result
						dostats(result, movecount, gamenumber, &wins, &draws, &losses, &unknowns, &blackwins, &blacklosses, matchlogstring);

						// finally, display stats in window title
						sprintf(windowtitle,"%s - %s", engine1, engine2);
						sprintf(Lstr,": W-L-D:%i-%i-%i",wins,losses,draws+unknowns);
						strcat(windowtitle,Lstr);
						SetWindowText(hwnd,windowtitle);
						if(!(gamenumber%2)) 
							strcat(matchlogstring,"  ");

						// write match statistics 
						sprintf(statsfilename, "%s\\stats%s.txt", gCBoptions.matchdirectory, g_app_instance_suffix);
						Lfp = fopen(statsfilename,"w");
						if(Lfp != NULL)
							{
							fprintf(Lfp,"%s - %s",engine1, engine2);
							fprintf(Lfp," %s\n",Lstr);
							fprintf(Lfp,"+:%i =:%i -:%i unknown:%i +B:%i -B:%i",wins,draws,losses,unknowns,blackwins,blacklosses);
							fclose(Lfp);
							}

						// write match_progress.txt file
						sprintf(statsfilename, "%s\\match_progress%s.txt", gCBoptions.matchdirectory, g_app_instance_suffix);
						logtofile(statsfilename, matchlogstring, "w");

						// save the game
						if(iselevenman == 1)
							sprintf(GPDNgame.event,"11-man #%i",(gamenumber-1)/2+1);
						else
							sprintf(GPDNgame.event,"ACF #%i",op+1);

						// dosave expects a fully initialized GPDNgame structure
						sprintf(filename, "%s\\match%s.pdn", gCBoptions.matchdirectory, g_app_instance_suffix);
						SendMessage(hwnd,WM_COMMAND,DOSAVE,0);

						Sleep(SLEEPTIME);
						}

					// set startmatch to FALSE, it is only true when the match starts to initialize
					startmatch = FALSE;

					// get the opening for the gamenumber, and check whether the match is over
					if(iselevenman == 1)
						matchcontinues = start11man(gamenumber/2);	
					else
						{
						op = getthreeopening(gamenumber, &gCBoptions);
						if(op == -1)
							matchcontinues = 0;
						else
							matchcontinues = 1;
						}
					
					// move on to the next game 
					movecount = 0;
					gameover = FALSE;
					gamenumber++;

					sprintf(str,"gamenumber is %i\n", gamenumber);

					if(matchcontinues == 0)				
						{
						changeCBstate(CBstate,NORMAL);
						setcurrentengine(1);
						// write final result in window title bar
						sprintf(Lstr,"Final result of %s", windowtitle);
						SetWindowText(hwnd,Lstr);
						break;
						}

					// set color of engine to start playing
					if(gamenumber % 2)
						setcurrentengine(1);
					else
						setcurrentengine(2);
					// post message so that main thread handles the request
					if(CBstate!= NORMAL)
						{
						if(iselevenman == 1)
							PostMessage(hwnd,WM_COMMAND,START11MAN,0);
						else
							PostMessage(hwnd,WM_COMMAND,START3MOVE,0);
						// give main thread some time to handle this message
						Sleep(SLEEPTIME);
						}
					break;
					}

				if(!gameover && CBstate==ENGINEMATCH)
					{
					// make next move in game 
					movecount++;

					// set which engine  
					if((gamenumber + color) %2)
						setcurrentengine(1);
					else
						setcurrentengine(2);

					PostMessage(hwnd,WM_COMMAND,MOVESPLAY,0);
					setenginestarting(TRUE);
					// give main thread some time to handle this message
					Sleep(SLEEPTIME);
					}
				break;
			}					// end switch CBstate
		}						// end for(;;)  
	}

int dostats(int result, int movecount, int gamenumber, int *wins, int *draws, int *losses, int *unknowns, int *blackwins, int *blacklosses, char *matchlogstring)
	{
	// handles statistics during an engine match
	const int maxmovecount = 200;

	if(movecount>maxmovecount)
		{
		(*unknowns)++;
		sprintf(GPDNgame.resultstring,"*");									
		strcat(matchlogstring,"?");
		}
	else
		{
		switch (result)
			{
			case CB_WIN:
				if (currentengine == 1) 
					{
					(*wins)++;
					strcat(matchlogstring,"+");
					if(gamenumber % 2) 
						{
						(*blackwins)++;
						sprintf(GPDNgame.resultstring,"1-0");
						}
					else
						sprintf(GPDNgame.resultstring,"0-1");
					}
				else 
					{
					(*losses)++;
					strcat(matchlogstring,"-");
					if(gamenumber % 2) 
						{
						(*blacklosses)++;
						sprintf(GPDNgame.resultstring,"0-1");
						}
					else
						sprintf(GPDNgame.resultstring,"1-0");
					}
				break;

			case CB_DRAW:
				strcat(matchlogstring,"=");
				(*draws)++;
				sprintf(GPDNgame.resultstring,"1/2-1/2");
				break;

			case CB_LOSS:
				if (currentengine == 1) 
					{
					(*losses)++; 
					strcat(matchlogstring,"-");
					if(gamenumber % 2) 
						{
						(*blacklosses)++;
						sprintf(GPDNgame.resultstring,"0-1");
						}
					else
						sprintf(GPDNgame.resultstring,"1-0");
					}
				else 
					{
					(*wins)++;
					strcat(matchlogstring,"+");
					if(gamenumber%2) 
						{
						(*blackwins)++;
						sprintf(GPDNgame.resultstring,"1-0");
						}
					else
						sprintf(GPDNgame.resultstring,"0-1");
					}
				break;

			case CB_UNKNOWN:
				(*unknowns)++;
				strcat(matchlogstring,"?");
				sprintf(GPDNgame.resultstring,"*");
				break;
			}
		}
	return 1;
	}

int CPUinfo(char *str)
	{
	// print CPU info into str
	int CPUInfo[4] = {-1};
	char CPUBrandString[0x40];

	// get processor info
	__cpuid(CPUInfo,0x80000002);
    memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000003);
    memcpy(CPUBrandString+16, CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000004);
    memcpy(CPUBrandString+32, CPUInfo, sizeof(CPUInfo));

	sprintf(str,"%s",CPUBrandString);

	return 1;
	}

int makeanalysisfile(char *filename)
	{
	// produce nice analysis output
	int i,j;
	char s[256];
	char titlestring[256];
	char c1[256]="D84020";
	char c2[256]="A0C0C0";
	char c3[256]="444444";
	int leveltime[16]={0,0,1,2,5,10,15,30,60,120,300,900,1800,0,0};
	FILE *fp;
	char CPUinfostring[64];
	
	fp = fopen(filename, "w");
	if(fp == NULL)
		{
		MessageBox(hwnd, "Could not open analysisfile - is\nyour analysis directory missing?", "Error", MB_OK);
		return 0;
		}

	// print game info
	sprintf(titlestring,"%s - %s",GPDNgame.black, GPDNgame.white);

	// print HTML head
	fprintf(fp,"<HTML>\n<HEAD>\n<META name=\"GENERATOR\" content=\"CheckerBoard %s\">\n<TITLE>%s</TITLE></HEAD>", VERSION, titlestring);

	// print HTML body
	fprintf(fp,"<BODY><H3>");
	fprintf(fp, "%s - %s", GPDNgame.black, GPDNgame.white);
	fprintf(fp,"</H3>");
	fprintf(fp, "\n%s<BR>%s<BR>", GPDNgame.date, GPDNgame.event);
	fprintf(fp, "\nResult: %s<P>",GPDNgame.resultstring);

	// print hardware and level info
	enginename(s);

	CPUinfo(CPUinfostring);
	
	fprintf(fp, "\nAnalysis by %s at %is/move on %s", s, leveltime[gCBoptions.level], CPUinfostring);
	fprintf(fp,"\n<BR>\ngenerated with <A HREF=\"http://www.fierz.ch/checkers.htm\">CheckerBoard %s</A><P>", VERSION);

	// print PDN and analysis
	fprintf(fp,"\n<TABLE cellspacing=\"0\" cellpadding=\"3\">");
	current = head;
	j=0;
	while(current->next != NULL)
		{
		fprintf(fp,"<TR>\n");
		i=getmovenumber(current);
		if(strcmp(current->analysis,"")==0)
			{
			if(i%2)
				fprintf(fp,"<TD></TD><TD bgcolor=\"%s\"></TD><TD>%s</TD><TD bgcolor=\"%s\"></TD>\n",c1,current->PDN,c2);
			else
				fprintf(fp,"<TD>%2i.</TD><TD bgcolor=\"%s\">%s</TD><TD></TD><TD bgcolor=\"%s\"></TD>\n",i/2,c1,current->PDN,c2);
			}
		else
			{
			if(i%2)
				fprintf(fp,"<TD></TD><TD bgcolor=\"%s\"></TD><TD>%s</TD><TD bgcolor=\"%s\">%s</TD>\n",c1,current->PDN,c2,current->analysis);
			else
				fprintf(fp,"<TD>%2i.</TD><TD bgcolor=\"%s\">%s</TD><TD></TD><TD bgcolor=\"%s\">%s</TD>\n",i/2,c1,current->PDN,c2,current->analysis);
			}
		current = current->next;
		fprintf(fp,"</TR>\n");
		// add a delimiter line between moves
		fprintf(fp,"<tr><td></td><td bgcolor=\"%s\"></td><td></td><td bgcolor=\"%s\"></td></tr>\n",c1,c3);


		// stupid safety check
		if(j>10000)
			break;
		}

	fprintf(fp,"</TABLE></BODY></HTML>");
	fclose(fp);

	// go back to start of game
	current = head;

	ShellExecute(NULL,"open",filename,NULL,NULL,SW_SHOW);

	return 1;
	}


void setcurrentengine(int engineN)
	{
	char s[256], windowtitle[256];
	// set the engine
	if(engineN==1)
		{
		getmove=getmove1;
		islegal=islegal1;
		}
	if(engineN==2)
		{
		getmove=getmove2;
		islegal=islegal2;
		}
	currentengine=engineN;

	if(CBstate != ENGINEMATCH)
		{
		enginename(s);
		sprintf(windowtitle,"CheckerBoard%s: ", g_app_instance_suffix);
		strcat(windowtitle,s);
		SetWindowText(hwnd,windowtitle);
		}

	toggleengine = currentengine;
	// get book state of current engine
	if(enginecommand("get book",s))
		togglebook = atoi(s);
	}


int gametype(void)
	{
	// returns the game type which the current engine plays
	// if the engine has no game type associated, it will return 21 for english checkers
	char reply[ENGINECOMMAND_REPLY_SIZE];
	char command[256];

	sprintf(reply,"");
	sprintf(command,"get gametype");

	if(enginecommand(command,reply))
		return atoi(reply);

	// return default game type
	return GT_ENGLISH;
	}


int enginecommand(char command[256], char reply[ENGINECOMMAND_REPLY_SIZE])
// sends a command to the current engine, defined with the currentengine variable
// wraps a 'safety layer around calls to engine command by checking if this is supported */
	{
	int result=0;
	sprintf(reply,"");

	if(currentengine==1 && enginecommand1 != 0)
		result = enginecommand1(command,reply);
		
	if(currentengine==2 && enginecommand2!=0)
		result = enginecommand2(command,reply);
		
	return result;
	}

int enginename(char Lstr[256])
// returns the name of the current engine in Lstr 
	{
	// set a default
	sprintf(Lstr,"no engine found");

	if(currentengine==1)
		{
		if (enginecommand1!=0)
			{
			if((enginecommand1)("name",Lstr))
				return 1;
			}
		if(enginename1!=0)
			{
			(enginename1)(Lstr);
			return 1;
			}
		}

	if(currentengine==2)
		{
		if (enginecommand2!=0)
			{
			if((enginecommand2)("name",Lstr))
				return 1;
			}
		if(enginename2!=0)
			{
			(enginename2)(Lstr);
			return 1;
			}
		}
	return 0;
	}


int domove(struct CBmove m,int b[8][8])
	{
	// do move m on board b 
	int i,x,y;

	x=m.from.x;y=m.from.y;
	b[x][y]=0;
	x=m.to.x;y=m.to.y;
	b[x][y]=m.newpiece;

	for(i=0;i<m.jumps;i++)
		{
		x=m.del[i].x;
		y=m.del[i].y;
		b[x][y]=0;
		}
	return 1;
	}


int undomove(struct CBmove m,int b[8][8])
	{
	// take back move m on board b 
	int i,x,y;

	x = m.to.x;
	y = m.to.y;
	b[x][y] = 0;

	x = m.from.x;
	y = m.from.y;
	b[x][y] = m.oldpiece;

	for(i=0; i<m.jumps; i++)
		{
		x = m.del[i].x;
		y = m.del[i].y;
		b[x][y] = m.delpiece[i];
		}
	return 1;
	}


void move4tonotation(struct CBmove m,char s[80])
// takes a move in coordinates, and transforms it to numbers.
	{
	int from,to;
	char c='-';
	int x1,y1,x2,y2;
	char Lstr[255];
	x1=m.from.x;
	y1=m.from.y;
	x2=m.to.x;
	y2=m.to.y;

	if(m.jumps) c='x';
	// for all versions of checkers
	from = coorstonumber(x1,y1, GPDNgame.gametype);
	to = coorstonumber(x2,y2, GPDNgame.gametype);

	sprintf(s,"%i",from);
	sprintf(Lstr,"%c",c);
	strcat(s,Lstr);
	sprintf(Lstr,"%i",to);
	strcat(s,Lstr);
	}

void PDNgametoPDNstring(struct PDNgame *game, char *pdnstring, char *lf)
	{
	// prints a formatted PDN in *pdnstring
	// uses lf as line feed; for the clipboard this should be \r\n, normally just \n
	// i have no idea why this is so!
	char s[256];
	size_t counter;
	int i;
	struct listentry *listentry;

	// I: print headers 
	sprintf(pdnstring,"");
	sprintf(s,"[Event \"%s\"]",game->event);
	strcat(pdnstring,s);
	strcat(pdnstring,lf);

	//sprintf(s,"[Site \"%s\"]",game->site);
	//strcat(pdnstring,s);
	//strcat(pdnstring,lf);

	sprintf(s,"[Date \"%s\"]",game->date);
	strcat(pdnstring,s);
	strcat(pdnstring,lf);

	//sprintf(s,"[Round \"%s\"]",game->round);
	//strcat(pdnstring,s);
	//strcat(pdnstring,lf);

	sprintf(s,"[Black \"%s\"]",game->black);
	strcat(pdnstring,s);
	strcat(pdnstring,lf);

	sprintf(s,"[White \"%s\"]",game->white);
	strcat(pdnstring,s);
	strcat(pdnstring,lf);

	sprintf(s,"[Result \"%s\"]",game->resultstring);
	strcat(pdnstring,s);
	strcat(pdnstring,lf);

	// if this was after a setup, add FEN and setup header
	if(strcmp(game->setup,"")!=0)
		{
		sprintf(s,"[Setup \"%s\"]",game->setup);
		strcat(pdnstring,s);
		strcat(pdnstring,lf);

		sprintf(s,"[FEN \"%s\"]",game->FEN);
		strcat(pdnstring,s);
		strcat(pdnstring,lf);

		}
	// print PDN 
	listentry=game->head;
	i=1;
	counter=0;
	while( listentry->next !=NULL)
		{
		move4tonotation(listentry->move, listentry->PDN);
		// print the move number 
		if(i%2) // only on black moves...
			{
			sprintf(s,"%i. ",(int)((i+1)/2));
			counter+=strlen(s);
			if(counter>79) 
				{
				strcat(pdnstring,lf);
				counter=strlen(s);
				}
			strcat(pdnstring,s);
			}
		// print the move 
		counter+=strlen(listentry->PDN);
		if(counter>79)	
			{
			strcat(pdnstring,lf);

			counter=strlen(listentry->PDN);
			}
		sprintf(s,"%s ",listentry->PDN);
		strcat(pdnstring,s);
		// if the move has a comment, print it too 
		if (strcmp(listentry->comment,"")!=0)
			{
			counter+=strlen(listentry->comment);
			if(counter>79)	
				{
				strcat(pdnstring,lf);

				counter=strlen(listentry->comment);
				}
			strcat(pdnstring,"{");
			strcat(pdnstring,listentry->comment);
			strcat(pdnstring,"} ");
			}
		i++;
		listentry = listentry->next;
		}

	// add the game terminator 
	sprintf(s, "*");	/* Game terminator is '*' as per PDN 3.0. See http://pdn.fmjd.org/ */
	counter+=strlen(s);
	if(counter>79)
		strcat(pdnstring,lf);

	strcat(pdnstring,s);

	strcat(pdnstring,lf);
	strcat(pdnstring,lf);
	}


int appendmovetolist(struct CBmove m)
	{
	struct listentry *newlistentry;
	char pdn[255];

	// enter move in list
	newlistentry = (struct listentry *) malloc(sizeof(struct listentry));
	//newlistentry =  malloc(sizeof(struct listentry));
	if(newlistentry == NULL)
		{
		CBlog("could not allocate memory for CB movelist");
		exit(0);
		}
	newlistentry->alternatenext = NULL;
	current->next = newlistentry;
	current->move = m;
	sprintf(newlistentry->comment,"");
	move4tonotation(m,pdn);
	sprintf(current->PDN,"%s",pdn);
	newlistentry->last = current;
	current = newlistentry;
	tail = current;
	tail->next = NULL;
	return 1;
	}

int getfilename(char filename[255], int what)
	{
	OPENFILENAME of;
	char dir[MAX_PATH];

	sprintf(filename,"");
	(of).lStructSize = sizeof(OPENFILENAME);
	(of).hwndOwner = NULL;
	(of).hInstance = g_hInst;
	(of).lpstrFilter = "checkers databases *.pdn\0 *.pdn\0 all files *.*\0 *.*\0\0";
	(of).lpstrCustomFilter = NULL;
	(of).nMaxCustFilter = 0;
	(of).nFilterIndex = 0;
	(of).lpstrFile = filename; // if user chooses something, it's printed in here!
	(of).nMaxFile = MAX_PATH;
	(of).lpstrFileTitle = NULL;
	(of).nMaxFileTitle = 0;
	(of).lpstrInitialDir = gCBoptions.userdirectory;
	(of).lpstrTitle = NULL;
	(of).Flags = OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;
	(of).nFileOffset = 0;
	(of).nFileExtension = 0;
	(of).lpstrDefExt = NULL;
	(of).lCustData = 0;
	(of).lpfnHook = NULL;
	(of).lpTemplateName = NULL;

	if(what == OF_SAVEGAME)
		/* save a game to disk */
		{
		(of).lpstrTitle="Select PDN database to save game to";
		if(GetSaveFileName(&of))
			return 1;
		}
	if(what == OF_LOADGAME)
		{
		/* load a game from disk */
		(of).lpstrTitle="Select PDN database to load";
		if(GetOpenFileName(&of))
			return 1;
		}
	if(what == OF_SAVEASHTML)
		// save game as html
		{
		(of).lpstrTitle="Select filename of HTML output";
		(of).lpstrFilter="HTML files *.htm\0 *.htm\0 all files *.*\0 *.*\0\0";
		sprintf(dir, "%s\\games", CBdocuments);
		(of).lpstrInitialDir = dir;
		if(GetSaveFileName(&of))
			return 1;
		}
	if(what == OF_USERBOOK)
		// select user book
		{
		(of).lpstrTitle= "Select the user book to use";
		(of).lpstrFilter="user book files *.bin\0 *.bin\0 all files *.*\0 *.*\0\0";
		(of).lpstrInitialDir = CBdocuments;
		if(GetSaveFileName(&of))
			return 1;
		}

	if (what == OF_BOOKFILE) {
		(of).lpstrTitle= "Select the opening book filename";
		(of).lpstrFilter="user book files *.odb\0 *.odb\0 all files *.*\0 *.*\0\0";
		sprintf(dir, "%s\\engines", CBdirectory);
		(of).lpstrInitialDir = dir;
		if (GetOpenFileName(&of))
			return 1;
	}

	return 0;
	}

void pdntogame(int startposition[8][8], int startcolor)
	{
	/* pdntogame takes a starting position, a side to move next as parameters. 
	it uses the global linked list with head, tail and current, which has
	to be initialized with pdn-text to generate the CBmoves in the linked list.*/

	/* called by loadgame and gamepaste */

	int col;
	int b8[8][8];
	int from, to;
	struct CBmove legal;

	/* set the starting values */
	col = startcolor;
	memcpy(b8, startposition, sizeof(b8));
	current = head;
	// TODO - this seems to move one too far - no PDN attached here on the last move!
	while( current->next !=NULL)
		{
		PDNparseTokentonumbers(current->PDN,&from, &to);
		if(islegal(b8,col,from,to,&legal))
			{
			current->move=legal;
			col = CB_CHANGECOLOR(col);
			domove(legal,b8);
			current=current->next;
			}
		else
			{
			current->next=NULL;
			break;
			}
		}
	current=head;
	}


int builtinislegal(int board8[8][8], int color, int from, int to, struct CBmove *move)
	{
	// make all moves and try to find out if this move is legal 
	int i,n;
	struct coor c;
	int Lfrom, Lto;
	int isjump;

	if(color==CB_BLACK)
		n=getmovelist(1,m,board8,&isjump);
	else
		n=getmovelist(-1,m,board8,&isjump);
	for(i=0;i<n;i++)
		{
		c.x = m[i].from.x;
		c.y = m[i].from.y;
		Lfrom = coortonumber(c,GPDNgame.gametype);
		c.x = m[i].to.x;
		c.y = m[i].to.y;
		Lto = coortonumber(c,GPDNgame.gametype);
		if(Lfrom == from && Lto == to)
			{
			// we have found a legal move 
			// todo: continue to see whether this move is ambiguous!
			*move = m[i];
			return 1;
			}
		}
	if(isjump)
		sprintf(str,"illegal move - you must jump! for multiple jumps, click only from and to square");
	else
		sprintf(str,"%i-%i not a legal move",from,to);
	return 0;
	}


void newgame(void)
	{
	InitCheckerBoard(board8);
	reset_current_game_pdn();
	newposition = TRUE;
	reset = 1;
	gCBoptions.mirror = is_mirror_gametype(GPDNgame.gametype);
	color = get_startcolor(GPDNgame.gametype);
	
	if(gCBoptions.level==17)
		maxtime=initialtime;	
	
	updateboardgraphics(hwnd);
	}

typedef enum {
	PDN_IDLE, PDN_READING_FROM, PDN_WAITING_SEP, PDN_WAITING_TO, PDN_READING_TO, PDN_WAITING_OPTIONAL_TO,
	PDN_WAITING_OPTIONAL_SEP, PDN_CURLY_COMMENT, PDN_NEMESIS_COMMENT, PDN_FLUFF, PDN_QUOTED_VALUE, PDN_DONE
	} PDN_PARSE_STATE;

void doload(struct PDNgame *PDNgame, char *gamestring, int *color, int board8[8][8])
	{
	// game is in gamestring. use pdnparser routines to convert
	// it into a PDNgame
	// read headers 
	char *p, *start;
	char header[256], token[1024];
	char headername[256], headervalue[256];
	int i;
	int issetup = 0;
	PDN_PARSE_STATE state;

	// gamestring may terminate in a move, i.e. "1. 11-15 21-17". in this
	// case the tokenizer will not find a space after "11-15 " and not 
	// parse the move 21-17. therefore:
	strcat(gamestring, " ");

	p=gamestring;
	sprintf(PDNgame->setup,"%s","");
	sprintf(PDNgame->black,"%s","");
	sprintf(PDNgame->date,"%s","");
	sprintf(PDNgame->event,"%s","");
	sprintf(PDNgame->FEN,"%s","");
	sprintf(PDNgame->resultstring,"%s","");
	sprintf(PDNgame->round,"%s","");
	sprintf(PDNgame->white,"%s","");
	sprintf(PDNgame->site,"%s","");
	PDNgame->result = CB_UNKNOWN;

	while(PDNparseGetnextheader(&p,header))
		{
		/* parse headers */
		start=header;
		PDNparseGetnexttoken(&start,headername);
		PDNparseGetnexttag(&start,headervalue);
		/* make header lowercase, so that 'event' and 'Event' will be recognized */
		for(i=0; i<(int)strlen(headername);i++)
			headername[i]=(char) tolower(headername[i]);

		if(strcmp(headername,"event")==0)
			sprintf(PDNgame->event,"%s",headervalue);
		if(strcmp(headername,"site")==0)
			sprintf(PDNgame->site,"%s",headervalue);
		if(strcmp(headername,"date")==0)
			sprintf(PDNgame->date,"%s",headervalue);
		if(strcmp(headername,"round")==0)
			sprintf(PDNgame->round,"%s",headervalue);
		if(strcmp(headername,"white")==0)
			sprintf(PDNgame->white,"%s",headervalue);
		if(strcmp(headername,"black")==0)
			sprintf(PDNgame->black,"%s",headervalue);
		if(strcmp(headername,"result")==0)
			{
			sprintf(PDNgame->resultstring,"%s",headervalue);
			if(strcmp(headervalue,"1-0")==0)
				PDNgame->result = CB_WIN;
			if(strcmp(headervalue,"0-1")==0)
				PDNgame->result = CB_LOSS;
			if(strcmp(headervalue,"1/2-1/2")==0)
				PDNgame->result = CB_DRAW;
			if(strcmp(headervalue,"*")==0)
				PDNgame->result = CB_UNKNOWN;

			}
		if(strcmp(headername,"setup")==0)
			sprintf(PDNgame->setup,"%s",headervalue);
		if(strcmp(headername,"fen")==0)
			{
			sprintf(PDNgame->FEN,"%s",headervalue);
			sprintf(PDNgame->setup,"1");
			issetup = 1;
			}
		}

	current=head;
	/* set defaults */
	*color = get_startcolor(GPDNgame.gametype);
	gCBoptions.mirror = is_mirror_gametype(GPDNgame.gametype);

	InitCheckerBoard(board8);
	sprintf(current->comment,"");
	current->alternatenext=0;

	/* if its a setup load position */
	if(issetup)
		FENtoboard8(board8,PDNgame->FEN,color,GPDNgame.gametype);

	/*ok, headers read, now parse PDN input:*/
	while((state = (PDN_PARSE_STATE) PDNparseGetnextPDNtoken(&p,token)))
		{
		/* check for special tokens*/
		/* move number - discard */
		if(token[strlen(token)-1]=='.') 
			continue;
		/* game terminators */
		if((strcmp(token,"*")==0) || (strcmp(token,"0-1")==0) || (strcmp(token,"1-0")==0) || (strcmp(token,"1/2-1/2")==0)) 
			{
			/* In PDN 3.0, the game terminator is '*'. Allow old style game result terminators, 
			 * but don't interpret them as results.
			 */
			/* sprintf(PDNgame->resultstring,"%s",token); */
			break;
			}
		if(token[0]=='{' || state==PDN_FLUFF)
			{
			/* we found a comment */
			/* write it to last move, because current is already the new move */
			start=token;
			// remove the curly braces by moving pointer one forward, and trimming
			// last character
			if(state!=PDN_FLUFF)
				{
				start++;
				token[strlen(token)-1]=0;
				}
			if(current->last!=NULL)
				sprintf(current->last->comment,"%s",start);
			continue;
			}
#ifdef NEMESIS
		if(token[0]=='(')
			{
			/* we found a comment */
			/* write it to last move, because current is already the new move */
			start=token;
			start++;
			token[strlen(token)-1]=0;
			if(current->last!=NULL)
				sprintf(current->last->comment,"%s",start);
			continue;
			}
#endif
		// ok, it was just a move 
		sprintf(current->PDN,"%s",token);
		newlistentry = (struct listentry *)calloc(1, sizeof(struct listentry));
		if(newlistentry != NULL)
			{
			sprintf(newlistentry->comment,"");
			sprintf(newlistentry->PDN,"");
			newlistentry->alternatenext = NULL;
			current->next = newlistentry;
			newlistentry->last = current;
			current = newlistentry;
			}
		else
			sprintf(str,"malloc failure!");
		}

	// when we arrive here, we allocated one item too much in the linked list. 
	tail = current;
	current = head;
	sprintf(filename,"");
	// linked list is initialized with PDN

	// determine the moves which belong to the PDN
	pdntogame(board8,*color);
	reset=1;
	newposition=TRUE;
	}


int getmovenumber(struct listentry *cur)
	{
	// returns the number of the move in the current game
	struct listentry *tmp;
	int number =0;

	tmp = cur;
	while(tmp->last !=0)
		{
		tmp=tmp->last;
		number++;
		}
	return number+2;
	}


///////////////////////////////////////////////////////////////////////////////////////////////////
// initializations below:


void InitStatus(HWND hwnd)
	{
	hStatusWnd = CreateWindow(STATUSCLASSNAME, "", WS_CHILD|WS_VISIBLE,0,0,0,0,
		hwnd,NULL,g_hInst,NULL);
	}

void InitCheckerBoard(int b[8][8])
	{
	// initialize board to starting position 
	int i,j;
	for(i=0;i<=7;i++)
		{
		for(j=0;j<=7;j++)
			{
			b[i][j] = 0;
			}
		}
	b[0][0]=CB_BLACK|CB_MAN;
	b[2][0]=CB_BLACK|CB_MAN;
	b[4][0]=CB_BLACK|CB_MAN;
	b[6][0]=CB_BLACK|CB_MAN;
	b[1][1]=CB_BLACK|CB_MAN;
	b[3][1]=CB_BLACK|CB_MAN;
	b[5][1]=CB_BLACK|CB_MAN;
	b[7][1]=CB_BLACK|CB_MAN;
	b[0][2]=CB_BLACK|CB_MAN;
	b[2][2]=CB_BLACK|CB_MAN;
	b[4][2]=CB_BLACK|CB_MAN;
	b[6][2]=CB_BLACK|CB_MAN;

	b[1][7]=CB_WHITE|CB_MAN;
	b[3][7]=CB_WHITE|CB_MAN;
	b[5][7]=CB_WHITE|CB_MAN;
	b[7][7]=CB_WHITE|CB_MAN;
	b[0][6]=CB_WHITE|CB_MAN;
	b[2][6]=CB_WHITE|CB_MAN;
	b[4][6]=CB_WHITE|CB_MAN;
	b[6][6]=CB_WHITE|CB_MAN;
	b[1][5]=CB_WHITE|CB_MAN;
	b[3][5]=CB_WHITE|CB_MAN;
	b[5][5]=CB_WHITE|CB_MAN;
	b[7][5]=CB_WHITE|CB_MAN;
	}


/*
 * Load an engine dll, and get pointers to the exported functions in the dll.
 * Return non-zero on error.
 */
int load_engine(HINSTANCE *lib, char *dllname, CB_ENGINECOMMAND *cmdfn, CB_GETSTRING *namefn, CB_GETMOVE *getmovefn, CB_ISLEGAL *islegalfn, char *pri_or_sec)
{
	char buf[256];

	// go to the right directory to load engines
	SetCurrentDirectory(CBdirectory);
	sprintf(buf, "engines\\%s", dllname);
	*lib = LoadLibrary(buf);

	// go back to the working dir
	SetCurrentDirectory(CBdirectory);
	
	// If the handle is valid, try to get the function addresses
	if (*lib != NULL) {
		*cmdfn = (CB_ENGINECOMMAND)GetProcAddress(*lib, "enginecommand");
		*namefn = (CB_GETSTRING)GetProcAddress(*lib, "enginename");
		*getmovefn = (CB_GETMOVE)GetProcAddress(*lib, "getmove");
		*islegalfn = (CB_ISLEGAL)GetProcAddress(*lib, "islegal");			
		if (*islegalfn == NULL) 
			*islegalfn = CBislegal;
		return(0);
	}
	else {
		sprintf(buf, 
				"CheckerBoard could not find\n"
				"the %s engine dll.\n\n"
				"Please use the \n"
				"'Engine->Select..' command\n"
				"to select a new %s engine",
				pri_or_sec, pri_or_sec);
		MessageBox(hwnd, buf, "Error", MB_OK);					
		*cmdfn = NULL;
		*namefn = NULL;
		*getmovefn = NULL;
		*islegalfn = NULL;			
		return(1);
	}
}


void loadengines(char *pri_fname, char *sec_fname)
// sets the engines
// this is first called from WinMain on the WM_CREATE command.
// the global strings "primaryenginestring", 
// "secondaryenginestring"  contain
// the filenames of these engines.
// TODO: need to unload engines properly, first unload, then load
// new engines. for this, however, CB needs to know which engines
// are loaded right now.
	{
	int status;
	HMODULE primaryhandle, secondaryhandle;
	char Lstr[256];

	// set built in functions 
	CBgametype = (CB_GETGAMETYPE)builtingametype;
	CBislegal = (CB_ISLEGAL)builtinislegal;

	// load engine dlls
	// first, primary engine
	// is there a way to check whether a module is already loaded?
	primaryhandle = GetModuleHandle(pri_fname);
	sprintf(Lstr,"handle = %i (primary engine)",PtrToLong(primaryhandle));
	CBlog(Lstr);
	secondaryhandle = GetModuleHandle(sec_fname);
	sprintf(Lstr,"secondaryhandle = %i (secondary engine)",PtrToLong(secondaryhandle));
	CBlog(Lstr);

	// now, if one of the two handles, primaryhandle or secondaryhandle is
	// != 0, then that engine is already loaded and doesn't need to be loaded
	// again.
	// however, if these handles are 0, then we have to unload the engine
	// that is currently loaded. or, put differently, if one of the 
	// handles oldprimary/oldsecondary is not equal to one of the new
	// handles, then it has to be unloaded.


	// free up engine modules that are no longer used!
	// in fact, we should do this first, before loading the new engines!!

	/*
	 * If there was a primary engine loaded, and it is different from the new primary and secondary
	 * engine handles, then unload it.
	 */
	if (hinstLib1) {
		if (hinstLib1 != primaryhandle && hinstLib1 != secondaryhandle) {
			status = FreeLibrary(hinstLib1);
			hinstLib1 = 0;
			enginecommand1 = NULL;
			enginename1 = NULL;
			getmove1 = NULL;
			islegal1 = NULL;
		}
	}

	/* If there was a secondary engine loaded, and it is different from the new primary and secondary
	 * engine handles, then unload it.
	 */
	if (hinstLib2) {
		if (hinstLib2 != primaryhandle && hinstLib2 != secondaryhandle) {
			status = FreeLibrary(hinstLib2);
			hinstLib2 = 0;
			enginecommand2 = NULL;
			enginename2 = NULL;
			getmove2 = NULL;
			islegal2 = NULL;
		}
	}

	/* Load a new primary engine if there isn't one already loaded, or
	 * if the requested new engine filename is different from the one presently loaded (this happens
	 * if the presently loaded primary engine handle is same as the secondary engine handle).
	 */
	if (!hinstLib1 || strcmp(pri_fname, gCBoptions.primaryenginestring)) {
		status = load_engine(&hinstLib1, pri_fname, &enginecommand1, &enginename1, &getmove1, &islegal1, "primary");
		if (!status)
			strcpy(gCBoptions.primaryenginestring, pri_fname);		/* Success. */
		else {
			if (strcmp(pri_fname, gCBoptions.primaryenginestring)) {
				status = load_engine(&hinstLib1, gCBoptions.primaryenginestring, &enginecommand1, &enginename1, &getmove1, &islegal1, "primary");
				if (status)
					gCBoptions.primaryenginestring[0] = 0;
			}
		}
	}
	if (!hinstLib2 || strcmp(sec_fname, gCBoptions.secondaryenginestring)) {
		status = load_engine(&hinstLib2, sec_fname, &enginecommand2, &enginename2, &getmove2, &islegal2, "secondary");
		if (!status)
			strcpy(gCBoptions.secondaryenginestring, sec_fname);	/* Success. */
		else {
			if (strcmp(sec_fname, gCBoptions.secondaryenginestring)) {
				status = load_engine(&hinstLib2, gCBoptions.secondaryenginestring, &enginecommand2, &enginename2, &getmove2, &islegal2, "secondary");
				if (status)
					gCBoptions.secondaryenginestring[0] = 0;
			}
		}
	}

	// set current engine 
	setcurrentengine(1);

	// reset game if an engine of different game type was selected!
	if (gametype() != GPDNgame.gametype) {
		PostMessage(hwnd, (UINT)WM_COMMAND, (WPARAM)GAMENEW, (LPARAM)0);
		PostMessage(hwnd, (UINT)WM_SIZE, (WPARAM) 0, (LPARAM) 0);
	}

	// reset the directory to the CB directory
	SetCurrentDirectory(CBdirectory);
	}


void initengines(void)
{
	loadengines(gCBoptions.primaryenginestring, gCBoptions.secondaryenginestring);
}


int initlinkedlist(void)
	{
	// initializes the linked list of CB which holds the game
	while (head != NULL)
		{
		tail = head->next;
		free(head);
		head = tail;
		}

	head = (struct listentry *)  malloc(sizeof(struct listentry));
//	head =  malloc(sizeof(struct listentry));
	if(head == NULL)
		exit(0);

	head->next=NULL;
	head->last=NULL;
	head->alternatenext=NULL;
	sprintf(head->comment,"");
	tail = head;
	current = head;

	return 1;
	}

// CreateAToolBar creates a toolbar and adds a set of buttons to it.
// The function returns the handle to the toolbar if successful, 
// or NULL otherwise. 
// hwndParent is the handle to the toolbar's parent window. 
HWND CreateAToolBar(HWND hwndParent) 
	{ 
	HWND hwndTB; 
	TBADDBITMAP tbab; 
	INITCOMMONCONTROLSEX icex;
	int i;
	int id[NUMBUTTONS] = {
		15,
		0,
		NUMBUTTONS+STD_FILENEW,NUMBUTTONS+STD_FILESAVE,NUMBUTTONS+STD_FILEOPEN,NUMBUTTONS+STD_FIND,
		0,
		7,NUMBUTTONS+STD_UNDO,NUMBUTTONS+STD_REDOW,8,
		0,
		2,
		3,
		0,
		0,
		10,11,17,
		0,
		12,13,14};
	int command[NUMBUTTONS] = {	HELPHOMEPAGE,
		0,
		GAMENEW, GAMESAVE, GAMELOAD, GAMEFIND,
		0,
		MOVESBACKALL, MOVESBACK, MOVESFORWARD, MOVESFORWARDALL,
		0,
		MOVESPLAY, TOGGLEBOOK, TOGGLEENGINE,
		0,
		SETUPCC,DISPLAYINVERT,TOGGLEMODE,
		0,
		BOOKMODE_VIEW,BOOKMODE_ADD,BOOKMODE_DELETE};
	int style[NUMBUTTONS] = {TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,TBSTYLE_BUTTON,TBSTYLE_BUTTON,TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,TBSTYLE_BUTTON,TBSTYLE_BUTTON,TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,TBSTYLE_BUTTON,TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,TBSTYLE_BUTTON,TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,TBSTYLE_BUTTON,TBSTYLE_BUTTON};

	// Ensure that the common control DLL is loaded. 
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_BAR_CLASSES;
	
	InitCommonControlsEx(&icex);

	// Create a toolbar. 
	hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL, 
		WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS|CCS_ADJUSTABLE|TBSTYLE_FLAT, 
		0, 0, 0, 0, hwndParent, 
		(HMENU) ID_TOOLBAR, g_hInst, NULL); 

	// Send the TB_BUTTONSTRUCTSIZE message, which is required for backward compatibility. 
	SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 

	// Fill the TBBUTTON array with button information, and add the 
	// buttons to the toolbar. The buttons on this toolbar have text 
	// but do not have bitmap images. 
	for(i=0;i<NUMBUTTONS;i++)
		{
		tbButtons[i].dwData = 0L;
		tbButtons[i].fsState = TBSTATE_ENABLED;
		tbButtons[i].fsStyle = (BYTE) style[i];
		tbButtons[i].iBitmap = id[i];
		tbButtons[i].idCommand = command[i];
		tbButtons[i].iString = 0; //"text";
		}

	// here's how toolbars work:
	// first, add bitmaps to the toolbar. the toolbar keeps a list of bitmaps.
	// then, add buttons to the toolbar, specifying the index of the bitmap you want to use
	// in this case, i first add custom bitmaps, NUMBUTTONS of them,
	// then i add the standard windows bitmaps. 
	// so instead of using the index STD_FIND for the bitmap to find things,
	// i need to add NUMBUTTONS to that so that it works out!

	// add custom bitmaps 
	tbab.hInst = g_hInst;
	tbab.nID = IDTB_BMP;
	SendMessage(hwndTB, TB_ADDBITMAP, NUMBUTTONS, (LPARAM)&tbab);

	// add default bitmaps
	tbab.hInst = HINST_COMMCTRL;
	tbab.nID = IDB_STD_SMALL_COLOR; 
	//tbab.nID = IDB_VIEW_LARGE_COLOR; 
	SendMessage(hwndTB, TB_ADDBITMAP, 0, (LPARAM)&tbab);

	// add buttons
	SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM) NUMBUTTONS, (LPARAM) (LPTBBUTTON) &tbButtons); 

	// finally, resize
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0); 

	ShowWindow(hwndTB, SW_SHOW); 
	return hwndTB; 
	} 

// synchronization functions below
int getenginebusy(void)
	{
	int returnvalue;
	EnterCriticalSection(&engine_criticalsection);
	returnvalue = enginebusy;
	LeaveCriticalSection(&engine_criticalsection);
	return returnvalue;
	}

/*
 * Tell engine to abort searching immediately.
 * Wait a maximum of 1 second for the engine to finish aborting.
 */
void abortengine()
{
	clock_t t0;

	if (!getenginebusy())
		return;

	abortcalculation = 1;
	playnow = 1;

	t0 = clock();
	while (getenginebusy()) {
		Sleep(10);
		if ((int)(clock() - t0) > 1000)
			break;
	}
}


int getanimationbusy(void)
	{
	int returnvalue;
	EnterCriticalSection(&ani_criticalsection);
	returnvalue = animationbusy;
	LeaveCriticalSection(&ani_criticalsection);
	return returnvalue;
	}
int getenginestarting(void)
	{
	int returnvalue;
	returnvalue = enginestarting;
	return returnvalue;
	}

int setenginebusy(int value)
	{
	EnterCriticalSection(&engine_criticalsection);
	enginebusy = value;
	LeaveCriticalSection(&engine_criticalsection);
	return 1;
	}
int setanimationbusy(int value)
	{
	EnterCriticalSection(&ani_criticalsection);
	animationbusy = value;
	LeaveCriticalSection(&ani_criticalsection);
	return 1;
	}

int setenginestarting(int value)
	{
	enginestarting = value;
	return 1;
	}