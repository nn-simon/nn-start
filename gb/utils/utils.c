#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

void get_data(const char *name, void *data, size_t need_read)
{
	int fd = open(name, O_RDONLY);
	
	if (fd == -1) {
		fprintf(stderr, "can't open %s!\n", name);
		exit(0); // not good
	}
	size_t len = need_read;
	while (len > 0) {
		ssize_t nread = read(fd, data, len);
		if (nread == -1) {
			fprintf(stderr, "Read %s error, need read %ld, actually read %ld!\n", name, need_read, need_read - len);
			close(fd);
			exit(0); // not good
		}
		if (nread == 0) {
			fprintf(stderr, "Read %s short of %ld bytes\n", name, len);
			close(fd);
			exit(0);
		}
		len -= nread;
		data = (char *)data + nread; 
	}
	close(fd);
}

void out_data(const char *name, void *data, int len)
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
		_reorder((char*)data + (size_t)nc * casesize, (char*)data + (size_t)order[nc] * casesize, casesize);
	}
}
