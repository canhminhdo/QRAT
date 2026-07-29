/* Section 1: Declarations and options */
/* not call to yywrap() after reaching the end of input. */
%option noyywrap
/* not generate a default rule for matching any unmatched characters (just copy unmatched input to yyout) */
%option nodefault
/*  provide the line number */
%option yylineno
%{
#include "utility/macros.hpp"
#include "parser/lexerAux.hpp"
#include "core/Token.hpp"
#include "core/Search.hpp"
#include "ast/Node.hpp"
#include "ast/KetExpNode.hpp"
#include "ast/QubitExpNode.hpp"
#include "ast/ExpNode.hpp"
#include "ast/OpExpNode.hpp"
#include "ast/ConstExpNode.hpp"
#include "ast/NumExpNode.hpp"
#include "ast/StmNode.hpp"
#include "ast/StmSeq.hpp"
#include "ast/SkipStmNode.hpp"
#include "ast/UnitaryStmNode.hpp"
#include "ast/MeasExpNode.hpp"
#include "ast/CondExpNode.hpp"
#include "ast/CondStmNode.hpp"
#include "ast/WhileStmNode.hpp"
#include "ast/BoolExpNode.hpp"
#include "ast/InitExpNode.hpp"
#include "ast/PropExpNode.hpp"
#include "dd/DDOperation.hpp"

#include "Definitions.hpp"
#include <vector>

DeclMode declFlag = NONE_DECL;

// bison generated token file
#include "parser.hpp"
void lexerIdMode(void);

#define RETURN(token) \
  { yylval.yyToken.tokenize(yytext, yylineno); return (token); }
%}

/* reusable patterns */
SingleTargetOp                  I|X|Y|Z|H|S|Sdg|T|Tdg|V|Vdg|SX|SXdg
SingleTargetOpWithOneParam      P|RX|RY|RZ
SingleTargetOpWithTwoParam      U2
SingleTargetOpWithThreeParam    U3
TwoTargetOp                     SWAP|iSWAP|iSWAPdg|Peres|Peresdg|DCX|ECR
TwoTargetOpWithOneParam         RXX|RYY|RZZ|RZX
TwoTargetOpWithTwoParam         XXminusYY|XXplusYY

/* start conditions */
%s ID_MODE
%x COMMENT_MODE
%s CMD_MODE

/* Section 2: A list of patterns and actions */
%%
"prog"      { declFlag = NONE_DECL; return KW_PROG; }
"is"        { return KW_IS; }
"var"       { declFlag = VAR_DECL ; return KW_VAR; }
"const"     { declFlag = CONST_DECL; return KW_CONST; }
"where"     { return KW_WHERE; }
"init"      { return KW_INIT; }
"prop"      { return KW_PROP; }
"begin"     { return KW_BEGIN; }
"end"       { declFlag = NONE_DECL; return KW_END; }
"while"     { return KW_WHILE; }
"do"        { return KW_DO; }
"od"        { return KW_OD; }
"if"        { return KW_IF; }
"then"      { return KW_THEN; }
"else"      { return KW_ELSE; }
"fi"        { return KW_FI; }
"skip"      { return KW_SKIP; }
"qubit"     { return KW_QUBIT; }
"complex"   { return KW_COMPLEX; }
[+\-*/:;,\[\]().{}]       { return *yytext; }
"="		    { return KW_EQUAL; }
":="		{ return KW_ASSIGN; }
"atomic"    { return KW_ATOMIC; }

"pcheck"    { return KW_PCHECK; }

"search"    { return KW_SEARCH; }
"psearch"   { return KW_PSEARCH; }
"in"        { return KW_IN; }
"with"      { return KW_WITH; }
"such"      { return KW_SUCH; }
"that"      { return KW_THAT; }
"prob"      { return KW_PROB; }
"amp"       { return KW_AMP; }
"of"        { return KW_OF; }
"state"     { return KW_STATE; }
"basis"     { return KW_BASIS; }
"=>1"       { return KW_ARROW_ONE; }
"=>*"       { return KW_ARROW_STAR; }
"=>+"       { return KW_ARROW_PLUS; }
"=>!"       { return KW_ARROW_EXCLAMATION; }
"true"      { return KW_TRUE; }
"false"     { return KW_FALSE; }
"P"         { return KW_PROJ; }
"not"       { return KW_NOT; }
"and"       { return KW_AND; }
"or"        { return KW_OR; }

"load"      { return KW_LOAD; }
"show"      { return KW_SHOW; }
"path"      { return KW_PATH; }
"set"       { return KW_SET; }
"timing"    { return KW_TIMING; }
"on"        { return KW_ON; }
"off"       { return KW_OFF; }
"seed"      { return KW_SEED; }
"backend"   { return KW_BACKEND; }
"mqt"       { return KW_MQT; }
"dw"        { return KW_DW; }

"|0>"		{ return KW_KET_ZERO; }
"|1>"		{ return KW_KET_ONE; }
"|+>"		{ return KW_KET_PLUS; }
"|->"		{ return KW_KET_MINUS; }
"random"	{ return KW_KET_RANDOM; }

"quit"      { return KW_QUIT; }

{SingleTargetOp}        { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_OP}; return KW_SINGLE_TARGET_OP; }
"C"{SingleTargetOp}     { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_COP}; return KW_SINGLE_TARGET_COP; }
"MC"{SingleTargetOp}    { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_MCOP}; return KW_SINGLE_TARGET_MCOP; }

{SingleTargetOpWithOneParam}        { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_OP_WITH_ONE_PARAM}; return KW_SINGLE_TARGET_OP_WITH_ONE_PARAM; }
"C"{SingleTargetOpWithOneParam}     { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_COP_WITH_ONE_PARAM}; return KW_SINGLE_TARGET_COP_WITH_ONE_PARAM; }
"MC"{SingleTargetOpWithOneParam}    { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_MCOP_WITH_ONE_PARAM}; return KW_SINGLE_TARGET_MCOP_WITH_ONE_PARAM; }

SingleTargetOpWithTwoParam          { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_OP_WITH_TWO_PARAM}; return KW_SINGLE_TARGET_OP_WITH_TWO_PARAM; }
"C"{SingleTargetOpWithTwoParam}     { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_COP_WITH_TWO_PARAM}; return KW_SINGLE_TARGET_COP_WITH_TWO_PARAM; }
"MC"{SingleTargetOpWithTwoParam}    { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_MCOP_WITH_TWO_PARAM}; return KW_SINGLE_TARGET_MCOP_WITH_TWO_PARAM; }

{SingleTargetOpWithThreeParam}      { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_OP_WITH_THREE_PARAM}; return KW_SINGLE_TARGET_OP_WITH_THREE_PARAM; }
"C"{SingleTargetOpWithThreeParam}   { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_COP_WITH_THREE_PARAM}; return KW_SINGLE_TARGET_COP_WITH_THREE_PARAM; }
"MC"{SingleTargetOpWithThreeParam}  { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, SINGLE_TARGET_MCOP_WITH_THREE_PARAM}; return KW_SINGLE_TARGET_MCOP_WITH_THREE_PARAM; }

{TwoTargetOp}           { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_OP}; return KW_TWO_TARGET_OP; }
"C"{TwoTargetOp}        { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_COP}; return KW_TWO_TARGET_COP; }
"MC"{TwoTargetOp}       { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_MCOP}; return KW_TWO_TARGET_MCOP; }

{TwoTargetOpWithOneParam}       { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_OP_WITH_ONE_PARAM}; return KW_TWO_TARGET_OP_WITH_ONE_PARAM; }
"C"{TwoTargetOpWithOneParam}    { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_COP_WITH_ONE_PARAM}; return KW_TWO_TARGET_COP_WITH_ONE_PARAM; }
"MC"{TwoTargetOpWithOneParam}   { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_MCOP_WITH_ONE_PARAM}; return KW_TWO_TARGET_MCOP_WITH_ONE_PARAM; }

{TwoTargetOpWithTwoParam}       { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_OP_WITH_TWO_PARAM}; return KW_TWO_TARGET_OP_WITH_TWO_PARAM; }
"C"{TwoTargetOpWithTwoParam}    { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_COP_WITH_TWO_PARAM}; return KW_TWO_TARGET_COP_WITH_TWO_PARAM; }
"MC"{TwoTargetOpWithTwoParam}   { Token token; token.tokenize(yytext, yylineno); yylval.gateInfo = {token, TWO_TARGET_MCOP_WITH_TWO_PARAM}; return KW_TWO_TARGET_MCOP_WITH_TWO_PARAM; }

"M"       { return KW_MEASURE; }
PI|pi     { return KW_PI; }
PI2|pi2   { return KW_PI_2; }
PI4|pi4   { return KW_PI_4; }
TAU|tau   { return KW_TAU; }
e         { return KW_E; }

("|phi+>"|"|PHI+>") { return KW_KET_PHI_PLUS; }
("|phi->"|"|PHI->") { return KW_KET_PHI_MINUS; }
("|psi+>"|"|psi+>") { return KW_KET_PSI_PLUS; }
("|psi->"|"|psi->") { return KW_KET_PSI_MINUS; }

"--backend="(PRISM|Storm)   { yylval.str = strdup(yytext); return KW_ARGUMENT; }
"--save-model="(true|false)  { yylval.str = strdup(yytext); return KW_ARGUMENT; }

[a-zA-Z0-9_\-\/\.]+\.[qQ][wW]   { yylval.str = strdup(yytext); return FILENAME; }
[+-]?[0-9]+                 { return INTEGER; }
[+-]?[0-9]+"/"[+-]?[0-9]+   { return RATIONAL; }
[+-]?[0-9]+"."[0-9]*        { return REAL; }
[+-]?("."[0-9]+)            { return REAL; }

\'[01]+\'|\"[01]+\" { yytext[yyleng - 1] = '\0'; yylval.str = strdup(yytext + 1); return BASIS_STR; }
\'[^\']*\'  { yylval.str = strdup(yytext); return FORMULA_STR; }

[_a-zA-Z]([_a-zA-Z0-9])*    {
                // printf("identifier: %s at line %d\n", yytext, yylineno);
                // yylval.codeNr = stringTable.encode(yytext);
                RETURN(IDENTIFIER);
            }

"//".*                  { /* Ignore single-line comment */ }
"/*"                    { lexerCommentMode(); }
<COMMENT_MODE>{
[^*]+     { /* Ignore content inside comments */ }
"*/"      { lexerInitialMode(); }
}

[ \t\f\v\t\r]       /* ignore whitespaces */
\n          /* ignore EOL, otherwise return EOL; */
.           { printf("Mystery character %c at line %d\n", *yytext, yylineno); }

<<EOF>>     {
                /* if no rule for <<EOF>>, flex automatically calls yyterminate() to stop scanning */
                /* handling other input files or just return 0 as end of input */
                return 0;
            }
%%

/* Section 3: C codes */
#include "lexerAux.cpp"