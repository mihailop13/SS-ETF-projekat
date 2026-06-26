#include "assembler/directive.hpp"
#include <stdexcept>

using namespace std;

void globalHandler(Assembler& assembler, const Directive& directive){
  for(Symbol& sym : *directive.symbolList){
    string name(sym.sym);
    if(assembler.symTable.hasSymbol(name)){
      SymbolTableEntry& symTableEntry = assembler.symTable.getSymbol(name);
      if(symTableEntry.type == SECTION)
        throw runtime_error("Section: " + name + " can not be global!");
      symTableEntry.binding = GLOBAL;
    } else {
      //if symbol does not exsits it would be defined in future
      assembler.addSymbol(name, 0, UNDEFINED, NONE, GLOBAL, false);
    }
  }
}

void externHandler(Assembler& assembler, const Directive& directive){
  for(Symbol& sym : *directive.symbolList){
    string name(sym.sym);
    if(assembler.symTable.hasSymbol(name)){
      SymbolTableEntry& symTableEntry = assembler.symTable.getSymbol(name);
      if(symTableEntry.status != UNDEFINED)
        throw runtime_error("Symbol: " + name + " has been already defined as extern!");
      symTableEntry.binding = GLOBAL;
      symTableEntry.isExtern = true;
      symTableEntry.type = NONE;
      symTableEntry.value = 0;
      symTableEntry.index = 0;
    } else {
      assembler.addSymbol(name, 0, UNDEFINED, NONE, GLOBAL, true);
    }
  }
}

void sectionHandler(Assembler& assembler, const Directive& directive){
  string sectionName(directive.name);
  assembler.addSection(sectionName);
}

void wordHandler(Assembler& assembler, const Directive& directive){
  for(Symbol& s : *directive.symbolList){
    if(s.sym == nullptr){
      assembler.addData((uint32_t)s.num);
    } else {
      string name(s.sym);
      int instrOffset = assembler.getCurrentSection().data.size();
      assembler.addData((uint32_t)0);  //placeholder
      
      if (!assembler.symTable.hasSymbol(name)) {
        //forward ref - symbol will be defined later
        assembler.forwardRefs.push_back(ForwardRef(
          name, instrOffset,
          assembler.currentSection,
          ABSOLUTE_PATCH
        ));
      } else {
        SymbolTableEntry& sym = assembler.symTable.getSymbol(name);
        //relocation must be here, because linker would know exact address
        Relocation r(instrOffset, sym.symbolName);
        assembler.getCurrentSection().relTable.relocations.push_back(r);
      }
    }
  }
}

void skipHandler(Assembler& assembler, const Directive& directive){
  for(int i = 0; i < directive.skipAmount; i++){
    assembler.addByte(0x00);
  }
}

void asciiHandler(Assembler& assembler, const Directive& directive){
  string sym(directive.asciiString);
  for(char c : sym){
    assembler.addByte(c);
  }
}