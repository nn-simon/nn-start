#include "train.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pr_array(FILE *f, void *array, int numcase, int lencase, char flag)
{
	union value {
		void *v;
		int *i;
		double *d;
	} v;
	v.v = array;
	if (flag == 'i'){
	int row, col;
	for (row = 0; row < numcase; row++) {
		for (col = 0; col < lencase; col++)
			fprintf(f, "%d ", v.i[row * lencase + col]);
		fprintf(f, "\n");
	}
	}
	if (flag == 'd'){
	int row, col;
	for (row = 0; row < numcase; row++) {
		for (col = 0; col < lencase; col++)
			fprintf(f, "%lf ", v.d[row * lencase + col]);
		fprintf(f, "\n");
	}
	}
}

void get_data(const char *name,  void *data, int len)
{
	FILE *fp = fopen(name, "rb");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s!\n", name);
		exit(0); // not good
	}
	if (fread(data, 1, len, fp) != len) {
		fprintf(stderr, "Read %s error!\n", name);
		fclose(fp);
		exit(0); // not good
	}
	fclose(fp);
}

void out_data(const char *name,  void *data, int len)
{
	FILE *fp = fopen(name, "wb");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s!\n", name);
		exit(0); // not good
	}
	if (fwrite(data, 1, len, fp) != len) {
		fprintf(stderr, "Write %s error!\n", name);
		fclose(fp);
		exit(0); // not good
	}
	fclose(fp);
}

void usage(const char *name)
{
	fprintf(stderr, "uasge: %s", name);
}

void randperm(int *order, int numcase)
{
	int nc;
	srand(time(NULL));
	for (nc = 0; nc < numcase; nc++)
		order[nc] = nc;
	for (nc = 0; nc < numcase; nc++) {
		int len = numcase - nc;
		int pos = (int)((double)rand() / RAND_MAX * len) + nc;
		int temp =  order[pos];
		order[pos] = order[nc];
		order[nc] = temp;
	}
}

static void _reorder(void *a, void *b, size_t size)
{
	char *ca = (char*)a;
	char *cb = (char*)b;
	int ns;
	for (ns = 0; ns < size; ns++) {
		//fprintf(stdout, "%d %d", *ca, *cb);
		*ca = *ca ^ *cb;
		*cb = *ca ^ *cb;
		*ca = *ca ^ *cb;
		//fprintf(stdout, " %d %d\n", *ca, *cb);
		ca++;
		cb++;
	}
}

void reorder(void *data, size_t size, const int *order, int numcase, int lencase)
{
	int nc;
	int casesize = lencase * size;
	for (nc = 0; nc < numcase; nc++) {
		if (nc == order[nc]) // when nc == order[nc], all values in array data will be 0.
			continue;
		_reorder((char*)data + nc * casesize, (char*)data + order[nc] * casesize, casesize);
	}
}
