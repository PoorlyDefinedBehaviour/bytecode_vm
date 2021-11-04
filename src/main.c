#include <stdio.h>
#include "./chunk.h"
#include "./debug.h"
#include "./vm.h"
#include "./repl.h"
#include "./run_file.h"

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