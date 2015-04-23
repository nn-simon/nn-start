#ifndef _RAND_SELF
#define _RAND_SELF
 
#include <limits.h>

#define RAND_SELF_MAX UINT_MAX

void open_random();
void close_random();
void test_random();
unsigned random_self();
double random01();

#endif
