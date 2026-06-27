#include "tables/SymbolTable.hpp"
#include <stdexcept>
#include <iostream>

inline void writeString(ostream& output, const string& s){
  int size = s.size();
  output.write((char*)&size, sizeof(size));
  output.write(s.c_str(), size);
}

bool SymbolTable::hasSymbol(const string& symbolName){
  return syms.find(symbolName) != syms.end();
}

void SymbolTable::addSymbol(SymbolTableEntry& sym){
  if(hasSymbol(sym.symbolName)){
    throw runtime_error("Symbol already exists: " + sym.symbolName);
  }
  syms[sym.symbolName] = sym;
}

SymbolTableEntry& SymbolTable::getSymbol(const string& symbolName){
  if(hasSymbol(symbolName))
    return syms[symbolName];
  throw runtime_error("Symbol not found: " + symbolName); 
}

void SymbolTable::write(ostream& output) {
  writeString(output, "#SYMBOL TABLE#");
  writeString(output, to_string(syms.size()) + "\n");
  for (const auto& [name, sym] : syms) {
    string type = sym.type == SECTION ? "SECTION" : "NOTYPE";
    string bind = sym.binding == GLOBAL ? "GLOBAL" : "LOCAL";
    string status = sym.status == DEFINED ? "DEF" : "UND";
    string symbolTableEntry = name + " " + to_string(sym.value) + " " + 
      to_string(sym.size) + " " + type + " " + bind + " " + status + " " + (sym.isExtern ? "1" : "0") + " " + to_string(sym.index) + "\n";
    writeString(output, symbolTableEntry);
  }
}