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
  Vm *vm;
} Parser;

ObjFunction *compile(Vm *vm, const char *source_code);
Parser new_parser(Vm *vm, const char *source_code);

#endif