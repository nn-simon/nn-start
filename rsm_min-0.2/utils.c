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


static void reorder(int *data, int numcase, int lencase)
{
	int *order = (int*)malloc(sizeof(int) * numcase);
	int *mem = (int*)malloc(sizeof(int) * numcase * lencase);
	if (!(order && mem)) {
		fprintf(stderr, "alloc err in reorder!\n");
		exit(0);
	}
	memcpy(mem, data, numcase * lencase * sizeof(int));
	int nc;
	for (nc = 0; nc < numcase; nc++)
		order[nc] = nc;
	srand(time(NULL));
	for (nc = 0; nc < numcase; nc++) {
		int len = numcase - nc;
		int pos = (int)((double)rand() / RAND_MAX * len) + nc;
		int temp =  order[pos];
		order[pos] = order[nc];
		order[nc] = temp;
	}
	for (nc = 0; nc < numcase; nc++) {
		memcpy(data + nc * lencase, mem + order[nc] * lencase, lencase * sizeof(int));
	}
	free(order);
	free(mem);
}
