#ifndef LINKER_HPP
#define LINKER_HPP

#include "linker/readFiles.hpp"
#include <map>
#include <vector>
#include <fstream>

struct Linker{
  map<string, GlobalSection> globalSections;
  //vector<string> sectionOrder;  // NOVO: čuva redosled dodavanja
  map<string, GlobalSymbol> globalSymbols;
  vector<ObjectFile> objectFiles;
 
  void mergeSections();
  void createGlobalSymbolTable();
  void assignAddresses(const LinkerOptions& options);
  void resolveSymbolAddresses();
  void applyRelocations();
  void generateHexOutput(ofstream& out);
};

#endif