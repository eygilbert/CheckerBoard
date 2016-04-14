
int PDNparseGetnextgame(char **start,char *game);		/* gets whats between **start and game terminator */
int PDNparseGetnextheader(char **start,char *header);/* gets whats betweeen [] from **start */
int PDNparseGetnexttag(char **start,char *tag);		/* gets whats between "" from **start */
int PDNparseTokentonumbers(char *token,int *from, int *to);
int PDNparseGetnexttoken(char **start, char *token);	/* gets the next token from **start */
int PDNparseGetnextPDNtoken(char ** start, char *token);
int PDNparseGetnumberofgames(char *filename);			/* tokens are: -> {everything in a comment}*/
size_t getfilesize(char *filename);							//-> a move: "11-15" or ""4x12" -> a text: "event"*/

#define MAXGAMESIZE 65536 // number of bytes we reasonably expect every game to be smaller than