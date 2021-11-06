#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "obj.h"
#include "hash_table.h"
#include "value.h"

#define HASH_TABLE_MAX_LOAD 0.75

HashTable new_hash_table()
{
  HashTable table;

  init_table(&table);

  return table;
}

static void init_table(HashTable *table)
{
  table->count = 0;
  table->capacity = 0;
  table->entries = 0;
}

void free_hash_table(HashTable *table)
{
  FREE_ARRAY(Entry, table->entries, table->capacity);
  init_table(table);
}

static Entry *find_entry(Entry *entries, int capacity, ObjString *key)
{
  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;

  for (;;)
  {
    Entry *entry = &entries[index];
    if (entry->key == NULL)
    {
      if (IS_NIL(entry->value))
      {
        // If we have a tombstone, we return it
        // so [key] can be inserted in it if [find_entry]
        // is being used in an insertion
        return tombstone != NULL ? tombstone : entry;
      }
      else
      {
        // We save the first tombstone we find to a variable
        // so it can be returned and used for insertions
        if (tombstone == NULL)
        {
          tombstone = entry;
        }
      }
    }

    // TODO: [entry->key == key] is comparing two pointers,
    // this is incorrect because we may have two different strings
    // that have the same contents and they should be considered equal.
    if (entry->key == key)
    {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

static void adjust_capacity(HashTable *table, int new_capacity)
{
  Entry *entries = ALLOCATE(Entry, new_capacity);

  for (int i = 0; i < new_capacity; i++)
  {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;

  // Rehashing every key in the hash table because
  // they end in different positions since the hash table
  // capacity changed.
  for (int i = 0; i < table->capacity; i++)
  {
    Entry *entry = &table->entries[i];

    if (entry->key == NULL)
    {
      continue;
    }

    Entry *destination = find_entry(entries, new_capacity, entry->key);

    destination->key = entry->key;
    destination->value = entry->value;
    table->count++;
  }

  // Free old hash table buckets
  FREE_ARRAY(Entry, table->entries, table->capacity);

  // Hash table uses the rehashed entries and the new capacity.
  table->entries = entries;
  table->capacity = new_capacity;
}

bool hash_table_set(HashTable *table, ObjString *key, Value value)
{
  if (table->count + 1 > table->capacity * HASH_TABLE_MAX_LOAD)
  {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);

  bool is_new_key = entry->key == NULL;

  // [entry->value] will not be nil if entry is a tombstone,
  // because of that we do not increment [table-count]
  // because [table-count] should be equal to the number of
  // filled buckets + the number of tombstones in the buckets.
  if (is_new_key && IS_NIL(entry->value))
  {
    table->count++;
  }

  entry->key = key;
  entry->value = value;

  return is_new_key;
}

void hash_table_extend(HashTable *table, HashTable *with)
{
  for (int i = 0; i < with->capacity; i++)
  {
    Entry *entry = &with->entries[i];

    if (entry->key != NULL)
    {
      hash_table_set(table, entry->key, entry->value);
    }
  }
}

Value *hash_table_get(HashTable *table, ObjString *key)
{
  if (table->count == 0)
  {
    return NULL;
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);

  if (entry->key == NULL)
  {
    return NULL;
  }

  return &entry->value;
}

ObjString *table_find_string(HashTable *table, const char *chars, int length, uint32_t hash)
{
  if (table->count == 0)
  {
    return NULL;
  }

  uint32_t index = hash % table->capacity;

  for (;;)
  {
    Entry *entry = &table->entries[index];

    if (entry->key == NULL)
    {
      if (IS_NIL(entry->value))
      {
        return NULL;
      }
    }
    // If the key we are looking at equals the key we are looking for,
    // we return it.
    // [length] and [hash] comparisons are optimizations.
    else if (entry->key->length == length && entry->key->hash == hash &&
             memcmp(entry->key->chars, chars, length) == 0)
    {
      return entry->key;
    }

    index = (index + 1) % table->capacity;
  }
}

bool hash_table_delete(HashTable *table, ObjString *key)
{
  if (table->count == 0)
  {
    return false;
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);

  if (entry->key == NULL)
  {
    return false;
  }

  // Tombstone the entry
  entry->key = NULL;
  entry->value = BOOL_VAL(true);

  return true;
}