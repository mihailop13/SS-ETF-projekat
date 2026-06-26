#ifndef RELTABLE_HPP
#define RELTABLE_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

struct Relocation {
  uint32_t offset;        //offset from the start of the section
  string symbol;          //which symbol has to be relocated
  //All relocations are ABS32
  Relocation(uint32_t offset, string symbol) : 
    offset(offset), symbol(symbol){}
};

struct RelocationTable{
    void write(ostream& output);
    vector<Relocation> relocations;
};
#endif