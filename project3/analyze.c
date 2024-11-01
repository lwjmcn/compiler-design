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

/* counter for variable memory locations */
static int location = 0;

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

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t)
{ switch (t->exprKind)
  { case VarDe:
    case FunDe:
    case Param:
      if (st_lookup(t->name) == -1)
        /* not yet in table, so treat as new definition */
        st_insert(t->name,t->lineno,location++);
      else
      /* already in table, so ignore location, 
         add line number of use only */ 
        st_insert(t->name,t->lineno,0);
      break;
    case Var: 
      if (st_lookup(t->name) == -1)
      /* not yet in table, so treat as new definition */
        st_insert(t->name,t->lineno,location++);
      else
      /* already in table, so ignore location, 
         add line number of use only */ 
        st_insert(t->name,t->lineno,0);
      break;      
    default:
      break;
  }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ traverse(syntaxTree,insertNode,nullProc);
  if (TraceAnalyze)
  { fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

// static void typeError(TreeNode * t, char * message)
// { fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
//   Error = TRUE;
// }

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ switch (t->exprKind)
  { case Const:
    case Var:
      t->type = Int;
      break;
    case OpExpr:
      if ((t->child[0]->type != Int) || (t->child[1]->type != Int))
        fprintf(listing, "Error: invalid operation at line %d\n", lineno);
      t->type = Int;
      break;
    case IfExpr:
    case IfElseExpr:
    case WhileExpr:
      if(t->child[0]->type != Int)
        fprintf(listing, "Error: invalid condition at line %d\n", lineno);
      break;
    case AssignExpr:
      if(t->child[0]->type != Int || t->child[1]->type != Int || t->child[0]->type != t->child[1]->type)
          fprintf(listing, "Error: invalid assignment at line %d\n", lineno);
      break;
    default:
      break;

  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}
