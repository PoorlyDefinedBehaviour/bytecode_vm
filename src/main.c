#include <stdio.h>
#include "./backend/chunk.h"
#include "./backend/debug.h"
#include "./backend/vm.h"

int main(int argc, const char *argv[])
{
  Vm vm = new_vm();

  Chunk chunk = new_chunk();

  write_chunk(&chunk, OP_CONSTANT, 123);

  write_chunk(&chunk, add_constant(&chunk, 2), 123);

  write_chunk(&chunk, OP_CONSTANT, 123);

  write_chunk(&chunk, add_constant(&chunk, 2), 123);

  write_chunk(&chunk, OP_ADD, 123);

  write_chunk(&chunk, OP_CONSTANT, 123);

  write_chunk(&chunk, add_constant(&chunk, 2), 123);

  write_chunk(&chunk, OP_DIVIDE, 123);

  write_chunk(&chunk, OP_NEGATE, 123);

  write_chunk(&chunk, OP_RETURN, 123);

  interpret(&vm, &chunk);

  free_chunk(&chunk);

  free_vm(&vm);

  return 0;
}