#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "obj.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(vm, type, object_type) \
  (type *)allocate_object(vm, sizeof(type), object_type)

// [size] must take into account not only the size of Obj.
// If we want allocate an ObjString for example,
// [size] should be the size of ObjString.
static Obj *allocate_object(Vm *vm, size_t size, ObjType type)
{
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;

  // Cons [object] into the list of heap allocated objects.
  object->next = vm->objects;
  vm->objects = object;

  return object;
}

static ObjString *allocate_string(Vm *vm, char *chars, int length)
{
  uint32_t hash = hash_string(chars, length);
  ObjString *interned_string = hash_table_find_string(&vm->strings, chars, length, hash);

  if (interned_string != NULL)
  {
    // Freeing the string because we already have
    // another string with the same contents in the vm.
    //
    // [length + 1] to take the \0 that is appended
    // to every string into account
    FREE_ARRAY(char, chars, length + 1);
    return interned_string;
  }

  ObjString *string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  // Interning the string
  hash_table_set(&vm->strings, string, NIL_VAL);
  return string;
}

// http://www.isthe.com/chongo/tech/comp/fnv/
static uint32_t hash_string(const char *string, int length)
{
  uint32_t hash = 2166136261u;

  for (int i = 0; i < length; i++)
  {
    hash ^= (uint8_t)string[i];
    hash *= 16777619;
  }

  return hash;
}

ObjString *copy_string(Vm *vm, const char *chars, int length)
{
  // length + 1 because we will add the null terminator
  // to the end of the string.
  char *heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocate_string(vm, heapChars, length);
}

ObjString *take_string(Vm *vm, const char *chars, int length)
{
  return allocate_string(vm, chars, length);
}