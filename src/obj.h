#ifndef OBJ_H
#define OBJ_H

#include "vm.h"
#include "common.h"
#include "value.h"
#include "chunk.h"

typedef enum
{
  OBJ_FUNCTION,
  OBJ_STRING,
} ObjType;

struct Obj
{
  ObjType type;
  // We don't have a garbage collector yet,
  // so we will use a linked list to be able
  // to reach every object thats allocated in the heap
  // whether or not the user's program or the VM's stack
  // still has a reference to it.
  Obj *next;
};

// C specifies that struct fields are
// arranged in memory in the order that
// they are declared.
//
// Since ObjString contains and Obj,
// we can cast a ObjString* to an Obj*
// and an Obj* to a ObjString* if we know it
// is an ObjString* because of how the struct
// will be laid out in memory.
struct ObjString
{
  Obj obj;
  int length;
  char *chars;
  // [hash] is pre computed to make indexing hash tables faster.
  uint32_t hash;
};

ObjString *copy_string(Vm *vm, const char *chars, int length);

ObjString *take_string(Vm *vm, const char *chars, int length);

typedef struct
{
  Obj obj;
  // [arity] is the number of parameters the function expects.
  int arity;
  // [chunk] contains the function body instructions.
  Chunk chunk;
  ObjString *name;
} ObjFunction;

ObjFunction *new_function(Vm *vm);

#define OBJ_TYPE(value) ((AS_OBJ(value))->type)

// NOTE: Why did we create isObjType instead of
// just adding the logic in the macro?
//
// Our logic to check if a value if is an Obj
// of a specific type uses the value twice,
// in C, if a macro uses a value more than once,
// the value will be evaluated more than once,
// which is undesirable because the value passed
// to the macro may contain side effects.
//
// Example:
//
// Note that POP pops a value from the stack
//
// IS_STRING(POP())
//
// If we had defined the logic inside the macro itself,
// POP() would be evaluated more than once.
#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_OBJSTRING(value) ((ObjString *)AS_OBJ(value))

#endif