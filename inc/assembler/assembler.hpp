#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP
#include "tables/RelocationTable.hpp"
#include "tables/SymbolTable.hpp"
#include "tables/Section.hpp"
#include "syntax.hpp"
#include <iostream>

enum ForwardRefType{
    PC_RELATIVE,
    ABSOLUTE_PATCH
};

struct ForwardRef{
    string symbol;          //name of a symbol to be resolved
    int instrOffset;        //offset for instrunction inside a section
    int32_t sectionInd;        //section index
    ForwardRefType type;
    ForwardRef(const string& symbol, int instrOffset, int32_t sectionInd, ForwardRefType type) :
    symbol(symbol), instrOffset(instrOffset), sectionInd(sectionInd), type(type) {}
};

struct Assembler{
    void addSymbol(const string& symbolName, uint64_t value, DefStatus status, SymbolType type, SymbolBinding binding, bool isExtern);
    void addSection(const string& section);
    void addLabel(const string& label);
    uint64_t getLocationCounter();
    void addData(uint32_t data);
    void addByte(uint8_t data);
    void assemble(const vector<Line>& lines);
    Section& getCurrentSection();  
    void backpatch();
    void writeAllLiteralPools();
    void refreshSectionSize(Section& sec);
    void writeOutput(ostream& output);
    SymbolTable symTable;
    vector<Section> sections;
    vector<ForwardRef> forwardRefs;
    int32_t currentSection = -1;
};

#endif