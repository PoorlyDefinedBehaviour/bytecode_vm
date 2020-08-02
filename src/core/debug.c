#include "debug.h"

static size_t simple_instruction(const char *name, size_t offset)
{
  printf("%s\n", name);
  return offset + 1;
}

void dissasamble_chunk(Chunk *chunk, const char *name)
{
  printf("== %s ==\n", name);

  for (size_t offset = 0; offset < chunk->count;)
  {
    offset = dissamble_instruction(chunk, offset);
  }
}

size_t dissamble_instruction(Chunk *chunk, size_t offset)
{
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];

  switch (instruction)
  {
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}