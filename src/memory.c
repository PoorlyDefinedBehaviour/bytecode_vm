#include "memory.h"
#include <stdlib.h>

void *reallocate(void *pointer, const size_t _old_size, const size_t new_size)
{
  if (new_size == 0)
  {
    free(pointer);
    return NULL;
  }

  return realloc(pointer, new_size);
}