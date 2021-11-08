#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "common.h"
#include "value.h"

typedef struct
{
  ObjString *key;
  Value value;
} Entry;

typedef struct
{
  // The ratio of [count] to [capacity] is called
  // the load factor of the hash table.
  int count;
  int capacity;
  Entry *entries;
} HashTable;

HashTable new_hash_table();

void free_hash_table(HashTable *table);

// Associates [key] to [value] in the [table]
bool hash_table_set(HashTable *table, ObjString *key, Value value);

// Inserts elements from [with] into [table]
void hash_table_extend(HashTable *table, HashTable *with);

// Returns the value associated to [key] in the [table].
// If [key] is not present in [table], returns NULL.
Value *hash_table_get(HashTable *table, ObjString *key);

ObjString *hash_table_find_string(HashTable *table, const char *chars, int length, uint32_t hash);

// Removes [key] from [table].
// Returns true if [key] was found in [table]
// and false otherwise.
bool hash_table_delete(HashTable *table, ObjString *key);

#endif