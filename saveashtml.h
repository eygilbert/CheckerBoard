int coortohtml(coor c, int gametype);
void PDNgametoPDNHTMLstring(PDNgame *game, std::string &pdnstring);
int PDNgametostartposition(PDNgame *game, int b[64]);
int saveashtml(char *filename, PDNgame *PDNgame);
int stripquotes(char *str, char *stripped);
