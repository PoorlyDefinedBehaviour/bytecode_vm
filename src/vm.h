#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"
#include "hash_table.h"

#define STACK_MAX 256

typedef struct
{
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stack_top;
  // Linked list of every heap allocated object.
  Obj *objects;
  // [strings] is used for string interning.
  //
  // String interning is a process of deduplication.
  // We create a collection of interned strings.
  // Any string in that collection is guaranteed to be textually
  // different from all others. When you intern a string,
  // you look for a matching string in the collection.
  // If found, you use that original one.
  // Otherwise, the string you have is unique,
  // so you add it to the collection.
  //
  // Since we will always have the same pointer for strings
  // with the same contents, string comparison is O(1).
  HashTable strings;
  // [globals] stores global variables.
  HashTable globals;
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
InterpretResult interpret(Vm *vm, const char *source_code);
void push(Vm *vm, Value value);
Value pop(Vm *vm);

#endif