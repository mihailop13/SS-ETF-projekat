#include "linker/linker.hpp"
#include "linker/readFiles.hpp"
#include <iostream>

using namespace std;

LinkerOptions parseOptions(int argc, char* argv[]) {
  LinkerOptions opts;
  opts.hexMode = false;
  
  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "-o") {
      opts.outputFile = argv[++i];
    } 
    else if (arg == "-hex") {
      opts.hexMode = true;
    } 
    else if (arg.rfind("-place=", 0) == 0) {
      string param = arg.substr(7);
      size_t at = param.find('@');
      if (at != string::npos) {
        string sec = param.substr(0, at);
        string addr = param.substr(at + 1);
        opts.place[sec] = stoul(addr, nullptr, 0);
      }
    } else {
      opts.inputFiles.push_back(arg);
    }
  }
  return opts;
}

int main(int argc, char* argv[]){

  try {
    LinkerOptions options = parseOptions(argc, argv);

    if (!options.hexMode) {
      cout << "Error: -hex option is required for level A" << endl;
      return 1;
    }

    if (options.inputFiles.empty()) {
      cout << "Error: no input files specified" << endl;
      return 1;
    }

    Linker linker;

    //Load all .o files
    for (auto& file : options.inputFiles)
      linker.objectFiles.push_back(readObjectFile(file));
    
    linker.mergeSections();                 //Merge sections
    linker.createGlobalSymbolTable();       //make global symTable
    linker.assignAddresses(options);        //assign addresses to the sections
    linker.resolveSymbolAddresses();        //calculate final absolute addresses of all defined global symbols
    linker.applyRelocations();              //apply relocations to all sections

    // 7. Generiši hex izlaz
    ofstream out(options.outputFile);
    if (!out.is_open()) {
      throw runtime_error("Cannot open output file: " + options.outputFile);
    }
    linker.generateHexOutput(out);          //generate hex output file
    out.close();                      
  } catch (const exception& e) {
    cout << "Linker error: " << e.what() << endl;
    return 1;
  }

  return 0;
}