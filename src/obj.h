#ifndef OBJ_H
#define OBJ_H

#include "common.h"
#include "value.h"

typedef enum
{
  OBJ_STRING,
} ObjType;

struct Obj
{
  ObjType type;
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
  // TODO: should length be unsigned int?
  int length;
  char *chars;
};

ObjString *copy_string(const char *chars, int length);

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

static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define AS_OBJSTRING(value) ((ObjString *)AS_OBJ(value))

#endif