#ifndef VALUE_H
#define VALUE_H

#include "common.h"

typedef double Value;

typedef struct
{
  size_t capacity;
  size_t count;
  Value *values;

} ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);
bool is_value_array_full(ValueArray *array);
void print_value(Value value);

#endif