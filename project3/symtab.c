/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/* modified by Yejin Lee                            */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "globals.h"
#include "util.h"

/* SIZE is the size of the hash table */
#define SIZE 211

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct Line
   { int lineno;
     struct Line * next;
   } * LineList;

/* the list of parameters in function */
typedef struct ParamType
   { int loc; // order in paramter list
     char* type;
     struct ParamType * next;
   } * ParamTypePointer;

/* The record in the bucket lists for
 * each variable, including name, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct Bucket
   { char * name;
     char* kind; /* "Function" or "Variable" */
     char* type;
     ParamTypePointer params; /* parameter list for function*/
     struct Scope* scope; /* scope pointer */
     LineList lines;
     struct Bucket * next;
   } * BucketPointer;

/* tree of symbol table (structured in list)
 * and a node of symbol table, scope
 */
typedef struct Scope
   {
     char * name;
     int level; /* nested level */
     struct Scope* parent; /* tree */
     struct Scope* next; /* list */
     BucketPointer hashTable[SIZE]; /* symbols */
   } * ScopePointer;

static ScopePointer globalScope;
static ScopePointer currentScope;

/* initialize the symbol table
 * by setting the current scope as the global scope
 */
void initSymtab() {
  globalScope = (ScopePointer)malloc(sizeof(struct Scope));
  globalScope->name="global";
  globalScope->level = 1;
  globalScope->next = NULL;
  globalScope->parent = NULL;

  currentScope = globalScope;
};

/* Function findInSymbolTable 
 * traverses the symbol table upward
 * and returns the BucketPointer of the symbol,
 * which has the same name as a parameter 
 * if the symbol is not found, returns NULL 
 * parameter 'kind' is either "Function" or "Variable"
 */
BucketPointer findInSymbolTable(char * name, char* kind)
{ int h = hash(name);
  ScopePointer s = currentScope;

  while (s != NULL) {
    BucketPointer l = s->hashTable[h];
    while (l != NULL){
      if ((!strcmp(name,l->name)) && (!strcmp(kind, l->kind)))
        return l; // found
      l = l->next;
    }

    s = s->parent; // traverse upward
  }
  return NULL;
}

/* Function findInScope 
 * searches for the symbol which has the same name as a paramter
 * in the current scope (difference with the function findInSymbolTable above)
 * and returns the BucketPointer of the symbol,
 * if the symbol is not found, returns NULL 
 */
BucketPointer findInScope(char * name, char* kind)
{ int h = hash(name);
  BucketPointer l = currentScope->hashTable[h];
  while (l != NULL){
    if ((!strcmp(name,l->name)) && (!strcmp(kind, l->kind)))
      return l; // found
    l = l->next;
  }
  return NULL;
}

/* Function checkPredefined
 * checks if the symbol with the same name was defined before in the same scope
 * if it was, returns the list of line numbers where it was defined before
 * list[0] is the size of the list
 * if not, returns NULL
 */
int* checkPredefined(char* name, char* kind, int lineno) {
  BucketPointer b = findInScope(name, kind);
  LineList l = b->lines;

  int capacity = 4;
  int* list = (int*)malloc(sizeof(int)*capacity);
  list[0] = 0;
  int index = 1;  

  while(l != NULL) {
    if(l->lineno < lineno) { // defined already
      if(index >= capacity) { // needs more memory space
        capacity *= 2;
        int* tmplist = (int*)malloc(sizeof(int)*capacity);
        for (int i = 0; i < capacity/2; i++)
          tmplist[i] = list[i];
        list = tmplist;    
      }

      list[index++] = l->lineno; // save in the list
    }
    l = l->next;
  }
  list[0] = index - 1; // size of the list
  return list;
}

/* Function findType
 * returns type of the closest symbol in symbol table
 * with the same name and kind
 * if not found, returns NULL
 * and if it gets NULL for a parameter name
 * , it means the name of current scope
 * , which can mean the current function declaration 
 */
char* findType(char * name, char* kind) {
  char* name_ = name;
  if(name_ == NULL) name_ = currentScope->name; // with current function name

  BucketPointer l = findInSymbolTable(name_, kind);
  if(l != NULL) 
    return l->type;
  return NULL;
}

/* Function addNode 
 * inserts the information of the symbol
 * into the symbol table
 * either newly defined or used after defined
 */
void addNode( char * name, char* kind, char * type, int lineno)
{ /* check if the symbol is already in symbol table */
  BucketPointer l = findInSymbolTable(name, kind);

  /* variable not yet in symbol table */
  if (l == NULL) 
  { /* initialize */
    l = (BucketPointer) malloc(sizeof(struct Bucket));
    l->name = name;
    l->kind = kind;
    l->type = type;
    l->params = NULL;
 
    l->scope = currentScope;

    l->lines = (LineList) malloc(sizeof(struct Line));
    l->lines->lineno = lineno;
    l->lines->next = NULL;    

    /* add to the front of symbol table */
    int h = hash(name);
    l->next = currentScope->hashTable[h];
    currentScope->hashTable[h] = l; 
  }  

  /* found in symbol table */
  else 
  { /* add line number */
    LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct Line));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
}

/* Function addParamType
 * adds ParamType to a function symbol
 */
void addParamType(char * functionName, int paramLocation, char* paramType) {
  BucketPointer func = findInSymbolTable(functionName, "Function");
  ParamTypePointer p = func->params;
  while(p != NULL) p = p->next;

  p = (ParamTypePointer)malloc(sizeof(struct ParamType));
  p->type = paramType;
  p->loc = paramLocation;
  p->next = NULL;

  if(func->params == NULL) func->params = p;
  else func->params->next = p; 
}

/* Function checkParam
 * checks if a argument in function call
 * matches to the parameter
 * in terms of the type and order
 * if matched returns 0, otherwise returns -1
 */
int checkParam(char* functionName, int argLoc, char* argType) {
  BucketPointer func = findInSymbolTable(functionName, "Function");
  if(func == NULL) 
    return -1;

  ParamTypePointer p = func->params;
  while(p != NULL && p->loc != argLoc) 
    p = p->next;
  if(p == NULL) return -1;

  if(!strcmp(p->type, argType)) 
    return 0; // match
  return -1;
}

/* Function checkVoidParam
 * checks if the parameter of the function
 * is void or not
 * if it is, returns 0, otherwise returns -1
 */
int checkVoidParam(char* functionName) {
  BucketPointer func = findInSymbolTable(functionName, "Function");
  if (func !=NULL && !strcmp(func->params->type, "void")) 
    return 0; // match
  return -1;
}

/* Function insertScope 
 * creates a new scope node
 * and add it to a symbol table as a child node
 */
void insertScope(char* scopeName)
{ char* newScopeName = scopeName;

  /* fake scope for C-Minus semantics, function should be declared before usage
   * or real scope for a compound statement scope
   * I just defined the scopes's name exactly like their parent */
  if (newScopeName == NULL) {
    newScopeName = copyString(currentScope->name);
    // strcat(newScopeName, "*");
  }

  // initialize
  ScopePointer newScope = (ScopePointer) malloc(sizeof(struct Scope));
  newScope->name = newScopeName;
  newScope->level = currentScope->level + 1;
  newScope->next = NULL;
  newScope->parent = currentScope; 

  // add new scope at the last position of the scope list
  ScopePointer s = currentScope;
  while(s->next != NULL) s = s->next;
  s->next = newScope;

  currentScope = newScope;
}

/* Function exitScope
 * exits from the current scope
 * and go upward in the symbol table
 */
void exitScope()
{ currentScope = currentScope->parent;
}

/* initialize the current scope as the top 
 * for type checking after the symbol table is built
 */
void initCurrentScope() {
  currentScope = globalScope;
}

/* Function enterScope
 * goes through the list of scopes
 * for type checking
 * this function should be called in the same way with 
 * insertScope to be in the right currentScope
 */
void enterScope (){
  currentScope = currentScope->next;
  // fprintf(listing, "enter to currentscope: %s\n", currentScope->name);
}

/* Function popScope
 * deletes the current scope from the symbol table
 * and goes upward in the symbol table
 */
void popScope() {
  if(currentScope != NULL && currentScope->parent != NULL){
    currentScope->parent->next = currentScope->next;
    currentScope = currentScope->parent;
  }
  // fprintf(listing, "pop to currentscope: %s\n", currentScope->name);
}

/* Function printSymTab 
 * prints a formatted listing of the symbol table contents 
 * line numbers are not correctly printed 
 * because variable usage is added to the table during type checking
 */
void printSymTab(FILE * listing)
{ int i;
  fprintf(listing," Symbol Name   Symbol Kind   Symbol Type    Scope Name   Line Numbers\n");
  fprintf(listing,"-------------  -----------  -------------  ------------  ------------\n");
  ScopePointer s = globalScope;
  while(s != NULL) {
    for (i=0;i<SIZE;++i)
    { if (s->hashTable[i] != NULL)
      { BucketPointer l = s->hashTable[i];
        while (l != NULL)
        { LineList t = l->lines;
          fprintf(listing,"%-14s ",l->name);
          fprintf(listing,"%-12s ",l->kind);
          fprintf(listing,"%-14s ",l->type);
          fprintf(listing,"%-14s ",l->scope->name);
          while (t != NULL)
          { fprintf(listing,"%4d ",t->lineno);
            t = t->next;
          }
          fprintf(listing,"\n");
          l = l->next;
        }
      }
    }
    s = s->next;
  }
}

/* Function printScopes
 * prints a formatted listing of variables of the scope
 */
void printScopes(FILE * listing)
{ fprintf(listing," Scope Name   Nested Level   Symbol Name   Symbol Type\n");
  fprintf(listing,"------------  ------------  -------------  -----------\n");
  ScopePointer s = globalScope;
  while (s != NULL)
  { 
    for (int i = 0; i < SIZE; i++) {
      BucketPointer l = s->hashTable[i];
      while(l != NULL) {
        if (!strcmp(l->kind, "Variable")){
          fprintf(listing,"%-13s ",s->name);
          fprintf(listing,"%-13d ",s->level);
          fprintf(listing,"%-14s ",l->name);
          fprintf(listing,"%-10s\n",l->type);
        }
        l = l->next;
      }
    }
    s = s->next;    
  }
}

/* Function printFunctions
 * prints a formatted listing of functions
 * , return type and parameter types of the functions
 */
void printFunctions(FILE * listing) 
{ fprintf(listing,"\n\nFunction Name   Return Type   Parameter Types\n");
  fprintf(listing,"-------------  -------------  --------------\n");
  ScopePointer s = globalScope;
  while (s != NULL)
  { 
    for (int i = 0; i < SIZE; i++) {
      BucketPointer l = s->hashTable[i];
      while(l != NULL) {
        if (!strcmp(l->kind, "Function")){
          fprintf(listing,"%-14s ",l->name);
          fprintf(listing,"%-14s ",l->type);
          ParamTypePointer p = l->params;
          while(p != NULL){
            fprintf(listing,"%s ",p->type);
            p = p->next;
          }
          fprintf(listing, "\n");
        }
        l = l->next;
      }
    }
    s = s->next;    
  }
}