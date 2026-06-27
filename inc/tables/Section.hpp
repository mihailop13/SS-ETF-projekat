#ifndef SCTION_HPP
#define SCTION_HPP
#include <string>
#include <vector>
#include "tables/RelocationTable.hpp"
#include "tables/SymbolTable.hpp"

struct Assembler;

using namespace std;

struct LiteralPoolEntry{
  uint32_t value;     //value of literal, or address of a symbol
  string sym;         //symbol name, "" if literal is not a symbol
  int32_t offset;     //offset inside a section
  int useCount;       //number of instructions that use this literal
};

struct LiteralPoolUse {
  uint32_t instrOffset;
  int32_t poolIndex;    

  LiteralPoolUse(uint32_t instrOffset, int32_t poolIndex)
    : instrOffset(instrOffset), poolIndex(poolIndex) {}
};

inline bool fits12Signed(int32_t value) { return value >= -2048 && value <= 2047; };

struct Section{
  string name;
  uint64_t size;
  uint64_t index;
  vector<uint8_t> data;             //machine code inside section
  vector<LiteralPoolEntry> pool;    //each section has a separate literalPool
  vector<LiteralPoolUse> poolUses;  //tracks the instructions that use literal
  void addToLiteralPool(uint32_t value, string symbol, uint32_t instrOffset);     //instrOffset - offset where backpatching has to be done
  void writeLiteralPool(Assembler& assembler);          //write literalPool at the end of section code
  void resolveLiterals();
  void removePoolUseByInstr(uint32_t instrOffset);
  Section(const string& sectionName) : name(sectionName), size(0), index(0) {}
  void patchData(int offset, uint32_t value);
  void patchD12(int instrOffset, int16_t D);
  void cleanUpLiterals();
  RelocationTable relTable;
};

#endif