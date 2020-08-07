#ifndef COMPILER_H
#define COMPILER_H

#include "vm.h"
#include "scanner.h"

typedef struct
{
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
  Scanner scanner;
} Parser;

bool compile(const char *source_code, Chunk *chunk);
Parser new_parser(const char *source_code);

#endif