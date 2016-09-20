// saveashtml.c
//
// part of checkerboard
//
// implements PDNgame structure -> HTML file


#include <windows.h>
#include <stdio.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "saveashtml.h"
#include "utility.h"
#include "fen.h"
#include "checkerboard.h"

// maximal number of moves in a game that we handle
#define MAXHTML 400


int saveashtml(char *filename, struct PDNgame *PDNgame)
	{
	// produces a html file with javascript replay of the current game
	
	FILE *fp;
	char *gamestring;
	int i,j;
	int movenumber = 0;
	int maxhtml = MAXHTML;
	int changes[MAXHTML];
	int changeto[MAXHTML][10];
	int square[MAXHTML][10];
	// move through linked list to find relevant numbers
	struct listentry *le;
	int b[64];	// starting position is saved here.
	char stripped[1024];

	le = PDNgame->head;
	
	// get starting position into our array:
	PDNgametostartposition(PDNgame, b);

	for(i=0;i<MAXHTML;i++)
		changes[i]=0;

	while (le->next != NULL)
		{
		// number of changes
		changes[movenumber]= le->move.jumps + 2;
		// squares the changes occur on
		square[movenumber][0] = coortohtml(le->move.from, PDNgame->gametype);
		square[movenumber][1] = coortohtml(le->move.to, PDNgame->gametype);
		for(i=2; i<changes[movenumber]; i++)
			{
			square[movenumber][i] = coortohtml(le->move.del[i-2], PDNgame->gametype);
			}
		// pieces to put on these squares
		changeto[movenumber][0]=0;
		changeto[movenumber][1]= le->move.newpiece;
		for(i=2;i<changes[movenumber];i++)
			changeto[movenumber][i]=0;
		
		movenumber++;
		
		if(movenumber == MAXHTML)
			break;
		le = le->next;
		}

	movenumber++;
	fp = fopen(filename,"w");

	fprintf(fp,"<HTML>\n<HEAD>\n<META name=\"GENERATOR\" content=\"CheckerBoard %s\">\n<TITLE>\n", VERSION);
	fprintf(fp,"%s - %s\n</TITLE>\n", PDNgame->black, PDNgame->white);
	fprintf(fp,"<STYLE TYPE='text/css'>\n<!--\n.move {font-weight: bold; text-decoration: none}\na.move {color:black}\n//-->\n</STYLE>");
	fprintf(fp,"<SCRIPT language=\"JavaScript\">\n<!-- hide script\n\nvar movenumber = 0;\nchanges = new Array(%i);\nfor(i=0;i<%i;i++)\n changes[i]=0;\nsquare = new Array(%i);\nfor (i=0; i < %i; i++) {\n   square[i] = new Array(10);\n   }\n\nchangeto = new Array(%i);\nfor (i=0; i < %i; i++) {\n   changeto[i] = new Array(10);\n   }\n",maxhtml,maxhtml,maxhtml,maxhtml,maxhtml,maxhtml);

	// create comment array
	fprintf(fp,"comment = new Array(%i);",maxhtml);
	le = PDNgame->head;
	i=0;
	while (le->next != NULL)
		{
		sprintf(stripped,"");
		stripquotes(le->comment,stripped);
		fprintf(fp,"\ncomment[%i]=\"%s\";",i,stripped);
		//fprintf(fp,"\ncomment[%i]=\%i;",i,i);
		i++;
		le = le->next;
		}
	
	for(i=0;i<=movenumber;i++)
		fprintf(fp,"\nchanges[%i]=%i;", i,changes[i]);

	for(i=0;i<movenumber;i++)
		{
		for(j=0;j<changes[i];j++)
			fprintf(fp,"\nchangeto[%i][%i]=%i;",i,j, changeto[i][j]);
		}

	for(i=0;i<movenumber;i++)
		{
		for(j=0;j<changes[i];j++)
			fprintf(fp,"\nsquare[%i][%i]=%i;",i,j, square[i][j]);
		}
	
	// function movetostart
	fprintf(fp,"\n\nfunction movetostart()\n{\n");
	fprintf(fp,"if (movenumber != 0)\n");
	fprintf(fp,"window.document.anchors[movenumber-1].style.background = \"white\";\n");
	// TODO: if we are in a setup position, we would intialize the board differently here!
	fprintf(fp,"movenumber =0;\n");
	
	// initialize board
	for(i=0; i<64; i++)
		{
		// what piece is on this square?
		switch(b[i])
			{
			case CB_BLACK|CB_MAN:
				fprintf(fp,"window.document.images[%i].src='gif/lightbm.gif';\n",i);
				break;
			case CB_BLACK|CB_KING:
				fprintf(fp,"window.document.images[%i].src='gif/lightbk.gif';\n",i);
				break;
			case CB_WHITE|CB_MAN:
				fprintf(fp,"window.document.images[%i].src='gif/lightwm.gif';\n",i);
				break;
			case CB_WHITE|CB_KING:
				fprintf(fp,"window.document.images[%i].src='gif/lightwk.gif';\n",i);
				break;
			default:
				if(PDNgame->gametype == GT_ITALIAN)
					{
					if((i+i/8)%2)
						fprintf(fp,"window.document.images[%i].src='gif/dark.gif';\n",i);
					else
						fprintf(fp,"window.document.images[%i].src='gif/light.gif';\n",i);
					}
				else
					{
					if((i+i/8)%2)
						fprintf(fp,"window.document.images[%i].src='gif/light.gif';\n",i);
					else
						fprintf(fp,"window.document.images[%i].src='gif/dark.gif';\n",i);
					}
				break;
			}
		}
		
	
	fprintf(fp,"\n}");
	// movetoend function
	fprintf(fp,"\n\nfunction movetoend()\n{\nfor(mendnumber=movenumber;mendnumber<%i;mendnumber++)\n	moveforward();\n}\n",maxhtml);
	
	// moveto function
	fprintf(fp,"\n\nfunction moveto(number)\n{\nmovetostart();\nfor(m1=0; m1<number ; m1++)\nmoveforward();\n}\n\n");

	// moveforward function
	fprintf(fp,"function moveforward()\n{\nn = changes[movenumber];\nif(n==0)\n return 0;\n");
	fprintf(fp,"for(i=0;i<n;i++){\n	sq=square[movenumber][i];\n");
	fprintf(fp,"image=changeto[movenumber][i];\nif(image==0)\nwindow.document.images[sq].src = 'gif/light.gif';\nif(image==6)\nwindow.document.images[sq].src = 'gif/lightbm.gif';\n");
	fprintf(fp,"if(image==5)\nwindow.document.images[sq].src = 'gif/lightwm.gif';\nif(image==10)\nwindow.document.images[sq].src = 'gif/lightbk.gif';\nif(image==9)\nwindow.document.images[sq].src = 'gif/lightwk.gif';\n}\n");
	fprintf(fp,"if(movenumber>0)\n{\nwindow.document.anchors[movenumber-1].style.background = \"white\";\n}\n");
	fprintf(fp,"window.document.anchors[movenumber].style.background = \"gray\";\n");
	fprintf(fp,"window.document.comment.pdncomment.value = comment[movenumber];\n");
	fprintf(fp,"movenumber++;\nreturn 1;\n}\n\n");
	
	// movebackward
	fprintf(fp,"function movebackward()\n{\nif(movenumber==0)\n return false;\n");
	fprintf(fp,"window.document.anchors[movenumber-1].style.background = \"white\";\n");
	fprintf(fp,"newmove = movenumber-1;\nmovetostart();\n");
	fprintf(fp,"for(mnumber=0;mnumber<newmove;mnumber++)\n{\nmoveforward();\n}\n}\n");

	//end head
	fprintf(fp,"// show html -->\n</script> \n</head> \n");

	fprintf(fp,"<BODY>\n<TABLE cellpadding=\"10\" cellspacing=\"10\">\n<TR>\n<TD width =\"400\"> \n<CENTER>\n");

	fprintf(fp,"<table border=\"1\" cellspacing=\"0\" cellpadding=\"0\"><tr><td>");

	if(strcmp(PDNgame->setup,"")!=0)
		{
		// setup
		for(i=0; i<64; i++)
			{
			if(i%8==0 && i!=0)
				fprintf(fp,"<br>");
			// what piece is on this square?
			switch(b[i])
				{
				case CB_BLACK|CB_MAN:
					fprintf(fp,"<img src=\"gif/lightbm.gif\">");
					break;
				case CB_BLACK|CB_KING:
					fprintf(fp,"<img src=\"gif/lightbk.gif\">");
					break;
				case CB_WHITE|CB_MAN:
					fprintf(fp,"<img src=\"gif/lightwm.gif\">");
					break;
				case CB_WHITE|CB_KING:
					fprintf(fp,"<img src=\"gif/lightwk.gif\">");
					break;
				default:
					if(PDNgame->gametype == GT_ITALIAN)
						{
						if((i+i/8)%2)
							fprintf(fp,"<img src=\"gif/dark.gif\">");
						else
							fprintf(fp,"<img src=\"gif/light.gif\">");
						}
					else
						{
						if((i+i/8)%2)
							fprintf(fp,"<img src=\"gif/light.gif\">");
						else
							fprintf(fp,"<img src=\"gif/dark.gif\">");
						}
					break;
				}
			}
		}
	else
		{
		// no setup
		if(PDNgame->gametype == GT_ITALIAN)
			{
			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><br>");

			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><br>");

			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><br>");


			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/light.gif\"><br>");
			fprintf(fp,"<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><br>");
				
			
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<br><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><br><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			}
		else
			{
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<br><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><br><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,"<br><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><br><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/light.gif\"><br><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><br>");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><br><img src=\"gif/lightbm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp,"<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp,"<img src=\"gif/dark.gif\">");
			}
		}

	fprintf(fp,"</td></tr></table>");
	fprintf(fp,"<P>\n<FORM NAME=\"tapecontrol\">\n<input type=button value=\" Start \" onClick=\"movetostart();\" onDblClick=\"movetostart();\">\n");
	fprintf(fp,"<input type=button value=\" &lt; \" onClick=\"movebackward();\" onDblClick=\"movebackward();\">\n<input type=button value=\" &gt; \" onClick=\"moveforward();\" onDblClick=\"moveforward();\">\n");
	fprintf(fp,"<input type=button value=\" End \" onClick=\"movetoend();\" onDblClick=\"movetoend();\">\n<P>\n");
	fprintf(fp,"</FORM>\n");
	fprintf(fp,"<form name=\"comment\">\n<textarea name=\"pdncomment\" rows=6 cols=46>\n</textarea>\n");
	fprintf(fp,"</CENTER></TD><TD valign=\"top\">");
	fprintf(fp,"<H3>%s - %s</H3>\n",PDNgame->black, PDNgame->white);
	// print moves
	
	gamestring = (char *) malloc(GAMEBUFSIZE);
	if(gamestring != NULL)
		{
		PDNgametoPDNHTMLstring(PDNgame,gamestring);
		fprintf(fp,"%s",gamestring);
		free(gamestring);
		}

	fprintf(fp, "<P><FONT SIZE=\"-2\">\ngenerated with <A HREF=\"http://www.fierz.ch/checkers.htm\">CheckerBoard %s</A><br>"
				"Use the buttons below the board to move through the game or click a move in the notation to jump to that position."
				"You can select and copy the PDN above and paste it into CheckerBoard.</FONT>", VERSION);
	fprintf(fp,"</TD></TR></TABLE></BODY>\n</HTML>");
	fclose(fp);
	sprintf(filename,"");
// put gifs into the save directory
install_gifs();
	return 1;
	}

int stripquotes(char *str, char *stripped)
	{
	int i=0;
	
	sprintf(stripped,"");
	while(str[i] != 0 && i<1024)
		{
		if(str[i] != '"')
			stripped[i] = str[i];
		else
			stripped[i] = ' ';
		i++;
		}
	stripped[i] = 0;
	return 1;
	}


int PDNgametostartposition(struct PDNgame *game, int b[64])
	{
	// fills the array b with the pieces in the starting position of a PDN game
	// needs to check whether it's a setup or not.
	int i,j;
	int dummy;
	int b8[8][8];

	for(i=0;i<64;i++)
		b[i]=0;
    
	if(strcmp(game->setup,"")==0)
		// no setup
		{
		if(game->gametype == GT_ITALIAN)
			{
			for(i=0;i<12;i++)
				{
				j= 2*i;
				if(i>=4 && i<8)
					j++;
				b[j] = CB_BLACK|CB_MAN;
				}

			for(i=20;i<32;i++)
				{
				j= 2*i+1;
				if(i>=24 && i<28)
					j--;
				b[j] = CB_WHITE|CB_MAN;
				}
			}
		else
			{
			for(i=0;i<12;i++)
				{
				j= 2*i+1;
				if(i>=4 && i<8)
					j--;
				b[j] = CB_WHITE|CB_MAN;
				}

			for(i=20;i<32;i++)
				{
				j= 2*i;
				if(i>=24 && i<28)
					j++;
				b[j] = CB_BLACK|CB_MAN;
				}
			}
		}
	else
		// setup: here, i'm only checking for GT_ITALIAN, nothing else - i.e. all other game types might fail.
		{
		FENtoboard8(b8, game->FEN , &dummy, game->gametype);
		for(i=0;i<8;i++)
			{
			for(j=0;j<8;j++)
				{
				b[8*(7-j)+i] = b8[i][j];
				if(game->gametype == GT_ITALIAN)
					b[8*(7-j)+i] = b8[i][7-j];
				}
			}
		}
		
		
	return 1;
	}

int coortohtml(struct coor c, int gametype)
	{
	// return html number for a coordinate

	// html: 
	// 0  1  2  3  4  5  6  7  
	// 8  9 10 11 12 13 14 15 
	// 16 etc.

	// coors
	//
	//
	switch(gametype)
		{
		case GT_ENGLISH:
			return c.x + 8*(7-c.y);
			break;
		case GT_ITALIAN:
			return c.x + 8*c.y;
			break;
		default:
			return c.x + 8*(7-c.y);
			break;
		}
	}

void PDNgametoPDNHTMLstring(struct PDNgame *game, char *pdnstring)
	{
	/* prints a formatted PDN in *pdnstring*/
	/* is used for PDN to html*/
	/* warning! \n is no good for printf, it s acutually \r\n*/
	char s[256];
	int counter,i;
	struct listentry *listentry;
	/* I: print headers */
	sprintf(pdnstring,"");
	sprintf(s,"[Event \"%s\"]<BR>",game->event);
	strcat(pdnstring,s);
	//sprintf(s,"[Site \"%s\"]<BR>",game->site);
	//strcat(pdnstring,s);
	//sprintf(s,"[Date \"%s\"]<BR>",game->date);
	//strcat(pdnstring,s);
	//sprintf(s,"[Round \"%s\"]<BR>",game->round);
	//strcat(pdnstring,s);
	sprintf(s,"[Black \"%s\"]<BR>",game->black);
	strcat(pdnstring,s);
	sprintf(s,"[White \"%s\"]<BR>",game->white);
	strcat(pdnstring,s);
	sprintf(s,"[Result \"%s\"]<BR>",game->resultstring);
	strcat(pdnstring,s);
	/* if this was after a setup, add FEN and setup header*/
	if(strcmp(game->setup,"")!=0)
		{
		sprintf(s,"[Setup \"%s\"]<BR>",game->setup);
		strcat(pdnstring,s);
   		sprintf(s,"[FEN \"%s\"]<BR>",game->FEN);
		strcat(pdnstring,s);
   		}
	/* print PDN */
	listentry=game->head;
	i=1;
	counter=0;
	while( listentry->next !=NULL)
   		{
     	move4tonotation(listentry->move, listentry->PDN);
     	
		// print anchor
		sprintf(s,"<a href=\"javascript:moveto(%i)\" name=\"%i\" class=\"move\">",i,i);
		
		strcat(pdnstring,s);

		/* print the move number */
		if(i%2)
			{
      		sprintf(s,"%i. ",(int)((i+1)/2));
			counter += (int)strlen(s);
			strcat(pdnstring,s);
			}
		/* print the move */
		sprintf(s,"%s",listentry->PDN);
		strcat(pdnstring,s);

		// close anchor
		strcat(pdnstring,"</a> ");

		i++;
      listentry = listentry->next;
      }

	/* add game terminator */
	sprintf(s, "*");		/* Game terminator is '*' as per PDN 3.0. See http://pdn.fmjd.org/ */
	counter += (int)strlen(s);
	strcat(pdnstring,s);
	}


/*
 * Copy the 6 board gif files to a "gif" directory under
 * the current directory.
 */
void install_gifs(void)
{
	char gifsrcdir[MAX_PATH];

	CreateDirectory("gif", NULL);
	SetCurrentDirectory("gif");
	sprintf(gifsrcdir, "%s\\games\\gif", CBdirectory);
	copy_file(gifsrcdir, "dark.gif");
	copy_file(gifsrcdir, "light.gif");
	copy_file(gifsrcdir, "lightbm.gif");
	copy_file(gifsrcdir, "lightbk.gif");
	copy_file(gifsrcdir, "lightwm.gif");
	copy_file(gifsrcdir, "lightwk.gif");
	SetCurrentDirectory("..");
}


/*
 * Copy a file from the source directory
 * to the current directory.
 */
void copy_file(char *srcdir, char *fname)
{
	int value;
	char fullname[MAX_PATH];
	FILE *srcfp, *destfp;

	/* Open source file. */
	sprintf(fullname, "%s\\%s", srcdir, fname);
	srcfp = fopen(fullname, "rb");
	if (!srcfp)
		return;

	/* Open destination file. */
	destfp = fopen(fname, "wb");
	if (!destfp) {
		fclose(srcfp);
		return;
	}

	/* Copy contents. */
	while (1) {
		value = getc(srcfp);
		if (value == EOF)
			break;

		putc(value, destfp);
	}
	fclose(srcfp);
	fclose(destfp);
}
