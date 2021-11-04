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

    interpret(vm, line);
  }
}