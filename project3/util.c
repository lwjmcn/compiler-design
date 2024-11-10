/****************************************************/
/* File: util.c                                     */
/* Utility function implementation                  */
/* for the C-Minus compiler                         */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/* modified by Yejin Lee                            */
/****************************************************/

#include "globals.h"
#include "util.h"

/* Procedure printToken prints a token 
 * and its lexeme to the listing file
 */
void printToken( TokenType token, const char* tokenString )
{ switch (token)
  { 
    // reserved word
    case IF:
    case ELSE:
    case WHILE:
    case RETURN:
    case VOID:
    case INT:
      fprintf(listing,
         "reserved word: %s\n", tokenString);
      break;

    // symbols
    case PLUS: fprintf(listing,"+\n"); break; 
    case MINUS: fprintf(listing,"-\n"); break; 
    case MUL: fprintf(listing,"*\n"); break; 
    case DIV: fprintf(listing,"/\n"); break; 
    case ASSIGN: fprintf(listing,"=\n"); break; 
    case SEMICOLON: fprintf(listing,";\n"); break; 
    case COMMA: fprintf(listing,",\n"); break; 
    case LPAREN: fprintf(listing,"(\n"); break; 
    case RPAREN: fprintf(listing,")\n"); break; 
    case LCURLY: fprintf(listing,"{\n"); break; 
    case RCURLY: fprintf(listing,"}\n"); break; 
    case LBRACE: fprintf(listing,"[\n"); break; 
    case RBRACE: fprintf(listing,"]\n"); break; 
    case LESSTHAN: fprintf(listing,"<\n"); break; 
    case LESSEQUAL: fprintf(listing,"<=\n"); break; 
    case GREATTHAN: fprintf(listing,">\n"); break; 
    case GREATEQUAL: fprintf(listing,">=\n"); break; 
    case EQ: fprintf(listing,"==\n"); break; 
    case NEQ: fprintf(listing,"!=\n"); break; 

    case ENDFILE: fprintf(listing,"EOF\n"); break;

    // number
    case NUM:
      fprintf(listing,
          "NUM, val= %s\n",tokenString);
      break;

    // identifier
    case ID:
      fprintf(listing,
          "ID, name= %s\n",tokenString);
      break;

    case ERROR:
      fprintf(listing,
          "ERROR: %s\n",tokenString);
      break;

    default: /* should never happen */
      fprintf(listing,"Unknown token: %d\n",token);
  }
}

/* Function newStmtNode creates a new node
 * for syntax tree construction
 */
TreeNode * newTreeNode(ExprKind exprKind)
{ TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
  int i;
  if (t==NULL)
    fprintf(listing,"Out of memory error at line %d\n",lineno);
  else {
    for (i=0;i<MAXCHILDREN;i++) t->child[i] = NULL;
    t->sibling = NULL;
    t->exprKind = exprKind;
    t->lineno = lineno;
    t->isArray = FALSE;
    t->type = Null;
  }
  return t;
}

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char * copyString(char * s)
{ int n;
  char * t;
  if (s==NULL) return NULL;
  n = strlen(s)+1;
  t = malloc(n);
  if (t==NULL)
    fprintf(listing,"Out of memory error at line %d\n",lineno);
  else strcpy(t,s);
  return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno+=2
#define UNINDENT indentno-=2

/* printSpaces indents by printing spaces */
static void printSpaces(void)
{ int i;
  for (i=0;i<indentno;i++)
    fprintf(listing," ");
}

/* print name and type of a tree node */
void printNameAndType (TreeNode * tree) {
  fprintf(listing, "name = %s, ", tree->name);

  char * type_name = copyString("unspecified");
  switch (tree->type)
  {
    case Int:
      if(tree->isArray)
        type_name = copyString("int[]");
      else
        type_name = copyString("int");
      break;
    case Void:
      if(tree->isArray)
        type_name = copyString("void[]");
      else
        type_name = copyString("void");
      break;
    default:
      break;
  }

  switch (tree->exprKind)
  {
  case FunDe:
    fprintf(listing, "return type = %s\n", type_name);
    break;  
  default:
    fprintf(listing, "type = %s\n", type_name);
    break;
  }
}

/* procedure printTree prints a syntax tree to the 
 * listing file using indentation to indicate subtrees
 */
void printTree( TreeNode * tree )
{ int i;
  INDENT;
  while (tree != NULL) {
    printSpaces();
    switch (tree->exprKind) {
      case VarDe:
        fprintf(listing, "Variable Declaration: ");
        printNameAndType(tree);
        break;
      case FunDe:
        fprintf(listing, "Function Declaration: ");
        printNameAndType(tree);
        break;
      case CmpdStmt:
        fprintf(listing, "Compound Statement:\n");
        break;
      case IfExpr:
        fprintf(listing,"If Statement:\n");
        break;
      case IfElseExpr:
        fprintf(listing,"If-Else Statement:\n");
        break;
      case WhileExpr:
        fprintf(listing, "While Statement:\n");
        break;
      case ReturnExpr:
        if(tree->child[0] == NULL)
          fprintf(listing, "Non-value Return Statement\n");
        else
          fprintf(listing, "Return Statement:\n");
        break;
      case AssignExpr:
        fprintf(listing,"Assign:\n");
        break;
      case OpExpr:
        fprintf(listing,"Op: ");
        printToken(tree->op,"\0");
        break;
      case Call:
        fprintf(listing, "Call: function name = %s\n", tree->name);
        break;
      case TypeN:
      case OpN:
        break;
      case Const:
        fprintf(listing,"Const: %d\n",tree->val);
        break;
      case Var:
        fprintf(listing,"Variable: name = %s\n",tree->name);
        break;
      case VoidParam:
        fprintf(listing, "Void Parameter\n");
        break;
      case Param:
        fprintf(listing, "Parameter: ");
        printNameAndType(tree);
        break;
      default:
        fprintf(listing,"Unknown Exprkind\n");
        printNameAndType(tree);
        break;
    }
    for (i=0;i<MAXCHILDREN;i++)
         printTree(tree->child[i]);
    tree = tree->sibling;
  }
  UNINDENT;
}
