#ifndef SYNTAX_HPP
#define SYNTAX_HPP
#include <vector>
#include <cstdint>

enum elemType{
  SYM,
  LIT
};

struct Symbol{
  elemType type;
  char *sym;
  int32_t num;
  int32_t size;
};

enum Directive_Type{
  DIR_GLOBAL,
  DIR_EXTERN,
  DIR_WORD,
  DIR_SECTION,
  DIR_SKIP,
  DIR_ASCII,
  DIR_END
};

struct Directive{
  Directive_Type type;
  std::vector<Symbol> *symbolList;
  char *name;
  int skipAmount;
  char *asciiString;
};

enum Instruction_Type{
  INS_HALT,
  INS_INT,
  INS_IRET,
  INS_CALL,
  INS_RET,
  INS_JMP,
  INS_BEQ,
  INS_BNE,
  INS_BGT,
  INS_PUSH,
  INS_POP,
  INS_XCHG,
  INS_ADD,
  INS_SUB,
  INS_MUL,
  INS_DIV,
  INS_NOT,
  INS_AND,
  INS_OR,
  INS_XOR,
  INS_SHL,
  INS_SHR,
  INS_LD,
  INS_ST,
  INS_CSRRD,
  INS_CSRWR
};

enum Operand_Type{
  IMMED,
  REGD,
  REGI,
  REGIPOM,
  MEM
};

struct Operand{
  Operand_Type type;
  int32_t num;
  int reg;    //if data_operand
  char* sym;
};

struct Instruction{
  Instruction_Type type;
  Operand op;
  int regD;
  int regS;
  int statusReg;
};

enum Line_Type{
  LINE_LABEL,
  LINE_DIRECTIVE,
  LINE_INSTRUCTION,
  LINE_LABEL_INSTRUCTION,
  LINE_LABEL_DIRECTIVE
};

struct Line {
  Line_Type type;
  char* label;
  Directive dir;
  Instruction inst;
};

inline Operand makeEmptyOperand() {
  Operand o;
  o.type = IMMED;
  o.num = -1;
  o.reg = -1;
  o.sym = nullptr;
  return o;
}

inline Directive makeEmptyDirective() {
  Directive d;
  d.type = DIR_END;
  d.symbolList = nullptr;
  d.name = nullptr;
  d.skipAmount = 0;
  d.asciiString = nullptr;
  return d;
}

inline Instruction makeEmptyInstruction() {
  Instruction i;
  i.type = INS_HALT;
  i.op = makeEmptyOperand();
  i.regD = -1;
  i.regS = -1;
  i.statusReg = -1;
  return i;
}

inline Line makeEmptyLine() {
  Line l;
  l.type = LINE_LABEL;
  l.label = nullptr;
  l.dir = makeEmptyDirective();
  l.inst = makeEmptyInstruction();
  return l;
}
#endif