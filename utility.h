void log_fen(char *msg, int board[8][8], int color);
void log_bitboard(char *msg, int32 black, int32 white, int32 king);
void	builddb(char *str);
int		builtingametype(void);
void	CBlog(char *text);
int		checklevelmenu(HMENU hmenu,int item, struct CBoptions *CBoptions);
int		errorlog(char *str);
int extract_path(char *name, char *path);
int		FENtoclipboard(HWND hwnd, int board8[8][8], int color, int gametype);
int		fileispresent(char *filename);
int		getopening(struct CBoptions *CBoptions);
int		getthreeopening(int n, struct CBoptions *CBoptions);
int		initcolorstruct(HWND hwnd, CHOOSECOLOR *ccs, int index);
int		logtofile(char *filename, char *str, char *mode);
int		PDNtoclipboard(HWND hwnd, struct PDNgame *game);
void	setmenuchecks(struct CBoptions *CBoptions, HMENU hmenu);
char	*textfromclipboard(HWND hwnd, char *str);
int		texttoclipboard(char *text);
void	toggle(int *x);






