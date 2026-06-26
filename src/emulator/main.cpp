#include "emulator/emulator.hpp"
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cout << "Usage: emulator <hex_file>\n";
    return 1;
  }

  ifstream file(argv[1]);
  if (!file) {
    cout << "Cannot open file: " << argv[1] << "\n";
    return 1;
  }

  Emulator emu;
  try {
    emu.loadHex(file);
  } catch (const exception& e) {
    cout << "Error loading hex: " << e.what() << "\n";
    return 1;
  }

  // Glavna petlja izvršavanja (prebačena iz run())
  bool halted = false;
  while (!halted) {
    uint32_t pc = emu.registers[15];
    uint32_t instr = emu.read32(pc);
    emu.registers[15] = pc + 4;   // unapred PC

    // Dekodiranje
    InstrDesc desc;
    desc.opcode = (instr >> 24) & 0xFF;
    desc.A = (instr >> 20) & 0x0F;
    desc.B = (instr >> 16) & 0x0F;
    desc.C = (instr >> 12) & 0x0F;
    int32_t D_raw = instr & 0x0FFF;
    if (D_raw & 0x0800) D_raw |= 0xFFFFF000; // sign extension
    desc.D = D_raw;

    emu.execute(desc);

    // Provera prekida (ako nije halt)
    if (!emu.halted) {
      emu.checkInterrupts();
    } else {
      halted = true;
    }
  }

  // Ispis stanja
  emu.dumpState();

  return 0;
}