#ifndef PARSER_HPP
#define PARSER_HPP
#include <vector>
#include <string>
#include <iostream>
#include "syntax.hpp"

using namespace std;

extern FILE* yyin;
extern int yylex();
extern void yylex_destroy();
extern int yyparse(std::vector<Line>& lines);

struct Parser {
  vector<Line> lines;
  Parser(const string& inputFile);
  int parse();
};

#endif