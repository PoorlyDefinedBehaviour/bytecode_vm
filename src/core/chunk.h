#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"

typedef enum
{
  OP_RETURN
} OpCode;

typedef struct
{
  size_t count;
  size_t capacity;
  uint8_t *code;
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte);
void free_chunk(Chunk *chunk);
bool is_full(Chunk *chunk);

#endif