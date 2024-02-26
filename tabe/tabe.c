/*  TABE.C -- Program to expand/compress tabs for Unix.
 *  This program expands tabs to spaces, or compresses
 *  multiple spaces to tabs.  This should be helpful in
 *  moving "C" source files between PCs (good development
 *  environment, but the editors don't generally recognize
 *  tabs) and Unix systems (where code will execute--editors
 *  routinely use tabs).
 *
 *  Written by Mark Riordan  on 6 June 1988.
 *  Modified by Mark Riodan  on 14 January 2011 to compress
 *    spaces at BOL even after leading tabs.
 */

#include "stdio.h"
#include "ctype.h"

#define FALSE 0
#define TRUE  1
#define TAB_EXPAND 0
#define TAB_COMPRESS 1
#define TAB_BOLONLY 2

int chpertab = 4;

main(argc,argv)
int argc;
char *argv[];
{

  int curcol=0;
  int nextcol;
  int ch;
  int k;
  int nspaces;
  int gottype = FALSE;
  int exptype;
  int whicharg;
  int ccerror = FALSE;

  for(whicharg=1; whicharg<argc; whicharg++) {
    if(strcmp(argv[whicharg],"-c") == 0) {
      exptype = TAB_COMPRESS;
      gottype = TRUE;
    } else if(strcmp(argv[whicharg],"-e") == 0) {
      exptype = TAB_EXPAND;
      gottype = TRUE;
    } else if(strcmp(argv[whicharg],"-b") == 0) {
      exptype = TAB_BOLONLY;
      gottype = TRUE;
    } else if((*(argv[whicharg]) == '-') && isdigit(*(argv[whicharg]+1))) {
      chpertab = atoi(argv[whicharg]+1);
    } else {
      ccerror = TRUE;
    }
  } /* end for whicharg */

  if(ccerror | !gottype) {
    fputs("Usage:  tabe {-e | -c | -b} [-tabcount]\n",stderr);
    fputs(" where:\n",stderr);
    fputs("  -e means expand tabs to spaces\n",stderr);
    fputs("  -c means compress multiple spaces to tabs\n",stderr);
    fputs("     (not implemented)\n",stderr);
    fputs("  -b means compress spaces to tabs only at beginning of line\n",
      stderr);
    fputs("  tabcount  is a decimal integer specifying how many columns\n",
      stderr);
    fputs("             are between consecutive tabs; default is 4.\n",
      stderr);
    goto endit;
  }

  if(exptype == TAB_EXPAND) {
    while((ch = getchar()) != EOF) {
      if(ch == '\t') {
        nextcol = ((curcol/chpertab)+1) * chpertab;
        nspaces = nextcol - curcol;
        for(k=0; k<nspaces; k++) {
          putchar(' ');
          curcol++;
        }
      } else if(ch == '\n') {
        curcol = 0;
        putchar(ch);
      } else {
        putchar(ch);
        curcol++;
      }
    } /* end while getchar */

  } else if(exptype == TAB_COMPRESS) {
    /*  We must compress blanks into tabs. */

    char  line[256];
    int   nwhite;
    int   lastnonblank;
    char  *chptr = line;
    int   jpos;
    int   nchars;
    int   k;

    chptr = line;
    while((ch = getchar()) != EOF) {
      if(ch != '\n') {
        *(chptr++) = ch;
      } else {
        nchars = chptr - line;
        chptr = line;
        nwhite = 0;
        lastnonblank = -1;

        for(jpos=0; jpos<nchars; jpos++) {
          if(line[jpos] != ' ') {
            if(lastnonblank == jpos-1) {
             /* putchar(line[jpos]); */
            } else {
              nwhite = jpos - lastnonblank - 1;
              if(nwhite < 2) {
                for(k=0; k<nwhite; k++) putchar(' ');
              } else {
                for(k=0; k<(jpos/chpertab - lastnonblank/chpertab); k++) {
                  putchar('\t');
                }
                for(k=0; k<(jpos%chpertab); k++) putchar(' ');
              }
            } /* end of else re. lastnonblank */
            putchar(line[jpos]);
            lastnonblank = jpos;
          } /* end of if line[jpos] */
        } /* end of for jpos */
        putchar('\n');
      } /* end of else ch */
    } /* end of while getchar */
  /* end of compression */
  } else {

    /* We must compress spaces at beginning of line */

    int nchars=0;
    int gotnonblank = FALSE;
    int ntabs, nspaces;
    int k;

    while((ch = getchar()) != EOF) {
      if(ch == '\n') {
        putchar('\n');
        nchars = 0;
        gotnonblank = FALSE;
      } else if(gotnonblank) {
       /*
        * If we've already processed the first non-blank on a line,
        * just copy all the remaining characters.
        */
        putchar(ch);
      } else if(ch == ' ') {
        /* Leading blanks in a line. */
        nchars++;
      } else if(ch == '\t') {
        /* Leading tab in a line--just copy it over. */
        putchar(ch);
      } else {
        /* This is the first non-blank in a line. */
        gotnonblank = TRUE;
        ntabs = nchars / chpertab;
        nspaces = nchars % chpertab;
        for (k=0; k<ntabs;   k++) putchar('\t');
        for (k=0; k<nspaces; k++) putchar(' ');
        putchar(ch);
      } /* end of if ch ... */
    } /* end of while getchar ... */
  } /* end of if gettype ... */
endit:;
}
