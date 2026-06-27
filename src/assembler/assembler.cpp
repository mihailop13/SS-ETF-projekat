#include "assembler/assembler.hpp"
#include "tables/SymbolTable.hpp"
#include "assembler/directive.hpp"
#include "assembler/instruction.hpp"

#include <cstdio>
#include <stdexcept>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

inline void writeString(ostream& output, const string& s){
  int size = s.size();
  output.write((char*)&size, sizeof(size));
  output.write(s.c_str(), size);
}

void Assembler::addSection(const string& section) {
  //Add section to sections vector and add symbol for it in symbol table
  if(symTable.hasSymbol(section))                                      
    throw runtime_error("Section " + section + " already exists");
  
  Section sec(section);
  sec.index = sections.size();    //section index is its position in sections vector
  sections.push_back(sec);
  currentSection = sections.size() - 1;

  addSymbol(section, 0, DEFINED, SECTION, LOCAL, false);  //add section to symbolTable, value is 0 for now, will be updated in linker
}

void Assembler::addSymbol(const string& symbolName, uint64_t value, DefStatus status, SymbolType type, SymbolBinding binding, bool isExtern) {
  int index;
  if(status == UNDEFINED)   //status is UND, when symbol is used before its definition, so it has no section index yet, so it is set to 0, which is reserved for undefined symbols
    index = 0;      //if symbol is undefined, it has no section index, so it is set to 0, which is reserved for undefined symbols
  else
    index = currentSection;
  SymbolTableEntry sym(symbolName, binding, status, type, value, index, 0, isExtern);
  symTable.addSymbol(sym);
}

void Assembler::addLabel(const string& label){
  if(symTable.hasSymbol(label)){
    //if label exsits, then it has been used in forward 
    SymbolTableEntry& symbol = symTable.getSymbol(label);
    if(symbol.status == UNDEFINED && symbol.type == NONE){
      //label is defined now, and its value is current location counter
      symbol.status = DEFINED;
      symbol.index = currentSection;
      symbol.value = getLocationCounter();
    } else
        throw runtime_error("Label " + label + " already defined");
  } else{
    SymbolTableEntry symbol(label, LOCAL, DEFINED, NONE, getLocationCounter(), currentSection, 0, false);
    symTable.addSymbol(symbol);
  }
}

uint64_t Assembler::getLocationCounter(){
  if(getCurrentSection().data.empty())
    return 0;
  return getCurrentSection().data.size();
}

void Assembler::refreshSectionSize(Section& sec) {
  //Used to update section size inside symbol table, and section header after adding data to section
  sec.size = sec.data.size();
  if (symTable.hasSymbol(sec.name)) 
    symTable.getSymbol(sec.name).size = sec.size;
}

void Assembler::addData(uint32_t data){
  Section& currSection = getCurrentSection();
  currSection.data.push_back(data & 0xFF);
  currSection.data.push_back((data >> 8) & 0xFF);
  currSection.data.push_back((data >> 16) & 0xFF);
  currSection.data.push_back((data >> 24) & 0xFF);
  refreshSectionSize(currSection);
}

void Assembler::addByte(uint8_t data) {
  Section& currSection = getCurrentSection();
  currSection.data.push_back(data);
  refreshSectionSize(currSection);
}

Section& Assembler::getCurrentSection() {
  /*if (currentSection < 0)
    throw runtime_error("No active section");*/
  return sections[currentSection];
}

bool handleDirective(Assembler& assembler, const Directive& dir){
  switch(dir.type){
    case DIR_GLOBAL:  globalHandler(assembler, dir); return true;
    case DIR_EXTERN:  externHandler(assembler, dir); return true;
    case DIR_WORD:    wordHandler(assembler, dir);   return true;
    case DIR_SECTION: sectionHandler(assembler, dir); return true;
    case DIR_SKIP:    skipHandler(assembler, dir);   return true;
    case DIR_ASCII:   asciiHandler(assembler, dir);  return true;
    case DIR_END:     return false;
  }
  return true;
}

void Assembler::writeAllLiteralPools(){
  for(Section& s : sections){
    s.writeLiteralPool(*this);
  }
  for(Section& s : sections){
    s.resolveLiterals();
  }
}

void Assembler::assemble(const vector<Line>& lines) {
  //Iterate through each line and handle it based on its type
  for(const Line& line : lines) {
    switch(line.type){
      case LINE_LABEL:
        addLabel(line.label);
        break;
      case LINE_DIRECTIVE:
        if (!handleDirective(*this, line.dir)){
          return;  //.end
        }
        break;
      case LINE_INSTRUCTION:
        insertInstruction(*this, line.inst);
        break;
      case LINE_LABEL_INSTRUCTION:
        addLabel(line.label);
        insertInstruction(*this, line.inst);
        break;
      case LINE_LABEL_DIRECTIVE:
        addLabel(line.label);
        if (!handleDirective(*this, line.dir)){
          return;  //.end
        }
        break;
    }
  }
}

void Assembler::backpatch() {
  //Resolve forward references
  for (auto& ref : forwardRefs) {
    //for each forward reference, check if the symbol is defined 
    Section& sec = sections[ref.sectionInd];
    if (!symTable.hasSymbol(ref.symbol)) 
      throw runtime_error("Undefined symbol: " + ref.symbol);

    //Get the symbol entry from the symTable
    SymbolTableEntry& sym = symTable.getSymbol(ref.symbol);

    if (sym.binding == GLOBAL || sym.isExtern || sym.index != ref.sectionInd){
      //Symbol is global/extern or in another section, instruction uses literalPool
      continue;
    }

    //sym is local and in current section
    if (ref.type == PC_RELATIVE) {
      int pc = ref.instrOffset + 4;
      int32_t D = (int32_t)sym.value - pc;

      uint32_t word = sec.data[ref.instrOffset] |
              (sec.data[ref.instrOffset + 1] << 8) |
              (sec.data[ref.instrOffset + 2] << 16) |
              (sec.data[ref.instrOffset + 3] << 24);
      uint8_t curOp = (word >> 24) & 0xFF;
      uint8_t B = (word >> 16) & 0x0F;
      uint8_t C = (word >> 12) & 0x0F;

      if (fits12Signed(D)) {
        uint8_t newOp = curOp;
        switch (curOp) {
            case 0x21: newOp = 0x20; break;
            case 0x38: newOp = 0x30; break;
            case 0x39: newOp = 0x31; break;
            case 0x3A: newOp = 0x32; break;
            case 0x3B: newOp = 0x33; break;
            default: break;
        }
        // Zadržavamo originalno A (koje je već u word-u na poziciji 20–23)
        uint8_t A = (word >> 20) & 0x0F;
        uint32_t newWord = ((uint32_t)newOp << 24) |
                            ((uint32_t)A << 20) |
                            ((uint32_t)B << 16) |
                            ((uint32_t)C << 12) |
                            ((uint32_t)D & 0x0FFFu);
        sec.patchData(ref.instrOffset, newWord);
        sec.removePoolUseByInstr(ref.instrOffset);
      }
      //else - displacement doesnt fit, goes through literalPool
    } else { //.word
      //Linker will resolve this
      Relocation r(ref.instrOffset, sym.symbolName);
      sec.relTable.relocations.push_back(r);
    }
  }

  for (auto& [name, sym] : symTable.syms) {
    if (sym.binding == GLOBAL && sym.status == UNDEFINED)
      sym.isExtern = true;
  }
}

void Assembler::writeOutput(ostream& output) {
  symTable.write(output);
  writeString(output, "#SECTIONS#\n");
  writeString(output, to_string(sections.size()) + "\n");
  for (Section& sec : sections) {
    string secNameSize = sec.name + " " + to_string(sec.data.size()) + "\n";
    writeString(output, secNameSize);
    for (uint8_t byte : sec.data) {
      output.put(byte);
    }
    output.put('\n');

    writeString(output, "#RELOCATIONS#\n");
    sec.relTable.write(output);
  }
}