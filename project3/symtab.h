/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/* modified by Yejin Lee                            */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

/* initialize for symbol table generation*/
void initSymtab();

/* check symbols in the table */
int* checkPredefined(char* name, char* kind, int lineno);
char* findType(char * name, char* kind);
 
/* add symbols to the table */
void addNode( char * name, char* kind, char* type, int lineno);

/* check parameter types of functions*/
void addParamType(char * functionName, int paramLocation, char* paramType);
int checkParam(char* functionName, int argLoc, char* argType);
int checkVoidParam(char* functionName);

/* generate the symbol table */
void insertScope(char* scopeName);
void exitScope();

/* traverse the symbol table */
void initCurrentScope() ;
void enterScope();
void popScope();

/* print the symbol table */
void printSymTab(FILE * listing);
void printScopes(FILE * listing);
void printFunctions(FILE * listing);

#endif
