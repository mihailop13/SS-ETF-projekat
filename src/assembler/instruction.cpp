#include "syntax.hpp"
#include <stdexcept>
#include <iostream>
#include "assembler/assembler.hpp"
#include "assembler/instruction.hpp"

using namespace std;

void ensureSymbolExists(Assembler& assembler, const string& symName){
  if(!assembler.symTable.hasSymbol(symName)){
    assembler.addSymbol(symName, 0, UNDEFINED, NONE, LOCAL, false);
  }
}

uint8_t indirectBranchOpcode(uint8_t opcode){
  switch(opcode){
    case 0x20: return 0x21;
    case 0x30: return 0x38;
    case 0x31: return 0x39;
    case 0x32: return 0x3A;
    case 0x33: return 0x3B;
    default: throw runtime_error("Unsupported branch opcode for literal pool");
  }
}

InstrDesc handleBranch(uint8_t opcode, Assembler& assembler, const Instruction& instr){
  int instrOffset = assembler.getCurrentSection().data.size(); //instruction start

  InstrDesc instDesc;
  instDesc.opcode = opcode;
  instDesc.A = 0xF;
  instDesc.B = (instr.type != INS_CALL && instr.type != INS_JMP) ? (uint8_t)instr.regS : 0;
  instDesc.C = (instr.type != INS_CALL && instr.type != INS_JMP) ? (uint8_t)instr.regD : 0;
  if(instr.op.sym == nullptr){
    if(fits12Signed(instr.op.num)){
      instDesc.D = (int16_t)instr.op.num;
      return instDesc;
    }

    assembler.getCurrentSection().addToLiteralPool((uint32_t)instr.op.num, "", instrOffset);
    instDesc.opcode = indirectBranchOpcode(opcode);
    instDesc.D = 0;
    return instDesc;
  }

  string symName(instr.op.sym);
  if(assembler.symTable.hasSymbol(symName)){
    SymbolTableEntry& sym = assembler.symTable.getSymbol(symName);
    if(sym.status == DEFINED && sym.binding == LOCAL && sym.index == assembler.currentSection){
      int32_t D = (int32_t)sym.value - (int32_t)(instrOffset + 4);
      if(fits12Signed(D)){
        instDesc.opcode = opcode;
        instDesc.D = (int16_t)D;
        return instDesc;
      }
    }
    if(sym.status == UNDEFINED){
      assembler.forwardRefs.push_back(ForwardRef(
        symName,
        instrOffset,
        assembler.currentSection,
        PC_RELATIVE
      ));
      //no need to add Symbol because it already exists in symTable
    }
  } else {
    assembler.forwardRefs.push_back(ForwardRef(
      symName,
      instrOffset,
      assembler.currentSection,
      PC_RELATIVE
    ));
    assembler.addSymbol(symName, 0, UNDEFINED, NONE, LOCAL, false);
  }
  assembler.getCurrentSection().addToLiteralPool(0, symName, instrOffset);
  instDesc.opcode = indirectBranchOpcode(opcode);
  //instDesc.A = 0xF;
  instDesc.D = 0;

  return instDesc;
}

InstrDesc handleAritmeticLogic(uint8_t opcode, const Instruction& instr){
  InstrDesc instDesc;
  instDesc.opcode = opcode;
  instDesc.A = (uint8_t)instr.regD;
  instDesc.B = (uint8_t)instr.regD;
  instDesc.C = (uint8_t)instr.regS;
  instDesc.D = (int16_t)0;
  return instDesc;
}

/*LD formats:
  - MMMM = 0001 (0x91): gpr[A] <= gpr[B] + D
  - MMMM = 0010 (0x92): gpr[A] <= mem32[gpr[B] + gpr[C] + D]
*/

InstrDesc handleLD(Assembler& assembler, const Instruction& instr){
  int32_t instrOffset = assembler.getCurrentSection().data.size();
  switch(instr.op.type){
    case IMMED: {
      if(instr.op.sym != nullptr){
        //$sym - symbol address in literal pool, gpr[A] -
        ensureSymbolExists(assembler, string(instr.op.sym));
        assembler.getCurrentSection().addToLiteralPool(0, string(instr.op.sym), instrOffset);
        //0x92 - read from literal pool in memory mem32[pc(B) + D]
        InstrDesc desc{0x92, (uint8_t)instr.regD, 0xF, 0, 0};
        return desc;
      } else if(fits12Signed(instr.op.num)){
        //$lit fits into instruction code
        return InstrDesc{0x91, (uint8_t)instr.regD, 0, 0, (int16_t)instr.op.num};
      } else {
        //$lit doesnt fit into instruction code, must go through literal pool
        //0x92 - read from literal pool in memory mem32[pc(B) + D]
        assembler.getCurrentSection().addToLiteralPool((uint32_t)instr.op.num, "", instrOffset);
        InstrDesc desc{0x92, (uint8_t)instr.regD, 0xF, 0, 0};
        return desc;
      }
    }
    case REGD:{
      InstrDesc inst{0x91, (uint8_t)instr.regD, (uint8_t)instr.op.reg, 0, 0};
      return inst;
    }
    case REGI:{
      InstrDesc inst{0x92, (uint8_t)instr.regD, (uint8_t)instr.op.reg, 0, 0};
      return inst;
    }
    case REGIPOM: {
      if (instr.op.sym != nullptr) {
        string symName(instr.op.sym);
        if (!assembler.symTable.hasSymbol(symName)) {
          throw runtime_error("Undefined symbol '" + symName + "' in [reg+symbol]");
        }
        SymbolTableEntry& sym = assembler.symTable.getSymbol(symName);
        if (sym.status != DEFINED || sym.index != assembler.currentSection) {
          throw runtime_error("Forward reference to symbol or symbol is in another section '" + symName + "' in [reg+symbol].");
        }
        int32_t D = (int32_t)sym.value;
        if (!fits12Signed(D)) {
          throw runtime_error("Displacement " + to_string(D) + " for [reg+symbol] does not fit in 12-bit signed range");
        }
        return InstrDesc{0x92, (uint8_t)instr.regD, (uint8_t)instr.op.reg, 0, (int16_t)D};
      } else {
        // literal
        if (!fits12Signed(instr.op.num)) {
          throw runtime_error("ld [reg+lit]: displacement does not fit in 12-bit signed range");
        }
        return InstrDesc{0x92, (uint8_t)instr.regD, (uint8_t)instr.op.reg, 0, (int16_t)instr.op.num};
      }
    }
    case MEM:{
      //sym or lit - value form memory on that adress
      if(instr.op.sym != nullptr){
        //sym: 2 instructions:
        //1. gpr[regD] <= mem32[pc+D] = symbol address
        ensureSymbolExists(assembler, string(instr.op.sym));
        assembler.getCurrentSection().addToLiteralPool(0, string(instr.op.sym), instrOffset);
        uint32_t instr1 = ((uint32_t)0x92 << 24)
                        | ((uint32_t)instr.regD << 20)
                        | (0xF << 16)   // pc
                        | 0x000;        // D=0 placeholder
        assembler.addData(instr1);
        //regD = address
        //2. gpr[regD] <= mem32[gpr[regD]]  
        return InstrDesc{0x92, (uint8_t)instr.regD, (uint8_t)instr.regD, 0, 0};
      }
      else if(fits12Signed(instr.op.num)){
        InstrDesc instruction({0x92, (uint8_t)instr.regD, 0, 0, (int16_t) instr.op.num});
        return instruction;
      }
      else {
        //1. gpr[regD] <= mem32[pc+D] = address in the literalPool
        assembler.getCurrentSection().addToLiteralPool((uint32_t) instr.op.num, "", instrOffset);
        uint32_t instr1 = ((uint32_t)0x92 << 24)
                        | ((uint32_t)instr.regD << 20)
                        | (0xF << 16)   // pc
                        | 0x000;        // D=0 placeholder
        assembler.addData(instr1);
        //regD = address
        //2. gpr[regD] <= mem32[gpr[regD]]  
        return InstrDesc{0x92, (uint8_t)instr.regD, (uint8_t)instr.regD, 0, 0};
      }
    }
  }
  return InstrDesc{0x00, 0, 0, 0, 0};
}

/*ST formats:
  - MMMM = 0000 (0x80): mem32[gpr[A]+gpr[B]+D] <= gpr[C]
  - MMMM = 0001 (0x81): gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C]
  - MMMM = 0010 (0x82): mem32[mem32[gpr[A]+gpr[B]+D]] <= gpr[C]
*/

InstrDesc handleST(Assembler& assembler, const Instruction& instr){
  switch(instr.op.type){
    case IMMED:
      throw runtime_error("st $immed: cannot store to an immediate value");
    case REGD:
      throw runtime_error("st %reg: register cannot be a memory destination");
    case REGI:{
      InstrDesc inst{0x80, (uint8_t)instr.op.reg, 0, (uint8_t)instr.regS, 0};
      return inst;
    }
    case REGIPOM: {
    if (instr.op.sym != nullptr) {
      string symName(instr.op.sym);
      if (!assembler.symTable.hasSymbol(symName)) {
        throw runtime_error("Undefined symbol '" + symName + "' in [reg+symbol]");
      }
      SymbolTableEntry& sym = assembler.symTable.getSymbol(symName);
      if (sym.status != DEFINED || sym.index != assembler.currentSection) {
        throw runtime_error("Forward reference to symbol or symbol is in another section '" + symName + "' in [reg+symbol].");
      }
      int32_t D = (int32_t)sym.value;
      if (!fits12Signed(D)) {
        throw runtime_error("Displacement " + to_string(D) + " for [reg+symbol] does not fit in 12-bit signed range");
      }
      return InstrDesc{0x80, (uint8_t)instr.op.reg, 0, (uint8_t)instr.regS, (int16_t)D};
    } else {
      //literal
      if (!fits12Signed(instr.op.num)) {
          throw runtime_error("st [reg+lit]: displacement does not fit in 12-bit signed range");
      }
      return InstrDesc{0x80, (uint8_t)instr.op.reg, 0, (uint8_t)instr.regS, (int16_t)instr.op.num};
    }
}
    case MEM:{
      int32_t instrOffset = assembler.getCurrentSection().data.size();
      if(instr.op.sym != nullptr){
        //sym: addres in pool
        //0x82: mem32[mem32[pc + D]] <= gpr[regS]
        ensureSymbolExists(assembler, string(instr.op.sym));
        assembler.getCurrentSection().addToLiteralPool(0, string(instr.op.sym), instrOffset);
        return InstrDesc{0x82, 0xF, 0, (uint8_t)instr.regS, 0};
      } else if(fits12Signed(instr.op.num)){
        return InstrDesc{0x80, 0, 0, (uint8_t)instr.regS, (int16_t)instr.op.num};
      } else {
        //Literal in pool
        //0x82: mem32[mem32[pc + D]] <= gpr[regS]
        assembler.getCurrentSection().addToLiteralPool((uint32_t)instr.op.num, "", instrOffset);
        return InstrDesc{0x82, 0xF, 0, (uint8_t)instr.regS, 0};
      }
    }
  }
  return InstrDesc{0x00, 0, 0, 0, 0};
}

void insertInstruction(Assembler& assembler, const Instruction& instr){
  InstrDesc instruction;
  switch (instr.type){
    case INS_HALT:
      instruction.opcode = instruction.A = instruction.B = instruction.C = instruction.D = 0;
    break;
    case INS_INT:
      instruction.opcode = 0x10; instruction.A = instruction.B = instruction.C = instruction.D = 0;
    break;
    case INS_IRET:
      instruction = {0x97, 0xF, 0xE, 0, 8};   //emulator will restor pc and status automatically, so D=8
    break;
    case INS_CALL:
      //assembler.addData(((uint32_t)0x81 << 24) | (0xE << 20) | (0xF << 16) | (4 & 0xFFF));  //push pc
      instruction = handleBranch(0x20, assembler, instr);   //check when the opcode should be 0x21, and when B is used
    break;
    case INS_RET:
      instruction = {0x93, 0xF, 0xE, 0, 4};
    break;
    case INS_JMP:
      instruction = handleBranch(0x30,assembler, instr);
    break;
    case INS_BEQ:
      instruction = handleBranch(0x31,assembler, instr);
    break;
    case INS_BNE:
      instruction = handleBranch(0x32,assembler, instr);
    break;
    case INS_BGT:
      instruction = handleBranch(0x33,assembler, instr);
    break;
    case INS_PUSH:
      instruction = {0x81, 0xE, 0, (uint8_t)instr.regS, (int16_t)-4};
    break;
    case INS_POP:
      instruction = {0x93, (uint8_t)instr.regD, 0xE, 0, (int16_t)4};
    break;
    case INS_XCHG:
      instruction.opcode = 0x40;
      instruction.A = 0;
      instruction.B = (uint8_t)instr.regD;
      instruction.C = (uint8_t)instr.regS;
      instruction.D = (int16_t)0;
    break;
    case INS_ADD:
      instruction = handleAritmeticLogic(0x50, instr);
    break;
    case INS_SUB:
      instruction = handleAritmeticLogic(0x51, instr);
    break;
    case INS_MUL:
      instruction = handleAritmeticLogic(0x52, instr);
    break;
    case INS_DIV:
      instruction = handleAritmeticLogic(0x53, instr);
    break;
    case INS_NOT:
      instruction.opcode = 0x60;
      instruction.A = (uint8_t) instr.regD;
      instruction.B = (uint8_t) instr.regD;
      instruction.C = (uint8_t) 0;
      instruction.D = (int16_t) 0;
    break;
    case INS_AND:
      instruction = handleAritmeticLogic(0x61, instr);
    break;
    case INS_OR:
      instruction = handleAritmeticLogic(0x62, instr);
    break;
    case INS_XOR:
      instruction = handleAritmeticLogic(0x63, instr);
    break;
    case INS_SHL:
      instruction = handleAritmeticLogic(0x70, instr);
    break;
    case INS_SHR:
      instruction = handleAritmeticLogic(0x71, instr);
    break;
    case INS_LD:
      instruction = handleLD(assembler, instr);
    break;
    case INS_ST:
      instruction = handleST(assembler, instr);
    break;
    case INS_CSRRD:
      instruction = {0x90, (uint8_t)instr.regD, (uint8_t)instr.statusReg, 0, 0};
    break;
    case INS_CSRWR:
      instruction = {0x94, (uint8_t)instr.statusReg, (uint8_t)instr.regS, 0, 0};
    break;
    default:
      throw runtime_error("Unknown instruction");
      break;
  }
  uint32_t instructionMachineCode =
    ((uint32_t)instruction.opcode << 24) |
    ((uint32_t)instruction.A      << 20) |
    ((uint32_t)instruction.B      << 16) |
    ((uint32_t)instruction.C      << 12) |
    ((uint32_t)instruction.D      & 0x0FFF);
  assembler.addData(instructionMachineCode);
}