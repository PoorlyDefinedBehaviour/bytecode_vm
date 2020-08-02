#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *chunk)
{
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
}

bool is_full(Chunk *chunk)
{
  return chunk->capacity < chunk->count + 1;
}

void write_chunk(Chunk *chunk, uint8_t byte)
{
  if (is_full(chunk))
  {
    size_t old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->count += 1;
}

void free_chunk(Chunk *chunk)
{
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  init_chunk(chunk);
}
