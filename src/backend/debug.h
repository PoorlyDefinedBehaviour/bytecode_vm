#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "chunk.h"

void dissasamble_chunk(Chunk *chunk, const char *name);
size_t dissamble_instruction(Chunk *chunk, size_t offset);

#endif