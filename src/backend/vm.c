#include <stdio.h>

#include "vm.h"
#include "debug.h"

Vm new_vm()
{
  Vm vm;
  init_vm(&vm);
  return vm;
}

void init_vm(Vm *vm)
{
}

void free_vm(Vm *vm)
{
}

static InterpretResult run(Vm *vm)
{
#define READ_BYTE() (*vm->ip += 1)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

  for (;;)
  {
#ifdef DEBUG_TRACE_EXECUTION
    dissasamble_instruction(vm->chunk, vm->ip - vm->chunk->code);
#endif

    uint8_t instruction;

    switch (instruction = READ_BYTE())
    {
    case OP_RETURN:
    {
      return INTERPRET_OK;
    }
    case OP_CONSTANT:
    {
      Value constant = READ_CONSTANT();
      print_value(constant);
      printf("\n");
      break;
    }
    }
  }

#undef READ_BYTE
}

InterpretResult interpret(Vm *vm, Chunk *chunk)
{
  vm->chunk = chunk;
  vm->ip = vm->chunk->code;
  return run(vm);
}