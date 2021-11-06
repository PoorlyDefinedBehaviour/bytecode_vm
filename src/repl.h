#include <string.h>
#include "./vm.h"

static void repl(Vm *vm)
{
  char line[1024];

  for (;;)
  {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin))
    {
      printf("\n");
      break;
    }

    int line_length = strlen(line);

    if (line[line_length - 1] == '\n')
    {
      line[line_length - 1] = '\0';
    }

    interpret(vm, line);
  }
}