#ifndef READFILES_HPP
#define READFILES_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include "tables/SymbolTable.hpp"
#include "tables/Section.hpp"
#include "tables/RelocationTable.hpp"

using namespace std;

//Represents one .o file
struct ObjectFile {
  string filename;
  SymbolTable symTable;           //symTable inside this objFile
  vector<Section> sections;       //all sections inside that file
};

//Represents one global symbol in final symbol table
struct GlobalSymbol {
  string name;
  bool isExtern;
  bool isDefined;
  uint32_t value;                 //final absolute address
  string sectionName;             //name of section in global section list
  ObjectFile* owner;              //in wich file symbol is defined
};

struct GlobalSection {
  string name;
  uint32_t baseAddr;                //address where merged section will be placed
  uint32_t size;                    //size of merged section
  vector<uint8_t> data;             //merged section data
  RelocationTable relTable;
};

struct LinkerOptions{
  string outputFile;
  bool hexMode;                     //-hex
  map<string, uint32_t> place;      //-place=sekcija@adresa
  vector<string> inputFiles;
};

string readString(istream& input);

uint32_t readUint32(istream& input);

ObjectFile readObjectFile(const string& filename);

#endif