#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

Chunk new_chunk()
{
  Chunk chunk;
  init_chunk(&chunk);
  return chunk;
}

void init_chunk(Chunk *chunk)
{
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;

  init_value_array(&chunk->constants);
}

bool is_chunk_full(Chunk *chunk)
{
  return chunk->capacity < chunk->count + 1;
}

void write_chunk(Chunk *chunk, uint8_t byte, size_t line)
{
  if (is_chunk_full(chunk))
  {
    size_t old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(size_t, chunk->lines, old_capacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count += 1;
}

void free_chunk(Chunk *chunk)
{
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(size_t, chunk->lines, chunk->capacity);
  init_chunk(chunk);
  free_value_array(&chunk->constants);
}

size_t add_constant(Chunk *chunk, Value value)
{
  write_value_array(&chunk->constants, value);
  return chunk->constants.count - 1;
}