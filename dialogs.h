typedef struct {
	int db_MB;
	int hash_MB;
	int book;
	int allscores;
	int solve;
} ENGINE_OPTIONS;

typedef struct {
	int max_dbpieces;			/* limits the maximum number of pieces to use in egdb init/lookups. */
	int wld_egdb_enable;
	char wld_directory[MAX_PATH];
	int mtc_egdb_enable;
	char mtc_directory[MAX_PATH];
	char book_filename[MAX_PATH];
	int search_threads;
} MORE_ENGINE_OPTIONS;


BOOL CALLBACK AboutDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ThreeMoveDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogFuncSavegame(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogFuncSelectgame(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogFuncAddcomment(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogSearchMask(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogFuncEnginecommand(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DirectoryDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EngineOptionsFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EngineDialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogIncrementalTimesFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogStartEngineMatchFunc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
int CenterDialog(HWND hdwnd);
int setengineoptions(HWND hdwnd, int availableMB, ENGINE_OPTIONS *oldoptions, ENGINE_OPTIONS *newoptions);
int getengineoptions(HWND hdwnd, ENGINE_OPTIONS *options);
int getoptionsfromdialog(HWND hdwnd, ENGINE_OPTIONS *options);
int get_more_engine_options(HWND hwnd, MORE_ENGINE_OPTIONS *options);
int set_more_engine_options(HWND hwnd, MORE_ENGINE_OPTIONS *old_options, MORE_ENGINE_OPTIONS *new_options);
int get_more_engine_options_from_dialog(HWND hwnd, MORE_ENGINE_OPTIONS *options);


/* engine dialog */
#define IDC_OK 1
#define IDC_CANCEL 2
#define IDC_APPLY 3
#define IDC_SECONDARY 102
#define IDC_PRIMARY 101

/* engine options dialog */
#define IDC_BOOKBEST 1005
#define IDC_BOOKGOOD 1006
#define IDC_BOOKALLKINDS 1007
#define IDC_BOOKOFF 1008
#define IDC_HASHSIZE 1002
#define IDC_EGDBSIZE 1003
#define IDC_STATIC 1004
#define IDC_ALLSCORES 1001
#define IDC_SOLVE 1009

/*3-move dialog */
#define IDC_CROSSBOARD 1000
#define IDC_MAILPLAY 1001
#define IDC_BARRED 1002

/* directory dialog*/
#define IDC_USER 1001
#define IDC_MATCH 1002
#define IDC_BMP 1003

/* save game dialog*/
#define IDC_BLACKNAME 1000
#define IDC_WHITENAME 1002
#define IDC_EVENT 1004
#define IDC_DATE 1009
#define IDC_BLACKWINS 1005
#define IDC_WHITEWINS 1006
#define IDC_DRAW 1007
#define IDC_UNKNOWN 1008

/* comment dialog */
#define IDC_COMMENT 1001

// engine command dialog
#define IDC_COMMAND 1001

// search mask dialog
#define IDC_PLAYERNAME 1001
#define IDC_EVENTNAME 1002
#define IDC_DATENAME 1003
#define IDC_COMMENTNAME 1004
#define IDC_SEARCHWITHPOSITION 1005

#define ICON1 11111

/* select game dialog */
/* header control */
#define ID_HEADCONTROL 500
#define PLAYERWIDTH 120
#define RESULTWIDTH 80
#define EVENTWIDTH 440
#define NUMCOLS 4
#define MINWIDTH 10
#define SPACING 8
#define IDC_SELECT 102
#define IDC_DELETE 3
#define IDC_PREVIEW 101

/* Incremental time settings dialog. */


extern int selected_game;
