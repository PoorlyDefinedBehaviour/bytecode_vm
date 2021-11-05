#include <stdio.h>

#include "memory.h"
#include "value.h"
#include "obj.h"

void init_value_array(ValueArray *array)
{
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

bool is_value_array_full(ValueArray *array)
{
  return array->capacity < array->count + 1;
}

void write_value_array(ValueArray *array, Value value)
{
  if (is_value_array_full(array))
  {
    int old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count += 1;
}

void free_value_array(ValueArray *array)
{
  FREE_ARRAY(Value, array->values, array->capacity);
  init_value_array(array);
}

void print_object(Value obj)
{
  switch (OBJ_TYPE(obj))
  {
  case OBJ_STRING:
    printf("%s", AS_OBJSTRING(obj)->chars);
    break;
  }
}

void print_value(const Value value)
{
  switch (value.type)
  {
  case VAL_BOOL:
    printf(AS_BOOL(value) ? "true" : "false");
    break;
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUMBER:
    printf("%g", AS_NUMBER(value));
    break;
  case VAL_OBJ:
    print_object(value);
    break;
  default:
    break;
  }
}

bool values_equal(const Value a, const Value b)
{
  if (a.type != b.type)
  {
    return false;
  }

  switch (a.type)
  {
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  case VAL_OBJ:
  {
    if (OBJ_TYPE(a) != OBJ_TYPE(b))
    {
      return false;
    }

    switch
      OBJ_TYPE(a)
      {
      case OBJ_STRING:
        if (AS_OBJSTRING(a)
                ->length != AS_OBJSTRING(b)->length)
        {
          return false;
        }

        memcmp(
            AS_OBJSTRING(a)->chars,
            AS_OBJSTRING(b)->chars,
            AS_OBJSTRING(a)->length) == 0;

        break;
      }
  }
  }
}