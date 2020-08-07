#include <stdio.h>
#include "./backend/chunk.h"
#include "./backend/debug.h"
#include "./backend/vm.h"
#include "./frontend/repl.h"
#include "./frontend/run_file.h"

int main(int argc, const char *argv[])
{

  Vm vm = new_vm();

  if (argc == 1)
  {
    repl(&vm);
  }
  else if (argc == 2)
  {
    run_file(&vm, argv[1]);
  }

  free_vm(&vm);

  return 0;
}