#include <stdio.h>

#include "memory.h"
#include "value.h"

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

void print_value(Value value)
{
  printf("%g'", value);
}