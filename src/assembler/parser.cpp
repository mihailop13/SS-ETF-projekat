#include "assembler/parser.hpp"

Parser::Parser(const string& input){
  yyin = fopen(input.c_str(), "r");
  if(yyin == NULL){
    cout << "Error while opening input file!" << endl;
    yyin = nullptr;
  }
}

int Parser::parse(){
  if (yyin == NULL) {
    cout << "yyin is null, parsing failed" << endl;
    return -1;
  }

  int ret = yyparse(lines);
  yylex_destroy();

  if (yyin) {
    fclose(yyin);
    yyin = nullptr;
  }

  return ret;
}