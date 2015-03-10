#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "random.h"
#include "rsm.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>

static void pr_array(FILE *f, void *array, int numcase, int lencase, char flag)
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

#define MAX_BUF 128

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

static void data2V_one(int *V, const int *numclass, const uint8_t *data, int channelcase, int numchannel, const int *position, int numvis, const double *mix, int nummix, int repetition)
{
	int nm, nv, nr;
	for (nm = 0; nm < nummix; nm++) {
		for (nv = 0; nv < numvis; nv++) {
			int channel;
			double tp = 0.0;
			for (channel = 0; channel < numchannel; channel++) {
				tp += (double)data[channel * channelcase + position[nv]] * mix[nm * numchannel + channel] * (double)numclass[nv] / 256.0;
				//printf("(%2d %4d %2d %3d %lf)\n", nm, nv, channel, data[channel * channelcase + position[nv]], tp);
			}
			V[nm * numvis + nv] = (int)tp;
		}
	}
	for (nr = 1; nr < repetition; nr++)
		memcpy(&V[nr * nummix * numvis], V, nummix * numvis * sizeof(int));
}

static void data2V(int *V, const int *numclass, const uint8_t *data, int numcase, int channelcase, int numchannel, const int *position, int numvis, const double *mix, int nummix)
{
	int *perm = (int *)malloc(numcase * sizeof(int));
	if (perm == NULL) {
		fprintf(stderr, "get_V alloc error!\n");
		exit(0);
	}
	int nc, nv;
	for (nc = 0; nc < numcase; nc++)
		perm[nc] = nc;
	for (nc = 0; nc < numcase; nc++) {
		int len = numcase - nc;
		int loc = (int)((double)rand() / RAND_MAX * len) + nc;
		int temp =  perm[loc];
		perm[loc] = perm[nc];
		perm[nc] = temp;
		const uint8_t *d = data + perm[nc] * channelcase * numchannel;
		int *vv = V + nc * numvis * nummix;
		data2V_one(vv, numclass, d, channelcase, numchannel, position, numvis, mix, nummix, 1);
	}
	free(perm);
}

int main(int argc, char *argv[])
{
	int numcase, channelcase, numchannel, nummix;
	int batchsize;
	int numvis, numhid;
	int mininumcase;
	int iteration = 1;
	int file_number[MAX_BUF] = {0};
	char name_out[MAX_BUF];
	char name_data[MAX_BUF];
	char name_w[MAX_BUF];
	char name_H[MAX_BUF];
	char name_pos[MAX_BUF];
	char name_mix[MAX_BUF];
	char name_class[MAX_BUF];
	name_H[0] = '\0';

	char ch;

	while((ch = getopt(argc, argv, "b:m:c:s:p:P:w:u:o:h:i:x:")) != -1) {
		switch(ch) {
		case 'i':
			iteration = atoi(optarg);
			break;
		case 'b':
			batchsize = atoi(optarg);
			break;
		case 'm':
			sscanf(optarg, "%d", &mininumcase);
			break;
		case 'p': //para
			sscanf(optarg, "%dx%dx%dx%d", &numcase, &channelcase, &numchannel, &nummix);
			break;
		case 'P': //postion
			strncpy(name_pos, optarg, MAX_BUF);
			break;
		case 's':
			sscanf(optarg, "%dx%d", &numvis, &numhid);
			break;
		case 'c':
			strncpy(name_class, optarg, MAX_BUF);
			break;
		case 'x':
			strncpy(name_mix, optarg, MAX_BUF);
			break;
		case 'w':
			//w, bh
			strncpy(name_w, optarg, MAX_BUF);
			break;
		case 'u':
			//data
			strncpy(name_data, optarg, MAX_BUF);
			break;
		case 'o':
			//output
			strncpy(name_out, optarg, MAX_BUF);
			break;
		case 'h':
			strncpy(name_H, optarg, MAX_BUF);
			break;
		default:
			fprintf(stderr, "don't know [%c]\n", ch);
			exit(0);
		}
	}

	long len_unlabeled = (long)numcase * channelcase * numchannel;
	size_t pagesize = getpagesize();
	size_t size_mmap = pagesize * (len_unlabeled * sizeof(uint8_t) / pagesize + 1);
	int fd = open(name_data, O_CREAT | O_RDONLY);
	uint8_t *unlabeled = (uint8_t*)mmap(NULL, size_mmap, PROT_READ, MAP_SHARED, fd, 0);
	if (unlabeled == (void *) -1) {
		fprintf(stderr, "mmap err: %d\n", errno);
		exit(0);
	}
	close(fd);

	int train_numcase = mininumcase * nummix;
	int *numclass = (int *) malloc(numvis * sizeof(int));
	if (!numclass) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(name_class, numclass, numvis * sizeof(int));
	int nv, lenw = 0;
	for (nv = 0; nv < numvis; nv++)
		lenw += numclass[nv] * (numhid + 1);
	int *position = (int*) malloc(numvis * sizeof(int));
	double *mix = (double *) malloc(nummix * numchannel * sizeof(double));
	double *H = (double*) malloc(train_numcase * (numhid + 1) * sizeof(double));
	int *V = (int*) malloc(train_numcase * numvis * sizeof(int));
	double *w = (double*) malloc((lenw + numhid) * sizeof(double));

	if (!(H && w && V && position && mix)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(name_mix, mix, nummix * numchannel * sizeof(double));
	get_data(name_pos, position, numvis * sizeof(int));
	get_data(name_w, w, (lenw + numhid) * sizeof(double));
	double *bh = w + lenw;

//	int xy;
//	for (xy = 0; xy < lenw; xy++)
//		w[xy] *= 0.01;

	open_random();
	int totalbatch = numcase / mininumcase;
	int tpr = rand() % numvis;
	fprintf(stdout, "\t[i]iteration:%d\n"
			"\t[m]mininumcase:%d\n"
			"\t[p]numcaseXchannelcaseXnumchannelXnummix:%dx%dx%dx%d\n"
			"\t[b]batchsize:%d\n"
			"\t[s]numvisXnumhid:%dx%d\n"
			"\t[cuxPwho]%s|%s|%s|%s|%s|%s|%s\n"
			"\tnumclass[%d]:%d\n"
			"\ttotalbatch:%d\n"
			"\ttrainnumcase:%d\n"
			"\tlen_unlabeled:%ld\n"
			"\tpagesize:%ld\n"
			"\tsize_mmap:%ld\n"
			"\tdata pointer:%p\n", 
			iteration,
			mininumcase, 
			numcase, channelcase, numchannel, nummix,
			batchsize,
			numvis, numhid,
			name_class, name_data, name_mix, name_pos, name_w, name_H, name_out,
			tpr, numclass[tpr],
			totalbatch, train_numcase, len_unlabeled, pagesize, size_mmap, unlabeled);

	int iter, curbatch = 0;
	double cost;
	//get_data(name_H, H, train_numcase * (numhid + 1) * sizeof(double));
	for (iter = 0; iter < iteration; iter++) {
		fprintf(stdout, "**** iter %d, loc %ld ****\n", iter, curbatch * (long)mininumcase * channelcase * numchannel);
		uint8_t *unlbl = unlabeled + curbatch * (long)mininumcase * channelcase * numchannel;
		printf("ok, %p\n", unlbl);
		data2V(V, numclass, unlbl, mininumcase, channelcase, numchannel, position, numvis, mix, nummix);
		rsmhid_sample(w, bh, V, H, numvis, numhid, numclass, train_numcase);
		int xx, yy;
		for (xx = 0; xx < 1; xx++) {
			for (yy=0; yy < numhid; yy++)
				fprintf(stdout, "%d", (int)(H[xx * (numhid + 1) + yy]));
			fprintf(stdout, "\n");
		}
		cost = rsm_train(w, bh, V, H, numvis, numhid, numclass, train_numcase, batchsize);
		fprintf(stdout, "**** iter %d: curbatch %d's cost is %lf ****\n", iter, curbatch, cost);
		curbatch++;
		if (curbatch >= totalbatch)
			curbatch = 0;
	}
	close_random();
	out_data(name_out, w, (lenw + numhid) * sizeof(double));
	if (munmap(unlabeled, size_mmap) != 0) {
		fprintf(stderr, "munmap err: %d\n", errno);
		exit(0);
	}
	free(w);
	free(V);
	free(mix);
	free(H);
	free(position);
	free(numclass);

	return 0;
}
