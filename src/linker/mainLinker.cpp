#include "linker/linker.hpp"
#include "linker/readFiles.hpp"

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

    // Provera: mora biti -hex (za nivo A)
    if (!options.hexMode) {
        cerr << "Error: -hex option is required for level A" << endl;
        return 1;
    }

    if (options.inputFiles.empty()) {
        cerr << "Error: no input files specified" << endl;
        return 1;
    }

    if (options.outputFile.empty()) {
        options.outputFile = "program.hex";
    }

    Linker linker;

    // 1. Učitaj sve .o fajlove
    for (auto& file : options.inputFiles) {
        linker.objectFiles.push_back(readObjectFile(file));
    }

    // 2. Spoji sekcije
    linker.mergeSections();

    // 3. Napravi globalnu tabelu simbola
    linker.createGlobalSymbolTable();

    // 4. Dodeli adrese sekcijama
    linker.assignAddresses(options);

    // 5. Izračunaj konačne adrese za globalne simbole
    linker.resolveSymbolAddresses();

    // 6. Primeni relokacije
    linker.applyRelocations();

    // 7. Generiši hex izlaz
    ofstream out(options.outputFile);
    if (!out.is_open()) {
        throw runtime_error("Cannot open output file: " + options.outputFile);
    }
    linker.generateHexOutput(out);
    out.close();

    cout << "Linker finished successfully. Output: " << options.outputFile << endl;

  } catch (const exception& e) {
    cerr << "Linker error: " << e.what() << endl;
    return 1;
  }

  return 0;
}