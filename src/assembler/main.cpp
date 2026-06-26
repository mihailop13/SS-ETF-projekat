#include <cstdio>
#include <iostream>
#include "assembler/parser.hpp"
#include "assembler/assembler.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc < 4) { 
        std::cout << "Less arguments than expected!";
        return -1;
    }

    Parser parser(argv[3]);
    if (parser.parse() != 0) {
        cout << "Parsing failed!";
        return -1;
    }

    Assembler assembler;
    try {
        assembler.assemble(parser.lines);                   //1. first pass, creates and fills symbol and rel tables, literal pools and forwardRefs
        assembler.backpatch();                              //2. resolves forwardRefs
        for (auto& sec : assembler.sections) {              //3. clean "dead" literals
            sec.cleanUpLiterals();
        }
        assembler.writeAllLiteralPools();                   //4. adds literal pool at the end of section (only live literals)
        
        string outName = argv[2];
        std::ofstream file(outName, std::ios::binary);
        std::ostream& output = file;
        assembler.writeOutput(output);                      //5. write to .o file
    } catch (const exception& e) {
        cout << "Assembler error: " << e.what();
        return -1;
    }

    return 0;
}