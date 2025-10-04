# Compilers_assignment
Assignment for the Compilers lesson of the 8th Semester of ece ntua. The code contains the compiler for the programming language Dana.

Εμανουήλ Ρεΐζης (03121067) 

Δημήτριος Δημητρακόπουλος (03121066)

# System Requirements 

## Build-Time Requirements
| Tool / Library | Minimum Version | Notes |
|----------------|----------------|-------|
| `g++` | 7+ | Must support C++17 (`-std=c++17`) |
| LLVM | 14 | Provides `llvm-config` for compile & link flags |
| Flex | Latest | Generates `lexer.cpp` from `lexer.l` |
| Bison | Latest | Generates `parser.cpp` / `parser.hpp` |
| Make | Standard | Required to run the Makefile |
| Ar (archiver) | Standard | Builds static library `lib.a` |
---
## Runtime Requirements
| Requirement | Minimum | Notes |
|-------------|---------|-------|
| OS | Linux x86_64 | Tested on Fedora and Ubuntu; should work on most modern distros |
| LLVM libraries | 14 | Must match linked `libLLVM-14.so` |
| C++ runtime libraries | Standard (`libstdc++`, `libc`, `libm`) | Usually included with Linux distributions |
| Supporting libraries | `libffi`, `libedit`, `libz`, `libtinfo` | Required by LLVM |
---
# Files in the project
## `Makefile`
The build script for the project.  
- Compiles all source files and generates the `danac` binary.  
- Handles building the lexer (`lexer.cpp`) from `lexer.l` using Flex and the parser (`parser.cpp` / `parser.hpp`) from `parser.y` using Bison.  
- Builds the static library `lib.a` from `lib.cpp`.  
- Provides `clean` and `distclean` targets to remove build artifacts.

---

## `symbol.hpp`
Header file defining the **symbol table structures and functions**.  
- Manages variable and function symbols during parsing and code generation.  
- Ensures proper scope handling and type checking.

---

## `lexer.l`
Flex source file for the **lexer (lexical analyzer)**.  
- Defines tokens and regular expressions for the Dana language.  
- Converts source code into a stream of tokens to be used by the parser.

---

## `parser.y`
Bison source file for the **parser (syntax analyzer)**.  
- Defines grammar rules of the Dana language.  
- Converts the token stream from the lexer into an Abstract Syntax Tree (AST).  
- Generates `parser.cpp` and `parser.hpp`.

---

## `danac.sh`
Command-line interface script for the compiler.  
- Provides an easy way to run the `danac` compiler from the terminal.  
- Accepts source files and optional arguments, handles compilation workflow, and outputs results.

---

## `ast.hpp`
Header file defining the **Abstract Syntax Tree (AST) structures**.  
- Represents the hierarchical structure of the parsed program.  
- Used during semantic analysis and code generation.

---

## `lexer.hpp`
Header file for the **lexer**.  
- Declares functions and structures generated from `lexer.l`.  
- Provides interfaces for the parser to access tokens.

---

## `lib.cpp`
C++ source file implementing **helper functions and runtime support**.  
- Contains utility functions used by the compiler and generated code.  
- Compiled into a static library `lib.a` that is linked with the main binary.
# Running the CLI

The `danac.sh` script is the **command-line interface** for the Dana compiler.  
It allows you to compile Dana source files, generate intermediate representation (IR), and produce final executables.

---

## Usage

./danac.sh [options] <source_file>

(May need to use chmod +x danac.sh to make the script executable)

## Arguments

|Flag|Description|
|----|-----------|
|-O|Enable optimization during compilation.|
|-o|<output_file>	Specify the output executable file name (default: a.out).|
|-f|Read the program from stdin and generate final assembly code to stdout.|
|-i|Read the program from stdin and generate intermediate representation (IR) to stdout.|
---

# Problems with our implementation
- Functions can pass variables one level down and no more
- Same name - different argument functions have not been implemented
- Error line (lineno) is not always accurate, it might show the liune of the next instruction instead of the intended one
# Working examples (27 / 31)
- programs-kostis/IntXor.dana
- programs-kostis/binarysearch.dana
- programs-kostis/bsort.dana
- programs-kostis/dot_product.dana
- programs-kostis/evenChecker.dana
- programs-kostis/factorial.dana
- programs-kostis/factors.dana
- programs-kostis/fibonacci.dana
- programs-kostis/gcd.dana
- programs-kostis/hanoi.dana
- programs-kostis/hello.dana
- programs-kostis/knapsack.dana
- programs-kostis/linemarket.dana
- programs-kostis/lis.dana
- programs-kostis/matrix_mul.dana
- programs-kostis/mergesort.dana
- programs-kostis/nextRand.dana
- programs-kostis/palindrome.dana
- programs-kostis/powint.dana
- programs-kostis/primeFactors.dana
- programs-kostis/quicksort.dana
- programs-kostis/reverseNumber.dana
- programs-kostis/rotatefun.dana
- programs-kostis/strrev.dana
- programs-kostis/sudoku.dana
- programs-kostis/sumOfDigits.dana
- programs-kostis/N_Queens.dana
- programs-kostis/calculator.dana
# Not working examples (4 / 31)
- programs-kostis/tsp.dana ---> Instruction does not dominate all uses!
- programs-kostis/perceptron.dana ---> Instruction does not dominate all uses!
- programs-kostis/def_scopes.dana ---> Instruction does not dominate all uses!

