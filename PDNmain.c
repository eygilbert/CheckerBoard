// PDNmain demonstrates the use of PDNparse-library

//#include "stdafx.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "PDNparser.h"


int main(int argc, char* argv[])
	{
	FILE *fp;
	char *start,*startheader,*starttoken,buffer[1000000],game[10000],header[256],token[256];
	size_t bytesread=0;
	int i=0;
	int from,to;
	
	//printf("online parser 0.01\n");
	fp=fopen("main.pdn","r");
	if(!fp)
		return 0;
	/* load a PDN database into buffer */
   bytesread=fread(buffer, 1, sizeof(buffer), fp );
	fclose(fp);
	buffer[bytesread]=0;
	//printf("%s\n\n",buffer);
	//getchar();
	/* start parsing */
	start=buffer;
	while(PDNparseGetnextgame(buffer,&start,game))
		{
		/* got a new game in the string "game" */
		//printf("%i",start);
		//printf("\n%s",game);
		/* load headers */
		startheader=game;
		while(PDNparseGetnextheader(game,&startheader,header))
			{
			/* got a new header */
			printf("%s",header);
			getchar();
			}
		/* load moves */
		starttoken=startheader;
		while(PDNparseGetnexttoken(game,&starttoken,token))
			{
			/* skip move numbers, that is, skip if the last character
				of the token is a full stop */
			if(token[strlen(token)-1]=='.') continue;
			printf("%s",token);
			PDNparseTokentonumbers(token,&from,&to);
			printf("  %i  %i",from,to);
			getchar();
			}
		}
	printf("\n\nno more games!");
	getchar();
	}