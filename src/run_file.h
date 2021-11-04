#include <stdio.h>
#include <stdlib.h>

#include "./vm.h"

static char *read_file(const char *path)
{
  FILE *file = fopen(path, "rb");

  if (file == NULL)
  {
    fprintf(stderr, "File %s not found\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(file_size + 1);
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  buffer[bytes_read] = '\0';

  fclose(file);

  return buffer;
}

static void run_file(Vm *vm, const char *path)
{
  char *source_code = read_file(path);

  InterpretResult result = interpret(vm, source_code);

  free(source_code);

  if (result == INTERPRET_COMPILE_ERROR)
  {
    exit(65);
  }
  if (result == INTERPRET_RUNTIME_ERROR)
  {
    exit(70);
  }
}