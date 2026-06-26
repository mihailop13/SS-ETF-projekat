# ------------------------------
# Makefile za asembler, linker i emulator (odvojeni objekti)
# ------------------------------

CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -Iinc -Imisc/build
LDFLAGS   =

SRC_DIR   = src
BUILD_DIR = build
MISC_DIR  = misc
BIN_DIR   = tests/nivo-a

# Generisani fajlovi
PARSER_C  = $(MISC_DIR)/build/parser.tab.c
PARSER_H  = $(MISC_DIR)/build/parser.tab.h
LEX_C     = $(MISC_DIR)/build/lex.yy.c

# ------------------------------------------------------------
# Eksplicitne liste objekata
ASSEMBLER_OBJS = \
	$(BUILD_DIR)/assembler/main.o \
	$(BUILD_DIR)/assembler/parser.o \
	$(BUILD_DIR)/assembler/assembler.o \
	$(BUILD_DIR)/assembler/directive.o \
	$(BUILD_DIR)/assembler/instruction.o \
	$(BUILD_DIR)/tables/RelocationTable.o \
	$(BUILD_DIR)/tables/SymbolTable.o \
	$(BUILD_DIR)/tables/Section.o \
	$(BUILD_DIR)/parser.tab.o \
	$(BUILD_DIR)/lex.yy.o

LINKER_OBJS = \
	$(BUILD_DIR)/linker/mainLinker.o \
	$(BUILD_DIR)/linker/linker.o \
	$(BUILD_DIR)/linker/readFiles.o \
	$(BUILD_DIR)/tables/RelocationTable.o \
	$(BUILD_DIR)/tables/SymbolTable.o

EMULATOR_OBJS = \
	$(BUILD_DIR)/emulator/main.o \
	$(BUILD_DIR)/emulator/emulator.o

# ------------------------------------------------------------
# Ciljevi
.PHONY: all clean

all: assembler linker emulator

assembler: $(ASSEMBLER_OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $^ $(LDFLAGS) -o $(BIN_DIR)/$@

linker: $(LINKER_OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $^ $(LDFLAGS) -o $(BIN_DIR)/$@

emulator: $(EMULATOR_OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $^ $(LDFLAGS) -o $(BIN_DIR)/$@

# ------------------------------------------------------------
# Pravila za generisanje parsera i leksera
$(PARSER_C) $(PARSER_H): $(MISC_DIR)/parser.y
	mkdir -p $(MISC_DIR)/build
	bison -d $< -o $(PARSER_C)

$(LEX_C): $(MISC_DIR)/lex.l
	mkdir -p $(MISC_DIR)/build
	flex -o $@ $<

# ------------------------------------------------------------
# Eksplicitna pravila za svaki objekat
$(BUILD_DIR)/assembler/main.o: $(SRC_DIR)/assembler/main.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/assembler/parser.o: $(SRC_DIR)/assembler/parser.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/assembler/assembler.o: $(SRC_DIR)/assembler/assembler.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/assembler/directive.o: $(SRC_DIR)/assembler/directive.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/assembler/instruction.o: $(SRC_DIR)/assembler/instruction.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tables/RelocationTable.o: $(SRC_DIR)/tables/RelocationTable.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tables/SymbolTable.o: $(SRC_DIR)/tables/SymbolTable.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tables/Section.o: $(SRC_DIR)/tables/Section.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/linker/mainLinker.o: $(SRC_DIR)/linker/mainLinker.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/linker/linker.o: $(SRC_DIR)/linker/linker.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/linker/readFiles.o: $(SRC_DIR)/linker/readFiles.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/emulator/main.o: $(SRC_DIR)/emulator/main.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/emulator/emulator.o: $(SRC_DIR)/emulator/emulator.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.tab.o: $(PARSER_C)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/lex.yy.o: $(LEX_C)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Čišćenje
clean:
	rm -rf $(BUILD_DIR) $(MISC_DIR)/build $(BIN_DIR)/assembler $(BIN_DIR)/linker $(BIN_DIR)/emulator