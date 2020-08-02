#ifndef VM_H
#define VM_H

#include "chunk.h"

typedef struct
{
  Chunk *chunk;
  uint8_t *ip;
} Vm;

typedef enum
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

Vm new_vm();
void init_vm(Vm *vm);
void free_vm(Vm *vm);
InterpretResult interpret(Chunk *chunk);

#endif