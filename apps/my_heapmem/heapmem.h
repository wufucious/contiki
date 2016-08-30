#include <unistd.h>
void *heapmem_alloc(size_t n);
void heapmem_free(void * ptr);
void memstats();
void memconsistency();
