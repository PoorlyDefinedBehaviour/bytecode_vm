#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "value.h"

typedef enum
{
  OP_CONSTANT,
  OP_RETURN
} OpCode;

typedef struct
{
  size_t count;
  size_t capacity;
  uint8_t *code;
  ValueArray constants;
  size_t *lines;
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, size_t line);
void free_chunk(Chunk *chunk);
bool is_chunk_full(Chunk *chunk);
size_t add_constant(Chunk *chunk, Value value);

#endif