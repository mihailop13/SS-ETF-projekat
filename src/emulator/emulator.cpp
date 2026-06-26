#include "emulator/emulator.hpp"
#include <string>
#include <sstream>
#include <stdexcept>
#include <iomanip>
using namespace std;

Emulator::Emulator() 
    : halted(false), timerInterruptPending(false), terminalInterruptPending(false){
    //init all registers to zero
    for (int i = 0; i < 16; i++) registers[i] = 0;
    status = 0;
    handler = 0;
    cause = 0;
    //PC is assigned in loadHex
}

uint32_t Emulator::read32(uint32_t addr) {
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) {
    auto it = memory.find(addr + i);
    uint8_t byte = (it != memory.end()) ? it->second : 0;
    value |= (uint32_t)byte << (8 * i);
  }
  return value;
}

void Emulator::write32(uint32_t addr, uint32_t value) {
  for (int i = 0; i < 4; i++) {
    memory[addr + i] = (value >> (8 * i)) & 0xFF;
  }
}

uint32_t& Emulator::getReg(uint8_t regInd) {
  if(regInd == 0) return dummyZero;
  return registers[regInd];
}

void Emulator::loadHex(ifstream& input){
  string line;
  while(getline(input, line)){
    size_t colon = line.find(':');

    if(colon == string::npos)
      throw runtime_error("Wrong input format!");

    uint32_t addr = std::stoul(line.substr(0, colon), nullptr, 16);   //string to unsigned long

    istringstream iss(line.substr(colon + 1));
    string byteStr;
    while (iss >> byteStr) {
        uint8_t byte = std::stoul(byteStr, nullptr, 16);
        memory[addr++] = byte;
    }
  }

  registers[15] = 0x40000000;
}

void Emulator::setReg(uint8_t registerInd, uint32_t value) {
  if(registerInd == 0) return;        //dummyZero
  registers[registerInd] = value;
}
  
void Emulator::execute(InstrDesc instrCode) {
  uint8_t opcode = (instrCode.opcode >> 4) & 0x0F;
  uint8_t mod = instrCode.opcode & 0x0F;

  switch(opcode){
    case 0x0:{
      //HALT - 0x0
      halted = true;
      break;
    }
    case 0x1:{
      //INT - 0x1
      push(status);
      push(registers[15]);
      cause = 4;
      //status &= ~0x1;
      status |= 0x1;
      registers[15] = handler;
      break;
    }
    case 0x2:{
      //CALL - 0x2
      //mod: 0 = direct, 1 = indirect
      push(registers[15]);   //return address
      if (mod == 0)
        registers[15] = getReg(instrCode.A) + getReg(instrCode.B) + (uint32_t)instrCode.D;
      else if (mod == 1) 
        registers[15] = read32(getReg(instrCode.A) + getReg(instrCode.B) + (uint32_t)instrCode.D);
      break;
    }
    case 0x3:{
      //BRANCH - 0x3
      bool condition = true;        //this will stay true if branch is unconditional
      if (mod <= 0x3) {
        if (mod == 0x1) condition = (getReg(instrCode.B) == getReg(instrCode.C));                         //beq
        else if (mod == 0x2) condition = (getReg(instrCode.B) != getReg(instrCode.C));                    //bne
        else if (mod == 0x3) condition = ((int32_t)getReg(instrCode.B) > (int32_t)getReg(instrCode.C));   //bgt
        if (condition) registers[15] = getReg(instrCode.A) + (uint32_t)instrCode.D;
      } else if (mod >= 0x8 && mod <= 0xB) {
        if (mod == 0x9) condition = (getReg(instrCode.B) == getReg(instrCode.C));                         //beq
        else if (mod == 0xA) condition = (getReg(instrCode.B) != getReg(instrCode.C));                    //bne
        else if (mod == 0xB) condition = ((int32_t)getReg(instrCode.B) > (int32_t)getReg(instrCode.C));   //bgt
        if (condition) registers[15] = read32(getReg(instrCode.A) + (uint32_t)instrCode.D);
      }
      break;
    }
    case 0x4:{
      //XCHNG - 0x4
      uint32_t temp = getReg(instrCode.B);
      setReg(instrCode.B, getReg(instrCode.C));
      setReg(instrCode.C, temp);
      break;
    }
    case 0x5:{
      //ARITHMETIC
      
      uint32_t src1 = getReg(instrCode.B);
      uint32_t src2 = getReg(instrCode.C);
      uint32_t result;
      switch(mod){
        case 0x0: result = src1 + src2; break;
        case 0x1: result = src1 - src2; break;
        case 0x2: result = src1 * src2; break;
        case 0x3: if (src2 != 0) result = src1 / src2; else cause = 1; break;
      }
      setReg(instrCode.A, result);
      break;
    }
    case 0x6:{
      //LOGIC
      //mod: 0 = not, 1 = and, 2 = or, 3 = xor
      uint32_t result;
      switch (mod) {
        case 0x0: result = ~getReg(instrCode.B); break;
        case 0x1: result = getReg(instrCode.B) & getReg(instrCode.C); break;
        case 0x2: result = getReg(instrCode.B) | getReg(instrCode.C); break;
        case 0x3: result = getReg(instrCode.B) ^ getReg(instrCode.C); break;
      }
      setReg(instrCode.A, result);
      break;
    }
    case 0x7:{
      //SHIFT
      //mod: 0 = shl, 1 = shr
      uint32_t result;
      if (mod == 0x0) result = getReg(instrCode.B) << getReg(instrCode.C);
      else if (mod == 0x1) result = getReg(instrCode.B) >> getReg(instrCode.C);
      setReg(instrCode.A, result);
      break;
    }
    case 0x8:{
      //STORE
      //mod: 0: mem32[gpr[A] + gpr[B] + D] = gpr[C]
      //     1: gpr[A] = gpr[A] + D; mem32[gpr[A]] = gpr[C]
      //     2: mem32[mem32[gpr[A] + gpr[B] + D]] = gpr[C]
      switch (mod){
        case 0x0:
          write32(getReg(instrCode.A) + getReg(instrCode.B) + (uint32_t)instrCode.D, getReg(instrCode.C));
          break;
        case 0x1:
          setReg(instrCode.A, getReg(instrCode.A) + (uint32_t)instrCode.D);
          write32(getReg(instrCode.A), getReg(instrCode.C));
          break;
        case 0x2:
          write32(read32(getReg(instrCode.A) + getReg(instrCode.B) + (uint32_t)instrCode.D), getReg(instrCode.C));
          break;
      }
      break;
    }
    case 0x9:{
      //LOAD
      //mod: 0: csrrd (gpr[A] = csr[B])
      //     1: gpr[A] = gpr[B] + D
      //     2: gpr[A] = mem32[gpr[B] + gpr[C] + D]
      //     3: gpr[A] = mem32[gpr[B]]; gpr[B] = gpr[B] + D  (post-increment)
      //     4: csrwr (csr[A] = gpr[B])
      //     5: csr[A] = csr[B] | D
      //     6: csr[A] = mem32[gpr[B] + gpr[C] + D]
      //     7: csr[A] = mem32[gpr[B]]; gpr[B] = gpr[B] + D  (post-increment)
      switch (mod){
        case 0x0: { //csrrd
          if (instrCode.B == 0) setReg(instrCode.A, status);
          else if (instrCode.B == 1) setReg(instrCode.A, handler);
          else if (instrCode.B == 2) setReg(instrCode.A, cause);
          break;
        }
        case 0x1: {
          setReg(instrCode.A, getReg(instrCode.B) + (uint32_t)instrCode.D);
          break;
        }
        case 0x2: {
          setReg(instrCode.A, read32(getReg(instrCode.B) + getReg(instrCode.C) + (uint32_t)instrCode.D));
          break;
        }
        case 0x3:{
          uint32_t addr = getReg(instrCode.B);
          setReg(instrCode.A, read32(addr));
          setReg(instrCode.B, addr + (uint32_t)instrCode.D);
          break;
        }
        case 0x4:{
          //csrwr
          if (instrCode.A == 0) status = getReg(instrCode.B);
          else if (instrCode.A == 1) handler = getReg(instrCode.B);
          else if (instrCode.A == 2) cause = getReg(instrCode.B);
          break;
        }
        case 0x5:{
         uint32_t val;
          if (instrCode.B == 0) val = status;
          else if (instrCode.B == 1) val = handler;
          else if (instrCode.B == 2) val = cause;
          val |= (uint32_t)instrCode.D;
          if (instrCode.A == 0) status = val;
          else if (instrCode.A == 1) handler = val;
          else if (instrCode.A == 2) cause = val;
          break; 
        }
        case 0x6:{
          if (instrCode.A == 0) status = read32(getReg(instrCode.B) + getReg(instrCode.C) + (uint32_t)instrCode.D);
          else if (instrCode.A == 1) handler = read32(getReg(instrCode.B) + getReg(instrCode.C) + (uint32_t)instrCode.D);
          else if (instrCode.A == 2) cause = read32(getReg(instrCode.B) + getReg(instrCode.C) + (uint32_t)instrCode.D);
          break;
        }
        case 0x7:{
          if (instrCode.A == 0xF && instrCode.B == 0xE && instrCode.D == 8) {
            uint32_t sp = registers[14];
            uint32_t returnPc = read32(sp);
            uint32_t restoredStatus = read32(sp + 4);
            registers[14] = sp + 8;
            registers[15] = returnPc;
            status = restoredStatus;
            break;
          }

          uint32_t addr = getReg(instrCode.B);
          uint32_t val = read32(addr);
          if (instrCode.A == 0) status = val;
          else if (instrCode.A == 1) handler = val;
          else if (instrCode.A == 2) cause = val;
          setReg(instrCode.B, addr + (uint32_t)instrCode.D);
          break;
        }
      }
      break;
    }
    default:{
      triggerInterrupt(1);
      break;
    }
  }
}

void Emulator::triggerInterrupt(uint8_t interruptCause) {
  push(status);         //push status word
  push(registers[15]);  //push PC
  cause = interruptCause;
  //status |= 0x4;
  status |= 0x1; 
  registers[15] = handler;
}

void Emulator::push(uint32_t value) {
  registers[14] -= 4;
  write32(registers[14], value);
}

uint32_t Emulator::pop() {
  uint32_t value = read32(registers[14]);
  registers[14] += 4;
  return value;
}

void Emulator::checkInterrupts() {
  if(status & 0x4) return;

  //timerPending
  if(timerInterruptPending && !(status & 0x4)){
    timerInterruptPending = false;
    triggerInterrupt(2);
    return;
  }

  //terminalPending
  if(terminalInterruptPending && !(status & 0x2)) {
    terminalInterruptPending = false;
    triggerInterrupt(3);
    return;
  }
}

void Emulator::dumpState() const{
  cout << "Emulated processor executed halt instruction\n";
  cout << "Emulated processor state:\n";
  for (int i = 0; i < 16; i += 4) {
    cout << "r" << i << "=0x" << setw(8) << setfill('0') << hex << registers[i];
    cout << " r" << i+1 << "=0x" << setw(8) << setfill('0') << hex << registers[i+1];
    cout << " r" << i+2 << "=0x" << setw(8) << setfill('0') << hex << registers[i+2];
    cout << " r" << i+3 << "=0x" << setw(8) << setfill('0') << hex << registers[i+3] << dec << "\n";
  }
}