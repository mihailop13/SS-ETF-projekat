%code requires {
    #include <vector>
    #include "syntax.hpp"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "syntax.hpp"

extern int yylineno;
extern char* yytext;
int yylex();
int yyerror(std::vector<Line>& lines, const char *s);

%}

%union {
    int reg;
    int num;
    char *sym;
    char *label;
    char *str;
    Operand operand;
    Directive directive;
    Instruction instr;
    Symbol symbol;
    std::vector<Symbol>* symbolList;
}

/*Tokeni*/

%token GLOBAL EXTERN SECTION WORD SKIP ASCII END
%token HALT INT IRET CALL RET JMP BEQ BNE BGT XCHG
%token PUSH POP
%token ADD SUB MUL DIV NOT AND OR XOR SHL SHR
%token LD ST CSRRD CSRWR NEWLINE
%token STATUS HANDLER CAUSE
%token <str> STRING

%token DOLLAR PLUS COMMA
%token SQUARE_OPEN SQUARE_CLOSED

%token <str> IDENT LABEL
%token <num> HEX_NUMBER DECIMAL_NUMBER REG
%token UNKNOWN

%left PLUS

%type <num> number reg csr
%type <symbolList> symbol_list init_list
%type <symbol> value
%type <sym> symbol
%type <directive> directive
%type <instr> instruction
%type <operand> data_operand

%parse-param {std::vector<Line>& lines}

%%

program:
    lines
    | 
    lines content_line
    ;

lines:
    | 
    lines line
    ;

line:
    NEWLINE
    | 
    content_line NEWLINE
    ;

content_line:
    LABEL                                   {Line line = makeEmptyLine(); line.type = LINE_LABEL; line.label = $1; lines.push_back(line);}
    |
    LABEL instruction                       {Line line = makeEmptyLine(); line.type = LINE_LABEL_INSTRUCTION; line.label = $1; line.inst = $2; lines.push_back(line);}
    |
    LABEL directive                         {Line line = makeEmptyLine(); line.type = LINE_LABEL_DIRECTIVE; line.label = $1; line.dir = $2; lines.push_back(line);}
    |
    instruction                             {Line line = makeEmptyLine(); line.type = LINE_INSTRUCTION; line.label = nullptr; line.inst = $1; lines.push_back(line);}
    |
    directive                               {Line line = makeEmptyLine(); line.type = LINE_DIRECTIVE; line.label = nullptr; line.dir = $1; lines.push_back(line);}
    ;

directive:
    GLOBAL symbol_list                      {$$ = makeEmptyDirective(); $$.type = DIR_GLOBAL; $$.symbolList = $2;}
    |
    EXTERN symbol_list                      {$$ = makeEmptyDirective(); $$.type = DIR_EXTERN; $$.symbolList = $2;}
    |
    SECTION IDENT                           {$$ = makeEmptyDirective(); $$.type = DIR_SECTION; $$.name = $2;}
    |
    WORD init_list                          {$$ = makeEmptyDirective(); $$.type = DIR_WORD; $$.symbolList = $2;}
    |
    SKIP number                             {$$ = makeEmptyDirective(); $$.type = DIR_SKIP; $$.skipAmount = $2;}
    |
    ASCII STRING                            {$$ = makeEmptyDirective(); $$.type = DIR_ASCII; $$.asciiString = $2;}
    |
    END                                     {$$ = makeEmptyDirective(); $$.type = DIR_END;}
    ;

symbol_list:
    symbol                                  {$$ = new std::vector<Symbol>(); Symbol s; s.type = SYM; s.sym = $1; s.num = 0; s.size = 0;  $$->push_back(s);}
    |
    symbol_list COMMA symbol                {$$ = $1; Symbol s; s.type = SYM; s.sym = $3; s.num = 0; s.size = 0; $$->push_back(s); }
    ;

init_list:
    value                                   {$$ = new std::vector<Symbol>(); $$->push_back($1);}
    |
    init_list COMMA value                   {$$ = $1; $$->push_back($3); } /*$$ = $1 kaze da je rezultat novog init_list onaj stari, pa se onda na kraj push_backuje nova vrednost $3 - predstavlja value*/
    ;

value:
    number                                  {$$.type = LIT; $$.num = $1; $$.sym = nullptr;}
    |
    symbol                                  {$$.type = SYM; $$.sym = $1; $$.num = 0;}
    ;

instruction:
    HALT                                    {$$ = makeEmptyInstruction(); $$.type = INS_HALT;}
    |
    INT                                     {$$ = makeEmptyInstruction(); $$.type = INS_INT;}
    |
    IRET                                    {$$ = makeEmptyInstruction(); $$.type = INS_IRET;}
    |
    CALL value                              {$$ = makeEmptyInstruction(); $$.type = INS_CALL; $$.regD = 15; $$.regS = 15; Operand o = makeEmptyOperand(); o.type = IMMED; o.num = $2.num; o.sym = $2.sym; $$.op = o;}
    |
    RET                                     {$$ = makeEmptyInstruction(); $$.type = INS_RET;}
    |
    JMP value                               {$$ = makeEmptyInstruction(); $$.type = INS_JMP; $$.regD = 15; $$.regS = -1; Operand o = makeEmptyOperand(); o.type = IMMED; o.num = $2.num; o.sym = $2.sym; $$.op = o;}
    |
    BEQ reg COMMA reg COMMA value           {$$ = makeEmptyInstruction(); $$.type = INS_BEQ; $$.regS = $2; $$.regD = $4; Operand o = makeEmptyOperand(); o.type = IMMED; o.num = $6.num; o.sym = $6.sym; $$.op = o;}
    |
    BNE reg COMMA reg COMMA value           {$$ = makeEmptyInstruction(); $$.type = INS_BNE; $$.regS = $2; $$.regD = $4; Operand o = makeEmptyOperand(); o.type = IMMED; o.num = $6.num; o.sym = $6.sym; $$.op = o;}
    |
    BGT reg COMMA reg COMMA value           {$$ = makeEmptyInstruction(); $$.type = INS_BGT; $$.regS = $2; $$.regD = $4; Operand o = makeEmptyOperand(); o.type = IMMED; o.num = $6.num; o.sym = $6.sym; $$.op = o;}
    |
    PUSH reg                                {$$ = makeEmptyInstruction(); $$.type = INS_PUSH; $$.regS = $2; $$.regD = 14;}
    |
    POP reg                                 {$$ = makeEmptyInstruction(); $$.type = INS_POP; $$.regD = $2;}
    |
    XCHG reg COMMA reg                      {$$ = makeEmptyInstruction(); $$.type = INS_XCHG; $$.regS = $2; $$.regD = $4;}
    |
    ADD reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_ADD; $$.regS = $2; $$.regD = $4;}
    |
    SUB reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_SUB; $$.regS = $2; $$.regD = $4;}
    |
    MUL reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_MUL; $$.regS = $2; $$.regD = $4;}
    |
    DIV reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_DIV; $$.regS = $2; $$.regD = $4;}
    |
    NOT reg                                 {$$ = makeEmptyInstruction(); $$.type = INS_NOT; $$.regS = $2; $$.regD = $2;}
    |
    AND reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_AND; $$.regS = $2; $$.regD = $4;}
    |
    OR reg COMMA reg                        {$$ = makeEmptyInstruction(); $$.type = INS_OR; $$.regS = $2; $$.regD = $4;}
    |
    XOR reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_XOR; $$.regS = $2; $$.regD = $4;}
    |
    SHL reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_SHL; $$.regS = $2; $$.regD = $4;}
    |
    SHR reg COMMA reg                       {$$ = makeEmptyInstruction(); $$.type = INS_SHR; $$.regS = $2; $$.regD = $4;}
    |
    LD data_operand COMMA reg               {$$ = makeEmptyInstruction(); $$.type = INS_LD; $$.regD = $4; $$.op = $2;}
    |
    ST reg COMMA data_operand               {$$ = makeEmptyInstruction(); $$.type = INS_ST; $$.regS = $2; $$.op = $4;}
    |
    CSRRD csr COMMA reg                     {$$ = makeEmptyInstruction(); $$.type = INS_CSRRD; $$.regD = $4; $$.statusReg = $2;}
    |
    CSRWR reg COMMA csr                     {$$ = makeEmptyInstruction(); $$.type = INS_CSRWR; $$.regS = $2; $$.statusReg = $4;}
    ;


data_operand:
    DOLLAR value                            {Operand o = makeEmptyOperand(); o.type = IMMED; o.num = $2.num; o.sym = $2.sym; $$ = o;}
    |
    value                                   {Operand o = makeEmptyOperand(); o.type = MEM; o.num = $1.num; o.sym = $1.sym; $$ = o;}
    |
    reg                                     {Operand o = makeEmptyOperand(); o.type = REGD; o.reg = $1; $$ = o;}
    |
    SQUARE_OPEN reg SQUARE_CLOSED           {Operand o = makeEmptyOperand(); o.type = REGI; o.reg = $2; $$ = o;}
    |
    SQUARE_OPEN reg PLUS value SQUARE_CLOSED {Operand o = makeEmptyOperand(); o.type = REGIPOM; o.reg = $2; o.num = $4.num; o.sym = $4.sym; $$ = o;}
    ;

symbol:
    IDENT                                   {$$ = $1;}
    ;

number:
    HEX_NUMBER                              {$$ = $1;}
    |
    DECIMAL_NUMBER                          {$$ = $1;}
    ;

reg:
    REG                                     {$$ = $1;}
    ;

csr:
    STATUS                                  {$$ = 0;}
    |
    HANDLER                                 {$$ = 1;}
    |
    CAUSE                                   {$$ = 2;}
    ;
%%

/*Error*/
int yyerror(std::vector<Line>& lines, const char *s) {
    printf("Parser error at line %d: %s (near '%s')\n", yylineno, s, yytext);
    return 0;
}