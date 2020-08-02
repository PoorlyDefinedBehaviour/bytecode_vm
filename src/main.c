#include <stdio.h>
#include "./core/chunk.h"
#include "./core/debug.h"

int main(int argc, const char *argv[])
{
  Chunk chunk;
  init_chunk(&chunk);

  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, add_constant(&chunk, 1.2), 123);

  write_chunk(&chunk, OP_RETURN, 123);

  dissasamble_chunk(&chunk, "test chunk");

  free_chunk(&chunk);

  return 0;
}