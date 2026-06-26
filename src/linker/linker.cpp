#include "linker/linker.hpp"
#include <stdexcept>
#include <iomanip>

using namespace std;

void Linker::mergeSections() {
  map<string, vector<pair<ObjectFile*, Section>>> groupedSections;
  for(auto& obj : objectFiles){
    for(auto& section : obj.sections){
      groupedSections[section.name].push_back({&obj, section});
    }
  }

  for(auto& [name, entries] : groupedSections){
    GlobalSection globalSection;
    globalSection.name = name;
    globalSection.size = 0;
    globalSection.baseAddr = 0;

    // Prvo izračunaj ukupnu veličinu
    for(auto& [objPtr, sec] : entries){
        globalSection.size += sec.size;
    }
    globalSection.data.reserve(globalSection.size);

    uint32_t currentOffset = 0;
    for(auto& [objPtr, sec] : entries){
      // Ažuriraj simbole u ovom objektu koji pripadaju ovoj sekciji
      for(auto& [symName, sym] : objPtr->symTable.syms){
          if(sym.index == sec.index){
              sym.value += currentOffset;
          }
      }
      // Kopiraj podatke sekcije (JEDNOM po sekciji)
      globalSection.data.insert(globalSection.data.end(), sec.data.begin(), sec.data.end());

      // Ažuriraj relokacije
      for(auto& reloc : sec.relTable.relocations){
        Relocation newRelocation(reloc.offset + currentOffset, reloc.symbol);
        globalSection.relTable.relocations.push_back(newRelocation);
      }

      // Povećaj currentOffset za veličinu ove sekcije (JEDNOM po sekciji)
      currentOffset += sec.data.size();
    }
    globalSections[name] = globalSection;
  }
}

void Linker::createGlobalSymbolTable(){
  for(auto& obj : objectFiles) {
    for(auto& [name, sym] : obj.symTable.syms){
      //only global and extern symbols are considered
      if(sym.binding != GLOBAL && !sym.isExtern) continue;
      
      if(globalSymbols.count(name)){
        //symbol is already in globalSymbol table, meaning that it has been in found in another file
        GlobalSymbol& existing = globalSymbols[name];
        if(sym.status == DEFINED && !existing.isDefined){
          existing.isDefined = true;
          existing.value = sym.value;
          existing.sectionName = obj.sections[sym.index].name;
        } else if (sym.status == DEFINED && existing.isDefined) { 
          //symbol is defined in multiple files
          throw runtime_error("Multiple definitions of symbol: " + name);
        }
      } else {
        //New symbol
        GlobalSymbol globalSymbol;
        globalSymbol.name = name;
        globalSymbol.isExtern = sym.isExtern;
        globalSymbol.isDefined = (sym.status == DEFINED);
        globalSymbol.value = sym.value;

        if (sym.status == DEFINED)
          globalSymbol.sectionName = obj.sections[sym.index].name;
        globalSymbols[name] = globalSymbol;  // <--- OVO DODAJ
      }
    }
  }
}

void Linker::assignAddresses(const LinkerOptions& options) {
  // 1. Prvo dodeli adrese place-ovanim sekcijama
  map<string, uint32_t> placed; // pamti sve dodeljene adrese
  for (auto& [name, addr] : options.place) {
    if (globalSections.count(name)) {
      globalSections[name].baseAddr = addr;
      placed[name] = addr;
    }
  }

  // 2. Zatim dodeli adrese ostalim sekcijama, počev od 0
  uint32_t currentAddr = 0;
  for (auto& [name, gs] : globalSections) {
    if (placed.count(name)) continue; // već dodeljena

    // Pronađi prvu slobodnu adresu (preskači zauzete)
    bool overlap = true;
    while (overlap) {
      overlap = false;
      for (auto& [pname, paddr] : placed) {
          uint32_t pend = paddr + globalSections[pname].size;
          // Proveri da li se [currentAddr, currentAddr+gs.size) preklapa sa [paddr, pend)
          if (currentAddr < pend && currentAddr + gs.size > paddr) {
              currentAddr = pend; // pomeri se iza ove sekcije
              overlap = true;
              break;
          }
      }
    }
    gs.baseAddr = currentAddr;
    placed[name] = currentAddr;
    currentAddr += gs.size;
  }
}


void Linker::resolveSymbolAddresses() {
  for(auto& [name, gs] : globalSymbols){
    if(gs.isDefined && !gs.sectionName.empty()) {
      auto it = globalSections.find(gs.sectionName);
      gs.value = it->second.baseAddr + gs.value;
    }
    else 
      throw runtime_error("Undefined global symbol: " + name);
  }
}

void Linker::applyRelocations() {
  for(auto& [name, section] : globalSections){
    for(Relocation& rel  : section.relTable.relocations){
      uint32_t value = 0;
      bool found = false;

      auto itGlobal = globalSymbols.find(rel.symbol);
      if(itGlobal != globalSymbols.end()){
        //Global symbol
        GlobalSymbol& globalSymbol = itGlobal->second;
        if(!globalSymbol.isDefined)
          throw runtime_error("Undefined global symbol: " + rel.symbol);

        value = globalSymbol.value;
        found = true;
      } else {
        //Local symbols, search through the local symTables
        for(ObjectFile& obj : objectFiles){
          if(!obj.symTable.hasSymbol(rel.symbol))
            continue;

          SymbolTableEntry& sym = obj.symTable.getSymbol(rel.symbol);
          if(sym.status == DEFINED){
            //find the name of section, where it is defined
            string localSectionName = obj.sections[sym.index].name;
            auto itSecction = globalSections.find(localSectionName);
            if(itSecction == globalSections.end())
              throw runtime_error("Local symbol: " + rel.symbol + " refrences unkonwon section!");
            value = itSecction->second.baseAddr + sym.value;      //local symbol, so it is not calculated at resolveSymbolAddresses
            found = true;
            break;  //symobl has been found
          }
        }
      }

      if (!found) {
        throw runtime_error("Undefined symbol: " + rel.symbol);
      }

      //write sym value in data:
      section.data[rel.offset]     = value & 0xFF;
      section.data[rel.offset + 1] = (value >> 8) & 0xFF;
      section.data[rel.offset + 2] = (value >> 16) & 0xFF;
      section.data[rel.offset + 3] = (value >> 24) & 0xFF;
    }

  }
}

void Linker::generateHexOutput(ofstream& out) {
  
  for(auto& [name, globalSection] : globalSections) {
    uint32_t addr = globalSection.baseAddr;
    for(size_t i = 0; i < globalSection.data.size(); i += 8){
      out << hex << setw(8) << setfill('0') << (addr + i) << ": ";
      for (size_t j = 0; j < 8 && i + j < globalSection.data.size(); ++j) {
        out << hex << setw(2) << setfill('0') << (uint32_t)globalSection.data[i + j] << " ";
      }
      out << endl;
    }
  }
}