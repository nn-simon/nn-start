#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "model/sm_train.h"
#include "model/sm.h"
#include "utils/utils.h"
#include "get_hid/sm_hid.h"

#define MAX_BUF 128

static void _pr_sm_info(sm_info_t *sm, char *msg)
{
	fprintf(stdout, "%s:\n\tnumvisXnumhid:%dx%d\n"
			"\tlambda:%lf\n"
			"\tlen_v2h_maxXclass_max:%dx%d\n", 
			msg, sm->numvis, sm->numhid, sm->lambda, sm->len_v2h_max, sm->class_max);
}

static void _pr_vh_info(sm_info_t *sm)
{
	int nv, nh, nl;
	for (nv = 0; nv < sm->numvis; nv++) {
		fprintf(stdout, "v.%d:(%d)", nv, sm->len_v2h[nv]);
		for (nl = 0; nl < sm->len_v2h[nv]; nl++)
			fprintf(stdout, "%d ", sm->v2h[nv][nl]);
		fprintf(stdout, "\n");
	}
	for (nh = 0; nh < sm->numhid; nh++) {
		fprintf(stdout, "h.%d:(%d)", nh, sm->len_h2v[nh]);
		for (nl = 0; nl < sm->len_h2v[nh]; nl++)
			fprintf(stdout, "%d ", sm->h2v[nh][nl]);
		fprintf(stdout, "\n");
		fprintf(stdout, "h.%d:(%d)", nh, sm->len_h2v[nh]);
		for (nl = 0; nl < sm->len_h2v[nh]; nl++)
			fprintf(stdout, "%d ", sm->pos_h2v[nh][nl]);
		fprintf(stdout, "\n");
	}
}

static void _char_replace(char *str, char before, char after)
{
	char *p;
	for (p = str; *p != '\0'; p++)
		if (*p == before)
			*p = after;
}

int main(int argc, char *argv[])
{
	sm_info_t sm;
	char out[MAX_BUF];
	char n_V[MAX_BUF], hid_name[MAX_BUF];
	int numcase, numsample;
	int thr_num;
	int iteration, batchsize, mininumcase;
	fun_get_hid get_hid;
	double learnrate;

	char ch;
	while((ch = getopt(argc, argv, "d:m:t:o:f:")) != -1) {
		switch(ch) {
		case 'd':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s", &numcase, n_V);
			printf("[d]%s:%d,%s[%ld]\n", optarg, numcase, n_V, strlen(n_V));
			break;
		case 'f':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %s", &numsample, &thr_num, hid_name);
			printf("[f]%s:%dx%d,%s[%ld]\n", optarg, numsample, thr_num, hid_name, strlen(hid_name));
			break;
		case 't':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %d %lf", 
				&iteration, &batchsize, &mininumcase, &learnrate);
			printf("[t]%s:%dx%dx%d,%lf\n", optarg, iteration, batchsize, mininumcase, learnrate);
			break;
		case 'm':
			_char_replace(optarg, ',', ' ');
			construct_sm(&sm, optarg);
			printf("[m]%s\n", optarg);
			break;
		case 'o':
			strncpy(out, optarg, MAX_BUF);
			printf("[o]%s\n", optarg);
			break;
		default:
			fprintf(stderr, "don't know [%c]\n", ch);
			exit(0);
		}
	}

	get_hid = choose_hid(hid_name, sm.type);
	if (get_hid == NULL)
		exit(0);
	double *H = (double*) malloc(mininumcase * sm.numhid * sizeof(double));
	int *V = (int*) malloc((size_t)numcase * sm.numvis * sizeof(int));
	int *order = malloc(sizeof(int) * numcase);
	double *bh = (double *) malloc(sm.numhid * sizeof(double));
	if (!(H && V && order && bh)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	_pr_sm_info(&sm, "sm info");
	fprintf(stdout, "numcase:%d\niteration:%d\n"
			"mininumcase:%d\nhid_type:%s\n"
			"batchsize:%d\nlearnrate:%lf\n",
			numcase, iteration, mininumcase, hid_name, batchsize, learnrate);

	get_data(n_V, V, (size_t)numcase * sm.numvis * sizeof(int));
	// notice: when get_hid == sm_hid_sa_thr, reserved = &thr_num
	*(int*)bh = thr_num;
	int totalbatch = numcase / mininumcase;
	int iter, curbatch = 0;
	open_random();
	for (iter = 0; iter < iteration; iter++) {
		if (curbatch >= totalbatch)
			curbatch = 0;
		if (curbatch == 0) {
			randperm(order, numcase);
			reorder(V, sizeof(int), order, numcase, sm.numvis);
		}
		int *V_batch = V + curbatch * mininumcase * sm.numvis;
		fprintf(stdout, "**** iter %d, loc %ld, current data pointer %p ****\n", iter, (size_t)curbatch * mininumcase * sm.numvis, V_batch);

		get_hid(&sm, NULL, V_batch, H, mininumcase, numsample, bh);
		int xx, yy;
		for (xx = 0; xx < 2; xx++) {
			for (yy=0; yy < sm.numhid; yy++)
				fprintf(stdout, "%d", (int)(H[xx * sm.numhid + yy]));
			fprintf(stdout, "\n");
		}
		sm_train(&sm, learnrate, V_batch, H, mininumcase, batchsize);

		curbatch++;
	}
	close_random();

	out_w(out, &sm);
	destroy_sm(&sm);
	free(V);
	free(H);
	free(order);
	return 0;
}
