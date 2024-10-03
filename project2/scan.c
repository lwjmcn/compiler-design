/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scan.h"

/* states in scanner DFA */
typedef enum
   { START,
     INSLASH, // /
     INCOMMENT, // /*
     INCOMMENTMUL, // */
     INLESS, // <, <=
     INGREAT, // >, >=
     INEQ, // =, ==
     INBANG, // !=
     INTWOSYMBOLS,
     INNUM,
     INID,
     DONE }
   StateType;

/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];

/* BUFLEN = length of the input buffer for
   source code lines */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* holds the current line */
static int linepos = 0; /* current position in LineBuf */
static int bufsize = 0; /* current size of buffer string */
static int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

/* getNextChar fetches the next non-blank character
   from lineBuf, reading in a new line if lineBuf is
   exhausted */
static int getNextChar(void)
{ if (!(linepos < bufsize))
  { lineno++;
    if (fgets(lineBuf,BUFLEN-1,source))
    { if (EchoSource) fprintf(listing,"%4d: %s",lineno,lineBuf);
      bufsize = strlen(lineBuf);
      linepos = 0;
      return lineBuf[linepos++];
    }
    else
    { EOF_flag = TRUE;
      return EOF;
    }
  }
  else return lineBuf[linepos++];
}

/* ungetNextChar backtracks one character
   in lineBuf */
static void ungetNextChar(void)
{ if (!EOF_flag) linepos-- ;}

/* lookup table of reserved words */
static struct
    { char* str;
      TokenType tok;
    } reservedWords[MAXRESERVED] = {
      {"if",IF},
      {"else",ELSE},
      {"while",WHILE},
      {"return",RETURN},
      {"int",INT},
      {"void",VOID},
    };

/* lookup an identifier to see if it is a reserved word */
/* uses linear search */
static TokenType reservedLookup (char * s)
{ int i;
  for (i=0;i<MAXRESERVED;i++)
    if (!strcmp(s,reservedWords[i].str))
      return reservedWords[i].tok;
  return ID;
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* function getToken returns the 
 * next token in source file
 */
TokenType getToken(void)
{  /* index for storing into tokenString */
   int tokenStringIndex = 0;
   /* holds current token to be returned */
   TokenType currentToken;
   /* current state - always begins at START */
   StateType state = START;
   /* flag to indicate save to tokenString */
   int save;
   while (state != DONE)
   { int c = getNextChar();
     save = TRUE;
     switch (state)
     { case START:
         if (isdigit(c)) { // number
           state = INNUM;
           currentToken = NUM; // if the last token was an error
         }
         else if (isalpha(c)) // letter
           state = INID;
         // more than one letter symbols
         else if (c == '=') {
           state = INTWOSYMBOLS;
           currentToken = ASSIGN;
         }
         else if (c == '<') {
           state = INTWOSYMBOLS;
           currentToken = LESSTHAN;
         }
         else if (c == '>') {
           state = INTWOSYMBOLS;
           currentToken = GREATTHAN;
         }
         else if (c == '!') {
           state = INTWOSYMBOLS; 
           currentToken = NEQ;
         }
         else if (c == '/') {
           save = FALSE; // can be the start point of comments, so don't print yet
           state = INSLASH;
         }
         else if ((c == ' ') || (c == '\t') || (c == '\n')) // whitespaces
           save = FALSE;
         else { 
          state = DONE;
           switch (c) { 
             case EOF:
               save = FALSE;
               currentToken = ENDFILE;
               break;
             // every one letter symbols
             case '+':
               currentToken = PLUS;
               break;
             case '-':
               currentToken = MINUS;
               break;
             case '*':
               currentToken = MUL;
               break;
             case ';':
               currentToken = SEMICOLON;
               break;
             case ',':
               currentToken = COMMA;
               break;
             case '(':
               currentToken = LPAREN;
               break;
             case ')':
               currentToken = RPAREN;
               break;
             case '{':
               currentToken = LCURLY;
               break;
             case '}':
               currentToken = RCURLY;
               break;
             case '[':
               currentToken = LBRACE;
               break;
             case ']':
               currentToken = RBRACE;
               break;
             default:
               currentToken = ERROR;
               break;
           }
         }
         break;
       case INSLASH:
         if (c == '*') { 
          // start point of comments
          save = FALSE; // don't print
          state = INCOMMENT;
         } 
         else {
          state = DONE; // just slash
          ungetNextChar();
          currentToken = DIV;
         } 
         break;
       case INCOMMENT:
         save = FALSE;
         if (c == EOF) { 
           state = DONE;
           currentToken = ENDFILE;
         }
         else if (c == '*') 
           state = INCOMMENTMUL;
         break;
       case INCOMMENTMUL:
         save = FALSE;
         if (c == '/') 
           state = START;
         else 
           state = INCOMMENT;
         break;
       case INTWOSYMBOLS: // last character was '<'
         state = DONE;
         if (c == '=') {
           switch (currentToken)
           {
             case LESSTHAN:
               currentToken = LESSEQUAL;
               break;
             case GREATTHAN:
               currentToken = GREATEQUAL;
               break;
             case ASSIGN:
               currentToken = EQ;
               break;
             default:
               break;
           }
         }
         else {
          ungetNextChar();
          save = FALSE;
         }
         break;
       case INNUM:
         if (isalpha(c)) {
          currentToken = ERROR;           
         }
         else if (!isdigit(c)) {
           ungetNextChar();
           save = FALSE;
           state = DONE;
           if(currentToken != ERROR)
             currentToken = NUM;
         }
         break;
       case INID:
         if (!isalpha(c) && !isdigit(c))
         {
           ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = ID;
         }
         break;
       case DONE:
       default: /* should never happen */
         fprintf(listing,"Scanner Bug: state= %d\n",state);
         state = DONE;
         currentToken = ERROR;
         break;
     }
     if ((save) && (tokenStringIndex <= MAXTOKENLEN))
       tokenString[tokenStringIndex++] = (char) c;
     if (state == DONE)
     { tokenString[tokenStringIndex] = '\0';
       if (currentToken == ID)
         currentToken = reservedLookup(tokenString); // check if it is a reserved word
     }
   }
   if (TraceScan) {
     fprintf(listing,"\t%d: ",lineno);
     printToken(currentToken,tokenString);
   }
   return currentToken;
} /* end getToken */

