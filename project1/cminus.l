/****************************************************/
/* File: tiny.l                                     */
/* Lex specification for C-Minus                    */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/* modified by Yejin Lee                            */
/****************************************************/

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
%}

digit       [0-9]
number      {digit}+
identifier  [a-zA-Z][a-zA-Z0-9]*
error       [0-9]+[a-zA-Z]+[a-zA-Z0-9]*
newline     \n
whitespace  [ \t]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"while"         {return WHILE;}
"return"        {return RETURN;}
"int"           {return INT;}
"void"          {return VOID;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return MUL;}
"/"             {return DIV;}
";"             {return SEMICOLON;}
","             {return COMMA;}
"("             {return LPAREN;}
")"             {return RPAREN;}
"{"             {return LCURLY;}
"}"             {return RCURLY;}
"["             {return LBRACE;}
"]"             {return RBRACE;}
"<="            {return LESSEQUAL;}
"<"             {return LESSTHAN;}
">="            {return GREATEQUAL;}
">"             {return GREATTHAN;}
"=="            {return EQ;}
"="             {return ASSIGN;}
"!="            {return NEQ;}
{identifier}    {return ID;}
{error}           {return ERROR;}
{number}        {return NUM;}
{whitespace}    {/* skip whitespace */}
{newline}       {lineno++;}
"/*"             { char c; char last_c='\0';
                  do
                  { c = input();
                    if (c == EOF) break;
                    if (c == '\n') lineno++;
                    if (last_c == '*' && c == '/') break;
                    last_c = c;
                  } while (1);
                }
error           {return ERROR;}
.               {return ERROR;}

%%

TokenType getToken(void)
{ static int firstTime = TRUE;
  TokenType currentToken;
  if (firstTime)
  { firstTime = FALSE;
    lineno++;
    yyin = source;
    yyout = listing;
  }
  currentToken = yylex();
  strncpy(tokenString,yytext,MAXTOKENLEN);
  if (TraceScan) {
    fprintf(listing,"\t%d: ",lineno);
    printToken(currentToken,tokenString);
  }
  return currentToken;
}

