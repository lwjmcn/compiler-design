/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the C-Minus compiler                         */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/* modified by Yejin Lee                            */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "util.h"

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* Function getTypeName
 * gets the name of type as a char array
 * from the tree node  
 */
char* getTypeName (TreeNode* t)
{ if(t == NULL) return "undetermined";
  switch (t->type)
  {
  case Int:
    if(t->isArray) return "int[]";
    else return "int";
  case Void:
    if(t->isArray) return "void[]";
    else return "void";
  case Null:
  default:
    return "undetermined";
  }
}

/* Function assignType
 * assigns the type given by a char array
 * to the tree node in the syntax tree
 */
void assignType (TreeNode** tp, char* typeName)
{ if (!strcmp(typeName, "int")) {
    (*tp)->type = Int;
    (*tp)->isArray = FALSE;
  } else if (!strcmp(typeName, "int[]")){
    (*tp)->type = Int;
    (*tp)->isArray = TRUE;
  } else if (!strcmp(typeName, "void")) {
    (*tp)->type = Void;
    (*tp)->isArray = FALSE;
  } else if (!strcmp(typeName, "void[]")){
    (*tp)->type = Void;
    (*tp)->isArray = TRUE;
  } else {
    (*tp)->type = Null;
    (*tp)->isArray = FALSE;
  }
}

/* used for saving param types */
/* name of the function which is just declared */
char * functionName = NULL; 
/* location of the upcoming param in the function "functionName" */
int paramLoc = 0;

/* Procedure insertNode inserts 
 * identifiers stored in t into the symbol table 
 * and insert scopes for function declarations and compound statments
 * preprocessing of first traverse of AST
 */
static void insertNode( TreeNode * t)
{ switch (t->exprKind)
  { case VarDe: 
      addNode(t->name, "Variable", getTypeName(t), t->lineno);
      break;
    case FunDe:
      insertScope(NULL); // fake scope
      addNode(t->name, "Function", getTypeName(t), t->lineno);
      insertScope(t->name); // scope for parameters and cmpd stmt 
      assignType(&(t->child[1]), getTypeName(t));
      /* for params */
      functionName = t->name;
      paramLoc = 0;
      break;
    case VoidParam:
      addParamType(functionName, paramLoc, getTypeName(t));
      break;
    case Param:
      addParamType(functionName, paramLoc++, getTypeName(t));
      addNode(t->name, "Variable", getTypeName(t), t->lineno);
      break;    
    case CmpdStmt:
      if(t->type == Null) // not function declaration
        insertScope(NULL);
      break;
    default:
      break;
  }
}

/* Function escapeScope
 * utilizes exitScope from symtab.c 
 * scope is escaped when 
 * the function declaration is over
 * post processing of first traverse of AST
 */
static void escapeScope(TreeNode * t)
{ switch (t->exprKind)
  { case FunDe:
      exitScope();
      break;
    case CmpdStmt:
      if(t->type == Null) // not function declaration
        exitScope();
      break;
    default:
      break;
  }

}

/* Funciton addBuiltInFunction
 * adds built-in functions
 * to the existing syntax tree
 */
void addBuiltInFunction(TreeNode** syntaxTree){
  /* int input(void) { return int; } */
  TreeNode* ifunc = newTreeNode(FunDe);
  TreeNode* voidParam = newTreeNode(VoidParam);
  TreeNode* cmpdStmt = newTreeNode(CmpdStmt);
  TreeNode* returnStmt = newTreeNode(ReturnStmt);
  TreeNode* returnValue = newTreeNode(Const);

  returnValue->type = Int;
  returnStmt->child[0] = returnValue;
  returnStmt->lineno = 0; 
  cmpdStmt->child[1] = returnStmt;
  cmpdStmt->lineno = 1; // for return stmt checking
  voidParam->type = Void;
  ifunc->name = copyString("input");
  ifunc->lineno = 0;
  ifunc->type = Int;
  ifunc->child[0] = voidParam;
  ifunc->child[1] = cmpdStmt;

  /* void output(int value) {} */
  TreeNode* cmpdStmt2 = newTreeNode(CmpdStmt);
  TreeNode* intParam = newTreeNode(Param);
  TreeNode* ofunc = newTreeNode(FunDe);

  intParam->name = copyString("value");
  intParam->type = Int;
  intParam->lineno = 0;
  ofunc->name = copyString("output");
  ofunc->lineno = 0;
  ofunc->type = Void;
  ofunc->child[0] = intParam; 
  ofunc->child[1] = cmpdStmt2;

  ifunc->sibling = ofunc;
  ofunc->sibling = *syntaxTree;
  *syntaxTree = ifunc;
}

/* Function buildSymtab 
 * constructs the symbol table 
 * by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ /* add built in functions */
  addBuiltInFunction(&syntaxTree);

  /* initialize global scope */
  initSymtab();
  
  /* traverse AST */
  traverse(syntaxTree,insertNode,escapeScope);

  if (TraceAnalyze)
  { fprintf(listing,"\n\n< Symbol table >\n");
    printSymTab(listing);
    fprintf(listing,"\n\n< Scopes >\n");
    printScopes(listing);
    fprintf(listing,"\n\n< Functions >\n");
    printFunctions(listing);
  }
}

/* the line number of a function whose return type is int */
int intFunctionLineno = -1;

/* Function enterScopes
 * enters the scope
 * at function declarations and return statements
 * and do things for function type checking
 * preprocessing of second traverse of AST
 */
static void enterScopes(TreeNode * t) 
{ switch (t->exprKind) 
  { case FunDe:
      enterScope();

      /* check if the function is defined before */
      int* funcDclrLines = checkPredefined(t->name, "Function", t->lineno);
      if (funcDclrLines[0] > 0){
        fprintf(listing, "Error: Symbol \"%s\" is redefined at line %d (already defined at line",t->name, t->lineno);
        for (int i = 1; i <= funcDclrLines[0]; i++)
          fprintf(listing, " %d", funcDclrLines[i]);
        fprintf(listing, ")\n");
      }

      /* set flag for function whose return type is int to check return stmt */
      if(t->type == Int){
        intFunctionLineno = t->lineno;
      }

      enterScope();
      assignType(&(t->child[1]), getTypeName(t));
      break;
    case CmpdStmt:
      if(t->type == Null) // not for function
        enterScope();
      break;
    default:
      break;
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 * post processing of second traverse of AST
 */
static void checkNode(TreeNode * t)
{ switch (t->exprKind)
  { case Var:
      /* find type from the symbol table */
      char* variableTypeName = findType(t->name, "Variable");
      
      /* set type information by declaration */
      if(variableTypeName != NULL){
        assignType(&t, variableTypeName); 
      } else {
        /* no declaration */
        fprintf(listing, "Error: undeclared variable \"%s\" is used at line %d\n", t->name, t->lineno);
        /* implicit declaration */    
        addNode(t->name, "Variable", "undetermined", t->lineno);
      }

      /* array */
      if(t->isArray){
        /* variable is not defined as an array type */
        if(strcmp(variableTypeName, "int[]"))
          fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indexing can only allowed for int[] variables\n", t->lineno, t->name);
        
        /* array index */
        if (t->child[0] != NULL) {
          /* set type of var[index] as int */
          if (t->child[0]->type == Int)
            assignType(&t, "int"); 
          /* array index should be int value */
          else fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indicies should be integer\n", t->lineno, t->name);
        }
      }
      break;
    case VarDe:     
      /* check if the variable is defined before */
      int* varDclrLines = checkPredefined(t->name, "Variable", t->lineno);
      if (varDclrLines[0] > 0){
        fprintf(listing, "Error: Symbol \"%s\" is redefined at line %d (already defined at line",t->name, t->lineno);
        for (int i = 1; i <= varDclrLines[0]; i++)
          fprintf(listing, " %d", varDclrLines[i]);
        fprintf(listing, ")\n");
      }

      /* cannot declare a void-type variable */
      if(t->type == Void)
        fprintf(listing, "Error: The void-type variable is declared at line %d (name : \"%s\")\n", t->lineno, t->name);      
     
      /* array indexing check */
      if((t->isArray) && (t->child[0]->type != Int)) 
        fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indicies should be integer\n", t->lineno, t->name);     
      break;
    case Call:
      /* find type from the symbol table */
      char* functionTypeName = findType(t->name, "Function");

      /* set type information by declaration */
      if(functionTypeName != NULL){
        if (!strcmp(functionTypeName, "undetermined")){
          /* params are also undetermined type */
          fprintf(listing, "Error: Invalid function call at line %d (name : \"%s\")\n", t->lineno, t->name);
          break;
        } else {
          /* good */
          assignType(&t, functionTypeName); 
        }  
      } else {
        /* no declaration */
        fprintf(listing, "Error: undeclared function \"%s\" is called at line %d\n", t->name, t->lineno);
        /* implicit declaration */
        addNode(t->name, "Function", "undetermined", t->lineno); 
        /* undetermined param type, return type */
        fprintf(listing, "Error: Invalid function call at line %d (name : \"%s\")\n", t->lineno, t->name);
        assignType(&t, "undetermined");
        break;
      }
      
      /* check function argument's type and location */
      if (t->child[0] != NULL) { 
        /* child: arguments */
        int i = 0;
        TreeNode* args = t->child[0];
        while(args != NULL){
          if (checkParam(t->name, i, getTypeName(args)) == -1){
            fprintf(listing, "Error: Invalid function call at line %d (name : \"%s\")\n", args->lineno, t->name);
            break;
          }
          args = args->sibling;
          i++;
        }
      } else { 
        /* should be void param */
        if(checkVoidParam(t->name) == -1)
          fprintf(listing, "Error: Invalid function call at line %d (name : \"%s\")\n", t->lineno, t->name);
      }
      break;  
    case OpExpr:
      /* only int variables are compatible with arithmetic and logical operations */
      if ((t->child[0]->type == Int) && (!t->child[0]->isArray) && (t->child[1]->type == Int) && (!t->child[1]->isArray))
        assignType(&t, "int");
      else {
        fprintf(listing, "Error: invalid operation at line %d\n", t->lineno);
        assignType(&t, "undetermined");
      }
      break;
    case AssignExpr:
      /* only allowed to assign int to int or int[] to int[] */
      if((t->child[0]->type == t->child[1]->type) && (t->child[0]->isArray == t->child[1]->isArray)){
        assignType(&t, getTypeName(t->child[0]));
      }
      else {
        assignType(&t, "undetermined");
        fprintf(listing, "Error: invalid assignment at line %d\n", t->lineno);
      }
      break;
    case IfStmt:
    case IfElseStmt:
    case WhileStmt:
      /* only allowed to use int value for condition */
      if(t->child[0]->type != Int) // condition
        fprintf(listing, "Error: invalid condition at line %d\n", t->lineno);
      break;
    case ReturnStmt:
      intFunctionLineno = -1; // return is stated

      char* functionReturnType = findType(NULL, "Function");
      
      /* check function return type */
      // if(functionReturnType == NULL || !strcmp(functionReturnType, "undetermined")){ 
      //   /* no declaration */
      //   fprintf(listing, "RETURN: undeclared function error");
      // }
      if (t->child[0] == NULL){ 
        /* return void */
        if(strcmp(functionReturnType, "void" ))
          fprintf(listing, "Error: Invalid return at line %d\n", t->lineno);
      }
      else {
        if(strcmp(getTypeName(t->child[0]), functionReturnType))
          fprintf(listing, "Error: Invalid return at line %d\n", t->lineno);
      }
      break;
    case CmpdStmt:
      if(t->type == Null) // not for function
        popScope();
      else // function's cmpd
        if (intFunctionLineno != -1) 
          fprintf(listing, "Error: missing return statement at line %d\n", intFunctionLineno);
    default:
      break;
      
  }
}

/* Procedure typeCheck 
 * performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ /* add built in functions */
  addBuiltInFunction(&syntaxTree);
  
  /* initialize current scope */  
  initCurrentScope();

  /* traverse AST */
  traverse(syntaxTree,enterScopes,checkNode);
}
