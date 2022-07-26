// saveashtml.c
//
// part of checkerboard
//
// implements PDNgame structure -> HTML file
#include <windows.h>
#include <stdio.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "saveashtml.h"
#include "utility.h"
#include "fen.h"
#include "checkerboard.h"

// maximal number of moves in a game that we handle
#define MAXHTML 400

int saveashtml(char *filename, PDNgame *game)
{
	// produces a html file with javascript replay of the current game
	FILE *fp;
	std::string gamestring;
	int i, j;
	int movei;
	int movenumber = 0;
	int maxhtml = MAXHTML;
	int changes[MAXHTML];
	int changeto[MAXHTML][10];
	int square[MAXHTML][10];

	// move through linked list to find relevant numbers
	int b[64];			// starting position is saved here.
	char stripped[1024];

	// get starting position into our array:
	PDNgametostartposition(game, b);

	for (i = 0; i < MAXHTML; i++)
		changes[i] = 0;

	for (movei = 0; movei < (int)game->moves.size(); ++movei) {

		// number of changes
		changes[movenumber] = game->moves[movei].move.jumps + 2;

		// squares the changes occur on
		square[movenumber][0] = coortohtml(game->moves[movei].move.from, game->gametype);
		square[movenumber][1] = coortohtml(game->moves[movei].move.to, game->gametype);
		for (i = 2; i < changes[movenumber]; i++)
			square[movenumber][i] = coortohtml(game->moves[movei].move.del[i - 2], game->gametype);

		// pieces to put on these squares
		changeto[movenumber][0] = 0;
		changeto[movenumber][1] = game->moves[movei].move.newpiece;
		for (i = 2; i < changes[movenumber]; i++)
			changeto[movenumber][i] = 0;

		movenumber++;

		if (movenumber == MAXHTML)
			break;
	}

	movenumber++;
	fp = fopen(filename, "w");

	fprintf(fp, "<HTML>\n<HEAD>\n<META name=\"GENERATOR\" content=\"CheckerBoard %s\">\n<TITLE>\n", VERSION);
	fprintf(fp, "%s - %s\n</TITLE>\n", game->black, game->white);
	fprintf(fp,
			"<STYLE TYPE='text/css'>\n<!--\n.move {font-weight: bold; text-decoration: none}\na.move {color:black}\n//-->\n</STYLE>");
	fprintf(fp,
			"<SCRIPT language=\"JavaScript\">\n<!-- hide script\n\nvar movenumber = 0;\nchanges = new Array(%i);\nfor(i=0;i<%i;i++)\n changes[i]=0;\nsquare = new Array(%i);\nfor (i=0; i < %i; i++) {\n   square[i] = new Array(10);\n   }\n\nchangeto = new Array(%i);\nfor (i=0; i < %i; i++) {\n   changeto[i] = new Array(10);\n   }\n",
		maxhtml,
			maxhtml,
			maxhtml,
			maxhtml,
			maxhtml,
			maxhtml);

	// create comment array
	fprintf(fp, "comment = new Array(%i);", maxhtml);
	for (movei = 0; movei < (int)game->moves.size(); ++movei) {
		sprintf(stripped, "");
		stripquotes(game->moves[movei].comment, stripped);
		fprintf(fp, "\ncomment[%i]=\"%s\";", movei, stripped);
	}

	for (movei = 0; movei < (int)game->moves.size(); ++movei)
		fprintf(fp, "\nchanges[%i]=%i;", movei, changes[movei]);

	for (movei = 0; movei < (int)game->moves.size(); ++movei) {
		for (j = 0; j < changes[movei]; j++)
			fprintf(fp, "\nchangeto[%i][%i]=%i;", movei, j, changeto[movei][j]);
	}

	for (movei = 0; movei < (int)game->moves.size(); ++movei) {
		for (j = 0; j < changes[movei]; j++)
			fprintf(fp, "\nsquare[%i][%i]=%i;", movei, j, square[movei][j]);
	}

	// function movetostart
	fprintf(fp, "\n\nfunction movetostart()\n{\n");
	fprintf(fp, "if (movenumber != 0)\n");
	fprintf(fp, "window.document.anchors[movenumber-1].style.background = \"white\";\n");

	// TODO: if we are in a setup position, we would intialize the board differently here!
	fprintf(fp, "movenumber =0;\n");

	// initialize board
	for (i = 0; i < 64; i++) {

		// what piece is on this square?
		switch (b[i]) {
		case CB_BLACK | CB_MAN:
			fprintf(fp, "window.document.images[%i].src='gif/lightbm.gif';\n", i);
			break;

		case CB_BLACK | CB_KING:
			fprintf(fp, "window.document.images[%i].src='gif/lightbk.gif';\n", i);
			break;

		case CB_WHITE | CB_MAN:
			fprintf(fp, "window.document.images[%i].src='gif/lightwm.gif';\n", i);
			break;

		case CB_WHITE | CB_KING:
			fprintf(fp, "window.document.images[%i].src='gif/lightwk.gif';\n", i);
			break;

		default:
			if (game->gametype == GT_ITALIAN) {
				if ((i + i / 8) % 2)
					fprintf(fp, "window.document.images[%i].src='gif/dark.gif';\n", i);
				else
					fprintf(fp, "window.document.images[%i].src='gif/light.gif';\n", i);
			}
			else {
				if ((i + i / 8) % 2)
					fprintf(fp, "window.document.images[%i].src='gif/light.gif';\n", i);
				else
					fprintf(fp, "window.document.images[%i].src='gif/dark.gif';\n", i);
			}
			break;
		}
	}

	fprintf(fp, "\n}");

	// movetoend function
	fprintf(fp,
			"\n\nfunction movetoend()\n{\nfor(mendnumber=movenumber;mendnumber<%i;mendnumber++)\n	moveforward();\n}\n",
			maxhtml);

	// moveto function
	fprintf(fp, "\n\nfunction moveto(number)\n{\nmovetostart();\nfor(m1=0; m1<number ; m1++)\nmoveforward();\n}\n\n");

	// moveforward function
	fprintf(fp, "function moveforward()\n{\nn = changes[movenumber];\nif(n==0)\n return 0;\n");
	fprintf(fp, "for(i=0;i<n;i++){\n	sq=square[movenumber][i];\n");
	fprintf(fp,
			"image=changeto[movenumber][i];\nif(image==0)\nwindow.document.images[sq].src = 'gif/light.gif';\nif(image==6)\nwindow.document.images[sq].src = 'gif/lightbm.gif';\n");
	fprintf(fp,
			"if(image==5)\nwindow.document.images[sq].src = 'gif/lightwm.gif';\nif(image==10)\nwindow.document.images[sq].src = 'gif/lightbk.gif';\nif(image==9)\nwindow.document.images[sq].src = 'gif/lightwk.gif';\n}\n");
	fprintf(fp, "if(movenumber>0)\n{\nwindow.document.anchors[movenumber-1].style.background = \"white\";\n}\n");
	fprintf(fp, "window.document.anchors[movenumber].style.background = \"gray\";\n");
	fprintf(fp, "window.document.comment.pdncomment.value = comment[movenumber];\n");
	fprintf(fp, "movenumber++;\nreturn 1;\n}\n\n");

	// movebackward
	fprintf(fp, "function movebackward()\n{\nif(movenumber==0)\n return false;\n");
	fprintf(fp, "window.document.anchors[movenumber-1].style.background = \"white\";\n");
	fprintf(fp, "newmove = movenumber-1;\nmovetostart();\n");
	fprintf(fp, "for(mnumber=0;mnumber<newmove;mnumber++)\n{\nmoveforward();\n}\n}\n");

	//end head
	fprintf(fp, "// show html -->\n</script> \n</head> \n");

	fprintf(fp, "<BODY>\n<TABLE cellpadding=\"10\" cellspacing=\"10\">\n<TR>\n<TD width =\"400\"> \n<CENTER>\n");

	fprintf(fp, "<table border=\"1\" cellspacing=\"0\" cellpadding=\"0\"><tr><td>");

	if (strcmp(game->FEN, "") != 0) {

		// setup
		for (i = 0; i < 64; i++) {
			if (i % 8 == 0 && i != 0)
				fprintf(fp, "<br>");

			// what piece is on this square?
			switch (b[i]) {
			case CB_BLACK | CB_MAN:
				fprintf(fp, "<img src=\"gif/lightbm.gif\">");
				break;

			case CB_BLACK | CB_KING:
				fprintf(fp, "<img src=\"gif/lightbk.gif\">");
				break;

			case CB_WHITE | CB_MAN:
				fprintf(fp, "<img src=\"gif/lightwm.gif\">");
				break;

			case CB_WHITE | CB_KING:
				fprintf(fp, "<img src=\"gif/lightwk.gif\">");
				break;

			default:
				if (game->gametype == GT_ITALIAN) {
					if ((i + i / 8) % 2)
						fprintf(fp, "<img src=\"gif/dark.gif\">");
					else
						fprintf(fp, "<img src=\"gif/light.gif\">");
				}
				else {
					if ((i + i / 8) % 2)
						fprintf(fp, "<img src=\"gif/light.gif\">");
					else
						fprintf(fp, "<img src=\"gif/dark.gif\">");
				}
				break;
			}
		}
	}
	else {

		// no setup
		if (game->gametype == GT_ITALIAN) {
			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><br>");

			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><br>");

			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><br>");

			fprintf(fp,
					"<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/light.gif\"><br>");
			fprintf(fp, "<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><br>");

			fprintf(fp,
					"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,
					"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,
					"<br><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><br><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
		}
		else {
			fprintf(fp,
					"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,
					"<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp,
					"<br><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><br><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightwm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightwm.gif\">");
			fprintf(fp, "<br><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><br><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/light.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/light.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/light.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/light.gif\"><br><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><br>");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><br><img src=\"gif/lightbm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\">");
			fprintf(fp, "<img src=\"gif/lightbm.gif\"><img src=\"gif/dark.gif\"><img src=\"gif/lightbm.gif\">");
			fprintf(fp, "<img src=\"gif/dark.gif\">");
		}
	}

	fprintf(fp, "</td></tr></table>");
	fprintf(fp,
			"<P>\n<FORM NAME=\"tapecontrol\">\n<input type=button value=\" Start \" onClick=\"movetostart();\" onDblClick=\"movetostart();\">\n");
	fprintf(fp,
			"<input type=button value=\" &lt; \" onClick=\"movebackward();\" onDblClick=\"movebackward();\">\n<input type=button value=\" &gt; \" onClick=\"moveforward();\" onDblClick=\"moveforward();\">\n");
	fprintf(fp, "<input type=button value=\" End \" onClick=\"movetoend();\" onDblClick=\"movetoend();\">\n<P>\n");
	fprintf(fp, "</FORM>\n");
	fprintf(fp, "<form name=\"comment\">\n<textarea name=\"pdncomment\" rows=6 cols=46>\n</textarea>\n");
	fprintf(fp, "</CENTER></TD><TD valign=\"top\">");
	fprintf(fp, "<H3>%s - %s</H3>\n", game->black, game->white);

	// print moves
	PDNgametoPDNHTMLstring(game, gamestring);
	fprintf(fp, "%s", gamestring.c_str());

	fprintf(fp,
			"<P><FONT SIZE=\"-2\">\ngenerated with <A HREF=\"http://www.fierz.ch/checkers.htm\">CheckerBoard %s</A><br>Use the buttons below the board to move through the game or click a move in the notation to jump to that position.You can select and copy the PDN above and paste it into CheckerBoard.</FONT>",
		VERSION);
	fprintf(fp, "</TD></TR></TABLE></BODY>\n</HTML>");
	fclose(fp);
	sprintf(filename, "");
	return 1;
}

int stripquotes(char *str, char *stripped)
{
	int i = 0;

	sprintf(stripped, "");
	while (str[i] != 0 && i < 1024) {
		if (str[i] != '"')
			stripped[i] = str[i];
		else
			stripped[i] = ' ';
		i++;
	}

	stripped[i] = 0;
	return 1;
}

int PDNgametostartposition(PDNgame *game, int b[64])
{
	// fills the array b with the pieces in the starting position of a PDN game
	// needs to check whether it's a setup or not.
	int i, j;
	int returncolor;
	Board8x8 b8;

	for (i = 0; i < 64; i++)
		b[i] = 0;

	if (strcmp(game->FEN, "") == 0) {

	// no setup
		if (game->gametype == GT_ITALIAN) {
			for (i = 0; i < 12; i++) {
				j = 2 * i;
				if (i >= 4 && i < 8)
					j++;
				b[j] = CB_BLACK | CB_MAN;
			}

			for (i = 20; i < 32; i++) {
				j = 2 * i + 1;
				if (i >= 24 && i < 28)
					j--;
				b[j] = CB_WHITE | CB_MAN;
			}
		}
		else {
			for (i = 0; i < 12; i++) {
				j = 2 * i + 1;
				if (i >= 4 && i < 8)
					j--;
				b[j] = CB_WHITE | CB_MAN;
			}

			for (i = 20; i < 32; i++) {
				j = 2 * i;
				if (i >= 24 && i < 28)
					j++;
				b[j] = CB_BLACK | CB_MAN;
			}
		}
	}
	else {

		// setup: here, i'm only checking for GT_ITALIAN, nothing else - i.e. all other game types might fail.
		FENtoboard8(b8, game->FEN, &returncolor, game->gametype);
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				b[8 * (7 - j) + i] = b8[i][j];
				if (game->gametype == GT_ITALIAN)
					b[8 * (7 - j) + i] = b8[i][7 - j];
			}
		}
	}

	return 1;
}

int coortohtml(coor c, int gametype)
{
	// return html number for a coordinate
	// html:
	// 0  1  2  3  4  5  6  7
	// 8  9 10 11 12 13 14 15
	// 16 etc.
	// coors
	//
	//
	switch (gametype) {
	case GT_ENGLISH:
		return c.x + 8 * (7 - c.y);
		break;

	case GT_ITALIAN:
		return c.x + 8 * c.y;
		break;

	default:
		return c.x + 8 * (7 - c.y);
		break;
	}
}

void PDNgametoPDNHTMLstring(PDNgame *game, std::string &pdnstring)
{
	/* prints a formatted PDN in pdnstring */
	/* is used for PDN to html*/

	/* warning! \n is no good for printf, it s actually \r\n*/
	char s[256];
	int counter, i;
	size_t movei;

	/* I: print headers */
	pdnstring.clear();
	sprintf(s, "[Event \"%s\"]<BR>", game->event);
	pdnstring += s;

	sprintf(s, "[Black \"%s\"]<BR>", game->black);
	pdnstring += s;
	sprintf(s, "[White \"%s\"]<BR>", game->white);
	pdnstring += s;
	sprintf(s, "[Result \"%s\"]<BR>", game->resultstring);
	pdnstring += s;

	/* if this was after a setup, add FEN header*/
	if (strcmp(game->FEN, "") != 0) {
		sprintf(s, "[FEN \"%s\"]<BR>", game->FEN);
		pdnstring += s;
	}

	/* print PDN */
	i = 1;
	counter = 0;
	for (movei = 0; movei < game->moves.size(); ++movei) {

		// print anchor
		sprintf(s, "<a href=\"javascript:moveto(%i)\" name=\"%i\" class=\"move\">", i, i);
		pdnstring += s;

		/* print the move number */
		if (i % 2) {
			sprintf(s, "%i. ", (int)((i + 1) / 2));
			counter += (int)strlen(s);
			pdnstring += s;
		}

		/* print the move */
		sprintf(s, "%s", game->moves[movei].PDN);
		pdnstring += s;

		// close anchor
		pdnstring += "</a> ";
		i++;
	}

	/* add game terminator */
	sprintf(s, "*");	/* Game terminator is '*' as per PDN 3.0. See http://pdn.fmjd.org/ */
	counter += (int)strlen(s);
	pdnstring += s;
}

