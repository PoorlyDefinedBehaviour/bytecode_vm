#include <stdio.h>
#include <stdarg.h>

#include "vm.h"
#include "compiler.h"
#include "obj.h"
#include "memory.h"
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
  vm->objects = NULL;
  vm->strings = new_hash_table();
}

void free_vm(Vm *vm)
{
  free_hash_table(&vm->strings);
  free_objects(vm);
}

void free_object(Obj *obj)
{
  switch (obj->type)
  {
  case OBJ_STRING:
  {
    ObjString *string = (ObjString *)obj;
    // [string->length + 1] because every string has
    // \0 appended to it even though we don't take \0
    // into account when setting the string length because
    // it is an implementation detail that we do not want
    // to leak to the user.
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjString, obj);
    break;
  }
  }
}

void free_objects(Vm *vm)
{
  Obj *current = vm->objects;

  while (current != NULL)
  {
    Obj *next = current->next;
    free_object(current);
    current = next;
  }
}

static Value peek(Vm *vm, size_t distance)
{
  return vm->stack_top[-1 - distance];
}

static void runtime_error(Vm *vm, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm->ip - vm->chunk->code - 1;
  size_t line = vm->chunk->lines[instruction];
  fprintf(stderr, "[line %zu] in script\n", line);

  reset_stack(vm);
}

static bool not(const Value value)
{
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate_strings(Vm *vm)
{
  ObjString *b = AS_OBJSTRING(pop(vm));
  ObjString *a = AS_OBJSTRING(pop(vm));

  int length = a->length + b->length;

  // [length + 1] because we will add \0 to the end of the string.
  char *chars = ALLOCATE(char, length + 1);

  // Copies every [a] character to [chars]
  // starting from the beginning of [chars]
  memcpy(chars, a->chars, a->length);
  // Copies every [b] character to [chars]
  // starting from the position after the last
  // [a] character that was just copied.
  memcpy(chars + a->length, b->chars, b->length);

  // Adding \0 to the end of the string because
  // C functions expect strings to terminate in \0.
  chars[length] = '\0';

  ObjString *result = take_string(vm, chars, length);

  push(vm, OBJ_VAL(result));
}

static InterpretResult run(Vm *vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op)                           \
  do                                                        \
  {                                                         \
    if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) \
    {                                                       \
      runtime_error(vm, "Operands must be numbers");        \
      return INTERPRET_RUNTIME_ERROR;                       \
    }                                                       \
    double b = AS_NUMBER(pop(vm));                          \
    double a = AS_NUMBER(pop(vm));                          \
    push(vm, value_type(a op b));                           \
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
    case OP_NIL:
    {
      push(vm, NIL_VAL);
      break;
    }
    case OP_TRUE:
    {
      push(vm, BOOL_VAL(true));
      break;
    }
    case OP_FALSE:
    {
      push(vm, BOOL_VAL(false));
      break;
    }
    case OP_EQUAL:
    {
      const Value b = pop(vm);
      const Value a = pop(vm);
      push(vm, BOOL_VAL(values_equal(a, b)));
      break;
    }
    case OP_GREATER:
    {
      BINARY_OP(BOOL_VAL, >);
      break;
    }
    case OP_LESS:
    {
      BINARY_OP(BOOL_VAL, <);
      break;
    }
    case OP_NEGATE:
    {
      if (!IS_NUMBER(peek(vm, 0)))
      {
        runtime_error(vm, "Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
      break;
    }
    case OP_ADD:
    {
      Value a = peek(vm, 0);
      Value b = peek(vm, 0);

      if (IS_STRING(a) && IS_STRING(b))
      {
        concatenate_strings(vm);
      }
      else if (IS_NUMBER(a) && IS_NUMBER(b))
        BINARY_OP(NUMBER_VAL, +);
      else
      {
        runtime_error(vm, "unexpected operands in with + operator");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT:
    {
      BINARY_OP(NUMBER_VAL, -);
      break;
    }
    case OP_MULTIPLY:
    {
      BINARY_OP(NUMBER_VAL, *);
      break;
    }
    case OP_DIVIDE:
    {
      BINARY_OP(NUMBER_VAL, /);
      break;
    }
    case OP_NOT:
    {
      push(vm, BOOL_VAL(not(pop(vm))));
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
  Chunk chunk = new_chunk();

  if (!compile(vm, source_code, &chunk))
  {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm->chunk = &chunk;
  vm->ip = vm->chunk->code;

  InterpretResult result = run(vm);

  free_chunk(&chunk);

  return result;
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