#include <stdio.h>

#include "vm.h"
#include "compiler.h"
#include "debug.h"

Vm new_vm()
{
  Vm vm;
  init_vm(&vm);
  return vm;
}

static void reset_stack(Vm *vm)
{
  vm->stack_top = vm->stack;
}

void init_vm(Vm *vm)
{
  reset_stack(vm);
}

void free_vm(Vm *vm)
{
}

static InterpretResult run(Vm *vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)   \
  do                    \
  {                     \
    double b = pop(vm); \
    double a = pop(vm); \
    push(vm, a op b);   \
  } while (false)

  for (;;)
  {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");

    for (Value *slot = vm->stack; slot < vm->stack_top; slot += 1)
    {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }

    dissamble_instruction(vm->chunk, vm->ip - vm->chunk->code);
#endif

    uint8_t instruction;

    switch (instruction = READ_BYTE())
    {
    case OP_RETURN:
    {
      print_value(pop(vm));
      printf("\n");
      return INTERPRET_OK;
    }
    case OP_CONSTANT:
    {
      Value constant = READ_CONSTANT();
      push(vm, constant);
      break;
    }
    case OP_NEGATE:
    {
      push(vm, -pop(vm));
      break;
    }
    case OP_ADD:
    {
      BINARY_OP(+);
      break;
    }
    case OP_SUBTRACT:
    {
      BINARY_OP(-);
      break;
    }
    case OP_MULTIPLY:
    {
      BINARY_OP(*);
      break;
    }
    case OP_DIVIDE:
    {
      BINARY_OP(/);
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(Vm *vm, const char *source_code)
{
  compile(vm, source_code);
  return INTERPRET_OK;
}

void push(Vm *vm, Value value)
{
  *vm->stack_top = value;
  vm->stack_top += 1;
}

Value pop(Vm *vm)
{
  vm->stack_top -= 1;
  return *vm->stack_top;
}