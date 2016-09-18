// dialogs.c - part of checkerboard
// 
// this file contains all the functions for the various dialog boxes in checkerboard
//
// ugly: it has a lot of references to extern variables 

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#pragma warning(disable:4091)
#include <shlobj.h>
#pragma warning(default:4091)
#include <vector>
#include "resource.h"
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "dialogs.h"
#include "CheckerBoard.h"
#include "pdnfind.h"


#ifdef _WIN64
#define GWL_HINSTANCE GWLP_HINSTANCE
#else
#ifndef GetWindowLongPtr
#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong
#endif
#endif

extern struct CBoptions gCBoptions;
extern char str[1024];
extern HINSTANCE g_hInst;
extern HWND hwnd;
extern struct PDNgame GPDNgame;
extern struct listentry *current, *head, *tail;

int HeaderHeight;
int columns[NUMCOLS]={PLAYERWIDTH,PLAYERWIDTH,RESULTWIDTH,EVENTWIDTH};
int Tabs[4];

/* Combo box entries. */
static int limit_pieces[] = {24, 10, 9, 8, 7, 6, 5, 4};
static int thread_limits[] = {8, 7, 6, 5, 4, 3, 2, 1};


INT CALLBACK BrowseCallbackProc(HWND hwnd, 
                                UINT uMsg,
                                LPARAM lp, 
                                LPARAM pData) 
{
	TCHAR szDir[MAX_PATH];

	switch(uMsg) {
	case BFFM_INITIALIZED:
		/* The initial directory was passed in pData. */
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pData);
		break;

	case BFFM_SELCHANGED: 
		// Set the status window to the currently selected path.
		if (SHGetPathFromIDList((LPITEMIDLIST) lp, szDir)) {
			SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
		}
		break;
	}
	return 0;
}


int browse_to_dir(char *title, char *path)
{
	int status;
	BROWSEINFO bi;
	LPITEMIDLIST pidl;
	char init_path[MAX_PATH];

	memset(&bi, 0, sizeof(bi));
	bi.lpszTitle = title;
	bi.hwndOwner = hwnd;
	bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lpfn = BrowseCallbackProc;

	strcpy(init_path, path);
	bi.lParam = (LPARAM)init_path;

	pidl = SHBrowseForFolder(&bi);
	if (pidl != 0) {
		status = SHGetPathFromIDList(pidl, path);
		if (SUCCEEDED(status)) {
			CoTaskMemFree(pidl);
			return(1);
		}
	}           
	return(0);
}


BOOL CALLBACK AboutDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{

	// localization: need to load the appropriate string resource from
	// the resource file depending on the current language.
	switch(message)
		{
		case WM_INITDIALOG: 
			// center dialog box on CB window
			CenterDialog(hdwnd);
			return 1;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_OK:
					EndDialog(hdwnd,0);
					return 1;
				}
			break;
		}
	return 0;
	}


BOOL CALLBACK ThreeMoveDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	// this dialog sets the global variables
	// op_board,op_mailplay & op_barred, which indicate
	// whether the respective openings are used in the 3movedeck 
	
	switch(message)
		{
		case WM_INITDIALOG:
			// center dialog box on CB window:
			CenterDialog(hdwnd);
			// initialize checkboxes:
			SendDlgItemMessage(hdwnd, IDC_BOARD, BM_SETCHECK, gCBoptions.op_crossboard,0);
			SendDlgItemMessage(hdwnd, IDC_MAILPLAY, BM_SETCHECK, gCBoptions.op_mailplay,0);
			SendDlgItemMessage(hdwnd, IDC_BARRED, BM_SETCHECK, gCBoptions.op_barred,0);
			// localization example
			//LoadString(g_hInst,101, buffer, 1024);
			///SendDlgItemMessage(hdwnd, 65535, WM_SETTEXT, 0, buffer);

			return 1;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_CANCEL:
					EndDialog(hdwnd,0);
					return 1;
				case IDC_OK:
					gCBoptions.op_crossboard=SendDlgItemMessage(hdwnd,IDC_BOARD,BM_GETCHECK,0,0);
					gCBoptions.op_mailplay=SendDlgItemMessage(hdwnd,IDC_MAILPLAY,BM_GETCHECK,0,0);
					gCBoptions.op_barred=SendDlgItemMessage(hdwnd,IDC_BARRED,BM_GETCHECK,0,0);
					EndDialog(hdwnd,0);
					return 1;
				}
			break;
		}
	return 0;
	}

BOOL CALLBACK DialogFuncSavegame(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	char Lstr[256];

	switch(message)
		{
		case WM_INITDIALOG: 
			// center dialog box on CB window
			CenterDialog(hdwnd);
			/* initialize the comboboxes */
			switch(GPDNgame.result)
				{
				case CB_WIN:
					SendDlgItemMessage(hdwnd,IDC_BLACKWINS,BM_SETCHECK,BST_CHECKED,0);
					break;
				case CB_DRAW:
					SendDlgItemMessage(hdwnd,IDC_DRAW,BM_SETCHECK,BST_CHECKED,0);
					break;
				case CB_LOSS:
					SendDlgItemMessage(hdwnd,IDC_WHITEWINS,BM_SETCHECK,BST_CHECKED,0);
					break;
				case CB_UNKNOWN:
					SendDlgItemMessage(hdwnd,IDC_UNKNOWN,BM_SETCHECK,BST_CHECKED,0);
					break;
				}
			
			SetDlgItemText(hdwnd, IDC_BLACKNAME, GPDNgame.black);
			SetDlgItemText(hdwnd, IDC_WHITENAME, GPDNgame.white);
			SetDlgItemText(hdwnd, IDC_EVENT, GPDNgame.event);
			SetDlgItemText(hdwnd, IDC_DATE, GPDNgame.date);
			
			return 1;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_OK:
					// save the results 
					if(SendDlgItemMessage(hdwnd,IDC_UNKNOWN,BM_GETCHECK,0,0))
						sprintf(GPDNgame.resultstring,"*");
					if(SendDlgItemMessage(hdwnd,IDC_DRAW,BM_GETCHECK,0,0))
						sprintf(GPDNgame.resultstring,"1/2-1/2");
					if(SendDlgItemMessage(hdwnd,IDC_BLACKWINS,BM_GETCHECK,0,0))
						sprintf(GPDNgame.resultstring,"1-0");
					if(SendDlgItemMessage(hdwnd,IDC_WHITEWINS,BM_GETCHECK,0,0))
						sprintf(GPDNgame.resultstring,"0-1");
					GetDlgItemText(hdwnd,IDC_BLACKNAME,Lstr,255);
					sprintf(GPDNgame.black,"%s",Lstr);
					GetDlgItemText(hdwnd,IDC_WHITENAME,Lstr,255);
					sprintf(GPDNgame.white,"%s",Lstr);
					GetDlgItemText(hdwnd,IDC_EVENT,Lstr,255);
					sprintf(GPDNgame.event,"%s",Lstr);
					GetDlgItemText(hdwnd, IDC_DATE, Lstr, 255);
					sprintf(GPDNgame.date,"%s",Lstr);
					EndDialog(hdwnd,1); 
					return 1;
				case IDC_CANCEL:
					EndDialog(hdwnd,0);
					return 0;
				}
			break;
		}
	return 0;
	}

BOOL CALLBACK DialogFuncSelectgame(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	// this dialog box appears when the user wants to load a game from
	// a PDN database. it lists the games which are contained in the game_previews[i]
	// array of type gamepreview. The index
	// of the game chosen is written to the external variable gameindex
	int i,n,j;
	HWND hHead;
	char Lstr[256];
	char black[256], white[256];
	extern std::vector<gamepreview> game_previews;
	extern int gameindex;
	HD_NOTIFY *hdnptr;
	HD_ITEM *hdiptr;
	extern RESULT r; // from checkerboard.c
	double percent;
	char c='%';

	// tabulators for the box:
	int cTabs = 4;

	switch(message)
		{
		case WM_INITDIALOG:
			// center dialog box on CB window
			CenterDialog(hdwnd);
			
			// create a header control: 
			hHead=InitHeader(hdwnd);
			ShowWindow(hHead,SW_SHOW);
			i=SendDlgItemMessage(hdwnd,IDC_SELECT,LB_RESETCONTENT,0,0);
			
			Tabs[0]=columns[0];
			Tabs[1]=columns[0]+columns[1];
			Tabs[2]=columns[0]+columns[1]+columns[2];
			for(j=0;j<3;j++)
				Tabs[j]=(Tabs[j]*80)/120;
			SendDlgItemMessage(hdwnd,IDC_SELECT,LB_SETTABSTOPS,(WPARAM)cTabs,(LPARAM)Tabs);
			
			// fill list with games; the data for this is contained
			// in game_previews

			// first, tell the listbox we will be adding lots of data
			//  (WPARAM) wParam,    // number of items
			//  (LPARAM) lParam     // amount of memory
			// this clearly speeds up the display of the dialog with a large number of
			// games (~3 instead of ~10 secs for 22'000 games)
			SendDlgItemMessage(hdwnd, IDC_SELECT, LB_INITSTORAGE, (WPARAM)game_previews.size(), (LPARAM)game_previews.size() * 120);

			for(i=0;i<(int)game_previews.size();i++)
				{
				sprintf(black,"%-.20s",game_previews[i].black);
				if(strlen(game_previews[i].black)>20)
					strcat(black,"...");
				

				sprintf(white,"%-.20s",game_previews[i].white);
				if(strlen(game_previews[i].white)>20)
					strcat(white,"...");
				
				sprintf(Lstr,"%-20.18s\t%-20.18s\t%-20.8s\t%-40.40s",black, white, game_previews[i].result, game_previews[i].event);
				SendDlgItemMessage(hdwnd,IDC_SELECT,LB_ADDSTRING,0,(LPARAM)Lstr); 			
				}

			if(r.draw + r.loss + r.win > 0)
				{
				// display stats in dialog title
				percent = 100.0*((double)r.win+0.5*r.draw)/(double)(r.win+r.draw+r.loss);
				sprintf(Lstr,"Search statistics: %i red wins, %i white wins, %i draws (%.1f%c)",r.win, r.loss,r.draw,percent,c);
				SetWindowText(hdwnd,Lstr);
				}
			return 1;
			break;
		
		case WM_NOTIFY:

			if(LOWORD(wParam == ID_HEADCONTROL))
				{
				hdnptr = (HD_NOTIFY *) lParam;
				hdiptr = (HD_ITEM *) hdnptr->pitem;
				switch(hdnptr->hdr.code)
					{
					case HDN_ENDTRACK:
						// user changed column width
						n=hdnptr->iItem; // column which changed width

						columns[hdnptr->iItem]=hdiptr->cxy;
						Tabs[0]=columns[0];
						Tabs[1]=columns[0]+columns[1];
						Tabs[2]=columns[0]+columns[1]+columns[2];
						for(j=0;j<3;j++)
							Tabs[j]=(Tabs[j]*80)/120;

						SendDlgItemMessage(hdwnd,IDC_SELECT,LB_SETTABSTOPS,(WPARAM)cTabs,(LPARAM)Tabs);
						InvalidateRect(hdwnd,0,1);
						SendMessage(hdwnd,WM_PAINT,0,0);
						break;
					}
				}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_SELECT:
					// a notify message...
					// set move preview 
					i=SendDlgItemMessage(hdwnd,IDC_SELECT,LB_GETCURSEL,0,0L);
					if (i >= 0)
						SetDlgItemText(hdwnd,IDC_PREVIEW,game_previews[i].PDN);
					if(HIWORD(wParam)==LBN_DBLCLK)
						{
						// select the game and end dialog 
						gameindex=SendDlgItemMessage(hdwnd,IDC_SELECT,LB_GETCURSEL,0,0L); 	
						EndDialog(hdwnd,1);
						return 1;
						}
					break;
				case IDC_DELETE:
					// delete the currently selected game 
					gameindex=SendDlgItemMessage(hdwnd,IDC_SELECT,LB_GETCURSEL,0,0L);
					SendDlgItemMessage(hdwnd,IDC_SELECT,LB_DELETESTRING,gameindex,0L);
					
					return 1;
				case IDC_OK:
					// select the game and end dialog 
					gameindex=SendDlgItemMessage(hdwnd,IDC_SELECT,LB_GETCURSEL,0,0L); 	
					EndDialog(hdwnd,1);
					return 1;
				case IDC_CANCEL:
					EndDialog(hdwnd,0);
					return 0;
				default:
					i=SendDlgItemMessage(hdwnd,IDC_SELECT,LB_GETCURSEL,0,0L);
					SetDlgItemText(hdwnd,IDC_PREVIEW,game_previews[i].black);					
					return 1;
				}
			break;
		}
	return 0;
	}

	
HWND InitHeader(HWND hParent)
	{
	HWND hHead;
	RECT rect;
	HD_LAYOUT layout;
	WINDOWPOS winpos;
	HD_ITEM hditem;
	HFONT hFont;

	GetClientRect(hParent,&rect);

	hHead=CreateWindow(WC_HEADER,NULL,WS_CHILD|WS_BORDER,CW_USEDEFAULT,CW_USEDEFAULT,0,0,hParent,(HMENU) ID_HEADCONTROL,g_hInst,NULL);
	hFont =(HFONT) GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hHead,WM_SETFONT,(WPARAM) hFont,MAKELPARAM(TRUE,0));
	// get header control layout that will fit the parent window 
	layout.pwpos=&winpos;
	layout.prc=&rect;
	Header_Layout(hHead, &layout);

	// dimension header to fit the current size of client area 
	MoveWindow(hHead,winpos.x, winpos.y,winpos.cx, winpos.cy,0);
	
	HeaderHeight=winpos.cy; // save height of header 

	// insert items into header 
	hditem.mask = HDI_FORMAT|HDI_WIDTH|HDI_TEXT;
	hditem.pszText="Black";
	hditem.cchTextMax = strlen(hditem.pszText);
	hditem.cxy=columns[0];
	hditem.fmt=HDF_STRING|HDF_LEFT;
	Header_InsertItem(hHead,0,&hditem);

	hditem.pszText="White";
	hditem.cxy=columns[1];
	hditem.cchTextMax = strlen(hditem.pszText);
	Header_InsertItem(hHead,1,&hditem);
	
	hditem.pszText="Result";
	hditem.cxy=columns[2];
	hditem.cchTextMax = strlen(hditem.pszText);
	Header_InsertItem(hHead,2,&hditem);

	hditem.pszText="Event";
	hditem.cxy=columns[3];
	hditem.cchTextMax = strlen(hditem.pszText);
	Header_InsertItem(hHead,3,&hditem);

	ShowWindow(hHead,SW_SHOW);
	return hHead;
	}

BOOL CALLBACK DialogFuncAddcomment(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	// this dialog adds a comment to a move
	char comment[COMMENTLENGTH];

	switch(message)
		{
		case WM_INITDIALOG:
			// center dialog box on CB window
			CenterDialog(hdwnd);
						
			SetDlgItemText(hdwnd, IDC_COMMENT, "");
			if(current->last != NULL)
				SetDlgItemText(hdwnd, IDC_COMMENT, current->last->comment);
			
			// set keyboard focus to IDC_COMMENT?!
			if (GetDlgCtrlID((HWND) wParam) != IDC_COMMENT) 
				{ 
				SetFocus(GetDlgItem(hdwnd, IDC_COMMENT)); 
				return FALSE; 
				} 

			return 1;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_CANCEL:
					EndDialog(hdwnd,0);
					return 1;
				case IDC_OK:
					GetDlgItemText(hdwnd,IDC_COMMENT,comment,-1);
					if(current != head)
						sprintf(current->last->comment,"%s",comment);
		
					EndDialog(hdwnd,0);
					return 1;
				}
			break;
		}
	return 0;
	}


BOOL CALLBACK DialogSearchMask(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	// the search mask dialog
	// it sets the global variables in checkerboard.c which hold
	// name of player, event, date, comments in pdn which the user
	// wants to search for.
	extern char playername[256];	// globals in checkerboard.c
	extern char eventname[256];		// i should wrap these in a struct
	extern char datename[256];		// and pass it as parameter!
	extern char commentname[256];
	extern int searchwithposition;
	
	switch(message)
		{
		case WM_INITDIALOG:
			// center dialog box on CB window
			CenterDialog(hdwnd);
			
			// clear search mask text
			SetDlgItemText(hdwnd, IDC_PLAYERNAME, "");
			SetDlgItemText(hdwnd, IDC_EVENTNAME, "");
			SetDlgItemText(hdwnd, IDC_DATENAME, "");
			SetDlgItemText(hdwnd, IDC_COMMENTNAME, "");

			// clear search with position check box
			SendDlgItemMessage(hdwnd, IDC_SEARCHWITHPOSITION, BM_SETCHECK, 0, 0);
			
			// set focus to the name field
			if (GetDlgCtrlID((HWND) wParam) != IDC_PLAYERNAME) 
				{ 
				SetFocus(GetDlgItem(hdwnd, IDC_PLAYERNAME)); 
				return FALSE; 
				} 

			return 1;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_CANCEL:
					EndDialog(hdwnd,0);
					break;
				case IDC_OK:
					GetDlgItemText(hdwnd,IDC_PLAYERNAME,playername,255);
					GetDlgItemText(hdwnd,IDC_EVENTNAME,eventname,255);
					GetDlgItemText(hdwnd,IDC_DATENAME,datename,255);
					GetDlgItemText(hdwnd,IDC_COMMENTNAME,commentname,255);
					searchwithposition = (int) SendDlgItemMessage(hdwnd,IDC_SEARCHWITHPOSITION,BM_GETCHECK,0,0);
					
					EndDialog(hdwnd,1);
					break;
				}
			break;
		}
	return 0;
	}


BOOL CALLBACK DialogFuncEnginecommand(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	// allow entry of an engine command.
	char command[256], reply[ENGINECOMMAND_REPLY_SIZE];

	switch(message)
		{
		case WM_INITDIALOG:
			// center dialog box on CB window
			CenterDialog(hdwnd);
			
			SetDlgItemText(hdwnd, IDC_COMMAND, "");
			
			// set keyboard focus to IDC_COMMAND
			if (GetDlgCtrlID((HWND) wParam) != IDC_COMMAND) 
				{ 
				SetFocus(GetDlgItem(hdwnd, IDC_COMMAND)); 
				return FALSE; 
				} 
			return 1;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_CANCEL:
					EndDialog(hdwnd,0);
					return 1;
				case IDC_OK:
					// get the command
					GetDlgItemText(hdwnd,IDC_COMMENT,command,255);
					// issue it and display reply
					enginecommand(command,reply);
					MessageBox(hdwnd,reply,"Engine reply:",MB_OK|MB_ICONINFORMATION);		
					EndDialog(hdwnd,0);
					return 1;
				}
			break;
		}
	return 0;
	}


BOOL CALLBACK DirectoryDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	// this dialog sets the global variables
	//	userdirectory and matchdirectory 
	extern char userdirectory[256],matchdirectory[256];

	switch(message)
		{
		case WM_INITDIALOG:
			// center dialog box on CB window
			CenterDialog(hdwnd);
			SetDlgItemText(hdwnd, IDC_USER, gCBoptions.userdirectory);
			SetDlgItemText(hdwnd, IDC_MATCH, gCBoptions.matchdirectory);
			return 1;
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
				{
				case IDC_CANCEL:
					EndDialog(hdwnd,0);
					return 1;
				case IDC_OK:
					GetDlgItemText(hdwnd,IDC_USER,gCBoptions.userdirectory,255);
					GetDlgItemText(hdwnd,IDC_MATCH,gCBoptions.matchdirectory,255);
					EndDialog(hdwnd,0);
					return 1;
				}
			break;
		}
	return 0;
	}


/*
 * Extract just the filename from the full file specification.
 */
int extract_filename(char *filespec, char *name)
{
	int i, len;

	len = (int)strlen(filespec);
	for (i = len - 1; i >= 0; --i)
		if (filespec[i] == '\\' || filespec[i] == '/' || filespec[i] == ':')
			break;

	strcpy(name, filespec + i + 1);
	return(0);
}


BOOL CALLBACK DialogFuncMoreOptions(HWND hwnd, UINT msg, UINT wparam, LONG lparam)
{
	char buf[MAX_PATH];
	static MORE_ENGINE_OPTIONS current_options, new_options;
	char command[256], reply[ENGINECOMMAND_REPLY_SIZE];

	switch (msg) {
	case WM_INITDIALOG:
		// center dialog box on CB window
		CenterDialog(hwnd);
		get_more_engine_options(hwnd, &current_options);
		new_options = current_options;
		return 1;
		break;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_CANCEL:
			EndDialog(hwnd, 0);
			return(1);

		case IDC_BOOKFILE_BROWSE_BUTTON:
			SetCurrentDirectory(gCBoptions.userdirectory);
			SendDlgItemMessage(hwnd, IDC_BOOKFILE_EDIT, WM_GETTEXT, sizeof(new_options.book_filename), (LPARAM)new_options.book_filename);
			if (getfilename(buf, OF_BOOKFILE)) {
				extract_filename(buf, new_options.book_filename);
				SendDlgItemMessage(hwnd, IDC_BOOKFILE_EDIT, WM_SETTEXT, 0, (LPARAM)new_options.book_filename);
			}
			SetCurrentDirectory(CBdirectory);
			return(1);

		case IDC_BROWSE_WLD_DIR_BUTTON:
			SendDlgItemMessage(hwnd, IDC_WLD_DIR_EDIT, WM_GETTEXT, sizeof(new_options.wld_directory), (LPARAM)new_options.wld_directory);
			if (browse_to_dir("Choose the WLD database directory", new_options.wld_directory))
				SendDlgItemMessage(hwnd, IDC_WLD_DIR_EDIT, WM_SETTEXT, 0, (LPARAM)new_options.wld_directory);
			return(1);

		case IDC_BROWSE_MTC_DIR_BUTTON:
			SendDlgItemMessage(hwnd, IDC_MTC_DIR_EDIT, WM_GETTEXT, sizeof(new_options.mtc_directory), (LPARAM)new_options.mtc_directory);
			if (browse_to_dir("Choose the MTC database directory", new_options.mtc_directory))
				SendDlgItemMessage(hwnd, IDC_MTC_DIR_EDIT, WM_SETTEXT, 0, (LPARAM)new_options.mtc_directory);
			return(1);

		case IDC_CHECK_WLD_DIR_BUTTON:
			get_more_engine_options_from_dialog(hwnd, &new_options);
			sprintf(command, "set check_wld_dir %s", new_options.wld_directory);
			enginecommand(command, reply);
			MessageBox(hwnd, reply, "WLD Endgame Database Directory Check", MB_OK);
			return(1);

		case IDC_CHECK_MTC_DIR_BUTTON:
			get_more_engine_options_from_dialog(hwnd, &new_options);
			sprintf(command, "set check_mtc_dir %s", new_options.mtc_directory);
			enginecommand(command, reply);
			MessageBox(hwnd, reply, "MTC Endgame Database Directory Check", MB_OK);
			return(1);

		case IDC_OK:
			get_more_engine_options_from_dialog(hwnd, &new_options);
			set_more_engine_options(hwnd, &current_options, &new_options);
			EndDialog(hwnd, 0);
			return(1);
		}
		break;
	}
	return 0;
}


BOOL CALLBACK EngineOptionsFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	// sets engine options for primary and secondary engines.
	// TODO: if settings are not changed, but OK pressed, don't send command to the
	// engine
	char Lstr[256];
	static ENGINE_OPTIONS primaryoptions;
	static ENGINE_OPTIONS secondaryoptions;
	static ENGINE_OPTIONS currentoptions;
	
	int rbprimary,rbsecondary;
	static int availableMB;
	int i, egdb_max;


	extern char reply[256];
	extern int currentengine;
	extern int togglebook;

	MEMORYSTATUSEX memstat;

	switch (message)
		{
		case WM_INITDIALOG:
			if(getenginebusy() == TRUE)
				EndDialog(hdwnd,0);
			// center dialog box on CB window
			CenterDialog(hdwnd);
			
			// display the current engine
			enginename(Lstr);
			SetDlgItemText(hdwnd,IDC_STATIC,Lstr);
			
			// display options for the primary engine: set radio button choice to primary engine
			if(currentengine == 1)
				SendDlgItemMessage(hdwnd, IDC_PRIMARY, BM_SETCHECK,1,0);
			else
				SendDlgItemMessage(hdwnd, IDC_SECONDARY, BM_SETCHECK,1,0);
		
			// initialize hashtable size combobox
			// get global memory size
			memstat.dwLength = sizeof(memstat);
			GlobalMemoryStatusEx(&memstat);
			availableMB = (int)(memstat.ullTotalPhys / (1024*1024));

			i = 8; 
			while (i <= availableMB/2) {
				sprintf(Lstr, "%i", i);
				SendDlgItemMessage(hdwnd,IDC_HASHSIZE,LB_ADDSTRING,0,(LPARAM)Lstr);
				i *= 2;
			}
			
			if (availableMB < 1024)
				egdb_max = availableMB - 64;
			else if (availableMB < 4096)
				egdb_max = availableMB - 512;
			else
				egdb_max = availableMB - 1024;

			// initialize EGDB combobox
			for (i = 0; i < egdb_max; i += 64) {
				if (i > 16384 && (i % 1024))
					continue;
				if (i > 4096 && (i % 256))
					continue;

				sprintf(Lstr, "%i", i);
				SendDlgItemMessage(hdwnd,IDC_EGDBSIZE,LB_ADDSTRING,0,(LPARAM)Lstr);
			}

			// get engine options
			getengineoptions(hdwnd, &currentoptions);
			if(currentengine == 1)
				primaryoptions = currentoptions;
			else
				secondaryoptions = currentoptions;
			break;

		case WM_COMMAND:
			// find out which engine we are setting params for:
			switch(LOWORD(wParam))
				{
				case IDC_PRIMARY:   //fall through
				case IDC_SECONDARY: //if a radiobutton for primary or secondary engine
									// was clicked, update data in dialog box
					// TODO: this is so ugly - this code is a duplicate from case WM_INIT
					rbprimary = (int) SendDlgItemMessage(hdwnd, IDC_PRIMARY, BM_GETCHECK, 0, 0);
					rbsecondary = (int) SendDlgItemMessage(hdwnd, IDC_SECONDARY, BM_GETCHECK, 0, 0);
					
					if(rbprimary)
						setcurrentengine(1);
					if(rbsecondary)
						setcurrentengine(2);
					
					enginename(Lstr);
					SetDlgItemText(hdwnd,IDC_STATIC,Lstr);
					getengineoptions(hdwnd, &currentoptions);
					break;

				case IDCANCEL:
					EndDialog(hdwnd,0);
					return 1;

				case IDC_OK:
					// find out which engine we are setting params for:
					rbprimary = (int) SendDlgItemMessage(hdwnd, IDC_PRIMARY, BM_GETCHECK, 0, 0);
					rbsecondary = (int ) SendDlgItemMessage(hdwnd, IDC_SECONDARY, BM_GETCHECK, 0, 0);
					
					getoptionsfromdialog(hdwnd, &currentoptions);
					if(rbprimary)
						{
						setcurrentengine(1);
						setengineoptions(hdwnd, availableMB, &primaryoptions, &currentoptions);
						}
					if(rbsecondary)
						{
						setcurrentengine(2);
						setengineoptions(hdwnd, availableMB, &secondaryoptions, &currentoptions);
						}
					
					EndDialog(hdwnd,0);
					return 1;

				case IDC_APPLY:
					// TODO: more duplicate code!
					// find out which engine we are setting params for:
					rbprimary = (int) SendDlgItemMessage(hdwnd, IDC_PRIMARY, BM_GETCHECK, 0, 0);
					rbsecondary = (int) SendDlgItemMessage(hdwnd, IDC_SECONDARY, BM_GETCHECK, 0, 0);
					
					getoptionsfromdialog(hdwnd, &currentoptions);
					if(rbprimary)
						{
						setcurrentengine(1);
						setengineoptions(hdwnd, availableMB, &primaryoptions, &currentoptions);
						primaryoptions = currentoptions;
						}
					if(rbsecondary)
						{
						setcurrentengine(2);
						setengineoptions(hdwnd, availableMB, &secondaryoptions, &currentoptions);
						secondaryoptions = currentoptions;
						}
					return 1;

				case IDC_MORE_OPTIONS_BUTTON:
					DialogBox((HINSTANCE)GetWindowLongPtr(hdwnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDD_MORE_ENGINE_OPTIONS_DIALOG), hdwnd, (DLGPROC)DialogFuncMoreOptions);
					break;
				}
		return 1;
		}
	return 0;
	}

int getoptionsfromdialog(HWND hdwnd, ENGINE_OPTIONS *options)
	{
	char Lstr[256];
	int index;

	// set hashtable size
	index = (int) SendDlgItemMessage(hdwnd, IDC_HASHSIZE,LB_GETCURSEL,0,0L);
	if(index>-1)
		{
		SendDlgItemMessage(hdwnd, IDC_HASHSIZE,LB_GETTEXT,index,(LPARAM)Lstr);
		options->hash_MB = atoi(Lstr);
		}

	// set egdb cache size
	index =  (int) SendDlgItemMessage(hdwnd, IDC_EGDBSIZE,LB_GETCURSEL,0,0L);
	if(index>-1)
		{
		SendDlgItemMessage(hdwnd, IDC_EGDBSIZE,LB_GETTEXT,index,(LPARAM)Lstr);
		options->db_MB = atoi(Lstr);
		}

	// set opening book on/off
	if(SendDlgItemMessage(hdwnd,IDC_BOOKOFF,BM_GETCHECK,0,0))
		options->book = 0;
	if(SendDlgItemMessage(hdwnd,IDC_BOOKALLKINDS,BM_GETCHECK,0,0))
		options->book = 1;
	if(SendDlgItemMessage(hdwnd,IDC_BOOKGOOD,BM_GETCHECK,0,0))
		options->book = 2;
	if(SendDlgItemMessage(hdwnd,IDC_BOOKBEST,BM_GETCHECK,0,0))
		options->book = 3;

	// set all scores on/off
	options->allscores = (int) SendDlgItemMessage(hdwnd,IDC_ALLSCORES,BM_GETCHECK,0,0);
	
	return 1;
	}

int setengineoptions(HWND hdwnd, int availableMB, ENGINE_OPTIONS *oldoptions, ENGINE_OPTIONS *newoptions)
	{
	char Lstr[256];
	char commandstring[256];
	char reply[256];
	
	if(newoptions->db_MB + newoptions->hash_MB > availableMB-32)
		{
		sprintf(Lstr,
				"You have selected a combined\n"
				"hashtable (%i) and endgame cache (%i)\n"
				"size which is too large for\n"
				"your system (%i). Please reduce\n"
				"the memory requirements.",
				newoptions->hash_MB, newoptions->db_MB, availableMB);
		MessageBox(hdwnd,Lstr,"Error",MB_OK);
		return 1;
		}
	
	if(newoptions->hash_MB != oldoptions->hash_MB)
		{
		sprintf(commandstring,"set hashsize %i",newoptions->hash_MB);
		enginecommand(commandstring,reply);
		}

	if(newoptions->db_MB != oldoptions->db_MB)
		{
		sprintf(commandstring,"set dbmbytes %i",newoptions->db_MB);
		enginecommand(commandstring,reply);
		if(strcmp(reply,"") != 0)
			MessageBox(hwnd,reply,"Engine reply",MB_OK);
		}
			
	if(newoptions->book != oldoptions->book)
		{
		sprintf(commandstring,"set book %i",newoptions->book);
		enginecommand(commandstring,reply);
		}
	
	// set all scores on/off
	if(newoptions->allscores != oldoptions->allscores)
		{
		sprintf(commandstring,"set allscores %i",newoptions->allscores);
		enginecommand(commandstring,reply);
		}
	
	return 1;
	}	


int getengineoptions(HWND hdwnd, ENGINE_OPTIONS *options)
	{
	char reply[256];
	HWND hwndControl;	

	//get current options for current engine
	// hashtable size
	if(enginecommand("get hashsize",reply))
		{
		SendDlgItemMessage(hdwnd,IDC_HASHSIZE,LB_SELECTSTRING,-1,(LPARAM)reply);
		options->hash_MB = atoi(reply);
		}
	else
		SendDlgItemMessage(hdwnd,IDC_HASHSIZE,LB_SETCURSEL,-1,0);

	hwndControl = GetDlgItem(hdwnd,IDC_HASHSIZE);
	if(strcmp(reply,"?")==0)
		// engine does not support hashtable size change
		EnableWindow(hwndControl,0);		
	else
		EnableWindow(hwndControl,1);

	// egtb cache size
	if(enginecommand("get dbmbytes",reply))
		{
		SendDlgItemMessage(hdwnd,IDC_EGDBSIZE,LB_SELECTSTRING,-1,(LPARAM)reply);
		options->db_MB = atoi(reply);
		}
	else
		SendDlgItemMessage(hdwnd,IDC_EGDBSIZE,LB_SETCURSEL,-1,0);
		
	hwndControl = GetDlgItem(hdwnd,IDC_EGDBSIZE);
	if(strcmp(reply,"?")==0)
		EnableWindow(hwndControl,0);
	else
		EnableWindow(hwndControl,1);

	// opening book
	if(enginecommand("get book",reply))
		options->book = atoi(reply);
		
	if(strcmp(reply,"?")==0)
		{
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKOFF);
		EnableWindow(hwndControl,0);
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKALLKINDS);
		EnableWindow(hwndControl,0);
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKGOOD);
		EnableWindow(hwndControl,0);
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKBEST);
		EnableWindow(hwndControl,0);
		}
	else
		{
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKOFF);
		EnableWindow(hwndControl,1);
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKALLKINDS);
		EnableWindow(hwndControl,1);
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKGOOD);
		EnableWindow(hwndControl,1);
		hwndControl = GetDlgItem(hdwnd,IDC_BOOKBEST);
		EnableWindow(hwndControl,1);
		}

	SendDlgItemMessage(hdwnd, IDC_BOOKOFF, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hdwnd, IDC_BOOKALLKINDS, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hdwnd, IDC_BOOKGOOD, BM_SETCHECK, 0, 0);
	SendDlgItemMessage(hdwnd, IDC_BOOKBEST, BM_SETCHECK, 0, 0);
	switch(options->book)
		{
		// can be 0 (off) ... 3 (best moves)
		case 0:
			SendDlgItemMessage(hdwnd, IDC_BOOKOFF, BM_SETCHECK,1,0);
			break;
		case 1:
			SendDlgItemMessage(hdwnd, IDC_BOOKALLKINDS, BM_SETCHECK,1,0);
			break;
		case 2:
			SendDlgItemMessage(hdwnd, IDC_BOOKGOOD, BM_SETCHECK,1,0);
			break;
		case 3: 
			SendDlgItemMessage(hdwnd, IDC_BOOKBEST, BM_SETCHECK,1,0);
			break;
		}

	// allscores
	if(enginecommand("get allscores",reply))
		options->allscores = atoi(reply);
		
	hwndControl = GetDlgItem(hdwnd,IDC_ALLSCORES);
	if(strcmp(reply,"?")==0)
		EnableWindow(hwndControl,0);
	else
		EnableWindow(hwndControl,1);

	SendDlgItemMessage(hdwnd, IDC_ALLSCORES, BM_SETCHECK, options->allscores,0);

	/* See if we should enable the "More Options..." button. */
	hwndControl = GetDlgItem(hdwnd, IDC_MORE_OPTIONS_BUTTON);
	if (enginecommand("get dbpath", reply))
		EnableWindow(hwndControl, 1);
	else
		EnableWindow(hwndControl, 0);		

	return 1;
	}


int get_more_engine_options(HWND hwnd, MORE_ENGINE_OPTIONS *options)
{
	int i;
	char reply[MAX_PATH];
	char buf[100];
	HWND hctrl;	

	/* Check if the engine supports the "dbpath" engine command. */
	hctrl = GetDlgItem(hwnd, IDC_WLD_DIR_EDIT);
	if (enginecommand("get dbpath", options->wld_directory)) {
		EnableWindow(hctrl, 1);
		SetDlgItemText(hwnd, IDC_WLD_DIR_EDIT, (LPSTR)options->wld_directory);

		/* Enable the browse button. */
		hctrl = GetDlgItem(hwnd, IDC_BROWSE_WLD_DIR_BUTTON); 
		EnableWindow(hctrl, 1);

		/* Enable the check button. */
		hctrl = GetDlgItem(hwnd, IDC_CHECK_WLD_DIR_BUTTON); 
		EnableWindow(hctrl, 1);
	}
	else {
		options->wld_directory[0] = 0;
		EnableWindow(hctrl, 0);
		SetDlgItemText(hwnd, IDC_WLD_DIR_EDIT, (LPSTR)options->wld_directory);

		/* Disable the browse button. */
		hctrl = GetDlgItem(hwnd, IDC_BROWSE_WLD_DIR_BUTTON); 
		EnableWindow(hctrl, 0);

		/* Disable the check button. */
		hctrl = GetDlgItem(hwnd, IDC_CHECK_WLD_DIR_BUTTON); 
		EnableWindow(hctrl, 0);
	}

	/* Check if the engine supports the "enable_wld" engine command. */
	hctrl = GetDlgItem(hwnd, IDC_WLD_ENAB_CHECK);
	if (enginecommand("get enable_wld", reply)) {
		EnableWindow(hctrl, 1);
		options->wld_egdb_enable = atoi(reply);
	}
	else {
		EnableWindow(hctrl, 0);
		options->wld_egdb_enable = 0;
	}
	SendMessage(hctrl, BM_SETCHECK, options->wld_egdb_enable, 0); 

	/* Check if the engine supports the "max_dbpieces" engine command. */
	hctrl = GetDlgItem(hwnd, IDC_MAX_PIECES_COMBO);
	if (enginecommand("get max_dbpieces", reply)) {
		EnableWindow(hctrl, 1);
		options->max_dbpieces = atoi(reply);
		for (i = 0; i < ARRAY_SIZE(limit_pieces); ++i) {
			if (limit_pieces[i] < 24)
				sprintf(buf, "%d", limit_pieces[i]);
			else
				sprintf(buf, "No limit");
			SendDlgItemMessage(hwnd, IDC_MAX_PIECES_COMBO, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)buf);

			/* Set the initial selection. */
			if (limit_pieces[i] == options->max_dbpieces)
				SendDlgItemMessage(hwnd, IDC_MAX_PIECES_COMBO, CB_SETCURSEL, i, 0);
		}
	}
	else {
		EnableWindow(hctrl, 0);
	}

	/* Check if the engins supports the "enable_mtc" engine command. */
	hctrl = GetDlgItem(hwnd, IDC_MTC_ENAB_CHECK);
	if (enginecommand("get enable_mtc", reply)) {
		EnableWindow(hctrl, 1);
		options->mtc_egdb_enable = atoi(reply);
	}
	else {
		EnableWindow(hctrl, 0);
		options->mtc_egdb_enable = 0;
	}
	SendMessage(hctrl, BM_SETCHECK, options->mtc_egdb_enable, 0); 

	/* Check if the engine supports the "mtcpath" engine command. */
	hctrl = GetDlgItem(hwnd, IDC_MTC_DIR_EDIT);
	if (enginecommand("get mtcpath", options->mtc_directory)) {
		EnableWindow(hctrl, 1);
		SetDlgItemText(hwnd, IDC_MTC_DIR_EDIT, (LPSTR)options->mtc_directory);

		/* Enable the browse button. */
		hctrl = GetDlgItem(hwnd, IDC_BROWSE_MTC_DIR_BUTTON); 
		EnableWindow(hctrl, 1);

		/* Enable the check button. */
		hctrl = GetDlgItem(hwnd, IDC_CHECK_MTC_DIR_BUTTON); 
		EnableWindow(hctrl, 1);
	}
	else {
		options->mtc_directory[0] = 0;
		EnableWindow(hctrl, 0);
		SetDlgItemText(hwnd, IDC_MTC_DIR_EDIT, (LPSTR)options->mtc_directory);

		/* Disable the browse button. */
		hctrl = GetDlgItem(hwnd, IDC_BROWSE_MTC_DIR_BUTTON); 
		EnableWindow(hctrl, 0);

		/* Disable the check button. */
		hctrl = GetDlgItem(hwnd, IDC_CHECK_MTC_DIR_BUTTON); 
		EnableWindow(hctrl, 0);
	}

	/* Check if the engine supports the "bookfile" engine command. */
	hctrl = GetDlgItem(hwnd, IDC_BOOKFILE_EDIT);
	if (enginecommand("get bookfile", options->book_filename)) {
		EnableWindow(hctrl, 1);
		SetDlgItemText(hwnd, IDC_BOOKFILE_EDIT, (LPSTR)options->book_filename);

		/* Enable the browse button. */
		hctrl = GetDlgItem(hwnd, IDC_BOOKFILE_EDIT); 
		EnableWindow(hctrl, 1);
	}
	else {
		options->book_filename[0] = 0;
		EnableWindow(hctrl, 0);
		SetDlgItemText(hwnd, IDC_BOOKFILE_EDIT, (LPSTR)options->book_filename);

		/* Disable the browse button. */
		hctrl = GetDlgItem(hwnd, IDC_BOOKFILE_EDIT); 
		EnableWindow(hctrl, 0);
	}

	/* Check if the engine supports the "searchthreads" engine command. */
	hctrl = GetDlgItem(hwnd, IDC_MAX_SEARCH_THREADS_COMBO);
	if (enginecommand("get searchthreads", reply)) {
		options->search_threads = atoi(reply);
		EnableWindow(hctrl, 1);

		/* Enable IDC_CPUS_EDIT control. */
		if (enginecommand("get cpus", reply)) {
			hctrl = GetDlgItem(hwnd, IDC_CPUS_EDIT); 
			EnableWindow(hctrl, 1);
			SetDlgItemText(hwnd, IDC_CPUS_EDIT, (LPSTR)reply);
		}

		for (i = 0; i < ARRAY_SIZE(thread_limits); ++i) {
			sprintf(buf, "%d", thread_limits[i]);
	        SendDlgItemMessage(hwnd, IDC_MAX_SEARCH_THREADS_COMBO, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)buf);

			/* Set the initial selection. */
			if (thread_limits[i] == options->search_threads)
				SendDlgItemMessage(hwnd, IDC_MAX_SEARCH_THREADS_COMBO, CB_SETCURSEL, i, 0);
		}
	}
	else {
		EnableWindow(hctrl, 0);

		/* Disable IDC_CPUS_EDIT control. */
		hctrl = GetDlgItem(hwnd, IDC_CPUS_EDIT); 
		EnableWindow(hctrl, 0);
	}

	return(1);
}


/*
 * The user OK'd out of the dialog, so read the settings from the controls
 * and send any changed settings to the engine.
 */
int get_more_engine_options_from_dialog(HWND hwnd, MORE_ENGINE_OPTIONS *options)
{
	HWND hctrl;	
	LRESULT status;

	/* Get the wld enable checkbox setting. */
	hctrl = GetDlgItem(hwnd, IDC_WLD_ENAB_CHECK);
	status = SendMessage(hctrl, BM_GETCHECK, 0, 0); 
	if (status == BST_CHECKED)
		options->wld_egdb_enable = 1;
	else if (status == BST_UNCHECKED)
		options->wld_egdb_enable = 0;

	/* Get the wld directory text box string. */
	hctrl = GetDlgItem(hwnd, IDC_WLD_DIR_EDIT);
	SendMessage(hctrl, WM_GETTEXT, sizeof(options->wld_directory), (LPARAM)options->wld_directory); 

	/* Get the 'max pieces' setting. */
	status = SendDlgItemMessage(hwnd, IDC_MAX_PIECES_COMBO, CB_GETCURSEL, 0, 0);
	if (status >= 0)
		options->max_dbpieces = limit_pieces[status];

	/* Get the mtc enable checkbox setting. */
	hctrl = GetDlgItem(hwnd, IDC_MTC_ENAB_CHECK);
	status = SendMessage(hctrl, BM_GETCHECK, 0, 0); 
	if (status == BST_CHECKED)
		options->mtc_egdb_enable = 1;
	else if (status == BST_UNCHECKED)
		options->mtc_egdb_enable = 0;

	/* Get the mtc directory text box string. */
	hctrl = GetDlgItem(hwnd, IDC_MTC_DIR_EDIT);
	SendMessage(hctrl, WM_GETTEXT, sizeof(options->mtc_directory), (LPARAM)options->mtc_directory); 

	/* Get the bookfile text box setting. */
	hctrl = GetDlgItem(hwnd, IDC_BOOKFILE_EDIT);
	SendMessage(hctrl, WM_GETTEXT, sizeof(options->book_filename), (LPARAM)options->book_filename);

	/* Get the max search threads combo box setting. */
	status = SendDlgItemMessage(hwnd, IDC_MAX_SEARCH_THREADS_COMBO, CB_GETCURSEL, 0, 0);
	if (status >= 0)
		options->search_threads = thread_limits[status];

	return(1);
}


int set_more_engine_options(HWND hwnd, MORE_ENGINE_OPTIONS *old_options, MORE_ENGINE_OPTIONS *new_options)
{
	char command[MAX_PATH + 50], reply[ENGINECOMMAND_REPLY_SIZE];

	/* See if max_dbpieces changed. */
	if (new_options->max_dbpieces != old_options->max_dbpieces) {
		sprintf(command, "set max_dbpieces %d", new_options->max_dbpieces);
		enginecommand(command, reply);
	}

	/* See if the wld_directory changed. */
	if (strcmp(new_options->wld_directory, old_options->wld_directory)) {
		sprintf(command, "set dbpath %s", new_options->wld_directory);
		enginecommand(command, reply);
	}

	/* See if the mtc_directory changed. */
	if (strcmp(new_options->mtc_directory, old_options->mtc_directory)) {
		sprintf(command, "set mtcpath %s", new_options->mtc_directory);
		enginecommand(command, reply);
	}

	/* See if the bookfile changed. */
	if (strcmp(new_options->book_filename, old_options->book_filename)) {
		sprintf(command, "set bookfile %s", new_options->book_filename);
		enginecommand(command, reply);
	}

	/* See if enable_wld changed. */
	if (new_options->wld_egdb_enable != old_options->wld_egdb_enable) {
		sprintf(command, "set enable_wld %d", new_options->wld_egdb_enable);
		enginecommand(command, reply);
	}

	/* See if enable_mtc changed. */
	if (new_options->mtc_egdb_enable != old_options->mtc_egdb_enable) {
		sprintf(command, "set enable_mtc %d", new_options->mtc_egdb_enable);
		enginecommand(command, reply);
	}

	/* See if search_threads changed. */
	if (new_options->search_threads != old_options->search_threads) {
		sprintf(command, "set searchthreads %d", new_options->search_threads);
		enginecommand(command, reply);
	}
	return(1);
}


BOOL CALLBACK EngineDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	/* this dialog box lets the user change the engines CheckerBoard is using.
		it uses hInstLib to open
		a library for checking, this, afterwards hInstLib1,..2,..A for real loading.
		
		if the game type of the new primary engine is not compatible with the current game type,
		a GAMENEW command is posted.
		
		if the new primary engine is playing dama italiana (gametype 22) the mirror flag is set.
	*/
	int i;
	
	char pri_fname[256], sec_fname[256];
	extern char CBdirectory[256];
	extern struct CBoptions gCBoptions;
	
	switch(message)
		{
		case WM_INITDIALOG: 
			// center dialog box on CB window
			CenterDialog(hdwnd);
			
			// set the working directory to CBdirectory so that engines are visible
			SetCurrentDirectory(CBdirectory);
			SetCurrentDirectory("engines");
			
			// initialize the comboboxes 
			SendDlgItemMessage(hdwnd,IDC_PRIMARY,LB_DIR,0,(LPARAM)"*.dll");
			SendDlgItemMessage(hdwnd,IDC_SECONDARY,LB_DIR,0,(LPARAM)"*.dll");
			SendDlgItemMessage(hdwnd,IDC_PRIMARY,LB_SELECTSTRING,-1,(LPARAM)gCBoptions.primaryenginestring);
			SendDlgItemMessage(hdwnd,IDC_SECONDARY,LB_SELECTSTRING,-1,(LPARAM)gCBoptions.secondaryenginestring);
			return 1;
		
		case WM_COMMAND:

			switch(LOWORD(wParam))
				{
				case IDCANCEL:
					EndDialog(hdwnd,0);
					SetCurrentDirectory(CBdirectory);
					return 1;

				case IDC_OK:
					
					i = (int)SendDlgItemMessage(hdwnd, IDC_PRIMARY,LB_GETCURSEL,0,0L);
					if (i > -1)
						SendDlgItemMessage(hdwnd, IDC_PRIMARY,LB_GETTEXT,i,(LPARAM)pri_fname);
					else
						pri_fname[0] = 0;

					// new version code below
					i = (int)SendDlgItemMessage(hdwnd, IDC_SECONDARY,LB_GETCURSEL,0,0L);
					if (i > -1)
						SendDlgItemMessage(hdwnd, IDC_SECONDARY,LB_GETTEXT,i,(LPARAM)sec_fname);
					else
						sec_fname[0] = 0;
					
					// if we have a new engine, call loadengines.
					if (strcmp(gCBoptions.primaryenginestring, pri_fname) != 0 || strcmp(gCBoptions.secondaryenginestring, sec_fname) != 0)
						loadengines(pri_fname, sec_fname);
					SetCurrentDirectory(CBdirectory);
	
					EndDialog(hdwnd,0);
					return 1;
				}
			break;
		}
	return 0;
	}

int CenterDialog(HWND hdwnd)
	{
	// centers a dialog box on the main window
	RECT rcOwner;
	RECT rcDlg;
	RECT rc;
	HWND hwndOwner;

	// Get the owner window and dialog box rectangles. 
	if ((hwndOwner = GetParent(hdwnd)) == NULL) 
		hwndOwner = GetDesktopWindow(); 
		
	GetWindowRect(hwndOwner, &rcOwner); 
	GetWindowRect(hdwnd, &rcDlg); 
	CopyRect(&rc, &rcOwner); 

	// Offset the owner and dialog box rectangles so that 
	// right and bottom values represent the width and 
	// height, and then offset the owner again to discard 
	// space taken up by the dialog box. 

	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
	OffsetRect(&rc, -rc.left, -rc.top); 
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

	// The new position is the sum of half the remaining 
	// space and the owner's original position. 

	SetWindowPos(hdwnd, 
		HWND_TOP, 
		rcOwner.left + (rc.right / 2), 
		rcOwner.top + (rc.bottom / 2), 
		0, 0,          // ignores size arguments 
		SWP_NOSIZE); 
	return 1;
	}