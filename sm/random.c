#include <stdio.h>
#include "random.h"
#include <stdlib.h>

static FILE *fp_rand = NULL;

void open_random()
{
	if (fp_rand == NULL) {
		fp_rand = fopen("/dev/urandom", "r");
		if (fp_rand == NULL) {
			fprintf(stderr, "can't open /dev/urandom\n");
			exit(0);
		}
	}
}

unsigned random_self()
{
	unsigned r;
	fread(&r, sizeof(unsigned), 1, fp_rand);
	return r;
}

void close_random()
{
	if (fp_rand == NULL)
		return;
	fclose(fp_rand);
	fp_rand = NULL;
}
