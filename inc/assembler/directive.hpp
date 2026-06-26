#ifndef DIRECTIVE_HPP
#define DIRECTIVE_HPP

#include "assembler/assembler.hpp"
#include "syntax.hpp"

void globalHandler(Assembler& assembler, const Directive& directive);
void externHandler(Assembler& assembler, const Directive& directive);
void sectionHandler(Assembler& assembler, const Directive& directive);
void wordHandler(Assembler& assembler, const Directive& directive);
void skipHandler(Assembler& assembler, const Directive& directive);
void asciiHandler(Assembler& assembler, const Directive& directive);
//void endHandler(Assembler& assembler);

#endif