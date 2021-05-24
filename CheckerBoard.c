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
//  int WINAPI getmove(Board8x8 board, int color, double maxtime, char str[1024], int *playnow, int info, int moreinfo, CBmove *move);
//  int WINAPI enginecommand(const char *command, char reply[1024]);
// TODO: bug report: if you hit takeback while CB is animating a move, you get an undefined state

/******************************************************************************/

// CB uses multithreading, up to 4 threads:
//	-> main thread for the window
//	-> checkers engine runs in 'Thread'
//	-> animation runs in 'AniThread'
//	-> game analysis & engine match are driven by 'AutoThread'

#define STRICT
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <stdio.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <time.h>
#include <io.h>
#include <intrin.h>
#include <vector>
#include <algorithm>

#include "standardheader.h"
#include "cb_interface.h"
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
#include "resource.h"
#include "saveashtml.h"
#include "graphics.h"
#include "registry.h"
#include "app_instance.h"

#ifdef _WIN64
#pragma message("_WIN64 is defined.")
#endif


//---------------------------------------------------------------------
// globals - should be identified in code by g_varname but aren't all...
PDNgame cbgame;						/* The current game. */

// all checkerboard options are collected in CBoptions; like this, they can be saved
// as one struct in the registry, instead of using lots of commands.
CBoptions cboptions;

int g_app_instance;					/* 0, 1, 2, ... */
char g_app_instance_suffix[10];		/* "", "[1]", "[2]", ... */
DWORD g_SearchThreadId, g_AniThreadId, AutoThreadId;
HANDLE hSearchThread, hAniThread, hAutoThread;
int enginethreadpriority = THREAD_PRIORITY_NORMAL;	/* default priority setting*/
int usersetpriority = THREAD_PRIORITY_NORMAL;		/* default priority setting*/
HICON hIcon;						/* CB icon for the window */
TBBUTTON tbButtons[NUMBUTTONS];		/* for the toolbar */

/* these globals are used to synchronize threads */
int abortcalculation;				// used to tell the SearchThreadFunc that the calculation has been aborted
static BOOL enginebusy;				/* true while engine thread is busy */
static BOOL animationbusy;			/* true while animation thread is busy */
static BOOL enginestarting;			// true when a play command is issued to the engine but engine has not started yet
BOOL gameover;						/* true when autoplay or engine match game is finished */
BOOL startmatch = TRUE;				/* startmatch is only true before engine match was started */
BOOL newposition = TRUE;			/* is true when position has changed. used in analysis mode to
										restart search and then reset */
BOOL startengine;					/* is true if engine is expected to start */
int game_result;					/* CB_DRAW, CB_WIN, CB_LOSS, or CB_UNKNOWN. */
time_ctrl_t time_ctrl;				/* Clock control. */
int toolbarheight = 30;
int clockheight;					/* 0 or CLOCKHEIGHT when clock is visible. */
int statusbarheight = 20;
int menuheight = 16;
int titlebarheight = 12;
int offset = 40;
int upperoffset = 20;
char szWinName[] = "CheckerBoard";	/* name of window class */
Board8x8 cbboard8;					/* the board being displayed in the GUI*/
int cbcolor = CB_BLACK;				/* the side to move next in the GUI */
bool setup_mode;
static int addcomment;
int testset_number;
int playnow;						/* playnow is passed to the checkers engines, it is set to nonzero if the user chooses 'play' */
bool reset_move_history;			/* send option to engine to reset its list of game moves. */
int gameindex;						/* game to load/replace from/in a database */

/* dll globals */

/* CB uses function pointers to access the dll.
enginename, engineabout, engineoptions, enginehelp, getmove point to the currently used functions
...1 and ...2 are the pointers to dll1 and dll2 as read from engines.ini. */

/* library instances for primary, secondary and analysis engines */
HINSTANCE hinstLib, hinstLib1, hinstLib2;

/* function pointers for the engine functions */
CB_GETMOVE getmove, getmove1, getmove2;
CB_ENGINECOMMAND enginecommand1, enginecommand2;

// multi-version support
CB_ISLEGAL islegal, islegal1, islegal2;
CB_GETSTRING enginename1, enginename2;
CB_GETGAMETYPE CBgametype;			// built in gametype and islegal functions

// instance and window handles
HINSTANCE g_hInst;				//instance of checkerboard
HWND hwnd;						// main window
HWND hStatusWnd;				// status window
static HWND tbwnd;				// toolbar window

std::vector<gamepreview> game_previews;	// preview info displayed in game select dialog.

// statusbar_txt holds the output string shown in the status bar - it is updated by WM_TIMER messages
char statusbar_txt[1024];
char playername[MAXNAME];		// name of the player we are searching games of
char eventname[MAXNAME];		// event we're searching for
char datename[MAXNAME];			// date we're searching for
char commentname[MAXNAME];		// comment we're searching for
int searchwithposition;			// search with position?
HMENU hmenu;					// menu handle
double xmetric, ymetric;		// gives the size of the board8: one square is xmetric*ymetric
Squarelist clicks;				// user clicks on the board

char reply[ENGINECOMMAND_REPLY_SIZE];	// holds reply of engine to command requests
char CBdirectory[MAX_PATH];				// holds the directory from where CB is started:
char CBdocuments[MAX_PATH];				// CheckerBoard directory under My Documents
char pdn_filename[MAX_PATH];			// current PDN database
char userbookname[MAX_PATH];			// current userbook
CBmove cbmove;
char savegame_filename[MAX_PATH];
emstats_t emstats;						// engine match stats and state
std::vector<BALLOT_INFO> user_ballots;

bool two_player_mode;					// true if in 2-player mode
int book_state;							// engine book state (0/1/2/3)
int currentengine = 1;					// 1=primary, 2=secondary
int maxmovecount = 300;					// engine match limit; use 200 if early_game_adjudication is enabled.
bool has_getmovelist;					// true if current engine has enginecommand "get movelist"

// keep a small user book
userbookentry userbook[MAXUSERBOOK];
int userbooknum;
int userbookcur;
static CHOOSECOLOR ccs;

// reindex tells whether we have to reindex a database when searching.
// reindex is set to 1 if a game is saved, a game is replaced, or the
// database changed. and initialized to 1.
int reindex = 1;
int re_search_ok;
char piecesetname[MAXPIECESET][256];
int maxpieceset;
CRITICAL_SECTION ani_criticalsection, engine_criticalsection;
int handletooltiprequest(LPTOOLTIPTEXT TTtext);
void reset_game(PDNgame &game);
void forward_to_game_end(void);

// checkerboard goes finite-state: it can be in one of the modes above.
//	normal:	after the user enters a move, checkerboard starts to calculate
//				with engine.
//	autoplay: checkerboard plays engine-engine
//	enginematch: checkerboard plays engine-engine2
//	analyzegame: checkerboard moves through a game and comments on every move
//	entergame: checkerboard does nothing while the user enters a game
//	observegame: checkerboard calculates while the user enters a game
enum state {
	NORMAL,
	AUTOPLAY,
	ENGINEMATCH,
	ENGINEGAME,
	ANALYZEGAME,
	OBSERVEGAME,
	ENTERGAME,
	BOOKVIEW,
	BOOKADD,
	RUNTESTSET,
	ANALYZEPDN
} CBstate = NORMAL;


void start_animation_thread(void)
{
	HANDLE handle = CreateThread(NULL,
								0,
								(LPTHREAD_START_ROUTINE)AnimationThreadFunc,
								hwnd,
								0,
								&g_AniThreadId);
}

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpszArgs, int nWinMode)
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
	wcl.hIcon = LoadIcon(hThisInst, "icon1");	// load CB icon
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);	// cursor style
	wcl.cbClsExtra = 0;						// no extra
	wcl.cbWndExtra = 0;						// information needed
	wcl.hbrBackground = (HBRUSH) GetSysColorBrush(GetSysColor(COLOR_MENU));

	// register the window class
	if (!RegisterClass(&wcl))
		return 0;

	// create the window
	hwnd = CreateWindow(szWinName,			// name of window class
						"CheckerBoard:",	// title
						WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX,	// window style - normal
						CW_USEDEFAULT,	// x coordinate - let windows decide
						CW_USEDEFAULT,	// y coordinate - let windows decide
						480,			// width
						560,			// height
						HWND_DESKTOP,	// no parent window
						NULL,			// no menu
						hThisInst,		// handle of this instance of the program
						NULL			// no additional arguments
						);

	// load settings from the registry
	createcheckerboard(hwnd);

	// display the window
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// set database filename in case of shell-doubleclick on a *.pdn file
	sprintf(savegame_filename, lpszArgs);

	// initialize common controls - toolbar and status bar need this
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
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
	clockheight = cboptions.use_incremental_time ? CLOCKHEIGHT : 0;

	// initialize status bar
	InitStatus(hwnd);

	// get status bar height
	GetWindowRect(hStatusWnd, &rect);
	statusbarheight = rect.bottom - rect.top;

	// get menu and title bar height:
	menuheight = GetSystemMetrics(SM_CYMENU);
	titlebarheight = GetSystemMetrics(SM_CXSIZE);

	// get offsets before the board is printed for the first time
	offset = toolbarheight + statusbarheight + clockheight - 1;
	upperoffset = toolbarheight + clockheight - 1;
	setoffsets(offset, upperoffset);

	// start a timer @ 10Hz: every time this timer goes off, handletimer() is called
	// this updates the status bar and the toolbar
	SetTimer(hwnd, 1, 100, NULL);

	start_animation_thread();

	// create the message loop
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hwnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

bool fixed_time_needed_msg()
{
	if (cboptions.use_incremental_time) {
		MessageBox(hwnd, "Change to a fixed time/move.", "Error", MB_OK);
		return(true);
	}
	return(false);
}

void reset_game_clocks()
{
	if (cboptions.use_incremental_time) {
		time_ctrl.clock_paused = true;
		time_ctrl.black_time_remaining = cboptions.initial_time + cboptions.time_increment;
		time_ctrl.white_time_remaining = cboptions.initial_time + cboptions.time_increment;
		time_ctrl.starttime = clock();
	}
	else {
		time_ctrl.black_time_remaining = 0;
		time_ctrl.white_time_remaining = 0;
	}
}

void start_clock()
{
	time_ctrl.clock_paused = false;
	time_ctrl.starttime = clock();
}

void stop_clock()
{
	if (cboptions.use_incremental_time && time_ctrl.clock_paused == false)
		time_ctrl.clock_paused = true;
}

/*
 * Get the instantaneous values of time left of black and white clocks.
 * Add the value spent on thinking for the current move to
 * the clock value that was last updated at the start of its turn.
 */
void get_game_clocks(double *black_clock, double *white_clock)
{
	double newtime;

	if (time_ctrl.clock_paused)
		newtime = 0;
	else
		newtime = (clock() - time_ctrl.starttime) / (double)CLOCKS_PER_SEC;
	if (cbcolor == CB_BLACK) {
		*black_clock = time_ctrl.black_time_remaining - newtime;
		*white_clock = time_ctrl.white_time_remaining;
	}
	else {
		*black_clock = time_ctrl.black_time_remaining;
		*white_clock = time_ctrl.white_time_remaining - newtime;
	}
}

/*
 * Decide if the move described by moveindex is a first player or second player move.
 * If the game has a normal start position, even moves are first player, odd moves are second player.
 * If the game has a FEN setup, see if the start color is the same as the gametype's start color.
 * If the same, then even moves are first player, odd moves are second player.
 * If not the same, then odd moves are first player, even moves are second player.
 */
bool is_second_player(PDNgame &game, int moveindex)
{
	int startcolor;

	if (game.FEN[0] == 0) {
		if (moveindex & 1)
			return(true);
		else
			return(false);
	}

	startcolor = get_startcolor(game.gametype);
	if (game.FEN[0] == 'B' && startcolor == CB_BLACK || game.FEN[0] == 'W' && startcolor == CB_WHITE) {
		if (moveindex & 1)
			return(true);
		else
			return(false);
	}
	else {
		if (moveindex & 1)
			return(false);
		else
			return(true);
	}
}

int moveindex2movenum(PDNgame &game, int moveindex)
{
	if (game.FEN[0] == 0)
		return(1 + moveindex / 2);

	int startcolor = get_startcolor(game.gametype);
	if (game.FEN[0] == 'B' && startcolor == CB_BLACK || game.FEN[0] == 'W' && startcolor == CB_WHITE)
		return(1 + moveindex / 2);
	else
		return(1 + (moveindex + 1) / 2);
}

/* Transition to or from setup mode. */
void set_setup_mode(bool state)
{
	if (state) {

		// entering setup mode
		PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
		CheckMenuItem(hmenu, SETUPMODE, MF_CHECKED);
		sprintf(statusbar_txt, "Setup mode...");
	}
	else {

		// leaving setup mode;
		CheckMenuItem(hmenu, SETUPMODE, MF_UNCHECKED);
		reset_move_history = true;
		clicks.clear();
		sprintf(statusbar_txt, "Setup done");

		// get FEN string
		reset_game(cbgame);
		board8toFEN(cbboard8, cbgame.FEN, cbcolor, cbgame.gametype);
	}
	setup_mode = state;
}

LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// this is the main function of checkerboard. it receives messages from winmain(), and
	// then acts appropriately
	FILE *fp;
	LPRECT lprec;
	int x, y;
	char str2[256], Lstr[256];
	char str1024[1024];
	char *gamestring;
	static enum state laststate;
	static int oldengine;
	RECT windowrect;
	RECT WinDim;
	static int cxClient, cyClient;
	MENUBARINFO mbi;
	HINSTANCE hinst;

	switch (message) {
	case WM_CREATE:
		InitializeCriticalSection(&ani_criticalsection);
		InitializeCriticalSection(&engine_criticalsection);
		PostMessage(hwnd, WM_COMMAND, LOADENGINES, 0);
		break;

	case WM_ACTIVATE:
		InvalidateRect(hwnd, NULL, true);
		break;

	case WM_NOTIFY:
		// respond to tooltip request //
		// lParam contains (LPTOOLTIPTEXT) - send it on to handletooltiprequest function
		handletooltiprequest((LPTOOLTIPTEXT) lParam);
		break;

	case WM_DROPFILES:
		DragQueryFile((HDROP) wParam, 0, pdn_filename, sizeof(pdn_filename));
		DragFinish((HDROP) wParam);
		PostMessage(hwnd, WM_COMMAND, GAMELOAD, 0);
		break;

	case WM_TIMER:
		// timer goes off, telling us to update status bar, toolbar
		// icons. handletimer does this, only if it's necessary.
		handletimer();
		break;

	case WM_RBUTTONDOWN:
		x = (int)(LOWORD(lParam) / xmetric);
		y = (int)(8 - (HIWORD(lParam) - toolbarheight - clockheight) / ymetric);
		handle_rbuttondown(x, y);
		break;

	case WM_LBUTTONDOWN:
		x = (int)(LOWORD(lParam) / xmetric);
		y = (int)(8 - (HIWORD(lParam) - toolbarheight - clockheight) / ymetric);
		handle_lbuttondown(x, y);
		break;

	case WM_PAINT:	// repaint window
		updategraphics(hwnd);
		break;

	case WM_SIZING: // keep window quadratic
		lprec = (LPRECT) lParam;
		mbi.cbSize = sizeof(MENUBARINFO);
		GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi);
		menuheight = mbi.rcBar.bottom - mbi.rcBar.top;
		offset = toolbarheight + statusbarheight + clockheight - 1;
		upperoffset = toolbarheight + clockheight - 1;
		setoffsets(offset, upperoffset);
		cxClient = lprec->right - lprec->left;
		cxClient -= cxClient % 8;
		cxClient += 2 * (GetSystemMetrics(SM_CXSIZEFRAME) - 4);
		cyClient = cxClient;
		lprec->right = lprec->left + cxClient;
		lprec->bottom = lprec->top + cyClient + offset + menuheight + titlebarheight + 2;	//+ cboptions.addoffset;
		break;

	case WM_SIZE:
		// window size has changed
		cxClient = LOWORD(lParam);
		cyClient = HIWORD(lParam);

		// check menu height
		mbi.cbSize = sizeof(MENUBARINFO);
		GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi);
		menuheight = mbi.rcBar.bottom - mbi.rcBar.top;
		offset = toolbarheight + statusbarheight + clockheight - 1;
		upperoffset = toolbarheight + clockheight - 1;
		setoffsets(offset, upperoffset);

		// get window size, set xmetric and ymetric which CB needs to know where user clicks
		GetClientRect(hwnd, &WinDim);
		xmetric = WinDim.right / 8.0;
		ymetric = (WinDim.bottom - offset) / 8.0;

		// get error:
		GetWindowRect(hwnd, &WinDim);

		// make window quadratic
		if ((xmetric - ymetric) * 8 != 0) {
			MoveWindow(hwnd,
					   WinDim.left,
					   WinDim.top,
					   WinDim.right - WinDim.left,
					   WinDim.bottom - WinDim.top + (int)((xmetric - ymetric) * 8),
					   1);
		}

		// update stretched stones etc
		resizegraphics(hwnd);
		updateboardgraphics(hwnd);
		SendMessage(hStatusWnd, WM_SIZE, wParam, lParam);
		SendMessage(tbwnd, WM_SIZE, wParam, lParam);
		break;

	case WM_COMMAND:
		// the following case structure handles user command (and also internal commands
		// that CB may generate itself
		switch (LOWORD(wParam)) {
		case LOADENGINES:
			hSearchThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) initengines, (HWND) 0, 0, &g_SearchThreadId);
			break;

		case GAMENEW:
			PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
			newgame();
			break;

		case GAMEANALYZE:
			if (CBstate == BOOKVIEW || CBstate == BOOKADD)
				break;
			if (fixed_time_needed_msg())
				break;
			changeCBstate(ANALYZEGAME);
			startmatch = TRUE;

			// the rest is taken care of in the AutoThreadFunc section
			break;

		case GAMEANALYZEPDN:
			if (CBstate == BOOKVIEW || CBstate == BOOKADD)
				break;
			if (fixed_time_needed_msg())
				break;
			changeCBstate(ANALYZEPDN);
			startmatch = TRUE;

			// the rest is taken care of in the AutoThreadFunc section
			break;

		case GAME3MOVE:
			PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
			if (cboptions.op_crossboard || cboptions.op_barred || cboptions.op_mailplay) {
				int opening_index = getopening(&cboptions);
				PostMessage(hwnd, WM_COMMAND, START3MOVE, opening_index);
			}
			else
				MessageBox(hwnd, "nothing selected in the 3-move deck!", "Error", MB_OK);
			break;

		case START3MOVE:
			start3move(LOWORD(lParam));
			break;

		case GAMEREPLACE:
			// replace a game in the pdn database
			// assumption: you have loaded a game, so now "pdn_filename" holds the db filename
			// and gameindex is the index of the game in the file
			handlegamereplace(gameindex, pdn_filename);
			break;

		case GAMESAVE:
			// show save game dialog. if OK, call 'dosave' to do the work
			SetCurrentDirectory(cboptions.userdirectory);
			if (DialogBox(g_hInst, "IDD_SAVEGAME", hwnd, (DLGPROC) DialogFuncSavegame)) {
				if (getfilename(savegame_filename, OF_SAVEGAME)) {
					SendMessage(hwnd, WM_COMMAND, DOSAVE, 0);
				}
			}

			SetCurrentDirectory(CBdirectory);
			break;

		case GAMESAVEASHTML:
			// show save game dialog. if OK is selected, call 'savehtml' to do the work
			if (DialogBox(g_hInst, "IDD_SAVEGAME", hwnd, (DLGPROC) DialogFuncSavegame)) {
				if (getfilename(savegame_filename, OF_SAVEASHTML)) {
					saveashtml(savegame_filename, &cbgame);
					sprintf(statusbar_txt, "game saved as HTML!");
				}
			}
			break;

		case DOSAVE:
			// saves the game stored in cbgame
			fp = fopen(savegame_filename, "at+");

			// file with savegame_filename opened - we append to that file
			// filename was set by save game
			if (fp != NULL) {
				std::string gamepdn;

				PDNgametoPDNstring(cbgame, gamepdn, "\n");
				fprintf(fp, "\n%s", gamepdn.c_str());
				fclose(fp);
			}

			// set reindex flag
			reindex = 1;
			break;

		case GAMEDATABASE:
			// set working database
			sprintf(pdn_filename, "%s", cboptions.userdirectory);
			getfilename(pdn_filename, OF_LOADGAME);

			// set reindex flag
			reindex = 1;
			break;

		case SELECTUSERBOOK:
			// set user book.
			sprintf(userbookname, "%s", CBdocuments);
			if (getfilename(userbookname, OF_USERBOOK)) {

				// load user book
				fp = fopen(userbookname, "rb");
				if (fp != NULL) {
					userbooknum = (int)fread(userbook, sizeof(userbookentry), MAXUSERBOOK, fp);
					fclose(fp);
				}

				sprintf(statusbar_txt, "found %d positions in user book", userbooknum);
			}
			break;

		case GAMELOAD:
			// call selectgame with GAMELOAD to let the user select from all games
			cblog("pdn load game\n");
			selectgame(GAMELOAD);
			break;

		case GAMEINFO:
			// display a box with information on the game
			if (get_startcolor(cbgame.gametype) == CB_BLACK) 
				sprintf(str1024,
						"Event: %s\nBlack: %s\nWhite: %s\nResult: %s",
						cbgame.event,
						cbgame.black,
						cbgame.white,
						cbgame.resultstring);
			else
				sprintf(str1024,
						"Event: %s\nWhite: %s\nBlack: %s\nResult: %s",
						cbgame.event,
						cbgame.white,
						cbgame.black,
						cbgame.resultstring);
			MessageBox(hwnd, str1024, "Game information", MB_OK);
			sprintf(statusbar_txt, "");
			break;

		case SEARCHMASK:
			// call selectgame with SEARCHMASK to let the user
			// select from games of a certain player/event/date
			cblog("pdn search with player, event, or date.\n");
			selectgame(SEARCHMASK);
			break;

		case RE_SEARCH:
			cblog("pdn research\n");
			selectgame(RE_SEARCH);
			break;

		case GAMEFIND:
			// find a game with the current position in the current database
			// index the database
			cblog("find current pos\n");
			selectgame(GAMEFIND);
			break;

		case GAMEFINDCR:
			// find games with current position color-reversed
			selectgame(GAMEFINDCR);
			cblog("find colors reversed pos\n");
			break;

		case GAMEFINDTHEME:
			// find a game with the current position in the current database
			// index the database
			selectgame(GAMEFINDTHEME);
			cblog("find theme\n");
			break;

		case LOADNEXT:
			sprintf(statusbar_txt, "load next game");
			cblog("load next game\n");
			loadnextgame();
			break;

		case LOADPREVIOUS:
			sprintf(statusbar_txt, "load previous game");
			cblog("load previous game\n");
			loadpreviousgame();
			break;

		case GAMEEXIT:
			PostMessage(hwnd, WM_DESTROY, 0, 0);
			break;

		case DIAGRAM:
			diagramtoclipboard(hwnd);
			break;

		case SAMPLEDIAGRAM:
			samplediagramtoclipboard(hwnd);
			break;

		case GAME_FENTOCLIPBOARD:
			if (setup_mode) {
				MessageBox(hwnd,
						   "Cannot copy position in setup mode.\nLeave the setup mode first if you\nwant to copy this position.",
					   "Error",
						   MB_OK);
			}
			else
				FENtoclipboard(hwnd, cbboard8, cbcolor, cbgame.gametype);
			break;

		case GAME_FENFROMCLIPBOARD:
			// first, get the stuff that is in the clipboard
			gamestring = textfromclipboard(hwnd, statusbar_txt);

			// now if we have something, do something with it
			if (gamestring != NULL) {
				PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);

				if (FENtoboard8(cbboard8, gamestring, &cbcolor, cbgame.gametype)) {
					display_board(cbboard8);
					reset_move_history = true;
					newposition = TRUE;
					sprintf(statusbar_txt, "position copied");
					PostMessage(hwnd, WM_COMMAND, GAMEINFO, 0);
					sprintf(cbgame.FEN, gamestring);
				}
				else
					sprintf(statusbar_txt, "no valid FEN position in clipboard!");
				free(gamestring);
			}
			break;

		case GAMECOPY:
			if (setup_mode) {
				MessageBox(hwnd,
						   "Cannot copy game in setup mode.\nLeave the setup mode first if you\nwant to copy this game.",
						   "Error",
						   MB_OK);
			}
			else
				PDNtoclipboard(hwnd, cbgame);
			break;

		case GAMEPASTE:
			// copy game or fen string from the clipboard...
			gamestring = textfromclipboard(hwnd, statusbar_txt);

			// now that the game is in gamestring doload() on it
			if (gamestring != NULL) {
				std::string errormsg;
				PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);

				/* Detect fen or game, load it in either case. */
				if (is_fen(gamestring)) {
					if (!FENtoboard8(cbboard8, gamestring, &cbcolor, cbgame.gametype)) {
						if (doload(&cbgame, gamestring, &cbcolor, cbboard8, errormsg))
							MessageBox(hwnd, errormsg.c_str(), "Error", MB_OK);

						sprintf(statusbar_txt, "game copied");
					}
					else {
						reset_game(cbgame);
						board8toFEN(cbboard8, cbgame.FEN, cbcolor, cbgame.gametype);
						sprintf(statusbar_txt, "position copied");
					}
				}
				else {
					if (doload(&cbgame, gamestring, &cbcolor, cbboard8, errormsg))
						MessageBox(hwnd, errormsg.c_str(), "Error", MB_OK);
					else
						sprintf(statusbar_txt, "position copied");
				}

				free(gamestring);

				// game is fully loaded, clean up
				display_board(cbboard8);
				reset_move_history = true;
				newposition = TRUE;
				PostMessage(hwnd, WM_COMMAND, GAMEINFO, 0);
			}
			else
				sprintf(statusbar_txt, "clipboard open failed");
			break;

		case MOVESPLAY:
			// force the engine to either play now, or to start calculating
			// this is the only place where the engine is started
			if (setup_mode)
				set_setup_mode(false);
			if (!getenginebusy()) {

				// TODO think about synchronization issues here!
				setenginebusy(TRUE);
				setenginestarting(FALSE);
				CloseHandle(hSearchThread);
				hSearchThread = CreateThread(NULL, 100000, (LPTHREAD_START_ROUTINE)SearchThreadFunc, (LPVOID) 0, 0, &g_SearchThreadId);
			}
			else
				SendMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
			clicks.clear();
			break;

		case INTERRUPTENGINE:
			// tell engine to stop thinking and play a move
			if (getenginebusy())
				playnow = 1;
			break;

		case ABORTENGINE:
			// tell engine to stop thinking and not play a move
			if (getenginebusy()) {
				abortcalculation = 1;
				playnow = 1;
			}
			break;

		case MOVESBACK:
			// take back a move
			abortengine();
			if (CBstate == BOOKVIEW && userbooknum != 0) {
				if (userbookcur > 0)
					userbookcur--;
				userbookcur %= userbooknum;
				sprintf(statusbar_txt,
						"position %d of %d: %i-%i",
						userbookcur + 1,
						userbooknum,
						coortonumber(userbook[userbookcur].move.from, cbgame.gametype),
						coortonumber(userbook[userbookcur].move.to, cbgame.gametype));

				// set up position
				if (userbookcur < userbooknum) {

					// only if there are any positions
					bitboardtoboard8(&(userbook[userbookcur].position), cbboard8);
					display_board(cbboard8);
				}
				break;
			}

			if (cbgame.movesindex == 0 && (CBstate == ANALYZEGAME || CBstate == ANALYZEPDN))
				gameover = TRUE;

			if (cbgame.movesindex > 0) {
				--cbgame.movesindex;

				gamebody_entry *tbmove = &cbgame.moves[cbgame.movesindex];
				undomove(tbmove->move, cbboard8);
				display_board(cbboard8);

				// shouldnt this color thing be handled in undomove?
				cbcolor = CB_CHANGECOLOR(cbcolor);
				sprintf(statusbar_txt, "takeback: ");

				// and print move number and move into the status bar
				// get move number:
				if (is_second_player(cbgame, cbgame.movesindex))
					sprintf(Lstr, "%i... %s", moveindex2movenum(cbgame, cbgame.movesindex), tbmove->PDN);
				else
					sprintf(Lstr, "%i. %s", moveindex2movenum(cbgame, cbgame.movesindex), tbmove->PDN);
				strcat(statusbar_txt, Lstr);

				if (strcmp(tbmove->comment, "") != 0) {
					sprintf(Lstr, " %s", tbmove->comment);
					strcat(statusbar_txt, Lstr);
				}

				if (CBstate == OBSERVEGAME)
					PostMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
				else
					abortengine();
			}
			else
				sprintf(statusbar_txt, "Takeback not possible: you are at the start of the game!");

			newposition = TRUE;
			reset_move_history = true;
			break;

		case MOVESFORWARD:
			// go forward one move
			// stop the engine if it is still running
			abortengine();

			// if in user book mode, move to the next position in user book
			if (CBstate == BOOKVIEW && userbooknum != 0) {
				if (userbookcur < userbooknum - 1)
					userbookcur++;
				userbookcur %= userbooknum;
				sprintf(statusbar_txt,
						"position %d of %d: %i-%i",
						userbookcur + 1,
						userbooknum,
						coortonumber(userbook[userbookcur].move.from, cbgame.gametype),
						coortonumber(userbook[userbookcur].move.to, cbgame.gametype));

				// set up position
				if (userbookcur < userbooknum) {

					// only if there are any positions
					bitboardtoboard8(&(userbook[userbookcur].position), cbboard8);
					display_board(cbboard8);
				}
				break;
			}

			// normal case - move forward one move
			if (cbgame.movesindex < (int)cbgame.moves.size()) {
				gamebody_entry *pmove = &cbgame.moves[cbgame.movesindex];
				domove(pmove->move, cbboard8);
				cbcolor = CB_CHANGECOLOR(cbcolor);
				display_board(cbboard8);

				// get move number:
				// and print move number and move into the status bar
				if (is_second_player(cbgame, cbgame.movesindex))
					sprintf(Lstr, "%i... %s", moveindex2movenum(cbgame, cbgame.movesindex), pmove->PDN);
				else
					sprintf(Lstr, "%i. %s", moveindex2movenum(cbgame, cbgame.movesindex), pmove->PDN);
				sprintf(statusbar_txt, "%s ", Lstr);

				if (strcmp(pmove->comment, "") != 0) {
					sprintf(Lstr, "%s", pmove->comment);
					strcat(statusbar_txt, Lstr);
				}

				++cbgame.movesindex;

				if (CBstate == OBSERVEGAME)
					PostMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
				newposition = TRUE;
				reset_move_history = true;
			}
			else
				sprintf(statusbar_txt, "Forward not possible: End of game");
			break;

		case MOVESBACKALL:
			// take back all moves
			if (CBstate == BOOKVIEW || CBstate == BOOKADD)
				break;
			PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
			while (cbgame.movesindex > 0) {
				--cbgame.movesindex;
				undomove(cbgame.moves[cbgame.movesindex].move, cbboard8);
				cbcolor = CB_CHANGECOLOR(cbcolor);
			}

			if (CBstate == OBSERVEGAME)
				PostMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
			display_board(cbboard8);
			sprintf(statusbar_txt, "you are now at the start of the game");
			newposition = TRUE;
			reset_move_history = true;
			break;

		case MOVESFORWARDALL:
			// go forward all moves
			if (CBstate == BOOKVIEW || CBstate == BOOKADD)
				break;
			PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);
			forward_to_game_end();
			if (CBstate == OBSERVEGAME)
				PostMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
			display_board(cbboard8);
			sprintf(statusbar_txt, "you are now at the end of the game");
			newposition = TRUE;
			reset_move_history = true;
			break;

		case MOVESCOMMENT:
			// add a comment to the last move
			DialogBox(g_hInst, "IDD_COMMENT", hwnd, (DLGPROC) DialogFuncAddcomment);
			break;

		case LEVELEXACT:
			if (cboptions.exact_time) {
				cboptions.exact_time = false;
				CheckMenuItem(hmenu, LEVELEXACT, MF_UNCHECKED);
			}
			else {
				cboptions.exact_time = true;
				CheckMenuItem(hmenu, LEVELEXACT, MF_CHECKED);
			}
			break;

		case LEVELINSTANT:
		case LEVEL01S:
		case LEVEL02S:
		case LEVEL05S:
		case LEVEL1S:
		case LEVEL2S:
		case LEVEL5S:
		case LEVEL10S:
		case LEVEL15S:
		case LEVEL30S:
		case LEVEL1M:
		case LEVEL2M:
		case LEVEL5M:
		case LEVEL15M:
		case LEVEL30M:
		case LEVELINFINITE:
			cboptions.use_incremental_time = false;
			clockheight = 0;
			RECT rect;
			GetWindowRect(tbwnd, &rect);
			toolbarheight = rect.bottom - rect.top;
			PostMessage(hwnd, (UINT) WM_SIZE, (WPARAM) 0, (LPARAM) 0);

			cboptions.level = timetoken_to_level(LOWORD(wParam));
			checklevelmenu(&cboptions, hmenu, LOWORD(wParam));
			if (LOWORD(wParam) == LEVELINFINITE)
				sprintf(statusbar_txt, "search time set to infinite");
			else
				sprintf(statusbar_txt, "search time set to %.1f sec/move", timelevel_to_time(cboptions.level));
			break;

		case LEVELINCREMENT:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_INCREMENTAL_TIMES), hwnd, (DLGPROC) DialogIncrementalTimesFunc);
			if (cboptions.use_incremental_time) {
				reset_game_clocks();
				clockheight = CLOCKHEIGHT;

				RECT rect;

				GetWindowRect(tbwnd, &rect);
				toolbarheight = rect.bottom - rect.top;
				checklevelmenu(&cboptions, hmenu, timelevel_to_token(cboptions.level));
				sprintf(statusbar_txt,
						"incremental time set: initial time %.0f sec, increment %.3f sec",
						cboptions.initial_time,
						cboptions.time_increment);
				PostMessage(hwnd, (UINT) WM_SIZE, (WPARAM) 0, (LPARAM) 0);
			}
			else
				clockheight = 0;
			break;

		case LEVELADDTIME:
			// add 1 second when '+' is pressed
			if (cboptions.use_incremental_time) {
				if (cbcolor == CB_BLACK)
					time_ctrl.black_time_remaining += 1.0;
				else
					time_ctrl.white_time_remaining += 1.0;
			}
			else
				sprintf(statusbar_txt, "not in increment mode!");
			break;

		case LEVELSUBTRACTTIME:
			// subtract 1 second when '-' is pressed
			if (cboptions.use_incremental_time) {
				if (cbcolor == CB_BLACK)
					time_ctrl.black_time_remaining -= 1.0;
				else
					time_ctrl.white_time_remaining -= 1.0;
			}
			else
				sprintf(statusbar_txt, "not in increment mode!");
			break;

		case ID_CLOCK_RESET:
			reset_game_clocks();
			break;

		// piece sets
		case PIECESET:
		case PIECESET + 1:
		case PIECESET + 2:
		case PIECESET + 3:
		case PIECESET + 4:
		case PIECESET + 5:
		case PIECESET + 6:
		case PIECESET + 7:
		case PIECESET + 8:
		case PIECESET + 9:
		case PIECESET + 10:
		case PIECESET + 11:
		case PIECESET + 12:
		case PIECESET + 13:
		case PIECESET + 14:
		case PIECESET + 15:
			cboptions.piecesetindex = LOWORD(wParam) - PIECESET;
			sprintf(statusbar_txt, "piece set %i: %s", cboptions.piecesetindex, piecesetname[cboptions.piecesetindex]);
			SetCurrentDirectory(CBdirectory);
			SetCurrentDirectory("bmp");
			initbmp(hwnd, piecesetname[cboptions.piecesetindex]);
			resizegraphics(hwnd);
			display_board(cbboard8);
			InvalidateRect(hwnd, NULL, 1);
			SetCurrentDirectory(CBdirectory);
			break;

		//  set highlighting color to draw frame around selected stone square
		case COLORHIGHLIGHT:
			initcolorstruct(hwnd, &ccs, 0);
			if (ChooseColor(&ccs)) {
				cboptions.colors[0] = (COLORREF) ccs.rgbResult;
				sprintf(statusbar_txt, "new highlighting color");
			}
			else
				sprintf(statusbar_txt, "no new colors! error %i", CommDlgExtendedError());
			display_board(cbboard8);
			break;

		//  set color for board numbers
		case COLORBOARDNUMBERS:
			initcolorstruct(hwnd, &ccs, 1);
			if (ChooseColor(&ccs)) {
				cboptions.colors[1] = (COLORREF) ccs.rgbResult;
				sprintf(statusbar_txt, "new board number color");
			}
			else
				sprintf(statusbar_txt, "no new colors! error %i", CommDlgExtendedError());
			display_board(cbboard8);
			break;

		case OPTIONS3MOVE:
			DialogBox(g_hInst, "IDD_3MOVE", hwnd, (DLGPROC) ThreeMoveDialogFunc);
			break;

		case OPTIONSDIRECTORIES:
			DialogBox(g_hInst, "IDD_DIRECTORIES", hwnd, (DLGPROC) DirectoryDialogFunc);
			break;

		case OPTIONSUSERBOOK:
			toggle(&(cboptions.userbook));
			setmenuchecks(&cboptions, hmenu);
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
			toggle(&(cboptions.priority));
			if (cboptions.priority) // low priority mode
				usersetpriority = THREAD_PRIORITY_BELOW_NORMAL;
			else
				usersetpriority = THREAD_PRIORITY_NORMAL;
			setmenuchecks(&cboptions, hmenu);
			break;

		case OPTIONSHIGHLIGHT:
			if (cboptions.highlight == TRUE)
				cboptions.highlight = FALSE;
			else
				cboptions.highlight = TRUE;
			if (cboptions.highlight == TRUE)
				CheckMenuItem(hmenu, OPTIONSHIGHLIGHT, MF_CHECKED);
			else
				CheckMenuItem(hmenu, OPTIONSHIGHLIGHT, MF_UNCHECKED);
			break;

		case OPTIONSSOUND:
			if (cboptions.sound == TRUE)
				cboptions.sound = FALSE;
			else
				cboptions.sound = TRUE;
			if (cboptions.sound == TRUE)
				CheckMenuItem(hmenu, OPTIONSSOUND, MF_CHECKED);
			else
				CheckMenuItem(hmenu, OPTIONSSOUND, MF_UNCHECKED);
			break;

		case BOOKMODE_VIEW:
			// go in view book mode
			if (userbooknum == 0) {
				sprintf(statusbar_txt, "no moves in user book");
				break;
			}

			if (CBstate == BOOKVIEW)
				changeCBstate(NORMAL);
			else {
				changeCBstate(BOOKVIEW);

				// now display the first user book position
				userbookcur = 0;
				sprintf(statusbar_txt,
						"position %d of %d: %i-%i",
						userbookcur + 1,
						userbooknum,
						coortonumber(userbook[userbookcur].move.from, cbgame.gametype),
						coortonumber(userbook[userbookcur].move.to, cbgame.gametype));

				// set up position
				if (userbookcur < userbooknum) {

					// only if there are any positions
					bitboardtoboard8(&(userbook[userbookcur].position), cbboard8);
					display_board(cbboard8);
				}
			}
			break;

		case BOOKMODE_ADD:
			// go in add/edit book mode
			if (CBstate == BOOKADD)
				changeCBstate(NORMAL);
			else
				changeCBstate(BOOKADD);
			break;

		case BOOKMODE_DELETE:
			// remove current user book position from book
			if (CBstate == BOOKVIEW && userbooknum != 0) {

				// want to delete book move here:
				for (int i = userbookcur; i < userbooknum - 1; i++)
					userbook[i] = userbook[i + 1];
				userbooknum--;

				// if we deleted last position, move to new last position.
				if (userbookcur == userbooknum)
					userbookcur--;

				// display what position we have:
				sprintf(statusbar_txt,
						"position %d of %d: %i-%i",
						userbookcur + 1,
						userbooknum,
						coortonumber(userbook[userbookcur].move.from, cbgame.gametype),
						coortonumber(userbook[userbookcur].move.to, cbgame.gametype));
				if (userbooknum == 0)
					sprintf(statusbar_txt, "no moves in user book");

				// set up position
				if (userbookcur < userbooknum && userbooknum != 0) {

					// only if there are any positions
					bitboardtoboard8(&(userbook[userbookcur].position), cbboard8);
					display_board(cbboard8);
				}

				// save user book
				fp = fopen(userbookname, "wb");
				if (fp != NULL) {
					fwrite(userbook, sizeof(userbookentry), userbooknum, fp);
					fclose(fp);
				}
				else
					sprintf(statusbar_txt, "unable to write to user book");
			}
			else {
				if (CBstate == BOOKVIEW)
					sprintf(statusbar_txt, "no moves in user book");
				else
					sprintf(statusbar_txt, "You must be in 'view user book' mode to delete moves!");
			}
			break;

		case CM_NORMAL:
			// go to normal play mode
			if (getenginebusy())
				playnow = 1;

			// stop engine
			PostMessage(hwnd, WM_COMMAND, GOTONORMAL, 0);
			break;

		case CM_AUTOPLAY:
			if (CBstate == BOOKVIEW || CBstate == BOOKADD)
				break;
			changeCBstate(AUTOPLAY);
			break;

		case CM_ENGINEMATCH:
			if (CBstate == BOOKVIEW || CBstate == BOOKADD)
				break;
			if (!enginecommand1 || !enginecommand2)
				break;
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_START_ENGINE_MATCH), hwnd, (DLGPROC)DialogStartEngineMatchFunc)) {
				startmatch = TRUE;
				changeCBstate(ENGINEMATCH);
			}
			break;

		case ENGINEVSENGINE:
			startmatch = TRUE;
			changeCBstate(ENGINEGAME);
			break;

		case CM_ANALYSIS:
			// go to analysis mode
			if (CBstate == BOOKVIEW || CBstate == BOOKADD)
				break;
			if (cboptions.use_incremental_time)
				PostMessage(hwnd, WM_COMMAND, LEVELINFINITE, 0);

			changeCBstate(OBSERVEGAME);
			newposition = TRUE;
			break;

		case CM_2PLAYER:
			SendMessage(hwnd, WM_COMMAND, TOGGLEMODE, 0);
			break;

		case GOTONORMAL:
			// the following is weird, posts the same message again, why?
			if (getenginebusy()) {
				PostMessage(hwnd, WM_COMMAND, GOTONORMAL, 0);
				Sleep(10);
			}
			else {
				changeCBstate(NORMAL);
				stop_clock();
			}
			break;

		case ENGINESELECT:
			// select engines
			DialogBox(g_hInst, "IDD_DIALOGENGINES", hwnd, (DLGPROC) EngineDialogFunc);
			break;

		case ENGINEOPTIONS:
			// select engine options
			oldengine = currentengine;
			DialogBox(g_hInst, "IDD_ENGINEOPTIONS", hwnd, (DLGPROC) EngineOptionsFunc);
			setcurrentengine(oldengine);
			enginecommand("get book", Lstr);
			book_state = atoi(Lstr);
			break;

		case ENGINEEVAL:
			// static eval of the current positions
			board8toFEN(cbboard8, str2, cbcolor, cbgame.gametype);
			sprintf(Lstr, "staticevaluation %s", str2);
			if (enginecommand(Lstr, reply))
				MessageBox(hwnd, reply, "Static Evaluation", MB_OK);
			else
				MessageBox(hwnd, "This engine does not support\nstatic evaluation", "About Engine", MB_OK);
			break;

		case ENGINEABOUT:
			// tell engine to display information about itself
			if (enginecommand("about", reply))
				MessageBox(hwnd, reply, "About Engine", MB_OK);
			break;

		case ENGINEHELP:
			// get a help-filename from engine and display file with default viewer
			if (enginecommand("help", str2)) {
				if (strcmp(str2, "")) {
					SetCurrentDirectory(CBdirectory);
					SetCurrentDirectory("engines");
					showfile(str2);
					SetCurrentDirectory(CBdirectory);
				}
				else
					MessageBox(hwnd, "This engine has no help file", "CheckerBoard says:", MB_OK);
			}
			else
				MessageBox(hwnd, "This engine has no help file", "CheckerBoard says:", MB_OK);
			break;

		case DISPLAYINVERT:
			// toggle: invert the board yes/no
			toggle(&(cboptions.invert));
			if (cboptions.invert)
				CheckMenuItem(hmenu, DISPLAYINVERT, MF_CHECKED);
			else
				CheckMenuItem(hmenu, DISPLAYINVERT, MF_UNCHECKED);
			display_board(cbboard8);
			break;

		case DISPLAYMIRROR:
			// toggle: mirror the board yes/no
			// this is a trick to make checkers variants like italian display properly.
			toggle(&(cboptions.mirror));
			if (cboptions.mirror)
				CheckMenuItem(hmenu, DISPLAYMIRROR, MF_CHECKED);
			else
				CheckMenuItem(hmenu, DISPLAYMIRROR, MF_UNCHECKED);
			display_board(cbboard8);
			break;

		case DISPLAYNUMBERS:
			// toggle display numbers on / off
			toggle(&(cboptions.numbers));
			if (cboptions.numbers)
				CheckMenuItem(hmenu, DISPLAYNUMBERS, MF_CHECKED);
			else
				CheckMenuItem(hmenu, DISPLAYNUMBERS, MF_UNCHECKED);
			display_board(cbboard8);
			break;

		case SETUPMODE:
			set_setup_mode(!setup_mode);
			break;

		case SETUPCLEAR:
			// clear board. Forces setup mode.
			if (!setup_mode)
				set_setup_mode(true);

			memset(cbboard8, 0, sizeof(cbboard8));
			display_board(cbboard8);
			break;

		case TOGGLEMODE:
			if (getenginebusy() || getanimationbusy())
				break;
			two_player_mode = !two_player_mode;
			if (two_player_mode)
				changeCBstate(ENTERGAME);
			else
				changeCBstate(NORMAL);
			break;

		case TOGGLEBOOK:
			if (getenginebusy() || getanimationbusy())
				break;
			book_state++;
			book_state %= 4;

			// set opening book on/off
			sprintf(Lstr, "set book %i", book_state);
			enginecommand(Lstr, statusbar_txt);
			break;

		case TOGGLEENGINE:
			if (getenginebusy() || getanimationbusy())
				break;

			togglecurrentengine();

			// reset game if an engine of different game type was selected!
			if (gametype() != cbgame.gametype) {
				PostMessage(hwnd, (UINT) WM_COMMAND, (WPARAM) GAMENEW, (LPARAM) 0);
				PostMessage(hwnd, (UINT) WM_SIZE, (WPARAM) 0, (LPARAM) 0);
			}
			break;

		case SETUPCC:
			handlesetupcc(&cbcolor);
			break;

		case HELPHELP:
			// open the help.htm file with default .htm viewer
			SetCurrentDirectory(CBdirectory);
			switch (cboptions.language) {
			case ESPANOL:
				showfile("helpspanish.htm");
				break;

			case DEUTSCH:
				if (fileispresent("helpdeutsch.htm"))
					showfile("helpdeutsch.htm");
				else
					showfile("help.htm");
				break;

			case ITALIANO:
				if (fileispresent("helpitaliano.htm"))
					showfile("helpitaliano.htm");
				else
					showfile("help.htm");
				break;

			case FRANCAIS:
				if (fileispresent("helpfrancais.htm"))
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
			DialogBox(g_hInst, "IDD_CBABOUT", hwnd, (DLGPROC) AboutDialogFunc);
			break;

		case HELPHOMEPAGE:
			// open the checkerboard homepage with default htm viewer
			hinst = ShellExecute(NULL, "open", "www.fierz.ch/checkers.htm", NULL, NULL, SW_SHOW);
			break;

		case CM_ENGINECOMMAND:
			DialogBox(g_hInst, "IDD_ENGINECOMMAND", hwnd, (DLGPROC) DialogFuncEnginecommand);
			break;

		case CM_ADDCOMMENT:
			toggle(&addcomment);
			if (addcomment)
				CheckMenuItem(hmenu, CM_ADDCOMMENT, MF_CHECKED);
			else
				CheckMenuItem(hmenu, CM_ADDCOMMENT, MF_UNCHECKED);
			break;

		case CM_RUNTESTSET:
			// let CB run over a set of test positions in the current pdn database
			testset_number = 0;
			changeCBstate(RUNTESTSET);
			break;
		}
		break;

	case WM_DESTROY:
		// terminate the program
		// save window size:
		GetWindowRect(hwnd, &windowrect);
		cboptions.window_x = windowrect.left;
		cboptions.window_y = windowrect.top;
		cboptions.window_width = windowrect.right - windowrect.left;
		cboptions.window_height = windowrect.bottom - windowrect.top;

		// save settings
		savesettings(&cboptions);

		// unload engines
		{
			BOOL fFreeResult = FreeLibrary(hinstLib1);
			fFreeResult = FreeLibrary(hinstLib2);
		}

		PostQuitMessage(0);
		break;

	default:
		// Let Windows process any messages not specified
		//	in the preceding switch statement
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

void reset_game(PDNgame &game)
{
	sprintf(game.black, "");
	sprintf(game.white, "");
	sprintf(game.resultstring, "*");
	sprintf(game.event, "");
	sprintf(game.date, "");
	sprintf(game.FEN, "");
	sprintf(game.round, "");
	sprintf(game.site, "");
	game.result = UNKNOWN_RES;
	game.moves.clear();
	game.movesindex = 0;
	game.gametype = gametype();
}

int SetMenuLanguage(int language)
{
	// load the proper menu
	DestroyMenu(hmenu);
	switch (language) {
	case ENGLISH:
	case OPTIONSLANGUAGEENGLISH:
		hmenu = LoadMenu(g_hInst, "MENUENGLISH");
		cboptions.language = ENGLISH;
		SetMenu(hwnd, hmenu);
		break;

	case DEUTSCH:
	case OPTIONSLANGUAGEDEUTSCH:
		hmenu = LoadMenu(g_hInst, "MENUDEUTSCH");
		cboptions.language = DEUTSCH;
		SetMenu(hwnd, hmenu);
		break;

	case ESPANOL:
	case OPTIONSLANGUAGEESPANOL:
		hmenu = LoadMenu(g_hInst, "MENUESPANOL");
		cboptions.language = ESPANOL;
		SetMenu(hwnd, hmenu);
		break;

	case ITALIANO:
	case OPTIONSLANGUAGEITALIANO:
		hmenu = LoadMenu(g_hInst, "MENUITALIANO");
		cboptions.language = ITALIANO;
		SetMenu(hwnd, hmenu);
		break;

	case FRANCAIS:
	case OPTIONSLANGUAGEFRANCAIS:
		hmenu = LoadMenu(g_hInst, "MENUFRANCAIS");
		cboptions.language = FRANCAIS;
		SetMenu(hwnd, hmenu);
		break;
	}

	// delete stuff we don't need
	SetCurrentDirectory(CBdirectory);
	if (fileispresent("db\\db6.cpr"))
		DeleteMenu(hmenu, 8, MF_BYPOSITION);

	// now insert stuff we do need: piece set choice depending on what is installed
	add_piecesets_to_menu(hmenu);

	DrawMenuBar(hwnd);
	return 1;
}

int get_startcolor(int gametype)
{
	int color = CB_BLACK;

	if (gametype == GT_ENGLISH)
		color = CB_BLACK;
	else if (gametype == GT_ITALIAN)
		color = CB_WHITE;
	else if (gametype == GT_SPANISH)
		color = CB_WHITE;
	else if (gametype == GT_RUSSIAN)
		color = CB_WHITE;
	else if (gametype == GT_CZECH)
		color = CB_WHITE;

	return(color);
}

char *pdn_result_to_string(PDN_RESULT result, int gametype)
{
	switch (result) {
	case UNKNOWN_RES:
		return("*");

	case WHITE_WIN_RES:
		if (get_startcolor(gametype) == CB_WHITE)
			return("1-0");
		else
			return("0-1");
		break;
		
	case BLACK_WIN_RES:
		if (get_startcolor(gametype) == CB_BLACK)
			return("1-0");
		else
			return("0-1");
		break;
		
	case DRAW_RES:
		return("1/2-1/2");
		break;
	}
	return("*");
}

PDN_RESULT string_to_pdn_result(char *resultstr, int gametype)
{
	if (strcmp(resultstr, "1/2-1/2") == 0)
		return(DRAW_RES);
	else if (strcmp(resultstr, "1-1") == 0)
		return(DRAW_RES);
	else if (strcmp(resultstr, "*") == 0)
		return(UNKNOWN_RES);
	else if (strcmp(resultstr, "1-0") == 0) {
		if (get_startcolor(gametype) == CB_BLACK)
			return(BLACK_WIN_RES);
		else
			return(WHITE_WIN_RES);
	}
	else if (strcmp(resultstr, "0-1") == 0) {
		if (get_startcolor(gametype) == CB_BLACK)
			return(WHITE_WIN_RES);
		else
			return(BLACK_WIN_RES);
	}
	else
		return(DRAW_RES);
}

int is_mirror_gametype(int gametype)
{
	if (gametype == GT_ITALIAN)
		return(1);
	if (gametype == GT_SPANISH)
		return(1);

	return(0);
}

int is_row_reversed_gametype(int gametype)
{
	return(gametype == GT_ITALIAN);
}

int handlesetupcc(int *color)
// handle change color request
{
	char str2[256];

	*color = CB_CHANGECOLOR(*color);

	reset_game(cbgame);
	cboptions.mirror = is_mirror_gametype(cbgame.gametype);

	// and the FEN string
	board8toFEN(cbboard8, str2, *color, cbgame.gametype);
	sprintf(cbgame.FEN, str2);
	return 1;
}

int handle_rbuttondown(int x, int y)
{
	if (setup_mode) {
		coorstocoors(&x, &y, cboptions.invert, cboptions.mirror);
		if (is_valid_board8_square(x, y)) {
			switch (cbboard8[x][y]) {
			case CB_WHITE | CB_MAN:
				cbboard8[x][y] = CB_WHITE | CB_KING;
				break;

			case CB_WHITE | CB_KING:
				cbboard8[x][y] = 0;
				break;

			default:
				cbboard8[x][y] = CB_WHITE | CB_MAN;
				break;
			}
		}

		display_board(cbboard8);
	}

	return 1;
}

int handle_lbuttondown(int x, int y)
{
	int i, legal, legalmovenumber;
	int square, from, to;
	CBmove localmove;

	/* Convert screen coords to board8 coords. */
	coorstocoors(&x, &y, cboptions.invert, cboptions.mirror);
	if (!is_valid_board8_square(x, y))
		return 0;

	// if we are in setup mode, add a black piece.
	if (setup_mode) {
		switch (cbboard8[x][y]) {
		case CB_BLACK | CB_MAN:
			cbboard8[x][y] = CB_BLACK | CB_KING;
			break;

		case CB_BLACK | CB_KING:
			cbboard8[x][y] = 0;
			break;

		default:
			cbboard8[x][y] = CB_BLACK | CB_MAN;
			break;
		}

		display_board(cbboard8);
		return 1;
	}

	// if the engine is calculating we don't accept any input - except if
	//	we are in "enter game" mode
	if ((getenginebusy() || getanimationbusy()) && (CBstate != OBSERVEGAME))
		return 0;

	/* Don't allow a square to appear in clicks more than twice.
	 * Only allow 'from' and 'to' squares to be identical.
	 */
	square = coorstonumber(x, y, cbgame.gametype);
	if (clicks.frequency(square) >= 2)
		return(1);
	if (clicks.frequency(square) == 1 && (cbboard8[x][y] & cbcolor) == 0)
		return(1);

	clicks.append(square);
	if (clicks.size() == 1) {

		// then its the first click
		from = clicks.first();

		// if there is only one move with this piece, then do it!
		if (islegal != NULL) {
			legal = 0;
			if (has_getmovelist || cbgame.gametype == GT_ENGLISH) {

				/* We can do a better job for English since we have a movelist generator. */
				legal = num_matching_moves(cbboard8, cbcolor, clicks, localmove, cbgame.gametype);
			}
			else {
				legalmovenumber = 0;
				for (i = 1; i <= 32; i++) {
					if (islegal(cbboard8, cbcolor, from, i, &localmove) != 0) {
						legal++;
						legalmovenumber = i;
						to = i;
					}
				}

				// look for a single move possible to an empty square
				if (legal == 0) {
					for (i = 1; i <= 32; i++) {
						if (islegal(cbboard8, cbcolor, i, clicks.first(), &localmove) != 0) {
							legal++;
							legalmovenumber = i;
							from = i;
							to = clicks.first();
						}
					}

					if (legal != 1)
						legal = 0;
				}
			}

			// remove the output that islegal generated, it's disturbing ("1-32 illegal move")
			sprintf(statusbar_txt, "");
			if (legal == 1) {

				// is it the only legal move?
				// if yes, do it!
				// if we are in user book mode, add it to user book!

				// a legal move! Add move to the game list.
				// For English checkers we can fully describe ambiguous captures.
				if (has_getmovelist || cbgame.gametype == GT_ENGLISH) {
					char pdn[40];
					move_to_pdn_english(cbboard8, cbcolor, &localmove, pdn, cbgame.gametype);
					addmovetogame(localmove, pdn);
				}
				else
					addmovetogame(localmove, nullptr);

				// animate the move:
				cbmove = localmove;

				// if we are in userbook mode, we save the move
				if (CBstate == BOOKADD)
					addmovetouserbook(cbboard8, &localmove);

				display_move(cbboard8, cbmove);
				domove(cbmove, cbboard8);
				cbcolor = CB_CHANGECOLOR(cbcolor);
				clicks.clear();

				// if we are in enter game mode: tell engine to stop
				if (CBstate == OBSERVEGAME)
					SendMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
				newposition = TRUE;
				if (CBstate == NORMAL)
					startengine = TRUE;

				// startengine = TRUE tells the autothread to start the engine
			}
		}

		// if the stone is the color of the side to move, allow it to be selected
		if
		(
			(cbcolor == CB_BLACK && cbboard8[x][y] & CB_BLACK) ||
			(cbcolor == CB_WHITE && cbboard8[x][y] & CB_WHITE)
		) {

			// re-print board to overwrite last selection if there was one
			display_board(cbboard8, x, y);
		}

		// else, reset the click count to 0.
		else {
			display_board(cbboard8);
			clicks.clear();
		}
	}
	else {

		// then its not the first click

		// now, perhaps the user selected another stone; i.e. the second
		// click is ALSO on a stone of the user. then we assume he has changed
		// his mind and now wants to move this stone.
		// if the stone is the color of the side to move, allow it to be selected
		// !! there is one exception to this: a round-trip move such as
		// here [FEN "W:WK14:B19,18,11,10."]
		// however, with the new one-click-move input, this will work fine now!
		if ((cbboard8[x][y] & cbcolor) && clicks.frequency(square) == 1) {

			// set this click to first click
			from = clicks.last();
			clicks.clear();
			clicks.append(from);

			// re-print board to overwrite last selection if there was one
			display_board(cbboard8, clicks);

			// check whether this is an only move
			legal = 0;
			legalmovenumber = 0;
			if (islegal != NULL) {
				if (has_getmovelist || cbgame.gametype == GT_ENGLISH) {

					/* We can do a better job for English since we have a movelist generator. */
					legal = num_matching_moves(cbboard8, cbcolor, clicks, localmove, cbgame.gametype);
				}
				else {
					legalmovenumber = 0;
					for (i = 1; i <= 32; i++) {
						if (islegal(cbboard8, cbcolor, clicks.first(), i, &localmove) != 0) {
							legal++;
							legalmovenumber = i;
						}
					}
				}
			}

			sprintf(statusbar_txt, "");
			if (legal == 1) {

				// only one legal move
				// insert move in the linked list
				if (has_getmovelist || cbgame.gametype == GT_ENGLISH) {
					char pdn[40];
					move_to_pdn_english(cbboard8, cbcolor, &localmove, pdn, cbgame.gametype);
					addmovetogame(localmove, pdn);
				}
				else
					addmovetogame(localmove, nullptr);

				// animate the move:
				cbmove = localmove;

				// if we are in userbook mode, we save the move
				if (CBstate == BOOKADD)
					addmovetouserbook(cbboard8, &localmove);

				// call animation function which will also execute the move
				display_move(cbboard8, cbmove);
				domove(cbmove, cbboard8);
				cbcolor = CB_CHANGECOLOR(cbcolor);

				// if we are in enter game mode: tell engine to stop
				if (CBstate == OBSERVEGAME)
					SendMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
				newposition = TRUE;
				if (CBstate == NORMAL)
					startengine = TRUE;

				// startengine = TRUE tells the autothread to start the engine
				return(0);
			}
			else {
				// and break so as not to execute the rest of this clause, because
				// that is for actually making a move.
				return 0;
			}
		}

		// check move and if ok
		if (islegal != NULL) {
			legal = 0;
			if (has_getmovelist || cbgame.gametype == GT_ENGLISH) {

				/* We can do a better job for English since we have a movelist generator. */
				legal = num_matching_moves(cbboard8, cbcolor, clicks, localmove, cbgame.gametype);
				if (legal > 1) {

					/* Keep the clicks, he needs to add more squares to fully describe the move. */
					display_board(cbboard8, clicks);
					return(1);
				}
			}
			else {
				if (islegal(cbboard8, cbcolor, clicks.first(), clicks.last(), &localmove) != 0)
					legal = 1;
			}
			if (legal == 1) {

				// a legal move!
				// insert move in the game
				if (has_getmovelist || cbgame.gametype == GT_ENGLISH) {
					char pdn[40];
					move_to_pdn_english(cbboard8, cbcolor, &localmove, pdn, cbgame.gametype);
					addmovetogame(localmove, pdn);
				}
				else
					addmovetogame(localmove, nullptr);

				// animate the move:
				cbmove = localmove;

				// if we are in userbook mode, we save the move
				if (CBstate == BOOKADD)
					addmovetouserbook(cbboard8, &localmove);

				// call animation function which will also execute the move
				display_move(cbboard8, cbmove);
				domove(cbmove, cbboard8);
				cbcolor = CB_CHANGECOLOR(cbcolor);

				// if we are in enter game mode: tell engine to stop
				if (CBstate == OBSERVEGAME)
					SendMessage(hwnd, WM_COMMAND, INTERRUPTENGINE, 0);
				newposition = TRUE;
				if (CBstate == NORMAL)
					startengine = TRUE;

				// startengine = TRUE tells the autothread to start the engine
			}
		}

		display_board(cbboard8);
		clicks.clear();
	}

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
	static bool old_two_player_mode;
	static int oldbook_state;
	static int oldengine;
	static int engineIcon;

	if (strcmp(oldstr, statusbar_txt) != 0) {
		strcpy(oldstr, statusbar_txt);
		SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) statusbar_txt);

		// if we're running a test set, create a pseudolog-file
		if (CBstate == RUNTESTSET) {
			if (strchr(statusbar_txt, '=') != NULL) {
				strcpy(filename, CBdocuments);
				PathAppend(filename, "testlog.txt");
				writefile(filename, "a", "%s\n", statusbar_txt);
			}
		}
	}

	// update toolbar to display whose turn it is
	if (oldcolor != cbcolor) {
		if (cbcolor == CB_BLACK)
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) SETUPCC, MAKELPARAM(10, 0));
		else
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) SETUPCC, MAKELPARAM(9, 0));
		oldcolor = cbcolor;
		InvalidateRect(hwnd, NULL, 0);
	}

	// update toolbar to display what mode (normal/2player) we're in
	if (old_two_player_mode != two_player_mode) {
		if (two_player_mode == false)
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEMODE, MAKELPARAM(17, 0));
		else
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEMODE, MAKELPARAM(18, 0));
		old_two_player_mode = two_player_mode;
		InvalidateRect(hwnd, NULL, 0);
	}

	// update toolbar to display book mode (on/off) we're in
	if (oldbook_state != book_state) {
		switch (book_state) {
		case 0:
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEBOOK, MAKELPARAM(3, 0));
			break;

		case 1:
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEBOOK, MAKELPARAM(6, 0));
			break;

		case 2:
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEBOOK, MAKELPARAM(5, 0));
			break;

		case 3:
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEBOOK, MAKELPARAM(4, 0));
			break;
		}

		oldbook_state = book_state;
		InvalidateRect(hwnd, NULL, 0);
	}

	// update toolbar to display active engine (primary/secondary)
	if (oldengine != currentengine) {
		if (currentengine == 1)
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEENGINE, MAKELPARAM(0, 0));
		else
			SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) TOGGLEENGINE, MAKELPARAM(1, 0));
		oldengine = currentengine;
		InvalidateRect(hwnd, NULL, 0);
	}

	refresh_clock(hwnd);
	return 1;
}

int addmovetouserbook(Board8x8 b, CBmove *move)
{
	int i, n;
	FILE *fp;
	pos userbookpos;

	// if we have too many moves, stop!
	// userbooknum is a global, as it is also used in removing stuff from book
	if (userbooknum >= MAXUSERBOOK) {
		sprintf(statusbar_txt, "user book size limit reached!");
		return 0;
	}

	boardtobitboard(b, &userbookpos);

	// check if we already have this position:
	n = userbooknum;
	for (i = 0; i < userbooknum; i++) {
		if
		(
			userbook[i].position.bm == userbookpos.bm &&
			userbook[i].position.bk == userbookpos.bk &&
			userbook[i].position.wm == userbookpos.wm &&
			userbook[i].position.wk == userbookpos.wk
		) {

			// we already have this position!
			// in this case, we overwrite the old entry!
			n = i;
			break;
		}
	}

	userbook[n].position = userbookpos;
	userbook[n].move = *move;
	if (n == userbooknum) {
		(userbooknum)++;
		sprintf(statusbar_txt, "added move to userbook (%d moves)", userbooknum);
	}
	else
		sprintf(statusbar_txt, "replaced move in userbook (%d moves)", userbooknum);

	// save user book
	fp = fopen(userbookname, "wb");
	if (fp != NULL) {
		fwrite(userbook, sizeof(userbookentry), userbooknum, fp);
		fclose(fp);
	}
	else
		sprintf(statusbar_txt, "unable to write to user book");

	return 1;
}

int handlegamereplace(int replaceindex, char *databasename)
{
	std::string gamestring;
	char *dbstring, *p;
	int i;
	FILE *fp;
	READ_TEXT_FILE_ERROR_TYPE etype;

	// give the user a chance to save new results / names
	if (DialogBox(g_hInst, "IDD_SAVEGAME", hwnd, (DLGPROC) DialogFuncSavegame)) {

		// if the user gives his ok, replace:
		// set reindex flag
		reindex = 1;

		// read database into memory */
		dbstring = read_text_file(databasename, etype);
		if (dbstring == NULL) {
			if (etype == RTF_FILE_ERROR)
				sprintf(statusbar_txt, "invalid filename");
			if (etype == RTF_MALLOC_ERROR)
				sprintf(statusbar_txt, "malloc error");
			return 0;
		}

		// rewrite file
		fp = fopen(databasename, "w");

		// get all games up to gameindex and write them into file
		p = dbstring;
		for (i = 0; i < replaceindex; i++) {
			PDNparseGetnextgame(&p, gamestring);
			fprintf(fp, "%s", gamestring.c_str());
		}

		// skip current game
		PDNparseGetnextgame(&p, gamestring);

		// write replaced game
		PDNgametoPDNstring(cbgame, gamestring, "\n");
		if (gameindex != 0)
			fprintf(fp, "\n\n%s", gamestring.c_str());
		else
			fprintf(fp, "%s", gamestring.c_str());

		// and read the rest of the file
		while (PDNparseGetnextgame(&p, gamestring))
			fprintf(fp, "%s", gamestring.c_str());

		fclose(fp);
		if (dbstring != NULL)
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

	if (game_previews.size() == 0) {
		sprintf(statusbar_txt, "no game list to move through");
		return(0);
	}

	for (i = 0; i < (int)game_previews.size(); i++)
		if (gameindex == game_previews[i].game_index)
			break;

	if (i >= (int)game_previews.size())
		return(0);

	if (gameindex != game_previews[i].game_index) {
		sprintf(statusbar_txt, "error while looking for next game...");
		return 0;
	}

	if (i >= (int)game_previews.size() - 1) {
		sprintf(statusbar_txt, "at last game in list");
		return 0;
	}

	gameindex = game_previews[i + 1].game_index;

	// ok, if we arrive here, we have a valid game index for the game to load.
	sprintf(statusbar_txt, "should load game %i", gameindex);

	// load the database into memory
	dbstring = loadPDNdbstring(pdn_filename);

	// extract game from database
	loadgamefromPDNstring(gameindex, dbstring);

	// free up database memory
	free(dbstring);
	sprintf(statusbar_txt, "loaded game %i of %i", i + 2, (int)game_previews.size());

	// return the number of the game we loaded
	return(i + 2);
}

int loadpreviousgame(void)
{
	// load the previous game of the last search.
	char *dbstring;
	int i;

	if (game_previews.size() == 0) {
		sprintf(statusbar_txt, "no game list to move through");
		return(0);
	}

	for (i = 0; i < (int)game_previews.size(); i++)
		if (gameindex == game_previews[i].game_index)
			break;

	if (i >= (int)game_previews.size())
		return(0);

	if (gameindex != game_previews[i].game_index) {
		sprintf(statusbar_txt, "error while looking for next game...");
		return 0;
	}

	if (i == 0) {
		sprintf(statusbar_txt, "at first game in list");
		return 0;
	}

	gameindex = game_previews[i - 1].game_index;

	sprintf(statusbar_txt, "should load game %i", gameindex);
	dbstring = loadPDNdbstring(pdn_filename);
	loadgamefromPDNstring(gameindex, dbstring);
	free(dbstring);
	sprintf(statusbar_txt, "loaded game %i of %i", i, (int)game_previews.size());

	return 0;
}

char *loadPDNdbstring(char *dbname)
{
	// attempts to load the file <dbname> into the
	// string dbstring - checks for existence of that
	// file, allocates enough memory for the file, and loads it.
	char *dbstring;
	READ_TEXT_FILE_ERROR_TYPE etype;

	// read pdn file into memory */
	dbstring = read_text_file(dbname, etype);
	if (dbstring == NULL) {
		if (etype == RTF_FILE_ERROR)
			MessageBox(hwnd,
				   "not a valid database!\nuse game->select database\nto select a valid database",
				   "Error",
				   MB_OK);

		if (etype == RTF_MALLOC_ERROR)
			MessageBox(hwnd, "not enough memory for this operation", "Error", MB_OK);

		SetCurrentDirectory(CBdirectory);
		return 0;
	}

	return dbstring;
}

/*
 * Get headers and moves for this game
 */
void assign_headers(gamepreview &preview, const char *pdn)
{
	const char *tag;
	char header[MAXNAME];
	char headername[MAXNAME], headervalue[MAXNAME];
	char token[1024];

	preview.white[0] = 0;
	preview.black[0] = 0;
	preview.event[0] = 0;
	preview.result[0] = 0;
	preview.date[0] = 0;

	// parse headers
	while (PDNparseGetnextheader(&pdn, header, sizeof(header))) {
		tag = header;
		PDNparseGetnexttoken(&tag, headername, sizeof(headername));
		PDNparseGetnexttag(&tag, headervalue, sizeof(headervalue));
		if (strcmp(headername, "Event") == 0)
			strncpy_terminated(preview.event, headervalue, sizeof(preview.event));
		else if (strcmp(headername, "White") == 0)
			strncpy_terminated(preview.white, headervalue, sizeof(preview.white));
		else if (strcmp(headername, "Black") == 0)
			strncpy_terminated(preview.black, headervalue, sizeof(preview.black));
		else if (strcmp(headername, "Result") == 0)
			strncpy_terminated(preview.result, headervalue, sizeof(preview.result));
		else if (strcmp(headername, "Date") == 0)
			strncpy_terminated(preview.date, headervalue, sizeof(preview.date));
	}

	// headers parsed
	// add the first few moves to the preview structure to display them
	// when the user selects a game.
	sprintf(preview.PDN, "");
	for (int i = 0; i < 48; ++i) {
		if (!PDNparseGetnextPDNtoken(&pdn, token, sizeof(token)))
			break;
		if (strlen(preview.PDN) + strlen(token) < sizeof(preview.PDN) - 1) {
			strcat(preview.PDN, token);
			strcat(preview.PDN, " ");
		}
		else
			break;
	}
}

void get_pdnsearch_stats(std::vector<gamepreview> &previews, RESULT_COUNTS &res)
{
	memset(&res, 0, sizeof(res));
	for (int i = 0; i < (int)previews.size(); ++i) {
		if (strcmp(game_previews[i].result, pdn_result_to_string(BLACK_WIN_RES, gametype())) == 0)
			res.black_wins++;
		else if (strcmp(game_previews[i].result, pdn_result_to_string(DRAW_RES, gametype())) == 0)
			res.draws++;
		else if (strcmp(game_previews[i].result, pdn_result_to_string(WHITE_WIN_RES, gametype())) == 0)
			res.white_wins++;
		else
			res.unknowns++;
	}
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
{
	int i, result;
	static int oldgameindex;
	int entry;
	char *dbstring = NULL;
	std::string gamestring;
	char *p;
	int searchhit;
	gamepreview preview;
	std::vector<int> pos_match_games;	/* Index of games matching the position part of search criteria */

	sprintf(statusbar_txt, "wait ...");
	SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) statusbar_txt);

	// stop engine
	PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);

	if (how == RE_SEARCH) {

		// the easiest: re-display the result of the last search.
		// only possible if there is a last search!
		// re-uses game_previews array.
		if (re_search_ok == 0) {
			sprintf(statusbar_txt, "no old search to re-search!");
			return 0;
		}

		sprintf(statusbar_txt, "re-searching! game_previews.size() is %zd", game_previews.size());

		// load database into dbstring:
		dbstring = loadPDNdbstring(pdn_filename);
	}
	else {

		// if we're looking for a player name, get it
		if (how == SEARCHMASK) {

			// this dialog box sets the variables
			// <playername>, <eventname> and <datename>
			if (DialogBox(g_hInst, "IDD_SEARCHMASK", hwnd, (DLGPROC) DialogSearchMask) == 0)
				return 0;
		}

		// set directory to games directory
		SetCurrentDirectory(cboptions.userdirectory);

		// get a valid database filename. if we already have one, we reuse it,
		// else we prompt the user to select a PDN database
		if (strcmp(pdn_filename, "") == 0) {

			// no valid database name
			// display a dialog box with the available databases
			sprintf(pdn_filename, "%s", cboptions.userdirectory);
			result = getfilename(pdn_filename, OF_LOADGAME);	// 1 on ok, 0 on cancel
			if (!result)
				sprintf(pdn_filename, "");
		}
		else {
			sprintf(statusbar_txt, "database is '%s'", pdn_filename);
			result = 1;
		}

		if (strcmp(pdn_filename, "") != 0 && result) {
			sprintf(statusbar_txt, "loading...");

			// get number of games
			i = PDNparseGetnumberofgames(pdn_filename);
			sprintf(statusbar_txt, "%i games in database", i);

			if
			(
				how == GAMEFIND ||
				how == GAMEFINDTHEME ||
				how == GAMEFINDCR ||
				(how == SEARCHMASK && searchwithposition == 1)
			) {
				pos currentposition;

				// search for a position: this is done by calling pdnopen to index
				// the pdn file, pdnfind to return a list of games with the current position
				if (reindex) {
					sprintf(statusbar_txt, "indexing database...");
					SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) statusbar_txt);

					// index database with pdnopen; fills pdn_positions[].
					pdnopen(pdn_filename, cbgame.gametype);
					reindex = 0;
				}

				// search for games with current position
				// transform the current position into a bitboard:
				boardtobitboard(cbboard8, &currentposition);
				if (how == GAMEFINDCR)
					boardtocrbitboard(cbboard8, &currentposition);

				sprintf(statusbar_txt, "searching database...");
				SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) statusbar_txt);
				if (how == GAMEFIND)
					pdnfind(&currentposition, cbcolor, pos_match_games);
				if (how == SEARCHMASK)
					pdnfind(&currentposition, cbcolor, pos_match_games);
				if (how == GAMEFINDCR)
					pdnfind(&currentposition, CB_CHANGECOLOR(cbcolor), pos_match_games);
				if (how == GAMEFINDTHEME)
					pdnfindtheme(&currentposition, pos_match_games);

				// pos_match_games now contains a list of games matching the position part of search criteria
				if (pos_match_games.size() == 0) {
					sprintf(statusbar_txt, "no games matching position criteria found");
					re_search_ok = 0;
					game_previews.clear();
					SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) statusbar_txt);
					return 0;
				}
				else {
					sprintf(statusbar_txt, "%zd games matching position criteria found", pos_match_games.size());
					re_search_ok = 1;
				}
			}

			SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) statusbar_txt);

			// read database file into buffer 'dbstring'
			dbstring = loadPDNdbstring(pdn_filename);

			p = dbstring;
			entry = 0;
			game_previews.clear();
			for (i = 0; PDNparseGetnextgame(&p, gamestring); ++i) {
				if (how == GAMEFIND || how == GAMEFINDTHEME || how == GAMEFINDCR) {

					// we already know what should go in the list, no point parsing
					if (entry >= (int)pos_match_games.size())
						break;	/* done*/

					if (i != pos_match_games[entry])
						continue;
				}

				/* Parse the game text and fill in the headers of the preview struct. */
				assign_headers(preview, gamestring.c_str());
				preview.game_index = i;

				// now, depending on what we are doing, we add this game to the list of
				// games to display
				// remember: entry is our running variable, from 0...numberofgames in db
				// i is index of game in the pdn database file.
				try {
					switch (how) {
					case GAMEFIND:
					case GAMEFINDCR:
					case GAMEFINDTHEME:
						game_previews.push_back(preview);
						entry++;
						break;

					case GAMELOAD:
						//	remember what game number this has
						game_previews.push_back(preview);
						entry++;
						break;

					case SEARCHMASK:
						// add the entry to the list
						// only if the name matches one of the players
						searchhit = 1;
						if (searchwithposition) {
							if (std::find(pos_match_games.begin(), pos_match_games.end(), i) == pos_match_games.end())
								searchhit = 0;
							else
								searchhit = 1;
						}

						// if a player name to search is set, search for that name
						if (strcmp(playername, "") != 0) {
							if (strstr(preview.black, playername) || strstr(preview.white, playername))
								searchhit &= 1;
							else
								searchhit = 0;
						}

						// if an event name to search is set, search for that event
						if (strcmp(eventname, "") != 0) {
							if (strstr(preview.event, eventname))
								searchhit &= 1;
							else
								searchhit = 0;
						}

						// if a date to search is set, search for that date
						if (strcmp(datename, "") != 0) {
							if (strstr(preview.date, datename))
								searchhit &= 1;
							else
								searchhit = 0;
						}

						// if a comment is defined, search for that comment
						if (strcmp(commentname, "") != 0) {
							if (strstr(gamestring.c_str(), commentname))
								searchhit &= 1;
							else
								searchhit = 0;
						}

						if (searchhit == 1) {
							game_previews.push_back(preview);
							entry++;
						}
						break;
					}
				}

				/* Catch memory allocation errors from push_back(). */
				catch(...) {
					MessageBox(hwnd, "not enough memory for this operation", "Error", MB_OK);
					SetCurrentDirectory(CBdirectory);
					return(0);
				}
			}

			assert(entry == game_previews.size());
			sprintf(statusbar_txt, "%i games found matching search criteria", entry);

			// save old game index
			oldgameindex = gameindex;

			// default game index
			gameindex = 0;
		}
	}

	// headers loaded into 'game_previews', display load game dialog
	if (game_previews.size()) {
		if (DialogBox(g_hInst, "IDD_SELECTGAME", hwnd, (DLGPROC) DialogFuncSelectgame)) {
			if (selected_game < 0 || selected_game >= (int)game_previews.size())
				// dialog box didn't select a proper preview index
				gameindex = oldgameindex;
			else {

				// a game was selected; with index <selected_game> in the dialog box
				sprintf(statusbar_txt, "game previews index is %i", selected_game);

				// transform dialog box index to game index in database
				gameindex = game_previews[selected_game].game_index;

				// load game with index 'gameindex'
				loadgamefromPDNstring(gameindex, dbstring);
			}
		}
		else {

			// dialog box was cancelled
			gameindex = oldgameindex;
		}
	}

	// free up memory
	if (dbstring != NULL)
		free(dbstring);

	SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM) 0, (LPARAM) statusbar_txt);
	return 1;
}

int loadgamefromPDNstring(int gameindex, char *dbstring)
{
	char *p;
	int i;
	std::string gamestring;
	std::string errormsg;

	p = dbstring;
	i = gameindex + 1;
	while (i) {
		if (!PDNparseGetnextgame(&p, gamestring))
			break;
		i--;
	}

	// now the game is in gamestring. use pdnparser routines to convert
	//	it into a cbgame
	if (doload(&cbgame, gamestring.c_str(), &cbcolor, cbboard8, errormsg))
		MessageBox(hwnd, errormsg.c_str(), "Error", MB_OK);

	// game is fully loaded, clean up
	display_board(cbboard8);
	reset_move_history = true;
	newposition = TRUE;
	sprintf(statusbar_txt, "game loaded");
	SetCurrentDirectory(CBdirectory);

	//  only display info if not in analyzepdnmode
	if (CBstate != ANALYZEPDN)
		PostMessage(hwnd, WM_COMMAND, GAMEINFO, 0);
	return 1;
}

int handletooltiprequest(LPTOOLTIPTEXT TTtext)
{
	if (TTtext->hdr.code != TTN_NEEDTEXT)
		return 0;

	switch (TTtext->hdr.idFrom) {
	// set tooltips
	case TOGGLEBOOK:
		TTtext->lpszText = "Change engine book setting";
		break;

	case TOGGLEMODE:
		TTtext->lpszText = "Switch from normal to 2-player mode and back";
		break;

	case TOGGLEENGINE:
		TTtext->lpszText = "Switch from primary to secondary engine and back";
		break;

	case GAMEFIND:
		TTtext->lpszText = "Find Game";
		break;

	case GAMENEW:
		TTtext->lpszText = "New Game";
		break;

	case MOVESBACK:
		TTtext->lpszText = "Take Back";
		break;

	case MOVESFORWARD:
		TTtext->lpszText = "Forward";
		break;

	case MOVESPLAY:
		TTtext->lpszText = "Play";
		break;

	case HELPHELP:
		TTtext->lpszText = "Help";
		break;

	case GAMELOAD:
		TTtext->lpszText = "Load Game";
		break;

	case GAMESAVE:
		TTtext->lpszText = "Save Game";
		break;

	case MOVESBACKALL:
		TTtext->lpszText = "Go to the start of the game";
		break;

	case MOVESFORWARDALL:
		TTtext->lpszText = "Go to the end of the game";
		break;

	case DISPLAYINVERT:
		TTtext->lpszText = "Invert Board";
		break;

	case SETUPCC:
		if (cbcolor == CB_BLACK)
			TTtext->lpszText = "Red to move";
		else
			TTtext->lpszText = "White to move";
		break;

	case BOOKMODE_VIEW:
		TTtext->lpszText = "View User Book";
		break;

	case BOOKMODE_ADD:
		TTtext->lpszText = "Add Moves to User Book";
		break;

	case BOOKMODE_DELETE:
		TTtext->lpszText = "Delete Position from User Book";
		break;

	case HELPHOMEPAGE:
		TTtext->lpszText = "CheckerBoard Homepage";
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
	int i = 0;

	hpop = GetSubMenu(hmenu, 4);
	hsubpop = GetSubMenu(hpop, 3);

	// first, we need to find the subdirectories in the bmp dir
	SetCurrentDirectory(CBdirectory);
	SetCurrentDirectory("bmp");

	FileData.dwFileAttributes = 0;

	hfind = FindFirstFile("*", &FileData);

	DeleteMenu(hsubpop, 0, MF_BYPOSITION);
	while (i < MAXPIECESET) {
		if (hfind == INVALID_HANDLE_VALUE)
			return;

		if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (strlen(FileData.cFileName) > 3 && FileData.cFileName[0] != '.') {

				/* Exclude ".svn" directory. */
				AppendMenu(hsubpop, MF_STRING, PIECESET + i, FileData.cFileName);
				sprintf(piecesetname[i], "%s", FileData.cFileName);
				i++;
				maxpieceset = 1;
			}
		}

		if (FindNextFile(hfind, &FileData) == 0)
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

	// load settings from registry: &cboptions is one key, CBdirectory another.
	loadsettings(&cboptions, CBdirectory);
	SetMenuLanguage(cboptions.language);

	// set appropriate pieceset - load bitmaps for the board
	SendMessage(hwnd, WM_COMMAND, PIECESET + cboptions.piecesetindex, 0);

	// resize window to last window size:
	if (cboptions.window_x != 0) {

		// check if we have stored something
		MoveWindow(hwnd, cboptions.window_x, cboptions.window_y, cboptions.window_width, cboptions.window_height, 1);
	}

	GetClientRect(hwnd, &rect);
	PostMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top));

	initgraphics(hwnd);

	//initialize global that stores the game
	reset_game(cbgame);

	// initialize colors
	ccs.lStructSize = (DWORD) sizeof(CHOOSECOLOR);
	ccs.hwndOwner = (HWND) hwnd;
	ccs.hInstance = (HWND) NULL;
	ccs.rgbResult = RGB(255, 0, 0);
	ccs.lpCustColors = dCustomColors;
	ccs.Flags = CC_RGBINIT | CC_FULLOPEN;
	ccs.lCustData = 0L;
	ccs.lpfnHook = NULL;
	ccs.lpTemplateName = (LPSTR) NULL;

	// load user book
	strcpy(userbookname, CBdocuments);
	PathAppend(userbookname, "userbook.bin");
	fp = fopen(userbookname, "rb");
	if (fp != 0) {
		userbooknum = (int)fread(userbook, sizeof(userbookentry), MAXUSERBOOK, fp);
		fclose(fp);
	}

	setmenuchecks(&cboptions, hmenu);
	checklevelmenu(&cboptions, hmenu, timelevel_to_token(cboptions.level));

	// in case of shell double click
	if (strcmp(savegame_filename, "") != 0) {
		sprintf(pdn_filename, "%s", savegame_filename);
		PostMessage(hwnd, WM_COMMAND, GAMELOAD, 0);
	}

	// support drag and drop
	DragAcceptFiles(hwnd, 1);

	// start autothread
	hAutoThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) AutoThreadFunc, (HWND) 0, 0, &AutoThreadId);
	if (hAutoThread == 0)
		CBlog("failed to start auto thread");

	// and issue a newgame command
	PostMessage(hwnd, WM_COMMAND, GAMENEW, 0);

	// display the engine name in the window title
	sprintf(windowtitle, "loading engine - please wait...");
	sprintf(str2, "CheckerBoard%s: %s", windowtitle, g_app_instance_suffix);
	SetWindowText(hwnd, str2);

	// initialize the board which is stored in board8
	InitCheckerBoard(cbboard8);

	// print version number in status bar
	sprintf(statusbar_txt, "This is CheckerBoard %s, %s", VERSION, PLACE);
	return 1;
}

int showfile(char *filename)
{
	// opens a file with the default viewer, e.g. a html help file
	int error;
	HINSTANCE hinst;

	hinst = ShellExecute(NULL, "open", filename, NULL, NULL, SW_SHOW);
	error = PtrToLong(hinst);

	sprintf(statusbar_txt, "CheckerBoard could not open\nthe file %s\nError code %i", filename, error);
	if (error < 32) {
		if (error == ERROR_FILE_NOT_FOUND)
			strcat(statusbar_txt, ": File not found");
		if (error == SE_ERR_NOASSOC)
			strcat(statusbar_txt, ": no .htm viewer configured");
		MessageBox(hwnd, statusbar_txt, "Error !", MB_OK);
	}
	else
		sprintf(statusbar_txt, "opened %s", filename);
	return 1;
}

int start_user_ballot(int bnum)
{
	char fen[260];

	reset_game(cbgame);
	memcpy(cbboard8, user_ballots[bnum].board, sizeof(cbboard8));
	cbcolor = user_ballots[bnum].color;
	board8toFEN(user_ballots[bnum].board, fen, user_ballots[bnum].color, gametype());
	sprintf(cbgame.FEN, "%s", fen);
	sprintf(cbgame.event, "ballot %d", bnum + 1);
	display_board(cbboard8);
	InvalidateRect(hwnd, NULL, 0);
	newposition = TRUE;
	reset_move_history = true;
	reset_game_clocks();
	return 1;
}

/*
 * Move to the end of the current game.
 */
void forward_to_game_end(void)
{
	while (cbgame.movesindex < (int)cbgame.moves.size()) {
		domove(cbgame.moves[cbgame.movesindex].move, cbboard8);
		cbcolor = CB_CHANGECOLOR(cbcolor);
		++cbgame.movesindex;
	}
}

/*
 * Start a new 3-move game.
 * Set the start position for a 3-move opening.
 * opening_index is the index into the three_move_table[].
 * opening_index is set by random if the user chooses 3-move, 
 * or it can be set by engine match.
 */
int start3move(int opening_index)
{
	int nmatching, color, length, nmoves, gtype;
	const char *p;
	Three_move *ballotp;
	char movestring[20], translated_movestring[60];
	PDNgame game;
	Squarelist squares;
	CBmove matching_move;
	Board8x8 board;
	extern Three_move three_move_table[174];

	assert(opening_index >= 0 && opening_index < 174);
	InitCheckerBoard(board);
	gtype = gametype();
	color = get_startcolor(gtype);
	ballotp = three_move_table + opening_index;
	p = ballotp->moves;
	translated_movestring[0] = 0;

	/* Parse moves and make them to get to the ballot start position. */
	for (nmoves = 0; PDNparseGetnexttoken(&p, movestring, sizeof(movestring)); ++nmoves) {
		PDNparseMove(movestring, squares);
		if (is_row_reversed_gametype(gtype) || get_startcolor(gtype) == CB_WHITE) {
			int status;

			if (get_startcolor(gtype) == CB_WHITE)
				squares.reverse_color();
			if (is_row_reversed_gametype(gtype))
				squares.reverse_rows();
			status = islegal(board, color, squares.first(), squares.last(), &matching_move);
			assert(status);
			move4tonotation(matching_move, movestring);
			sprintf(translated_movestring + strlen(translated_movestring), "%s%s", 
					nmoves == 0 ? "" : " ",
					movestring);
		}
		else {
			nmatching = num_matching_moves(board, color, squares, matching_move, gtype);
			assert(nmatching == 1);
		}
		domove(matching_move, board);
		color = CB_CHANGECOLOR(color);
	}

	if (!is_row_reversed_gametype(gtype) && (get_startcolor(gtype) != CB_WHITE))
		strcpy(translated_movestring, ballotp->moves);
	cbcolor = color;
	memcpy(cbboard8, board, sizeof(board));
	reset_game(cbgame);
	board8toFEN(board, cbgame.FEN, cbcolor, gtype);
	length = sprintf(cbgame.event, "ACF #%d", opening_index + 1);
	if (ballotp->name != nullptr)
		length += sprintf(cbgame.event + length, " (%s)", ballotp->name);
	length += sprintf(cbgame.event + length, ": %s", translated_movestring);

	sprintf(statusbar_txt, cbgame.event);
	InvalidateRect(hwnd, NULL, 0);
	display_board(cbboard8);

	reset_move_history = true;
	reset_game_clocks();
	newposition = TRUE;

	return 1;
}

void format_time_args(double increment, double remaining, uint32_t *info, uint32_t *moreinfo)
{
	int i;
	const double limit = 65535 * 0.1;
	uint16_t increment16, remaining16;
	double mult[] = {0.001, 0.01, 0.1};

	remaining = max(remaining, 0.005);	/* Dont allow negative remaining time. */
	double largest = max(increment, remaining);
	if (largest > limit) {
		largest = min(largest, limit);
		increment = min(increment, limit);
		remaining = min(remaining, limit);
	}

	for (i = 0; i < ARRAY_SIZE(mult); ++i) {
		if (largest <= 65535 * mult[i]) {

			/* Pack the 2 times into a 32-bit int. */
			increment16 = (uint16_t)(increment / mult[i]);
			remaining16 = (uint16_t)(remaining / mult[i]);
			*moreinfo = ((remaining16 << 16) & 0xffff0000) | (increment16 & 0xffff);

			/* Write the multiplier into *info. */
			*info |= (i + 1) << CB_INCR_TIME_SHIFT;
			return;
		}
	}

	assert(0);
}

/*
 * Return a reasonable setting for the maxtime argument to getmove() in incremental time mode.
 * This is only for legacy engines that don't understand incremental settings.
 */
double maxtime_for_incremental_tc(double remaining)
{
	double divisor;

	if (remaining < 0.4 * cboptions.time_increment)
		return(0.4 * cboptions.time_increment);
	if (remaining <= cboptions.time_increment)
		return(remaining);
	if (cboptions.time_increment == 0)
		divisor = 11;
	else if (cboptions.initial_time / cboptions.time_increment >= 50)
		divisor = 10;
	else if (cboptions.initial_time / cboptions.time_increment >= 20)
		divisor = 9;
	else if (cboptions.initial_time / cboptions.time_increment >= 10)
		divisor = 7;
	else if (cboptions.initial_time / cboptions.time_increment >= 5)
		divisor = 5;
	else 
		divisor = 4;
	return(min(remaining, cboptions.time_increment + remaining / divisor));
}


/*
 * Return the maxtime argument to send to the engine when fixed time per move is used in
 * an engine match. 
 */
double maxtime_for_non_incremental_tc(double remaining, double increment)
{
	if (remaining < 0.4 * increment)
		return(0.4 * increment);
	if (remaining > increment)
		return(increment + (remaining - increment) / 2.5);
	return(remaining);
}


/*
 * Keep track of the ratio of maxtime to actual search time for each engine's searches.
 * Write the average ratio to cblog every 100 searches.
 */
void save_time_stats(int enginenum, double maxtime, double elapsed)
{
	static double ratio_sum1, ratio_sum2;
	static int count1, count2;
	const int interval = 100;

	/* Dont count searches that were probably terminated early due to forced moves, etc. */
	if (elapsed < 0.5 * maxtime || elapsed < .04)
		return;
	if (enginenum == 1) {
		ratio_sum1 += elapsed / maxtime;
		++count1;
		if ((count1 % interval) == 0 && count1)
			cblog("%d: primary engine search time ratio %.2f\n", count1, ratio_sum1 / count1);
	}
	else {
		ratio_sum2 += elapsed / maxtime;
		++count2;
		if ((count2 % interval) == 0 && count2)
			cblog("%d: secondary engine search time ratio %.2f\n", count2, ratio_sum2 / count2);
	}
}


DWORD SearchThreadFunc(LPVOID param)
// SearchThreadfunc calls the checkers engine to find a move
// it also logs the return string of the checkers engine
// to a file if CB is in either ANALYZEGAME or ENGINEMATCH mode
{
	int i, nmoves;
	CBmove movelist[MAXMOVES];
	CBmove localmove;
	char PDN[40];
	bool found_move, have_valid_movelist;
	int iscapture;
	pos userbookpos;
	double elapsed, maxtime;

	PDN[0] = 0;
	found_move = false;
	if (cboptions.use_incremental_time && CBstate != ENGINEMATCH && CBstate != AUTOPLAY && CBstate != ENGINEGAME) {

		/* Player must have just made a move.
		 * Subtract his accumulated clock time, add his increment.
		 */
		if (time_ctrl.clock_paused)
			start_clock();
		else {
			if (cbcolor == CB_BLACK)
				/* Player was white. */
				time_ctrl.white_time_remaining += cboptions.time_increment - (clock() - time_ctrl.starttime) / (double)CLK_TCK;
			else
				time_ctrl.black_time_remaining += cboptions.time_increment - (clock() - time_ctrl.starttime) / (double)CLK_TCK;
		}
	}

	abortcalculation = 0;				// if this remains 0, we will execute the move - else not

	/* Test if there is a move at all.
	 * If the engine has the optional "get movelist" engine command then query that, otherwise
	 * checkerboard rather recklessly uses the English checkers built-in movelist generator,
	 * regardless of game type.
	 */
	if (get_movelist_from_engine(cbboard8, cbcolor, movelist, &nmoves, &iscapture)) {
		nmoves = getmovelist(cbcolor, movelist, cbboard8, &iscapture);
		have_valid_movelist = (gametype() == GT_ENGLISH);
	}
	else
		have_valid_movelist = true;
	if (nmoves == 0) {
		if (CBstate == ENGINEMATCH || CBstate == ENGINEGAME || CBstate == AUTOPLAY) {
			gameover = TRUE;
			game_result = CB_LOSS;
			sprintf(statusbar_txt, "game over");
		}
		else
			sprintf(statusbar_txt, "there is no move in this position");

		setenginebusy(FALSE);
		setenginestarting(FALSE);
		return 1;
	}

	// check if this position is in the userbook
	if (cboptions.userbook) {
		boardtobitboard(cbboard8, &userbookpos);
		for (i = 0; i < userbooknum; i++) {
			if (memcmp(&userbookpos, &userbook[i].position, sizeof(userbookpos)) == 0) {

				// we have this position in the userbook!
				cbmove = userbook[i].move;
				found_move = true;
				sprintf(statusbar_txt, "found move in user book");
			}
		}
	}

	if (!found_move) {
		Board8x8 original_board8;		/* The position before calling getmove. */
		Board8x8 engine_board8;			/* The new position returned by getmove. */

		// we did not find a move in our user book, so continue
		// set thread priority
		// next lower ist '_LOWEST', higher '_NORMAL'
		enginethreadpriority = usersetpriority;
		SetThreadPriority(hSearchThread, enginethreadpriority);

		// set directory to CB directory
		SetCurrentDirectory(CBdirectory);

		//--------------------------------------------------------------//
		//						do a search								//
		//--------------------------------------------------------------//
		if (getmove != NULL) {
			uint32_t info, moreinfo;	/* arguments sent to engine. */

			/* Display the Play! bitmap with red foreground when the engine is searching. */
			PostMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) MOVESPLAY, MAKELPARAM(19, 0));

			info = 0;
			moreinfo = 0;
			if (reset_move_history)
				info |= CB_RESET_MOVES;
			if (cboptions.use_incremental_time) {
				double increment = cboptions.time_increment;
				if (cboptions.handicap_enable && currentengine == 2)
					increment *= cboptions.handicap_mult;
				if (cbcolor == CB_BLACK) {
					format_time_args(increment, time_ctrl.black_time_remaining, &info, &moreinfo);
					maxtime = maxtime_for_incremental_tc(time_ctrl.black_time_remaining);
				}
				else {
					format_time_args(increment, time_ctrl.white_time_remaining, &info, &moreinfo);
					maxtime = maxtime_for_incremental_tc(time_ctrl.white_time_remaining);
				}
			}
			else if (CBstate == ENGINEMATCH) {
				maxtime = timelevel_to_time(cboptions.level);

				// if in engine match handicap mode, give adjust secondary engine's time.
				if (cboptions.handicap_enable && currentengine == 2)
					maxtime *= cboptions.handicap_mult;

				if (cbcolor == CB_BLACK) {
					time_ctrl.black_time_remaining += maxtime;
					maxtime = maxtime_for_non_incremental_tc(time_ctrl.black_time_remaining, maxtime);
				}
				else {
					time_ctrl.white_time_remaining += maxtime;
					maxtime = maxtime_for_non_incremental_tc(time_ctrl.white_time_remaining, maxtime);
				}
			}
			else {
				if (cboptions.exact_time)
					info |= CB_EXACT_TIME;

				maxtime = timelevel_to_time(cboptions.level);
			}

			/* Send the most recent reversible moves so the engine can detect repetition draws. */
			send_game_history(cbgame, cbboard8, cbcolor);

			memcpy(original_board8, cbboard8, sizeof(cbboard8));
			memcpy(engine_board8, cbboard8, sizeof(cbboard8));
			start_clock();
			game_result = (getmove)(engine_board8, cbcolor, maxtime, statusbar_txt, &playnow, info, moreinfo, &localmove);
			elapsed = (clock() - time_ctrl.starttime) / (double)CLK_TCK;

			/* Display the Play! bitmap with black foreground when the engine is not searching. */
			PostMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM) MOVESPLAY, MAKELPARAM(2, 0));

#ifdef LOG_CUMULATIVE_TIME_USED
			if (CBstate == ENGINEMATCH) {
				time_ctrl.cumulative_time_used[currentengine] += elapsed;
				++time_ctrl.searchcount;
				if ((time_ctrl.searchcount & 127) == 0)
					cblog("Total time used (engine 1, engine 2): %.2f min, %.2f min\n",
						time_ctrl.cumulative_time_used[1] / 60, time_ctrl.cumulative_time_used[2] / 60);
			}
#endif
			if (cboptions.use_incremental_time) {
				double increment = cboptions.time_increment;

				if (CBstate == ENGINEMATCH && cboptions.handicap_enable && currentengine == 2)
					increment *= cboptions.handicap_mult;

				if (cbcolor == CB_BLACK) {
					if (time_ctrl.black_time_remaining - elapsed < 0)
						cblog("engine %d has negative remaining time %.3f\n", currentengine, time_ctrl.black_time_remaining - elapsed);
					time_ctrl.black_time_remaining += increment - elapsed;
				}
				else {
					if (time_ctrl.white_time_remaining - elapsed < 0)
						cblog("engine %d has negative remaining time %.3f\n", currentengine, time_ctrl.white_time_remaining - elapsed);
					time_ctrl.white_time_remaining += increment - elapsed;
				}

				/* If not engine match, then human player's clock starts now. For engine matches the
				 * starttime will be set again just before calling getmove().
				 */
				time_ctrl.starttime = clock();
			}
			else if (CBstate == ENGINEMATCH) {
				if (cbcolor == CB_BLACK)
					time_ctrl.black_time_remaining -= elapsed;
				else
					time_ctrl.white_time_remaining -= elapsed;

				/* Keep track of how well the engine's average search time tracks the maxtime param. */
#ifdef LOG_CUMULATIVE_TIME_USED
				save_time_stats(currentengine, maxtime, elapsed);
#endif
			}
		}
		else
			sprintf(statusbar_txt, "error: no engine defined!");

		// reset playnow immediately
		playnow = 0;

		// now, we execute the move on the board, but only if we are not in observe or analyze mode
		// in observemode, the user will provide all moves, in analyse mode the autothread drives the
		// game forward
		if (CBstate != OBSERVEGAME && CBstate != ANALYZEGAME && CBstate != ANALYZEPDN && !abortcalculation)
			memcpy(cbboard8, engine_board8, sizeof(cbboard8));

		// got board8 & a copy before move was made
		if (CBstate != OBSERVEGAME && CBstate != ANALYZEGAME && CBstate != ANALYZEPDN && !abortcalculation) {

			// determine the move that was made: we only do this if we have a real movelist
			// else the engine must return the appropriate information in localmove
			if (have_valid_movelist) {
				cbmove = movelist[0];
				for (i = 0; i < nmoves; i++) {
					Board8x8 temp_board;

					//put original board8 in a local copy, execute move and compare with returned board8...
					memcpy(temp_board, original_board8, sizeof(temp_board));
					domove(movelist[i], temp_board);
					if (memcmp(cbboard8, temp_board, sizeof(cbboard8)) == 0) {
						cbmove = movelist[i];
						found_move = true;
						move_to_pdn_english(nmoves, movelist, &cbmove, PDN, gametype());
						break;
					}
				}

				if (!found_move)
					memcpy(cbboard8, original_board8, sizeof(cbboard8));
			}
			else {

				// gametype not GT_ENGLISH and we don't have a real movelist, use the move of the engine
				cbmove = localmove;
				move4tonotation(localmove, PDN);
				memcpy(cbboard8, original_board8, sizeof(cbboard8));
				found_move = true;
			}
		}

		// ************* finished determining what move was made....
	}

	// Update logfiles.
	// if CBstate is ANALYZEGAME, we have to print the analysis to a logfile,
	// make the move played in the game & also print it into the logfile
	switch (CBstate) {
	case ANALYZEPDN:					// drop through to analyzegame
	case ANALYZEGAME:
		// don't add analysis if there is only one move
		if (nmoves == 1)
			break;

		if (cbgame.movesindex < (int)cbgame.moves.size())
			sprintf(cbgame.moves[cbgame.movesindex].analysis, "%s", statusbar_txt);
		break;

	case ENGINEMATCH:
		{
			char filename[MAX_PATH];
			char name[250];
			double total;

			/* Elapsed has already been subtracted from time_remaining, but we want to print the engine's
			 * clock time at the beginning of its turn.
			 */
			if (cbcolor == CB_BLACK)
				total = time_ctrl.black_time_remaining + elapsed;
			else
				total = time_ctrl.white_time_remaining + elapsed;

			emlog_filename(filename);
			enginename(name);
			if (cboptions.use_incremental_time)
				writefile(filename, "a", "%s played %s; Times (clock, 'maxtime' sent, used): %.3f, %.3f, %.3f\nanalysis: %s\n", 
					name, PDN, total, maxtime, elapsed, statusbar_txt);
			else
				writefile(filename, "a", "%s played %s; Times (clock, given to engine, used): %.3f, %.3f, %.3f\nanalysis: %s\n",
					name, PDN, total, maxtime, elapsed, statusbar_txt);
		}
		break;
	}

	// Add the move to the UI's current game.
	// Save the game search string as a comment if addcomment is true or
	// if ENGINEMATCH and the engine claimed a result.
	// Play a sound if that option is on.
	// Start the animation thread to animate the move and play it on the UI's board.
	if ((CBstate != OBSERVEGAME) && (CBstate != ANALYZEGAME) && (CBstate != ANALYZEPDN) && found_move && !abortcalculation) {
		bool add_gameover_comment = false;
		bool is_draw_by_repetition = false;
		bool is_draw_by_40move_rule = false;

		addmovetogame(cbmove, PDN);		/* Add the move to the current game. */
		detect_nonconversion_draws(cbgame, &is_draw_by_repetition, &is_draw_by_40move_rule);
		if (is_draw_by_repetition || is_draw_by_40move_rule)
			gameover = TRUE;
		else if (CBstate == ENGINEMATCH) {
			// If we are in ENGINEMATCH state and the engine claims a result then we stop.
			if (game_result == CB_DRAW) {
				gameover = TRUE;
				add_gameover_comment = true;
			}
			else {
				if (cboptions.early_game_adjudication && game_result != CB_UNKNOWN) {
					gameover = TRUE;
					add_gameover_comment = true;
				}
			}
		}

		// save engine string as comment if it's an engine match
		// actually, always save if add comment is on
		// For enginematch, add comments for draws by 40-move rule or 3-fold repetition. 
		if ((addcomment || add_gameover_comment || is_draw_by_repetition || is_draw_by_40move_rule) && cbgame.movesindex > 0) {
			gamebody_entry *pgame = &cbgame.moves[cbgame.movesindex - 1];
			strncpy(pgame->comment, statusbar_txt, COMMENTLENGTH - 1);
			pgame->comment[COMMENTLENGTH - 1] = 0;

			if (add_gameover_comment) {
				strncat(pgame->comment, " : gameover claimed", COMMENTLENGTH - 1);
				pgame->comment[COMMENTLENGTH - 1] = 0;
			}

			if (is_draw_by_repetition) {
				strncat(pgame->comment, "; CB declared draw by 3-fold repetition", COMMENTLENGTH - 1);
				pgame->comment[COMMENTLENGTH - 1] = 0;
				game_result = CB_DRAW;
			}

			if (is_draw_by_40move_rule) {
				strncat(pgame->comment, "; CB declared draw by 40-move rule", COMMENTLENGTH - 1);
				pgame->comment[COMMENTLENGTH - 1] = 0;
				game_result = CB_DRAW;
			}
		}

		// if sound is on we make a beep
		if (cboptions.sound)
			PlaySound("start.wav", NULL, SND_FILENAME | SND_ASYNC);

		display_move(cbboard8, cbmove);
		domove(cbmove, cbboard8);
		cbcolor = CB_CHANGECOLOR(cbcolor);
	}

	reset_move_history = false;
	setenginebusy(FALSE);
	setenginestarting(FALSE);
	return 1;
}

bool read_user_ballots_file(void)
{
	char *pdnstring, *p;
	PDNgame game;
	BALLOT_INFO ballot;
	READ_TEXT_FILE_ERROR_TYPE etype;
	std::string gamestring;
	std::string errormsg;

	user_ballots.clear();

	// read pdn file into memory */
	pdnstring = read_text_file(cboptions.start_pos_filename, etype);
	if (pdnstring == NULL) {
		if (etype == RTF_FILE_ERROR)
			MessageBox(hwnd, "Cannot open start positions file", "Error", MB_OK);

		if (etype == RTF_MALLOC_ERROR)
			MessageBox(hwnd, "not enough memory for this operation", "Error", MB_OK);

		return(true);
	}

	p = pdnstring;
	while (1) {
		if (!PDNparseGetnextgame(&p, gamestring))
			break;

		/* The game is in gamestring. Parse it into a PDNgame. */
		if (doload(&game, gamestring.c_str(), &ballot.color, ballot.board, errormsg)) {
			errormsg = "Start position #" + std::to_string(1 + user_ballots.size()) + "\n" + errormsg;
			errormsg += "\n\nEngine match will be aborted.";
			MessageBox(hwnd, errormsg.c_str(), "Error", MB_OK);
			free(pdnstring);
			return(true);
		}

		/* Extract the Event header. */
		ballot.event = game.event;

		/* Move to the last position in the game. */
		for (int i = 0; i < (int)game.moves.size(); ++i) {
			int status;
			Squarelist squares;
			CBmove move;

			PDNparseMove(game.moves[i].PDN, squares);
			status = islegal_check(ballot.board, ballot.color, squares, &move, gametype());
			if (status) {
				game.moves[i].move = move;
				ballot.color = CB_CHANGECOLOR(ballot.color);
				domove(move, ballot.board);
			}
		}

		user_ballots.push_back(ballot);
	}
	free(pdnstring);
	return(false);
}

int changeCBstate(int newstate)
{
	// changes the state of Checkerboard from old state to new state
	// does whatever is necessary to do this - checks/unchecks menu buttons
	// changes state of buttons in toolbar etc.
	// when we change the state, we first of all make sure that the
	// engine is not running
	PostMessage(hwnd, WM_COMMAND, ABORTENGINE, 0);

	// toolbar buttons
	if (CBstate == BOOKVIEW)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM) BOOKMODE_VIEW, MAKELONG(0, 0));
	if (CBstate == BOOKADD)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM) BOOKMODE_ADD, MAKELONG(0, 0));

	CBstate = (enum state)newstate;

	// toolbar buttons
	if (CBstate == BOOKVIEW)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM) BOOKMODE_VIEW, MAKELONG(1, 0));
	if (CBstate == BOOKADD)
		SendMessage(tbwnd, TB_CHECKBUTTON, (WPARAM) BOOKMODE_ADD, MAKELONG(1, 0));

	// mode menu checks are taken care of in autothread function
	// set menu
	CheckMenuItem(hmenu, CM_NORMAL, MF_UNCHECKED);
	CheckMenuItem(hmenu, CM_ANALYSIS, MF_UNCHECKED);
	CheckMenuItem(hmenu, CM_AUTOPLAY, MF_UNCHECKED);
	CheckMenuItem(hmenu, CM_ENGINEMATCH, MF_UNCHECKED);
	CheckMenuItem(hmenu, CM_2PLAYER, MF_UNCHECKED);
	CheckMenuItem(hmenu, BOOKMODE_VIEW, MF_UNCHECKED);
	CheckMenuItem(hmenu, BOOKMODE_ADD, MF_UNCHECKED);

	/* Update animation state. */
	if (CBstate == ENGINEMATCH) {
		if (cboptions.use_incremental_time) {
			if (cboptions.initial_time / 30 + cboptions.time_increment <= 1.5)
				set_animation(false);
			else
				set_animation(true);
		}
		else {
			if (timelevel_to_time(cboptions.level) <= 1)
				set_animation(false);
			else
				set_animation(true);
		}
	}
	else
		set_animation(true);

	switch (CBstate) {
	case NORMAL:
		two_player_mode = false;
		CheckMenuItem(hmenu, CM_NORMAL, MF_CHECKED);
		break;

	case OBSERVEGAME:
		two_player_mode = false;
		CheckMenuItem(hmenu, CM_ANALYSIS, MF_CHECKED);
		break;

	case AUTOPLAY:
		two_player_mode = false;
		CheckMenuItem(hmenu, CM_AUTOPLAY, MF_CHECKED);
		break;

	case ENGINEMATCH:
		/* Read ballots file if being used. */
		two_player_mode = false;
		if (cboptions.em_start_positions == START_POS_FROM_FILE) {
			if (read_user_ballots_file()) {
				changeCBstate(NORMAL);
				break;
			}
		}

		CheckMenuItem(hmenu, CM_ENGINEMATCH, MF_CHECKED);
		if (cboptions.early_game_adjudication)
			maxmovecount = 200;
		else
			maxmovecount = 300;

		break;

	case ENTERGAME:
		two_player_mode = true;
		CheckMenuItem(hmenu, CM_2PLAYER, MF_CHECKED);
		break;

	case BOOKVIEW:
		CheckMenuItem(hmenu, BOOKMODE_VIEW, MF_CHECKED);
		break;

	case BOOKADD:
		CheckMenuItem(hmenu, BOOKMODE_ADD, MF_CHECKED);
		break;
	}

	// clear status bar
	sprintf(statusbar_txt, "");
	return 1;
}

/*
 * Do a quick search with both engines to init the endgame dbs
 * before starting an engine match.
 */
void quick_search_both_engines()
{
	int playnow;
	CBmove move;

	newgame();
	playnow = 0;
	SetCurrentDirectory(CBdirectory);
	setcurrentengine(1);
	send_game_history(cbgame, cbboard8, cbcolor);
	getmove(cbboard8, cbcolor, 0.1, statusbar_txt, &playnow, 0, 0, &move);
	newgame();
	playnow = 0;
	SetCurrentDirectory(CBdirectory);
	setcurrentengine(2);
	send_game_history(cbgame, cbboard8, cbcolor);
	getmove(cbboard8, cbcolor, 0.1, statusbar_txt, &playnow, 0, 0, &move);
}

void emstats_filename(char *filename)
{
	sprintf(filename, "%s\\stats%s.txt", cboptions.matchdirectory, g_app_instance_suffix);
}

void emprogress_filename(char *filename)
{
	sprintf(filename, "%s\\match_progress%s.txt", cboptions.matchdirectory, g_app_instance_suffix);
}

void empdn_filename(char *filename)
{
	sprintf(filename, "%s\\match%s.pdn", cboptions.matchdirectory, g_app_instance_suffix);
}

void emlog_filename(char *filename)
{
	sprintf(filename, "%s\\matchlog%s.txt", cboptions.matchdirectory, g_app_instance_suffix);
}

/*
 * Read engine match stats.txt file, write stats to emstats struct.
 * Return the number of games that have been played.
 */
int read_match_stats(void)
{
	FILE *fp;
	char filename[MAX_PATH];
	char linebuf[150];
	int count;

	emstats.games = 0;
	emstats_filename(filename);
	fp = fopen(filename, "r");
	if (fp != NULL) {

		// stats.txt exists
		// read first line just to read second line, which holds the actual stats
		fgets(linebuf, sizeof(linebuf), fp);
		fgets(linebuf, sizeof(linebuf), fp);
		count = sscanf(linebuf,
				"+:%i =:%i -:%i unknown:%i +B:%i -B:%i",
				&emstats.wins,
				&emstats.draws,
				&emstats.losses,
				&emstats.unknowns,
				&emstats.blackwins,
				&emstats.blacklosses);
		fclose(fp);
		if (count == 6)
			emstats.games = emstats.wins + emstats.losses + emstats.draws + emstats.unknowns;
		else {
			emstats.games = 0;
			emstats.wins = 0;
			emstats.losses = 0;
			emstats.draws = 0;
			emstats.unknowns = 0;
			emstats.blackwins = 0;
			emstats.blacklosses = 0;
		}
	}
	return(emstats.games);
}

int num_ballots(void)
{
	if (cboptions.em_start_positions == START_POS_FROM_FILE)
		return((int)user_ballots.size());
	else
		return(num_3move_ballots(&cboptions));
}

bool match_is_resumable(void)
{
	int ngames;

	ngames = read_match_stats();
	if (ngames <= 0)
		return(false);

	if (cboptions.em_start_positions == START_POS_FROM_FILE || ngames < 2 * num_ballots() * cboptions.match_repeat_count)
		return(true);
	else
		return(false);
}

void reset_match_stats(void)
{
	char filename[MAX_PATH];

	emstats_filename(filename);
	DeleteFile(filename);
	emprogress_filename(filename);
	DeleteFile(filename);
	empdn_filename(filename);
	DeleteFile(filename);
	emlog_filename(filename);
	DeleteFile(filename);
	emstats.blacklosses = 0;
	emstats.blackwins = 0;
	emstats.draws = 0;
	emstats.games = 0;
	emstats.losses = 0;
	emstats.unknowns = 0;
	emstats.wins = 0;
	time_ctrl.cumulative_time_used[1] = 0;
	time_ctrl.cumulative_time_used[2] = 0;
	time_ctrl.searchcount = 0;
}

/* Given a 0-based game number, return a 0-based ballot number. */
int game0_to_ballot0(int game0)
{
	return((game0 / 2) % num_ballots());
}

/* Given a 0-based game number, return a 0-based match number. */
int game0_to_match0(int game0)
{
	return(game0 / (2 * num_ballots()));
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
	//  it uses the boolean enginebusy to
	//  detect if it is allowed to do anything
	char Lstr[256];
	char windowtitle[256];
	char analysisfilename[MAX_PATH];
	char testlogname[MAX_PATH];
	char testsetname[MAX_PATH];
	char statsfilename[MAX_PATH];
	FILE *Lfp;
	static int gamenumber;
	static int movecount;
	int i;
	static char FEN[256];
	char engine1[MAXNAME], engine2[MAXNAME];	// holds engine names
	int matchcontinues = 0;

	// autothread is started at startup, and keeps running until the program
	// terminates, that's what for(;;) is here for.
	for (;;) {

		// thread sleeps for AUTOSLEEPTIME (10ms) so that this loop runs at
		// approximately 100x per second
		Sleep(AUTOSLEEPTIME);

		// if CB is doing something else, wait for it to finish by dropping back to sleep command above
		if (getanimationbusy() || getenginebusy() || getenginestarting())
			continue;

		switch (CBstate) {
		case NORMAL:
			if (startengine) {

				/* after determining the user move startengine flag is set and the move is animated. */
				setenginestarting(TRUE);
				PostMessage(hwnd, (UINT) WM_COMMAND, (WPARAM) MOVESPLAY, (LPARAM) 0);
				startengine = FALSE;
			}
			break;

		case RUNTESTSET:
			// sleep for 0.2 seconds to allow handletimer() to update the testlog file
			Sleep(200);

			// create or update testlog file
			strcpy(testlogname, CBdocuments);
			PathAppend(testlogname, "testlog.txt");
			if (testset_number == 0) {

				// testset start: clear file testlog.txt
				Lfp = fopen(testlogname, "w");
				fclose(Lfp);
			}
			else
				// write analysis
				logtofile(testlogname, "\n\n", "a");

			// load the next position from the test set
			strcpy(testsetname, CBdocuments);
			PathAppend(testsetname, "testset.txt");
			Lfp = fopen(testsetname, "r");
			if (Lfp == NULL) {
				sprintf(statusbar_txt, "could not find %s", testsetname);
				break;
			}

			for (i = 0; i <= testset_number; i++) {
				fgets(FEN, 255, Lfp);
				if (feof(Lfp)) {
					changeCBstate(NORMAL);
				}
			}

			fclose(Lfp);
			testset_number++;

			// write FEN in testlog
			sprintf(statusbar_txt, "#%i: %s", testset_number, FEN);
			logtofile(testlogname, statusbar_txt, "a");

			// convert position to internal board
			FENtoboard8(cbboard8, FEN, &cbcolor, cbgame.gametype);
			display_board(cbboard8);
			reset_move_history = true;
			newposition = TRUE;
			setenginestarting(TRUE);
			PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);
			Sleep(SLEEPTIME);
			break;

		case AUTOPLAY:
			// check if game is over, if yes, go from autoplay to normal state
			if (gameover == TRUE) {
				gameover = FALSE;
				changeCBstate(NORMAL);
				sprintf(statusbar_txt, "game over");
			}
			else {
				// else continue game by sending a play command
				setenginestarting(TRUE);
				PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);

				// sleep a bit to allow engine to start
				Sleep(SLEEPTIME);
			}
			break;

		case ENGINEGAME:
			if (gameover == TRUE) {
				gameover = FALSE;
				changeCBstate(NORMAL);
				setcurrentengine(1);
				break;
			}

			if (startmatch == TRUE)
				startmatch = FALSE;
			else {
				togglecurrentengine();
				enginename(Lstr);
				sprintf(statusbar_txt, "CheckerBoard%s: ", g_app_instance_suffix);
				strcat(statusbar_txt, Lstr);
				SetWindowText(hwnd, statusbar_txt);
			}

			setenginestarting(TRUE);
			PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);

			break;

		case ANALYZEGAME:
			if (gameover == TRUE) {
				gameover = FALSE;
				changeCBstate(NORMAL);
				sprintf(statusbar_txt, "Game analysis finished!");
				strcpy(analysisfilename, CBdocuments);
				PathAppend(analysisfilename, "analysis");
				PathAppend(analysisfilename, "analysis.htm");
				makeanalysisfile(analysisfilename);
				break;
			}

			if (currentengine != 1)
				setcurrentengine(1);

			PostMessage(hwnd, WM_COMMAND, MOVESFORWARDALL, 0);
			Sleep(SLEEPTIME);

			// start analysis logfile - overwrite anything old
			strcpy(analysisfilename, CBdocuments);
			PathAppend(analysisfilename, "analysis.txt");
			Lfp = fopen(analysisfilename, "w");
			fclose(Lfp);
			sprintf(statusbar_txt, "played in game: 1. %s", cbgame.moves[cbgame.movesindex - 1].PDN);
			logtofile(analysisfilename, statusbar_txt, "a");

			setenginestarting(TRUE);
			PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);

			// go into a for loop until the game is completely analyzed
			for (;;) {
				Sleep(SLEEPTIME);
				if ((CBstate != ANALYZEGAME) || (gameover == TRUE))
					break;
				if (!getenginebusy() && !getanimationbusy()) {
					PostMessage(hwnd, WM_COMMAND, MOVESBACK, 0);
					if (CBstate == ANALYZEGAME && gameover == FALSE) {
						setenginestarting(TRUE);
						PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);
					}
				}
			}
			break;

		case ANALYZEPDN:
			if (startmatch == TRUE) {

			// this is the case when the user chooses analyzepdn in the menu;
			// at this point it is true for the first and only time in the
			// analyzepdn mode.
				gamenumber = 1;
				startmatch = FALSE;
				strcpy(analysisfilename, CBdocuments);
				PathAppend(analysisfilename, "analysis");
				PathAppend(analysisfilename, "analysis1.htm");
			}

			if (gameover == TRUE) {
				gameover = FALSE;
				makeanalysisfile(analysisfilename);

				// get number of next game; loadnextgame returns 0 if
				// there is no further game.
				gamenumber = loadnextgame();
				sprintf(analysisfilename, "%s\\analysis\\analysis%i.htm", CBdocuments, gamenumber);
			}

			if (gamenumber == 0) {

				// we're done with the file
				changeCBstate(NORMAL);
				sprintf(statusbar_txt, "PDN analysis finished!");
				break;
			}

			// this is the signal that we are at the start of the analysis of a game
			if (currentengine != 1)
				setcurrentengine(1);

			PostMessage(hwnd, WM_COMMAND, MOVESFORWARDALL, 0);
			setenginestarting(TRUE);
			PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);

			// analyze entire game in this for loop
			for (;;) {
				Sleep(SLEEPTIME);
				if ((CBstate != ANALYZEPDN) || (gameover == TRUE))
					break;
				if (!getenginebusy() && !getanimationbusy()) {
					PostMessage(hwnd, WM_COMMAND, MOVESBACK, 0);

					// give the main thread some time to stop analysis if
					// we are at the end of the game
					if (CBstate == ANALYZEPDN && gameover == FALSE) {
						setenginestarting(TRUE);
						PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);
					}
				}
			}
			break;

		case OBSERVEGAME:
			// start engine if we have a new position.
			if (newposition) {
				playnow = 0;
				setenginestarting(TRUE);
				PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);
				newposition = 0;
			}
			break;

		case ENGINEMATCH:
			if (startmatch) {

				/* We've already initialized the emstats. */
				gamenumber = 0;
				if (emstats.games) {
					gamenumber = emstats.games;
					sprintf(statusbar_txt,
							"resuming match at game #%i, (+:%i -:%i =:%i unknown:%i)",
							emstats.games + 1,
							emstats.wins,
							emstats.losses,
							emstats.draws,
							emstats.unknowns);
				}

				// finally, display stats in window title
				SetCurrentDirectory(CBdirectory);
				enginecommand1("name", engine1);
				SetCurrentDirectory(CBdirectory);
				enginecommand2("name", engine2);

				/* Restrict engine names in title to 30 chars. */
				if (strlen(engine1) > 30)
					sprintf(engine1 + 30, "...");
				if (strlen(engine2) > 30)
					sprintf(engine2 + 30, "...");

				sprintf(windowtitle, "%s - %s", engine1, engine2);
				sprintf(Lstr, ": W-L-D:%i-%i-%i", emstats.wins, emstats.losses, emstats.draws + emstats.unknowns);
				strcat(windowtitle, Lstr);
				SetWindowText(hwnd, windowtitle);

				/* Do a quick search with each engine to init endgame dbs if not already done. */
				quick_search_both_engines();

			}	// end if startmatch

			// stuff below is for regular games
			// stop games which have been going for too long
			if (movecount > maxmovecount)
				gameover = TRUE;

			// when a game is terminated, save result and save game
			if ((gameover || startmatch == TRUE)) {
				if (gameover) {
					int round_gamenumber;

					// set white and black players
					if (gamenumber % 2) {
						sprintf(cbgame.black, "%s", engine1);
						sprintf(cbgame.white, "%s", engine2);
					}
					else {
						sprintf(cbgame.black, "%s", engine2);
						sprintf(cbgame.white, "%s", engine1);
					}

					sprintf(cbgame.resultstring, "?");
					round_gamenumber = 1 + ((gamenumber - 1) % (2 * num_ballots()));
					if (!((round_gamenumber - 1) % 20)) {
						if (cboptions.match_repeat_count > 1 && round_gamenumber == 1) {
							emprogress_filename(statsfilename);
							if (gamenumber != 1)
								writefile(statsfilename, "a", "\n");

							writefile(statsfilename, "a", "Match %d", 1 + (gamenumber - 1) / (2 * num_ballots()));
						}
						if (gamenumber != 1 || cboptions.match_repeat_count > 1) {
							emprogress_filename(statsfilename);
							writefile(statsfilename, "a", "\n");
						}
					}

					/* If the first game of a new ballot, write the ballot number. */
					if (gamenumber % 2) {
						emprogress_filename(statsfilename);
						if (cboptions.em_start_positions == START_POS_FROM_FILE)
							writefile(statsfilename, "a", "%4i:", 1 + (round_gamenumber - 1) / 2);
						else
							writefile(statsfilename, "a", "%3i:", emstats.opening_index + 1);
					}

					update_match_stats(game_result, movecount, gamenumber, &emstats);

					// finally, display stats in window title
					sprintf(windowtitle, "%s - %s", engine1, engine2);
					sprintf(Lstr, ": W-L-D:%i-%i-%i", emstats.wins, emstats.losses, emstats.draws + emstats.unknowns);
					strcat(windowtitle, Lstr);
					SetWindowText(hwnd, windowtitle);
					if (!(gamenumber % 2)) {
						emprogress_filename(statsfilename);
						writefile(statsfilename, "a", "  ");
					}

					// write match statistics
					emstats_filename(statsfilename);
					Lfp = fopen(statsfilename, "w");
					if (Lfp != NULL) {
						fprintf(Lfp, "%s - %s", engine1, engine2);
						fprintf(Lfp, " %s\n", Lstr);
						fprintf(Lfp,
								"+:%i =:%i -:%i unknown:%i +B:%i -B:%i",
								emstats.wins,
								emstats.draws,
								emstats.losses,
								emstats.unknowns,
								emstats.blackwins,
								emstats.blacklosses);
						fclose(Lfp);
					}

					// save the game
					// gamenumber is base 1 here.
					std::string event;
					if (cboptions.match_repeat_count > 1)
						event = "match " + std::to_string(1 + game0_to_match0(gamenumber - 1)) + ", ";

					event += "game " + std::to_string(round_gamenumber) + ": ";
					if (cboptions.em_start_positions == START_POS_3MOVE)
						event += cbgame.event;
					else
						event += user_ballots[game0_to_ballot0(gamenumber - 1)].event;
					sprintf(cbgame.event, event.c_str());

					/* Save the Event text to the matchlog file. */
					emlog_filename(statsfilename);
					writefile(statsfilename, "a", "---------- end of %s\n\n", cbgame.event);

					// dosave expects a fully initialized cbgame structure
					empdn_filename(savegame_filename);
					SendMessage(hwnd, WM_COMMAND, DOSAVE, 0);

					Sleep(SLEEPTIME);
				}

				// set startmatch to FALSE, it is only true when the match starts to initialize
				startmatch = FALSE;

				// get the opening for the gamenumber, and check whether the match is over
				// gamenumber is base 0 here.
				if (gamenumber >= 2 * num_ballots() * cboptions.match_repeat_count)
					matchcontinues = 0;
				else {
					matchcontinues = 1;
					if (cboptions.em_start_positions == START_POS_FROM_FILE)
						start_user_ballot(game0_to_ballot0(gamenumber));
					else {
						emstats.opening_index = get_3move_index(game0_to_ballot0(gamenumber), &cboptions);
						assert(emstats.opening_index >= 0);
					}
				}

				// move on to the next game
				movecount = 0;
				gameover = FALSE;
				gamenumber++;

				sprintf(statusbar_txt, "gamenumber is %i\n", gamenumber);

				if (matchcontinues == 0) {
					changeCBstate(NORMAL);
					setcurrentengine(1);

					// write final result in window title bar
					sprintf(Lstr, "Final result of %s", windowtitle);
					SetWindowText(hwnd, Lstr);
					break;
				}

				// The main thread handles the 3-move ballot setup
				if (CBstate != NORMAL) {
					if (cboptions.em_start_positions == START_POS_3MOVE)
						PostMessage(hwnd, WM_COMMAND, START3MOVE, emstats.opening_index);

					// give main thread some time to handle this message
					Sleep(SLEEPTIME);
				}
				break;
			}

			if (!gameover && CBstate == ENGINEMATCH) {

				/* Select the engine for each search.
				 * Engine 1 plays black in odd numbered games.
				 * gamenumber is option base 1 here.
				 */
				setcurrentengine(emstats.get_enginenum(gamenumber, cbcolor));

				// make next move in game
				movecount++;
				if (movecount <= 2)
					reset_move_history = true;		/* First search of a new game for each side. */

				/* If this is the first move of a new game, and using incremental time mode, and handicap is enabled,
				 * adjust engine2's initial time clock by the handicap multiplier.
				 */
				if (movecount == 1 && cboptions.use_incremental_time && cboptions.handicap_enable) {
					if (emstats.engine1_plays_black(gamenumber))
						time_ctrl.white_time_remaining *= cboptions.handicap_mult;
					else
						time_ctrl.black_time_remaining *= cboptions.handicap_mult;
				}

				setenginestarting(TRUE);
				PostMessage(hwnd, WM_COMMAND, MOVESPLAY, 0);

				// give main thread some time to handle this message
				Sleep(SLEEPTIME);
			}
			break;
		}		// end switch CBstate
	}			// end for(;;)
}

/*
 * This function updates various engine match stats at the end of a match game.
 * 1) It writes the result to the match_progress.txt file. This file has a "+" or "-" for each game 
 *    from the point of reference of engine 1.
 * 2) It updates the counts of wins, draw, losses, and unknowns, from the point of reference of engine 1,
 *	  and the counts of blackwins and blacklosses.
 * 3) It updates the resultstring field (e.g. "1/2-1/2", "1-0", etc.) of cbgame, which 
 *	  contains details of the current game.
 *
 * The "gamenumber" param is a 1-based number here (first game is 1).
 */
int update_match_stats(int result, int movecount, int gamenumber, emstats_t *stats)
{
	// handles statistics during an engine match
	char progress_filename[MAX_PATH];

	emprogress_filename(progress_filename);
	if (movecount > maxmovecount) {
		++stats->unknowns;
		sprintf(cbgame.resultstring, pdn_result_to_string(UNKNOWN_RES, gametype()));
		writefile(progress_filename, "a", "?");
	}
	else {
		switch (result) {
		case CB_WIN:
			if (currentengine == 1) {
				++stats->wins;
				writefile(progress_filename, "a", "+");
				if (stats->engine1_plays_black(gamenumber)) {
					++stats->blackwins;
					sprintf(cbgame.resultstring, pdn_result_to_string(BLACK_WIN_RES, gametype()));
				}
				else {
					++stats->blacklosses;
					sprintf(cbgame.resultstring, pdn_result_to_string(WHITE_WIN_RES, gametype()));
				}
			}
			else {
				++stats->losses;
				writefile(progress_filename, "a", "-");
				if (stats->engine1_plays_black(gamenumber)) {
					++stats->blacklosses;
					sprintf(cbgame.resultstring, pdn_result_to_string(WHITE_WIN_RES, gametype()));
				}
				else {
					++stats->blackwins;
					sprintf(cbgame.resultstring, pdn_result_to_string(BLACK_WIN_RES, gametype()));
				}
			}
			break;

		case CB_DRAW:
			writefile(progress_filename, "a", "=");
			++stats->draws;
			sprintf(cbgame.resultstring, pdn_result_to_string(DRAW_RES, gametype()));
			break;

		case CB_LOSS:
			if (currentengine == 1) {
				++stats->losses;
				writefile(progress_filename, "a", "-");
				if (stats->engine1_plays_black(gamenumber)) {
					++stats->blacklosses;
					sprintf(cbgame.resultstring, pdn_result_to_string(WHITE_WIN_RES, gametype()));
				}
				else {
					++stats->blackwins;
					sprintf(cbgame.resultstring, pdn_result_to_string(BLACK_WIN_RES, gametype()));
				}
			}
			else {
				++stats->wins;
				writefile(progress_filename, "a", "+");
				if (stats->engine1_plays_black(gamenumber)) {
					++stats->blackwins;
					sprintf(cbgame.resultstring, pdn_result_to_string(BLACK_WIN_RES, gametype()));
				}
				else {
					++stats->blacklosses;
					sprintf(cbgame.resultstring, pdn_result_to_string(WHITE_WIN_RES, gametype()));
				}
			}
			break;

		case CB_UNKNOWN:
			++stats->unknowns;
			writefile(progress_filename, "a", "?");
			sprintf(cbgame.resultstring, pdn_result_to_string(UNKNOWN_RES, gametype()));
			break;
		}
	}

	return 1;
}

int CPUinfo(char *str)
{
	// print CPU info into str
	int CPUInfo[4] = { -1 };
	char CPUBrandString[0x40];

	// get processor info
	__cpuid(CPUInfo, 0x80000002);
	memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo, 0x80000003);
	memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo, 0x80000004);
	memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));

	sprintf(str, "%s", CPUBrandString);

	return 1;
}

int makeanalysisfile(char *filename)
{
	// produce nice analysis output
	int i;
	char s[256];
	char titlestring[256];
	char c1[] = "D84020";
	char c2[] = "A0C0C0";
	char c3[] = "444444";
	FILE *fp;
	char CPUinfostring[64];

	fp = fopen(filename, "w");
	if (fp == NULL) {
		MessageBox(hwnd, "Could not open analysisfile - is\nyour analysis directory missing?", "Error", MB_OK);
		return 0;
	}

	// print game info
	sprintf(titlestring, "%s - %s", cbgame.black, cbgame.white);

	// print HTML head
	fprintf(fp,
			"<HTML>\n<HEAD>\n<META name=\"GENERATOR\" content=\"CheckerBoard %s\">\n<TITLE>%s</TITLE></HEAD>",
			VERSION,
			titlestring);

	// print HTML body
	fprintf(fp, "<BODY><H3>");
	fprintf(fp, "%s - %s", cbgame.black, cbgame.white);
	fprintf(fp, "</H3>");
	fprintf(fp, "\n%s<BR>%s<BR>", cbgame.date, cbgame.event);
	fprintf(fp, "\nResult: %s<P>", cbgame.resultstring);

	// print hardware and level info
	enginename(s);

	CPUinfo(CPUinfostring);

	fprintf(fp, "\nAnalysis by %s at %.1fs/move on %s", s, timelevel_to_time(cboptions.level), CPUinfostring);
	fprintf(fp, "\n<BR>\ngenerated with <A HREF=\"http://www.fierz.ch/checkers.htm\">CheckerBoard %s</A><P>", VERSION);

	// print PDN and analysis
	fprintf(fp, "\n<TABLE cellspacing=\"0\" cellpadding=\"3\">");
	for (i = 0; i < (int)cbgame.moves.size(); ++i) {
		fprintf(fp, "<TR>\n");
		if (strcmp(cbgame.moves[i].analysis, "") == 0) {
			if (is_second_player(cbgame, i)) {
				fprintf(fp,
						"<TD></TD><TD bgcolor=\"%s\"></TD><TD>%s</TD><TD bgcolor=\"%s\"></TD>\n",
						c1,
						cbgame.moves[i].PDN,
						c2);
			}
			else {
				fprintf(fp,
						"<TD>%2i.</TD><TD bgcolor=\"%s\">%s</TD><TD></TD><TD bgcolor=\"%s\"></TD>\n",
						moveindex2movenum(cbgame, i),
						c1,
						cbgame.moves[i].PDN,
						c2);
			}
		}
		else {
			if (is_second_player(cbgame, i)) {
				fprintf(fp,
						"<TD></TD><TD bgcolor=\"%s\"></TD><TD>%s</TD><TD bgcolor=\"%s\">%s</TD>\n",
						c1,
						cbgame.moves[i].PDN,
						c2,
						cbgame.moves[i].analysis);
			}
			else {
				fprintf(fp,
						"<TD>%2i.</TD><TD bgcolor=\"%s\">%s</TD><TD></TD><TD bgcolor=\"%s\">%s</TD>\n",
						moveindex2movenum(cbgame, i),
						c1,
						cbgame.moves[i].PDN,
						c2,
						cbgame.moves[i].analysis);
			}
		}

		fprintf(fp, "</TR>\n");

		// add a delimiter line between moves
		fprintf(fp, "<tr><td></td><td bgcolor=\"%s\"></td><td></td><td bgcolor=\"%s\"></td></tr>\n", c1, c3);
	}

	fprintf(fp, "</TABLE></BODY></HTML>");
	fclose(fp);

	ShellExecute(NULL, "open", filename, NULL, NULL, SW_SHOW);

	return 1;
}

void togglecurrentengine(void)
{
	setcurrentengine(currentengine ^ 3);
}

void setcurrentengine(int engineN)
{
	char reply[256], windowtitle[256];

	// set the engine
	if (engineN == 1) {
		getmove = getmove1;
		islegal = islegal1;
	}

	if (engineN == 2) {
		getmove = getmove2;
		islegal = islegal2;
	}

	currentengine = engineN;

	if (CBstate != ENGINEMATCH) {
		enginename(reply);
		sprintf(windowtitle, "CheckerBoard%s: ", g_app_instance_suffix);
		strcat(windowtitle, reply);
		SetWindowText(hwnd, windowtitle);
	}

	// get book state of current engine
	if (enginecommand("get book", reply))
		book_state = atoi(reply);

	/* Set has_getmovelist. */
	if (!enginecommand("get movelist B:W1:B", reply))
		has_getmovelist = false;
	else if (strcmp(reply, "movelist 0") == 0)
		has_getmovelist = true;
	else
		has_getmovelist = false;
}

int gametype(void)
{
	// returns the game type which the current engine plays
	// if the engine has no game type associated, it will return 21 for english checkers
	char reply[ENGINECOMMAND_REPLY_SIZE];
	char command[MAXNAME];

	sprintf(reply, "");
	sprintf(command, "get gametype");

	if (enginecommand(command, reply))
		return atoi(reply);

	// return default game type
	return GT_ENGLISH;
}

int enginecommand(const char *command, char reply[ENGINECOMMAND_REPLY_SIZE])
// sends a command to the current engine, defined with the currentengine variable
// wraps a 'safety layer around calls to engine command by checking if this is supported */
{
	int result = 0;
	sprintf(reply, "");

	if (currentengine == 1 && enginecommand1 != 0)
		result = enginecommand1(command, reply);

	if (currentengine == 2 && enginecommand2 != 0)
		result = enginecommand2(command, reply);

	return result;
}

int enginename(char Lstr[MAXNAME])
// returns the name of the current engine in Lstr
{
	// set a default
	sprintf(Lstr, "no engine found");
	SetCurrentDirectory(CBdirectory);

	if (currentengine == 1) {
		if (enginecommand1 != 0) {
			if ((enginecommand1)("name", Lstr))
				return 1;
		}

		if (enginename1 != 0) {
			(enginename1)(Lstr);
			return 1;
		}
	}

	if (currentengine == 2) {
		if (enginecommand2 != 0) {
			if ((enginecommand2)("name", Lstr))
				return 1;
		}

		if (enginename2 != 0) {
			(enginename2)(Lstr);
			return 1;
		}
	}

	return 0;
}

int domove(const CBmove &m, Board8x8 b)
{
	// do move m on board b
	int i, x, y;

	x = m.from.x;
	y = m.from.y;
	b[x][y] = 0;
	x = m.to.x;
	y = m.to.y;
	b[x][y] = m.newpiece;

	for (i = 0; i < m.jumps; i++) {
		x = m.del[i].x;
		y = m.del[i].y;
		b[x][y] = 0;
	}

	return 1;
}

int undomove(CBmove &m, Board8x8 b)
{
	// take back move m on board b
	int i, x, y;

	x = m.to.x;
	y = m.to.y;
	b[x][y] = 0;

	x = m.from.x;
	y = m.from.y;
	b[x][y] = m.oldpiece;

	for (i = 0; i < m.jumps; i++) {
		x = m.del[i].x;
		y = m.del[i].y;
		b[x][y] = m.delpiece[i];
	}

	return 1;
}

void move4tonotation(const CBmove &m, char s[80])
// takes a move in coordinates, and transforms it to numbers.
{
	int from, to;
	char c = '-';
	int x1, y1, x2, y2;
	char Lstr[255];
	x1 = m.from.x;
	y1 = m.from.y;
	x2 = m.to.x;
	y2 = m.to.y;

	if (m.jumps)
		c = 'x';

	// for all versions of checkers
	from = coorstonumber(x1, y1, cbgame.gametype);
	to = coorstonumber(x2, y2, cbgame.gametype);

	sprintf(s, "%i", from);
	sprintf(Lstr, "%c", c);
	strcat(s, Lstr);
	sprintf(Lstr, "%i", to);
	strcat(s, Lstr);
}

std::string make_header(char *name, char *value)
{
	std::string header;

	header += "[";
	header += name;
	header += " \"";
	header += value;
	header += "\"]";
	return(header);
}

void PDNgametoPDNstring(PDNgame &game, std::string &pdnstring, char *lineterm)
{
	// prints a formatted PDN in *pdnstring
	// uses lineterm as the line terminator; for the clipboard this should be \r\n, normally just \n
	// i have no idea why this is so!
	std::string movenumber;
	size_t counter;
	int i;

	// print headers
	pdnstring.clear();
	pdnstring += make_header("Event", game.event) + lineterm;
	pdnstring += make_header("Date", game.date) + lineterm;
	
	/* List player colors in order: first-player first. */
	if (get_startcolor(game.gametype) == CB_BLACK) {
		pdnstring += make_header("Black", game.black) + lineterm;
		pdnstring += make_header("White", game.white) + lineterm;
	}
	else {
		pdnstring += make_header("White", game.white) + lineterm;
		pdnstring += make_header("Black", game.black) + lineterm;
	}

	pdnstring += make_header("Result", game.resultstring) + lineterm;

	// if this was after a setup, add FEN and setup header
	if (strcmp(game.FEN, "") != 0)
		pdnstring += make_header("FEN", game.FEN) + lineterm;

	// print PDN
	counter = 0;
	for (i = 0; i < (int)game.moves.size(); ++i) {

		// print the move number
		if (!is_second_player(game, i)) {
			movenumber = std::to_string(moveindex2movenum(game, i)) + ". ";
			counter += movenumber.size();
			if (counter > 79) {
				pdnstring += lineterm;
				counter = movenumber.size();
			}

			pdnstring += movenumber;
		}

		// print the move
		counter += strlen(game.moves[i].PDN);
		if (counter > 79) {
			pdnstring += lineterm;
			counter = strlen(game.moves[i].PDN);
		}

		pdnstring += game.moves[i].PDN;
		pdnstring += " ";

		// if the move has a comment, print it too
		if (strcmp(game.moves[i].comment, "") != 0) {
			counter += strlen(game.moves[i].comment);
			if (counter > 79) {
				pdnstring += lineterm;
				counter = strlen(game.moves[i].comment);
			}

			pdnstring += "{";
			pdnstring += game.moves[i].comment;
			pdnstring += "} ";
		}
	}

	// add the game terminator
	++counter;		/* Game terminator is '*' as per PDN 3.0. See http://pdn.fmjd.org/ */
	if (counter > 79)
		pdnstring += lineterm;

	pdnstring += "*";
	pdnstring += lineterm;
}

/*
 * Adds a move to the cbgame.moves vector, and fills in the PDN field.
 * Initializes the analysis and comment fields to an empty string.
 * The move is added after the current position into the moves list, cbgame.movesindex.
 * If this is not the end of moves[], delete all the entries starting at movesindex.
 */
void addmovetogame(CBmove &move, char *pdn)
{
	gamebody_entry entry;

	/* Delete entries in cbgame.moves[] from movesindex to end. */
	if (cbgame.movesindex < (int)cbgame.moves.size())
		cbgame.moves.erase(cbgame.moves.begin() + cbgame.movesindex, cbgame.moves.end());

	entry.analysis[0] = 0;
	entry.comment[0] = 0;
	entry.move = move;
	if (pdn != nullptr && pdn[0])
		strcpy(entry.PDN, pdn);
	else
		move4tonotation(move, entry.PDN);
	try {
		cbgame.moves.push_back(entry);
	}
	catch(...) {
		char *msg = "could not allocate memory for CB movelist";
		CBlog(msg);
		strcpy(statusbar_txt, msg);
	}

	cbgame.movesindex = (int)cbgame.moves.size();
}

int getfilename(char filename[255], int what)
{
	OPENFILENAME of;
	char dir[MAX_PATH];

	sprintf(filename, "");
	(of).lStructSize = sizeof(OPENFILENAME);
	(of).hwndOwner = NULL;
	(of).hInstance = g_hInst;
	(of).lpstrFilter = "checkers databases *.pdn\0 *.pdn\0 all files *.*\0 *.*\0\0";
	(of).lpstrCustomFilter = NULL;
	(of).nMaxCustFilter = 0;
	(of).nFilterIndex = 0;
	(of).lpstrFile = filename;	// if user chooses something, it's printed in here!
	(of).nMaxFile = MAX_PATH;
	(of).lpstrFileTitle = NULL;
	(of).nMaxFileTitle = 0;
	(of).lpstrInitialDir = cboptions.userdirectory;
	(of).lpstrTitle = NULL;
	(of).Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	(of).nFileOffset = 0;
	(of).nFileExtension = 0;
	(of).lpstrDefExt = NULL;
	(of).lCustData = 0;
	(of).lpfnHook = NULL;
	(of).lpTemplateName = NULL;

	if (what == OF_SAVEGAME) {

	/* save a game to disk */
		(of).lpstrTitle = "Select PDN database to save game to";
		if (GetSaveFileName(&of))
			return 1;
	}

	if (what == OF_LOADGAME) {

		/* load a game from disk */
		(of).lpstrTitle = "Select PDN database to load";
		if (GetOpenFileName(&of))
			return 1;
	}

	if (what == OF_SAVEASHTML) {

		// save game as html
		(of).lpstrTitle = "Select filename of HTML output";
		(of).lpstrFilter = "HTML files *.htm\0 *.htm\0 all files *.*\0 *.*\0\0";
		sprintf(dir, "%s\\games", CBdocuments);
		(of).lpstrInitialDir = dir;
		if (GetSaveFileName(&of))
			return 1;
	}

	if (what == OF_USERBOOK) {

		// select user book
		(of).lpstrTitle = "Select the user book to use";
		(of).lpstrFilter = "user book files *.bin\0 *.bin\0 all files *.*\0 *.*\0\0";
		(of).lpstrInitialDir = CBdocuments;
		if (GetSaveFileName(&of))
			return 1;
	}

	if (what == OF_BOOKFILE) {
		(of).lpstrTitle = "Select the opening book filename";
		(of).lpstrFilter = "user book files *.odb\0 *.odb\0 all files *.*\0 *.*\0\0";
		sprintf(dir, "%s\\engines", CBdirectory);
		(of).lpstrInitialDir = dir;
		if (GetOpenFileName(&of))
			return 1;
	}

	return 0;
}

bool pdntogame(PDNgame &game, Board8x8 startposition, int startcolor, std::string &errormsg)
{
	/* pdntogame takes a starting position, a side to move next as parameters. 
	it uses cbgame, which has to be initialized with pdn-text to generate the CBmoves. */

	/* called by loadgame and gamepaste */
	int i, color;
	Board8x8 b8;
	CBmove legalmove;

	/* set the starting values */
	color = startcolor;
	memcpy(b8, startposition, sizeof(b8));
	for (i = 0; i < (int)game.moves.size(); ++i) {
		int status;
		Squarelist move;

		PDNparseMove(game.moves[i].PDN, move);
		status = islegal_check(b8, color, move, &legalmove, gametype());
		if (status) {
			game.moves[i].move = legalmove;
			color = CB_CHANGECOLOR(color);
			domove(legalmove, b8);
		}
		else {
			errormsg = "[Event \"" + std::string(game.event) + "\"]\n";
			errormsg += "Illegal move " + std::string(game.moves[i].PDN);
			errormsg += ", near move " + std::to_string(1 + i / 2);
			errormsg += "\nGame was truncated at that point.";

			/* Delete every move from bad move to end of game. */
			game.moves.erase(game.moves.begin() + i, game.moves.end());
			return(true);
		}
	}
	return(false);
}

int builtinislegal(Board8x8 board8, int color, Squarelist &squares, CBmove *move, int gametype)
{
	// make all moves and try to find out if this move is legal
	int i, n;
	int Lfrom, Lto;
	int isjump;
	CBmove movelist[MAXMOVES];

	if (has_getmovelist)
		get_movelist_from_engine(board8, color, movelist, &n, &isjump);
	else {
		n = getmovelist(color, movelist, board8, &isjump);
		assert(gametype == GT_ENGLISH);
	}
	for (i = 0; i < n; i++) {
		Lfrom = coortonumber(movelist[i].from, gametype);
		Lto = coortonumber(movelist[i].to, gametype);
		if (Lfrom == squares.first() && Lto == squares.last()) {

			/* If more than 2 squares, the intermediates have to match also. */
			if (squares.size() > 2) {
				if (squares.size() - 2 != movelist[i].jumps - 1)	/* jumps has the number of landed squares in path[]. */
					continue;

				bool match = true;
				for (int k = 1; k < squares.size() - 1; ++k) {
					int intermediate = coortonumber(movelist[i].path[k], gametype);
					if (squares.read(k) != intermediate) {
						match = false;
						break;
					}
				}
				if (match) {
					/* Found a match of fully described capture move. */
					*move = movelist[i];
					return(1);
				}
			}
			else {
				/* Found a matching move described with only from and to squares. */
				*move = movelist[i];
				return(1);
			}
		}
	}

	if (isjump) {
		sprintf(statusbar_txt, "illegal move - you must jump! for multiple jumps, click only from and to square");
	}
	else
		sprintf(statusbar_txt, "%d-%d not a legal move", squares.first(), squares.last());
	return 0;
}

/*
 * Although we assign the islegal function pointer to this function for English checkers, it
 * does not get used. All islegal decisions are made through islegal_check().
 */
int builtinislegal(Board8x8 board8, int color, int from, int to, CBmove *move)
{
	Squarelist squares;

	squares.append(from);
	squares.append(to);
	return(builtinislegal(board8, color, squares, move, GT_ENGLISH));
}

/*
 * For English game type we can use the builtin legal checker to resolve ambiguous moves, so send
 * it all squares that are needed to uniquely describe the move. Unfortunately, the interface to the 
 * engines does not allow sending intermediate squares, so we can't do this for the other game types.
 */
int islegal_check(Board8x8 board8, int color, Squarelist &squares, CBmove *move, int gametype)
{
	if (has_getmovelist || gametype == GT_ENGLISH)
		return(builtinislegal(board8, color, squares, move, gametype));
	else
		return(islegal(board8, color, squares.first(), squares.last(), move));
}

/*
 * For gametype English only.
 * Return true if square is a from, to, or intermediate landed square in move.
 */
bool square_in_move(int square, CBmove &move, int gametype)
{
	if (square == coortonumber(move.from, gametype))
		return(true);
	if (square == coortonumber(move.to, gametype))
		return(true);
	for (int i = 1; i < move.jumps; ++i)
		if (square == coortonumber(move.path[i], gametype))
			return(true);

	return(false);
}

/*
 * For gametype English only.
 * Return true if every square in squares is either a from, to, or intermediate landed square in move.
 */
bool all_squares_match(Squarelist &squares, CBmove &move, int gametype)
{
	/* Special case for 2 squares that both match the from square. They must also match
	 * the to square to return true.
	 */
	if (squares.size() == 2 && squares.first() == squares.last())
		return(squares.first() == coortonumber(move.from, gametype) && squares.last() == coortonumber(move.to, gametype));

	for (int i = 0; i < squares.size(); ++i)
		if (!square_in_move(squares.read(i), move, gametype))
			return(false);

	return(true);
}

/*
 * For gametype English only.
 * Return the sum of the from, to, and intermediate landed squares in move.
 * Used as a check to see if two moves are identical.
 */
uint32_t get_sum_squares(CBmove &move)
{
	uint32_t sum;

	sum = coortonumber(move.from, GT_ENGLISH);
	sum += coortonumber(move.to, GT_ENGLISH);
	for (int i = 1; i < move.jumps; ++i)
		sum += coortonumber(move.path[i], GT_ENGLISH);

	return(sum);
}

/*
 * For gametype English only.
 * We've already determined that every square in squares matches a square in move (but there may be
 * more than one move that meets that constraint).
 * If we find that every square in move is matched by a square in squares, then we have found the move.
 * This covers pathalogical cases like B:W6,7,8,14,15,16,22,23,24:BK2. There are three ways to capture 2x4.
 * 1) To capture 2x11x4, click 2, 11, and 4.
 * 2) To capture 2x9x18x11x4, click 2, 4, 9, 18, and 11. We cannot click 4 last because after 2, 9, 18, and 11,
 *		the move 2x9x18x11x2 is matched.
 * 3) To capture 2x9x18x27x20x11x4, click 2, 9, 20, and 4.
 */
bool all_move_squares_matched(Squarelist &squares, CBmove &move, int gametype)
{
	if (squares.first() != coortonumber(move.from, gametype))
		return(false);
	if (!squares.frequency(coortonumber(move.to, gametype)))
		return(false);

	for (int i = 1; i < move.jumps; ++i)
		if (!squares.frequency(coortonumber(move.path[i], gametype)))
			return(false);

	return(true);
}

int num_moves_matching_fromto(CBmove movelist[], int nmoves, int from, int to, CBmove &move, int gametype)
{
	int nmatches, sum_squares;

	nmatches = 0;
	for (int i = 0; i < nmoves; ++i) {
		if (from == coortonumber(movelist[i].from, gametype) && to == coortonumber(movelist[i].to, gametype)) {
			if (nmatches == 0) {
				++nmatches;
				move = movelist[i];
				sum_squares = get_sum_squares(move);
			}

			/* Use sum of squares to detect identical moves that are
			 * captures by kings in a different order.
			 */
			if (sum_squares != get_sum_squares(movelist[i]))
				++nmatches;
		}
	}
	return(nmatches);
}

/*
 * For gametype English only.
 * Return the number of moves in movelist that match the squares in the Squarelist.
 * The squares can be any of from, to, or any intermediate landing square during a capture.
 * If a single matching move is found, it is returned in move.
 */
int num_matching_moves(CBmove movelist[], int nmoves, Squarelist &squares, CBmove &move, int gametype)
{
	int nmatches, sum_squares;

	nmatches = 0;
	for (int i = 0; i < nmoves; ++i) {
		if (all_squares_match(squares, movelist[i], gametype)) {

			/* Now we know every square in squares has a match in this move.
			 * If from, to, and every intermediate landed square in move has a match in squares,
			 * then declare this move a singular match.
			 */
			if (all_move_squares_matched(squares, movelist[i], gametype)) {
				nmatches = 1;
				move = movelist[i];
				break;
			}
			if (nmatches == 0) {
				++nmatches;
				move = movelist[i];
				sum_squares = get_sum_squares(move);
			}
			else {
				/* Use sum of squares to detect identical moves that are
				 * captures by kings in a different order.
				 */
				if (sum_squares != get_sum_squares(movelist[i]))
					++nmatches;
			}
		}
	}

	return(nmatches);
}

/*
 * For gametype English only or Engines that have the optional getmovelist command.
 * Return the number of moves in the current position that match the squares in the Squarelist.
 * The squares can be any of from, to, or any intermediate landing square during a capture.
 * If a single matching move is found, it is returned in move.
 */
int num_matching_moves(Board8x8 board8, int color, Squarelist &squares, CBmove &move, int gametype)
{
	int nmoves, isjump;
	CBmove movelist[MAXMOVES];

	if (has_getmovelist)
		get_movelist_from_engine(board8, color, movelist, &nmoves, &isjump);
	else
		nmoves = getmovelist(color, movelist, board8, &isjump);
	return(num_matching_moves(movelist, nmoves, squares, move, gametype));
}

/*
 * For gametype English only or Engines that have the optional getmovelist command.
 * Take a CBmove and write the move in PDN text format.
 * Write capture moves in long format if needed to unambiguously describe them.
 * This function is only for English checkers.
 * Return true on error, false on success.
 */
bool move_to_pdn_english(int nmoves, CBmove movelist[MAXMOVES], CBmove *move, char *pdn, int gametype)
{
	int i, fromto_count, all_match_count;
	char separator;
	CBmove matching_move;
	Squarelist squares;

	/* Find the number of moves that match the from and to squares. */
	pdn[0] = 0;
	fromto_count = num_moves_matching_fromto(movelist, nmoves, coortonumber(move->from, gametype), coortonumber(move->to, gametype), matching_move, gametype);
	if (fromto_count == 0)
		return(true);

	separator = move->jumps ? 'x' : '-';
	if (fromto_count == 1)
		sprintf(pdn, "%d%c%d", coortonumber(move->from, gametype), separator, coortonumber(move->to, gametype));
	else {
		/* Add the path squares to the squares array. */
		squares.append(coortonumber(move->from, gametype));
		for (i = 1; i <= move->jumps; ++i)
			squares.append(coortonumber(move->path[i], gametype));
		squares.append(coortonumber(move->to, gametype));

		/* Get count of moves that match all the squares. */
		all_match_count = num_matching_moves(movelist, nmoves, squares, matching_move, gametype);
		if (fromto_count > all_match_count) {
			/* Need to use the full move notation. */
			sprintf(pdn, "%d%c", coortonumber(move->from, gametype), separator);
			for (i = 1; i < move->jumps; ++i)
				sprintf(pdn + strlen(pdn), "%d%c", coortonumber(move->path[i], gametype), separator);
			sprintf(pdn + strlen(pdn), "%d", coortonumber(move->to, gametype));
		}
		else
			sprintf(pdn, "%d%c%d", coortonumber(move->from, gametype), separator, coortonumber(move->to, gametype));
	}
	return(false);
}

bool move_to_pdn_english(Board8x8 board8, int color, CBmove *move, char *pdn, int gametype)
{
	int isjump, nmoves;
	CBmove movelist[MAXMOVES];

	if (has_getmovelist)
		get_movelist_from_engine(board8, color, movelist, &nmoves, &isjump);
	else
		nmoves = getmovelist(color, movelist, board8, &isjump);
	return(move_to_pdn_english(nmoves, movelist, move, pdn, gametype));
}

void newgame(void)
{
	InitCheckerBoard(cbboard8);
	reset_game(cbgame);
	newposition = TRUE;
	reset_move_history = true;
	cboptions.mirror = is_mirror_gametype(cbgame.gametype);
	cbcolor = get_startcolor(cbgame.gametype);
	display_board(cbboard8);
	reset_game_clocks();
}

bool doload(PDNgame *game, const char *gamestring, int *color, Board8x8 board8, std::string &errormsg)
{
	// game is in gamestring. use pdnparser routines to convert
	// it into a game
	// read headers
	bool result;
	const char *start;
	const char *p;
	char header[MAXNAME], token[1024];
	char headername[MAXNAME], headervalue[MAXNAME];
	int issetup = 0;
	PDN_PARSE_STATE state;
	gamebody_entry entry;

	reset_game(*game);
	p = gamestring;
	while (PDNparseGetnextheader(&p, header, sizeof(header))) {

		/* parse headers */
		start = header;
		PDNparseGetnexttoken(&start, headername, sizeof(headername));
		PDNparseGetnexttag(&start, headervalue, sizeof(headervalue));

		/* make header name lowercase, so that 'event' and 'Event' will be recognized */
		_strlwr(headername);

		if (strcmp(headername, "event") == 0)
			strncpy_terminated(game->event, headervalue, sizeof(game->event));
		else if (strcmp(headername, "site") == 0)
			strncpy_terminated(game->site, headervalue, sizeof(game->site));
		else if (strcmp(headername, "date") == 0)
			strncpy_terminated(game->date, headervalue, sizeof(game->date));
		else if (strcmp(headername, "round") == 0)
			strncpy_terminated(game->round, headervalue, sizeof(game->round));
		else if (strcmp(headername, "white") == 0)
			strncpy_terminated(game->white, headervalue, sizeof(game->white));
		else if (strcmp(headername, "black") == 0)
			strncpy_terminated(game->black, headervalue, sizeof(game->black));
		else if (strcmp(headername, "result") == 0) {
			strncpy_terminated(game->resultstring, headervalue, sizeof(game->resultstring));
			game->result = string_to_pdn_result(headervalue, gametype());
		}
		else if (strcmp(headername, "fen") == 0) {
			strncpy_terminated(game->FEN, headervalue, sizeof(game->FEN));
			issetup = 1;
		}
	}

	/* set defaults */
	*color = get_startcolor(game->gametype);
	cboptions.mirror = is_mirror_gametype(game->gametype);

	InitCheckerBoard(board8);

	/* if its a setup load position */
	if (issetup)
		FENtoboard8(board8, game->FEN, color, game->gametype);

	/* ok, headers read, now parse PDN input:*/
	while ((state = (PDN_PARSE_STATE) PDNparseGetnextPDNtoken(&p, token, sizeof(token)))) {

		/* check for special tokens*/

		/* move number - discard */
		if (token[strlen(token) - 1] == '.')
			continue;

		/* game terminators */
		if
		(
			(strcmp(token, "*") == 0) ||
			(strcmp(token, "0-1") == 0) ||
			(strcmp(token, "1-0") == 0) ||
			(strcmp(token, "1/2-1/2") == 0)
		) {

			/* In PDN 3.0, the game terminator is '*'. Allow old style game result terminators, 
			 * but don't interpret them as results.
			 */
			break;
		}

		if (token[0] == '{' || state == PDN_FLUFF) {

			/* we found a comment */
			start = token;

			// remove the curly braces by moving pointer one forward, and trimming
			// last character
			if (state != PDN_FLUFF) {
				start++;
				token[strlen(token) - 1] = 0;
			}

			/* This comment is for the previous move. */
			if (game->moves.size() > 0)
				sprintf(game->moves[game->moves.size() - 1].comment, "%s", start);
			continue;
		}

#ifdef NEMESIS
		if (token[0] == '(') {

			/* we found a comment */

			/* write it to last move, because current entry is already the new move */
			start = token;
			start++;
			token[strlen(token) - 1] = 0;
			if (game->moves.size() > 0)
				sprintf(game->moves[game->moves.size() - 1].comment, "%s", start);
			continue;
		}
#endif

		// ok, it was just a move. Save just the move string now, and we will fill in
		// the move details when done reading the pdn.
		sprintf(entry.PDN, "%s", token);
		entry.analysis[0] = 0;
		entry.comment[0] = 0;
		memset(&entry.move, 0, sizeof(entry.move));
		game->moves.push_back(entry);
	}

	// fill in the move information.
	result = pdntogame(*game, board8, *color, errormsg);
	reset_move_history = true;
	newposition = TRUE;
	return(result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// initializations below:
void InitStatus(HWND hwnd)
{
	hStatusWnd = CreateWindow(STATUSCLASSNAME, "", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, g_hInst, NULL);
}

void InitCheckerBoard(Board8x8 b)
{
	// initialize board to starting position
	memset(b, 0, 64 * sizeof(int));
	b[0][0] = CB_BLACK | CB_MAN;
	b[2][0] = CB_BLACK | CB_MAN;
	b[4][0] = CB_BLACK | CB_MAN;
	b[6][0] = CB_BLACK | CB_MAN;
	b[1][1] = CB_BLACK | CB_MAN;
	b[3][1] = CB_BLACK | CB_MAN;
	b[5][1] = CB_BLACK | CB_MAN;
	b[7][1] = CB_BLACK | CB_MAN;
	b[0][2] = CB_BLACK | CB_MAN;
	b[2][2] = CB_BLACK | CB_MAN;
	b[4][2] = CB_BLACK | CB_MAN;
	b[6][2] = CB_BLACK | CB_MAN;

	b[1][7] = CB_WHITE | CB_MAN;
	b[3][7] = CB_WHITE | CB_MAN;
	b[5][7] = CB_WHITE | CB_MAN;
	b[7][7] = CB_WHITE | CB_MAN;
	b[0][6] = CB_WHITE | CB_MAN;
	b[2][6] = CB_WHITE | CB_MAN;
	b[4][6] = CB_WHITE | CB_MAN;
	b[6][6] = CB_WHITE | CB_MAN;
	b[1][5] = CB_WHITE | CB_MAN;
	b[3][5] = CB_WHITE | CB_MAN;
	b[5][5] = CB_WHITE | CB_MAN;
	b[7][5] = CB_WHITE | CB_MAN;
}

/*
 * Load an engine dll, and get pointers to the exported functions in the dll.
 * Return non-zero on error.
 */
int load_engine
	(
		HINSTANCE *lib,
		char *dllname,
		CB_ENGINECOMMAND *cmdfn,
		CB_GETSTRING *namefn,
		CB_GETMOVE *getmovefn,
		CB_ISLEGAL *islegalfn,
		char *pri_or_sec
	)
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
		*cmdfn = (CB_ENGINECOMMAND) GetProcAddress(*lib, "enginecommand");
		*namefn = (CB_GETSTRING) GetProcAddress(*lib, "enginename");
		*getmovefn = (CB_GETMOVE) GetProcAddress(*lib, "getmove");
		*islegalfn = (CB_ISLEGAL) GetProcAddress(*lib, "islegal");
		if (*islegalfn == NULL)
			*islegalfn = builtinislegal;
		return(0);
	}
	else {
		sprintf(buf,
				"CheckerBoard could not find\nthe %s engine dll.\n\nPlease use the \n'Engine->Select..' command\nto select a new %s engine",
			pri_or_sec,
				pri_or_sec);
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
	CBgametype = (CB_GETGAMETYPE) builtingametype;

	// load engine dlls
	// first, primary engine
	// is there a way to check whether a module is already loaded?
	primaryhandle = GetModuleHandle(pri_fname);
	sprintf(Lstr, "handle = %i (primary engine)", PtrToLong(primaryhandle));
	CBlog(Lstr);
	secondaryhandle = GetModuleHandle(sec_fname);
	sprintf(Lstr, "secondaryhandle = %i (secondary engine)", PtrToLong(secondaryhandle));
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
	if (!hinstLib1 || strcmp(pri_fname, cboptions.primaryenginestring)) {
		status = load_engine(&hinstLib1, pri_fname, &enginecommand1, &enginename1, &getmove1, &islegal1, "primary");
		if (!status)
			strcpy(cboptions.primaryenginestring, pri_fname);	/* Success. */
		else {
			if (strcmp(pri_fname, cboptions.primaryenginestring)) {
				status = load_engine(&hinstLib1,
									 cboptions.primaryenginestring,
									 &enginecommand1,
									 &enginename1,
									 &getmove1,
									 &islegal1,
									 "primary");
				if (status)
					cboptions.primaryenginestring[0] = 0;
			}
		}
	}

	if (!hinstLib2 || strcmp(sec_fname, cboptions.secondaryenginestring)) {
		status = load_engine(&hinstLib2, sec_fname, &enginecommand2, &enginename2, &getmove2, &islegal2, "secondary");
		if (!status)
			strcpy(cboptions.secondaryenginestring, sec_fname); /* Success. */
		else {
			if (strcmp(sec_fname, cboptions.secondaryenginestring)) {
				status = load_engine(&hinstLib2,
									 cboptions.secondaryenginestring,
									 &enginecommand2,
									 &enginename2,
									 &getmove2,
									 &islegal2,
									 "secondary");
				if (status)
					cboptions.secondaryenginestring[0] = 0;
			}
		}
	}

	// set current engine
	setcurrentengine(1);

	// reset game if an engine of different game type was selected!
	if (gametype() != cbgame.gametype) {
		PostMessage(hwnd, (UINT) WM_COMMAND, (WPARAM) GAMENEW, (LPARAM) 0);
		PostMessage(hwnd, (UINT) WM_SIZE, (WPARAM) 0, (LPARAM) 0);
	}

	// reset the directory to the CB directory
	SetCurrentDirectory(CBdirectory);
}

void initengines(void)
{
	loadengines(cboptions.primaryenginestring, cboptions.secondaryenginestring);
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
	int id[NUMBUTTONS] =
	{
		15,
		0,
		NUMBUTTONS + STD_FILENEW,
		NUMBUTTONS + STD_FILESAVE,
		NUMBUTTONS + STD_FILEOPEN,
		NUMBUTTONS + STD_FIND,
		0,
		7,
		NUMBUTTONS + STD_UNDO,
		NUMBUTTONS + STD_REDOW,
		8,
		0,
		2,
		3,
		0,
		0,
		10,
		11,
		17,
		0,
		12,
		13,
		14
	};
	int command[NUMBUTTONS] =
	{
		HELPHOMEPAGE,
		0,
		GAMENEW,
		GAMESAVE,
		GAMELOAD,
		GAMEFIND,
		0,
		MOVESBACKALL,
		MOVESBACK,
		MOVESFORWARD,
		MOVESFORWARDALL,
		0,
		MOVESPLAY,
		TOGGLEBOOK,
		TOGGLEENGINE,
		0,
		SETUPCC,
		DISPLAYINVERT,
		TOGGLEMODE,
		0,
		BOOKMODE_VIEW,
		BOOKMODE_ADD,
		BOOKMODE_DELETE
	};
	int style[NUMBUTTONS] =
	{
		TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_SEP,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON,
		TBSTYLE_BUTTON
	};

	// Ensure that the common control DLL is loaded.
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_BAR_CLASSES;

	InitCommonControlsEx(&icex);

	// Create a toolbar.
	hwndTB = CreateWindowEx(0,
							TOOLBARCLASSNAME,
							(LPSTR) NULL,
							WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE | TBSTYLE_FLAT,
							0,
							0,
							0,
							0,
							hwndParent,
							(HMENU) ID_TOOLBAR,
							g_hInst,
							NULL);

	// Send the TB_BUTTONSTRUCTSIZE message, which is required for backward compatibility.
	SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

	// Fill the TBBUTTON array with button information, and add the
	// buttons to the toolbar. The buttons on this toolbar have text
	// but do not have bitmap images.
	for (i = 0; i < NUMBUTTONS; i++) {
		tbButtons[i].dwData = 0L;
		tbButtons[i].fsState = TBSTATE_ENABLED;
		tbButtons[i].fsStyle = (BYTE) style[i];
		tbButtons[i].iBitmap = id[i];
		tbButtons[i].idCommand = command[i];
		tbButtons[i].iString = 0;	//"text";
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
	SendMessage(hwndTB, TB_ADDBITMAP, NUMBUTTONS, (LPARAM) & tbab);

	// add default bitmaps
	tbab.hInst = HINST_COMMCTRL;
	tbab.nID = IDB_STD_SMALL_COLOR;

	//tbab.nID = IDB_VIEW_LARGE_COLOR;
	SendMessage(hwndTB, TB_ADDBITMAP, 0, (LPARAM) & tbab);

	// add buttons
	SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM) NUMBUTTONS, (LPARAM) (LPTBBUTTON) & tbButtons);

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

enum MLTOK {
	MLCMD, MLCOMMA, MLNUM, MLSEMI, MLEND
};

int getmltok(char msg[], int &idx, char num[])
{
	if (isdigit(msg[idx])) {
		while(isdigit(msg[idx])) {
			*num = msg[idx];
			++idx;
			++num;
		}
		*num = 0;
		return(MLNUM);
	}
	if (msg[idx] == ',') {
		++idx;
		return(MLCOMMA);
	}
	if (msg[idx] == ';') {
		++idx;
		return(MLSEMI);
	}
	if (!strncmp(msg + idx, "movelist ", 9)) {
		idx += 9;
		return(MLCMD);
	}
	return(MLEND);
}

/*
 * If the current engine has the optional "get movelist" engine command, then use it to obtain a movelist
 * The function returns a non-zero value if the engine cannot use "get movelist" engine command.
 */
int get_movelist_from_engine(Board8x8 board8, int color, CBmove movelist[], int *nmoves, int *iscapture)
{
	int idx, square;
	char buf[256];
	char command[256];
	char reply[ENGINECOMMAND_REPLY_SIZE];

	if (!has_getmovelist)
		return(1);

	board8toFEN(board8, buf, color, cbgame.gametype);
	sprintf(command, "get movelist %s", buf);
	if (!enginecommand(command, reply))
		return(1);

	/* Reply should look like:
	 * "movelist <nmoves>,<njumps1>,<oldpiece1>,<newpiece1>,<from sq1>,<to sq1>,
	 * <landed sq1>,<cap. sq1>,<cappiece1>,<landed sq2>,<cap. sq2>,<cappiece2>,...;"
	 * Each move terminates in a semicolon.
	 * If no moves, the response is "movelist 0".
	 */
	idx = 0;
	if (getmltok(reply, idx, buf) != MLCMD)
		return(1);
	if (getmltok(reply, idx, buf) != MLNUM)
		return(1);
	*nmoves = atoi(buf);
	if (*nmoves == 0)
		return(0);
	if (getmltok(reply, idx, buf) != MLCOMMA)
		return(0);
	for (int mv = 0; mv < *nmoves; ++mv) {
		if (getmltok(reply, idx, buf) != MLNUM)
			return(1);
		movelist[mv].jumps = atoi(buf);

		if (getmltok(reply, idx, buf) != MLCOMMA)
			return(1);
		if (getmltok(reply, idx, buf) != MLNUM)
			return(1);
		movelist[mv].oldpiece = atoi(buf);

		if (getmltok(reply, idx, buf) != MLCOMMA)
			return(1);
		if (getmltok(reply, idx, buf) != MLNUM)
			return(1);
		movelist[mv].newpiece = atoi(buf);

		if (getmltok(reply, idx, buf) != MLCOMMA)
			return(1);
		if (getmltok(reply, idx, buf) != MLNUM)
			return(1);
		square = atoi(buf);
		numbertocoors(square, &movelist[mv].from, cbgame.gametype);

		if (getmltok(reply, idx, buf) != MLCOMMA)
			return(1);
		if (getmltok(reply, idx, buf) != MLNUM)
			return(1);
		square = atoi(buf);
		numbertocoors(square, &movelist[mv].to, cbgame.gametype);

		for (int i = 0; i < movelist[mv].jumps; ++i) {
			if (getmltok(reply, idx, buf) != MLCOMMA)
				return(1);
			if (getmltok(reply, idx, buf) != MLNUM)
				return(1);
			square = atoi(buf);
			numbertocoors(square, &movelist[mv].path[i + 1], cbgame.gametype);

			if (getmltok(reply, idx, buf) != MLCOMMA)
				return(1);
			if (getmltok(reply, idx, buf) != MLNUM)
				return(1);
			square = atoi(buf);
			numbertocoors(square, &movelist[mv].del[i], cbgame.gametype);

			if (getmltok(reply, idx, buf) != MLCOMMA)
				return(1);
			if (getmltok(reply, idx, buf) != MLNUM)
				return(1);
			movelist[mv].delpiece[i] = atoi(buf);
		}

		if (getmltok(reply, idx, buf) != MLSEMI)
			return(1);
	}
	if (getmltok(reply, idx, buf) != MLEND)
		return(1);

	*iscapture = movelist[0].jumps;
	return(0);
}


