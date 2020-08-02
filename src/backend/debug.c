#include "debug.h"

static size_t simple_instruction(const char *name, size_t offset)
{
  printf("%s\n", name);
  return offset + 1;
}

static size_t constant_instruction(const char *name, Chunk *chunk, size_t offset)
{
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, 0);
  print_value(chunk->constants.values[constant]);
  printf("\n");
  return offset + 2;
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

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
  {
    printf("   | ");
  }
  else
  {
    printf("%d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];

  switch (instruction)
  {
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_NEGATE:
    return simple_instruction("OP_NEGATE", offset);
  case OP_ADD:
    return simple_instruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simple_instruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simple_instruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simple_instruction("OP_DIVIDE", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}