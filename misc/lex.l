%{
  #include "parser.tab.h"
%}
%option yylineno

digit         [0-9]
letter        [a-zA-Z_]

%%
".global"     { return GLOBAL; }
".extern"     { return EXTERN; }
".section"    { return SECTION; }
".word"       { return WORD; }
".skip"       { return SKIP; }
".ascii"      { return ASCII; }
".end"        { return END; }
"halt"        { return HALT; }
"int"         { return INT; }
"iret"        { return IRET; }
"call"        { return CALL; }
"ret"         { return RET; }
"jmp"         { return JMP; }
"beq"         { return BEQ; }
"bne"         { return BNE; }
"bgt"         { return BGT; }
"push"        { return PUSH; }
"pop"         { return POP; }
"xchg"        { return XCHG; }
"add"         { return ADD; }
"sub"         { return SUB; }
"mul"         { return MUL; }
"div"         { return DIV; }
"not"         { return NOT; }
"and"         { return AND; }
"or"          { return OR; }
"xor"         { return XOR; }
"shl"         { return SHL; }
"shr"         { return SHR; }
"ld"          { return LD; }
"st"          { return ST; }
"csrrd"       { return CSRRD; }
"csrwr"       { return CSRWR; }
"%status"     { return STATUS; }
"%handler"    { return HANDLER; }
"%cause"      { return CAUSE; }

"$"           { return DOLLAR; }
"+"           { return PLUS; }
"["           { return SQUARE_OPEN; }
"]"           { return SQUARE_CLOSED; }
","           { return COMMA; }

"%r"([0-9]|1[0-3])    { yylval.num = atoi(yytext + 2); return REG; }
"%r14"|"%sp"          { yylval.num = 14; return REG; }
"%r15"|"%pc"          { yylval.num = 15; return REG; }


#.*                               {}
\n                                { return NEWLINE; }
[ \t\r]+                          {}
-?0[xX][0-9a-fA-F]+               { yylval.num = (long)strtol(yytext, NULL, 0); return HEX_NUMBER; }
-?{digit}+                        { yylval.num = atoi(yytext); return DECIMAL_NUMBER; }
\"[^\"]*\"                        { yylval.str = strdup(yytext + 1); yylval.str[strlen(yylval.str) - 1] = '\0'; return STRING; }
{letter}({letter}|{digit})*":"    { yylval.str = strdup(yytext);yylval.str[strlen(yylval.str)-1] = '\0';return LABEL; }
{letter}({letter}|{digit})*       { yylval.str = strdup(yytext); return IDENT; }

. { 
  printf("Unknown character [%c]\n",yytext[0]);
  return UNKNOWN;    
  }

%%

int yywrap() { return 1; }