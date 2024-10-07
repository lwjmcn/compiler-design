/****************************************************/
/* File: cminus.y                                   */
/* The C-Minus Yacc/Bison specification file        */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/* modified by Yejin Lee                            */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token WHILE RETURN VOID INT
%token IF
%nonassoc NOELSE 
%nonassoc ELSE
%token ID NUM 
%right ASSIGN
%left PLUS MINUS
%left MUL DIV
%nonassoc LESSEQUAL LESSTHAN GREATEQUAL GREATTHAN EQ NEQ
%token LPAREN LBRACE LCURLY COMMA SEMICOLON
%token RPAREN RBRACE RCURLY
%token ERROR 

%% /* Grammar for C-Minus */

program     : dclr_seq
                 { savedTree = $1; } 
            ;
dclr_seq    : dclr_seq dclr
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $2;
                     $$ = $1; }
                     else $$ = $2;
                 }
            | dclr  { $$ = $1; }
            ;
dclr        : var_dclr { $$ = $1; }
            | func_dclr { $$ = $1; }
            ;
var_dclr    : type id SEMICOLON
                 { $$ = newTreeNode(VarDe);
                   $$->name = $2->name;
                   $$->type = $1->type;
                   $$->lineno = $2->lineno;
                 }
            | type id LBRACE num RBRACE SEMICOLON
                 { $$ = newTreeNode(VarDe);
                   $$->name = $2->name;
                   $$->type = $1->type;
                   $$->isArray = TRUE;
                   $$->child[0] = newTreeNode(Const);
                    $$->child[0]->val = $4->val;
                   $$->lineno = $2->lineno;
                 }
            ;
type        : INT 
                 { $$ = newTreeNode(TypeN); 
                   $$->type = Int; 
                 }
            | VOID 
                 { $$ = newTreeNode(TypeN); 
                   $$->type = Void; 
                 }
            ;
func_dclr   : type id LPAREN params RPAREN cmpd_stmt
                 { $$ = newTreeNode(FunDe); 
                   $$->name = $2->name;
                   $$->lineno = $2->lineno;
                   $$->type = $1->type;
                   $$->child[0] = $4;
                   $$->child[1] = $6;
                 }
            ;
params      : param_list { $$ = $1; }
            | VOID 
                 { $$ = newTreeNode(VoidParam);
                   $$->type = Void; 
                 }
            ;
param_list  : param_list COMMA param
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
            | param { $$ = $1; }
            ;
param       : type id
                 { $$ = newTreeNode(Param);
                   $$->type = $1->type;
                   $$->name = $2->name;
                 }
            | type id LBRACE RBRACE
                 { $$ = newTreeNode(Param);
                   $$->type = $1->type;
                   $$->name = $2->name;
                   $$->isArray = TRUE;
                 }
            ;
cmpd_stmt   : LCURLY local_dclr stmt_list RCURLY
                 { $$ = newTreeNode(CmpdStmt);
                   $$->child[0] = $2;
                   $$->child[1] = $3;
                 }
            ;
local_dclr  : local_dclr var_dclr
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $2;
                     $$ = $1; }
                     else $$ = $2;
                 }
            | empty { $$ = NULL; }
            ;
stmt_list   : stmt_list stmt
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $2;
                     $$ = $1; }
                     else $$ = $2;
                 }
            | empty { $$ = NULL; }
            ;
stmt        : expr_stmt { $$ = $1; }
            | cmpd_stmt { $$ = $1; }
            | select_stmt { $$ = $1; }
            | iter_stmt { $$ = $1; }
            | return_stmt { $$ = $1; }
            ;
expr_stmt   : expr SEMICOLON { $$ = $1; }
            | SEMICOLON { $$ = NULL; }
            ;
select_stmt : IF LPAREN expr RPAREN stmt %prec NOELSE
                 { $$ = newTreeNode(IfExpr);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                 }
            | IF LPAREN expr RPAREN stmt ELSE stmt
                 { $$ = newTreeNode(IfElseExpr);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                   $$->child[2] = $7;
                 }
            ;
iter_stmt   : WHILE LPAREN expr RPAREN stmt
                 { $$ = newTreeNode(WhileExpr);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                 }
            ;
return_stmt : RETURN SEMICOLON
                 { $$ = newTreeNode(ReturnExpr); }
            | RETURN expr SEMICOLON
                 { $$ = newTreeNode(ReturnExpr);
                   $$->child[0] = $2;
                 }
            ;
expr        : var ASSIGN expr
                 { $$ = newTreeNode(AssignExpr);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                 }
            | simple_expr { $$ = $1; }
            ;
var         : id
                 { $$ = newTreeNode(Var);
                   $$->name = $1->name;
                 }
            | id LBRACE expr RBRACE
                 { $$ = newTreeNode(Var);
                   $$->name = $1->name;
                   $$->isArray = TRUE;
                   $$->child[0] = $3;
                 }
            ;
simple_expr : add_expr relop add_expr 
                 { $$ = newTreeNode(OpExpr);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->op = $2->op; 
                 }
            | add_expr { $$ = $1; }
            ;
relop       : LESSEQUAL 
                 { $$ = newTreeNode(OpN); 
                   $$->op = LESSEQUAL; 
                 }
            | LESSTHAN
                 { $$ = newTreeNode(OpN); 
                   $$->op = LESSTHAN; 
                 }
            | GREATTHAN
                 { $$ = newTreeNode(OpN); 
                   $$->op = GREATTHAN; 
                 }
            | GREATEQUAL
                 { $$ = newTreeNode(OpN); 
                   $$->op = GREATEQUAL; 
                 }
            | EQ {
                   $$ = newTreeNode(OpN); 
                   $$->op = EQ; 
                 }
            | NEQ
                 { $$ = newTreeNode(OpN); 
                   $$->op = NEQ; 
                 }
            ;
add_expr    : add_expr add_op term
                 { $$ = newTreeNode(OpExpr);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->op = $2->op;
                 }
            | term { $$ = $1; }
            ;
add_op      : PLUS 
                 { $$ = newTreeNode(OpN); 
                   $$->op = PLUS; 
                 }
            | MINUS 
                 { $$ = newTreeNode(OpN); 
                   $$->op = MINUS;
                 }
            ;
term        : term mul_op factor
                 { $$ = newTreeNode(OpExpr);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->op = $2->op;
                 }
            | factor { $$ = $1; }
            ;
mul_op      : MUL 
                 { $$ = newTreeNode(OpN); 
                   $$->op = MUL; 
                 }
            | DIV 
                 { $$ = newTreeNode(OpN); 
                   $$->op = DIV; 
                 }
            ;
factor      : LPAREN expr RPAREN
                 { $$ = $2; }
            | var { $$ = $1; }
            | call { $$ = $1; }
            | num 
                 { $$ = newTreeNode(Const);
                   $$->val = $1->val;
                 }
            ;
call        : id LPAREN args RPAREN
                 { $$ = newTreeNode(Call);
                   $$->name = $1->name; 
                   $$->child[0] = $3;
                 }
            ;
args      : arg_list { $$ = $1; }
            | empty { $$ = NULL; }
            ;
arg_list  : arg_list COMMA expr
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
            | expr { $$ = $1; }
            ;
id          : ID
                 { $$ = newTreeNode(Var);
                   $$->name = copyString(tokenString);
                 }
            ;
num         : NUM
                 { $$ = newTreeNode(Var);
                   $$->val = atoi(tokenString);
                 }
            ;
empty       :;

%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

