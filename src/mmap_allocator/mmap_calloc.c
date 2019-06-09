#include <string.h>
#include "mmap_allocator.h"

void *mmap_calloc(void *map, unsigned int size)
{
	void *ptr = mmap_alloc(map, size);

	if (ptr == NULL)
		return (NULL);
	memset(ptr, 0, size);
	return (ptr);
}

