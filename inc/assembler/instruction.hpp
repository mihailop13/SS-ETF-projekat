#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "syntax.hpp"
#include "assembler/assembler.hpp"

struct InstrDesc{
  uint8_t opcode;
  uint8_t A, B, C;
  int16_t D;   //has to be 12bit value
};

void insertInstruction(Assembler& assembler, const Instruction& instr);

#endif 