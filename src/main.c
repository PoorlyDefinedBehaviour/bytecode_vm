#include <stdio.h>
#include "./backend/chunk.h"
#include "./backend/debug.h"
#include "./backend/vm.h"

int main(int argc, const char *argv[])
{
  Vm vm = new_vm();

  if (argc == 1)
  {
    repl();
  }
  else if (argc == 2)
  {
    run_file(argv[1]);
  }

  free_vm(&vm);

  return 0;
}