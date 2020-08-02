#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct
{
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stack_top;
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
InterpretResult interpret(Vm *vm, Chunk *chunk);
void push(Vm *vm, Value value);
Value pop(Vm *vm);

#endif