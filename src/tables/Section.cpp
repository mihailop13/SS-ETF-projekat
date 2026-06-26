#include "tables/Section.hpp"
#include "assembler/assembler.hpp"
#include <iomanip>

int32_t Section::returnLiteralOffset(uint32_t value){
  for(LiteralPoolEntry l : pool){
    if(l.sym == "" && l.value == value)
      return l.offset;
  }
  return -1;
}

int32_t Section::returnSymbolOffset(string symbol) {
  for(LiteralPoolEntry& l : pool){
    if(l.sym == symbol)   
      return l.offset;
  }
  return -1;
}

void Section::addToLiteralPool(uint32_t value, string symbol, uint32_t instrOffset) {
  for(int32_t i = 0; i < (int32_t)pool.size(); i++) {
    bool match = symbol.empty() ? (pool[i].sym.empty() && pool[i].value == value)
                                : (pool[i].sym == symbol);
    if(match) {
      pool[i].useCount++;
      poolUses.push_back(LiteralPoolUse(instrOffset, i));
      return;
    }
  }
  LiteralPoolEntry entry;
  entry.sym    = symbol;
  entry.value  = value;
  entry.useCount = 1;
  entry.offset = -1;
  pool.push_back(entry);
  poolUses.push_back(LiteralPoolUse(instrOffset, (int32_t)pool.size() - 1));
}

void Section::patchData(int offset, uint32_t value){
  data[offset]   = value & 0xFF;
  data[offset+1] = (value >> 8) & 0xFF;
  data[offset+2] = (value >> 16) & 0xFF;
  data[offset+3] = (value >> 24) & 0xFF;
}

void Section::patchD12(int instrOffset, int16_t D) {
  uint32_t word = data[instrOffset]
      | (data[instrOffset+1] << 8)
      | (data[instrOffset+2] << 16)
      | (data[instrOffset+3] << 24);
  word = (word & 0xFFFFF000u) | ((uint32_t)D & 0x0FFFu);
  data[instrOffset]   = word & 0xFF;
  data[instrOffset+1] = (word >> 8) & 0xFF;
  data[instrOffset+2] = (word >> 16) & 0xFF;
  data[instrOffset+3] = (word >> 24) & 0xFF;
}

void Section::resolveLiterals(){
  for(LiteralPoolUse& use : poolUses) {
    LiteralPoolEntry& entry = pool[use.poolIndex];
    int32_t D = (int32_t)entry.offset - (int32_t)(use.instrOffset + 4);
    if(!fits12Signed(D))
      throw runtime_error("Literal pool too far from instruction (section: " + name + ")");
    patchD12(use.instrOffset, (int16_t)D);
  }
}

void Section::writeLiteralPool(Assembler& assembler) {        
  for(LiteralPoolEntry& entry : pool){
    if(entry.offset != -1)
      continue;

    entry.offset = data.size();                       //offset inside a section that need to be in relocationEntry
    //add data
    data.push_back(entry.value & 0xFF);
    data.push_back((entry.value >> 8) & 0xFF);
    data.push_back((entry.value >> 16) & 0xFF);
    data.push_back((entry.value >> 24) & 0xFF);
    assembler.refreshSectionSize(*this);

    if(!entry.sym.empty()) {
      /*if(!assembler.symTable.hasSymbol(entry.sym))
        assembler.addSymbol(entry.sym, 0, UNDEFINED, NONE, LOCAL, false);*/
      //if literal is a symbol, then add relocation entry for it
      Relocation r(entry.offset, entry.sym);
      relTable.relocations.push_back(r);
    } 
  }
}

void Section::removePoolUseByInstr(uint32_t instrOffset) {
  //function will be called when the move to the literal pool can be fit into instruction code, so the literalPoolUse for that instruction can be removed
  for (auto it = poolUses.begin(); it != poolUses.end(); ++it) {
    if (it->instrOffset == instrOffset) {
      pool[it->poolIndex].useCount--; 
      poolUses.erase(it);       //remove literalPoolUse from vector
      return;
    }
  }
}

void Section::cleanUpLiterals() {
  //cleans up the literals, which uses has been removed
  vector<int> newIndex(pool.size(), -1);      //creates a vector with pool.size, initialized with -1
  vector<LiteralPoolEntry> newPool;
  newPool.reserve(pool.size());

  for(size_t i = 0; i < pool.size(); i++){
    if(pool[i].useCount > 0){
      newIndex[i] = newPool.size();
      newPool.push_back(pool[i]);
    }
  }

  if (newPool.size() == pool.size()) return;    //there was no change

  for (auto& use : poolUses) {
    use.poolIndex = newIndex[use.poolIndex];    //every literal use must be updated with new poolIndex
  }

  pool.swap(newPool);
}