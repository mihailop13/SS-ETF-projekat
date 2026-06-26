#include "tables/RelocationTable.hpp"

using namespace std;

inline void writeString(ostream& output, const string& s){
  int size = s.size();
  output.write((char*)&size, sizeof(size));
  output.write(s.c_str(), size);
}

void RelocationTable::write(ostream& output) {
  writeString(output, to_string(relocations.size()) + "\n");
  for (auto& r : relocations) {
    string relocation = to_string(r.offset) + " " + r.symbol + "\n";
    writeString(output, relocation);
  }
}