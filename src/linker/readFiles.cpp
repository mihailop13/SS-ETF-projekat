#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdexcept>
#include "linker/readFiles.hpp"

using namespace std;

string readString(istream& input){
  int size;
  input.read((char*)&size, sizeof(size));     //(char*)&size -> pointer to size address, next four bytes will be the size
  string s(size, '\0');
  input.read(&s[0], size);                    //start writing in string from 0 place
  return s;
}

uint32_t readUint32(istream& input) {
  uint32_t value;
  input.read((char*)&value, sizeof(value));
  return value;
}

SymbolTableEntry readSymbolTableEntry(const string& line){
  //line: name value size type bind status isExtern index 
  string name, type, binding, status;
  uint32_t value, size;
  int isExtern, index;
  istringstream iss(line);

  iss >> name >> value >> size >> type >> binding >> status >> isExtern >> index;

  SymbolType symType;
  if (type == "SECTION") symType = SECTION;
  else symType = NONE;

  SymbolBinding symBind = (binding == "GLOBAL") ? GLOBAL : LOCAL;
  DefStatus symStatus = (status == "DEF") ? DEFINED : UNDEFINED;
  bool externFlag = (isExtern != 0);

  return SymbolTableEntry(name, symBind, symStatus, symType, value, index, size, externFlag);
}

ObjectFile readObjectFile(const string& filename){
  ifstream file(filename, ios::binary);
  ObjectFile obj;
  obj.filename = filename;

  readString(file);     //#SYMBOL TABLE#
  int numSymEntries = stoi(readString(file));
  //read symbol table
  for(int i = 0; i < numSymEntries; i++){
    string line = readString(file);
    //Parsing:
    SymbolTableEntry entry = readSymbolTableEntry(line);
    obj.symTable.addSymbol(entry);
  }

  readString(file);             //#SECTIONS#
  int numOfSections = stoi(readString(file));
  for(int i = 0; i < numOfSections; i++){
    string header = readString(file);       //section header
    istringstream iss(header);
    string sectionName; int size;
    iss >> sectionName >> size;
    Section s(sectionName);
    s.index = i;
    //Section data
    vector<uint8_t> data(size);
    file.read((char*)data.data(), size);    //data.data points to start of a vector
    char newline; file.get(newline);        //read newLine
    s.data.swap(data);
    s.size = s.data.size();
    //#RELOCATIONS#
    readString(file);         //#RELOCATIONS#
    int numRelocs = stoi(readString(file));
    for(int j = 0; j < numRelocs; j++){
      string reloc = readString(file);
      string symbolName; uint32_t offset;
      istringstream iss(reloc);
      iss >> offset >> symbolName;
      Relocation r(offset, symbolName);
      s.relTable.relocations.push_back(r);
    }
    obj.sections.push_back(s);
  }

  return obj;
}