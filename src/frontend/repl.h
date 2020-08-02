#include "../backend/vm.h"

static void repl()
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

    interpret(line);
  }
}