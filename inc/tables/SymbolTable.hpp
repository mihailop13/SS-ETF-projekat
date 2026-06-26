#ifndef SYMTABLE_HPP
#define SYMTABLE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <iostream>

using namespace std;

enum SymbolBinding{
  LOCAL,
  GLOBAL
};

enum SymbolType{
  NONE,
  //ABSOLUTE,
  SECTION
};

enum DefStatus{
  DEFINED,
  UNDEFINED
  //UNDEFINED + GLOBAL + isExtern = EXTERN
};

struct SymbolTableEntry{
  SymbolBinding binding;
  SymbolType type;
  DefStatus status;   
  string symbolName;    //name
  uint32_t value;      //offset
  int index;          //sectionIndex
  int size;
  bool isExtern;
  vector<uint32_t> forwardRefs;     //list of forward references, that hasnt been resolved yet
  SymbolTableEntry()
    : binding(LOCAL), type(NONE), status(UNDEFINED),
      symbolName(""), value(0), index(0), size(0), isExtern(false) {} 
  SymbolTableEntry(const string& symbolName, SymbolBinding binding, DefStatus status ,SymbolType type, uint64_t value, int index, int size, bool isExtern)
    : binding(binding), type(type), status(status), symbolName(symbolName), value(value), index(index), size(size), isExtern(isExtern) {}
};

struct SymbolTable {
  int getSize();
  void addSymbol(SymbolTableEntry& sym);
  bool hasSymbol(const string& symbolName);
  SymbolTableEntry& getSymbol(const string& symbolName);
  void write(ostream& output);
  unordered_map<string, SymbolTableEntry> syms;
};

#endif