#ifndef VALUE_H
#define VALUE_H

#include "common.h"

typedef enum
{
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  // Values that live on the heap have
  // a ValueType of VAL_OBJ.
  VAL_OBJ,
} ValueType;

typedef struct Obj Obj;
typedef struct ObjString ObjString;

// Small, fixed-size types will
// be stored directly inside the Value struct itself.
//
// Examples of small or fixed-size values:
//
// booleans
// numbers
//
// If the object is large or it's size is unknown at
// compile time, its data will live on the heap,
// and the Value struct will contain a pointer to it.
//
// Example of large or of unknown size values:
//
// String
// Recursive algebraic data structures
//
// ┌─────────────────────────────┐                   ┌────────────────────────────┐
// │    Stack                    │                   │    Heap                    │
// │                             │                   │                            │
// │ Value {                     │                   │                            │
// │  type: VAL_BOOL             │                   │                            │
// │  as: true                   │                   │                            │
// │ }                           │                   │                            │
// │                             │                   │   hello world\0 ◄──────────┼─────┐
// │ Value {                     │                   │                            │     │
// │  type: VAL_NUMBER           │                   │                            │     │
// │  as: 10                     │                   │                            │     │
// │ }                           │                   │                            │     │
// │                             │                   │                            │     │
// │ Value {                     │                   │     ObjString {            │     │
// │  type: VAL_OBJ              │                   │      obj: Obj{             │     │
// │  as: Obj*───────────────────┼───────────────────┼───►    type: OBJ_STRING    │     │
// │ }                           │                   │      }                     │     │
// │                             │                   │      length: 11            │     │
// │                             │                   │      chars: ───────────────┼─────┘
// │                             │                   │     }                      │
// │                             │                   │                            │
// │                             │                   │                            │
// └─────────────────────────────┘                   └────────────────────────────┘
typedef struct
{
  ValueType type;
  union
  {
    bool boolean;
    double number;
    // Values that live on the heap are represented by Obj.
    Obj *obj;
  } as;
} Value;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(value) ((Value){VAL_OBJ, {.obj = value}})
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
bool values_equal(const Value a, const Value b);

#endif