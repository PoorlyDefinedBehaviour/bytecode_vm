#include "debug.h"

static size_t simple_instruction(const char *name, size_t offset)
{
  printf("%s\n", name);
  return offset + 1;
}

static size_t constant_instruction(const char *name, Chunk *chunk, size_t offset)
{
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d ", name, 0);
  print_value(chunk->constants.values[constant]);
  printf("\n");
  return offset + 2;
}

static size_t byte_instruction(const char *name, Chunk *chunk, size_t offset)
{
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s $d\n", name, slot);
  return offset + 2;
}

static jump_instruction(const char *name, int sign, Chunk *chunk, int offset)
{
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d\n", name, offset, offset + 3, sign * jump);
  return offset + 3;
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
  printf("%04zu ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
  {
    printf("   | ");
  }
  else
  {
    printf("%zu ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];

  switch (instruction)
  {
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_NIL:
    return simple_instruction("OP_NIL", offset);
  case OP_TRUE:
    return simple_instruction("OP_TRUE", offset);
  case OP_FALSE:
    return simple_instruction("OP_FALSE", offset);
  case OP_EQUAL:
    return simple_instruction("OP_EQUAL", offset);
  case OP_GREATER:
    return simple_instruction("OP_GREATER", offset);
  case OP_LESS:
    return simple_instruction("OP_LESS", offset);
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
  case OP_NOT:
    return simple_instruction("OP_NOT", offset);
  case OP_PRINT:
    return simple_instruction("OP_PRINT", offset);
  case OP_POP:
    return simple_instruction("OP_POP", offset);
  case OP_DEFINE_GLOBAL:
    return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
  case OP_GET_GLOBAL:
    return constant_instruction("OP_GET_GLOBAL", chunk, offset);
  case OP_SET_GLOBAL:
    return constant_instruction("OP_SET_GLOBAL", chunk, offset);
  case OP_GET_LOCAL:
    return byte_instruction("OP_GET_LOCAL", chunk, offset);
  case OP_SET_LOCAL:
    return byte_instruction("OP_SET_LOCAL", chunk, offset);
  case OP_JUMP:
    return jump_instruction("OP_JUMP", 1, chunk, offset);
  case OP_JUMP_IF_FALSE:
    return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}