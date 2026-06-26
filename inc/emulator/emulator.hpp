#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include <map>
#include <cstdint>
#include <fstream>
#include "assembler/instruction.hpp"

using namespace std;

struct Emulator{
  map<uint32_t, uint8_t> memory;        //adress -> byte

  uint32_t registers[16];
  uint32_t status;        //status register
  uint32_t handler;       //address of interupt handler
  uint32_t cause;         //cause of interupt
  uint32_t dummyZero = 0;

  bool halted;
  bool timerInterruptPending;
  bool terminalInterruptPending;

  Emulator();
  void loadHex(ifstream& input);
  uint32_t read32(uint32_t addr);
  void write32(uint32_t addr, uint32_t value);
  uint32_t& getReg(uint8_t regInd);
  void setReg(uint8_t registerInd, uint32_t value);
  void push(uint32_t);
  void triggerInterrupt(uint8_t);
  uint32_t pop();
  void execute(InstrDesc instrCode);
  void dumpState() const;
  void checkInterrupts();
};

#endif