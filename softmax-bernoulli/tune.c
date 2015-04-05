#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "model/sm_train.h"
#include "model/sm.h"

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
	char n_V[MAX_BUF], n_H[MAX_BUF];
	int numcase;
	int iteration, batchsize;
	double learnrate;

	char ch;
	while((ch = getopt(argc, argv, "d:m:t:o:")) != -1) {
		switch(ch) {
		case 'd':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s %s", &numcase, n_V, n_H);
			printf("[t]%s:%d,%s[%ld],%s[%ld]\n", optarg, numcase, n_V, strlen(n_V), n_H, strlen(n_H));
			break;
		case 't':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %lf", 
				&iteration, &batchsize, &learnrate);
			printf("[n]%s:%dx%d,%lf\n", optarg, iteration, batchsize, learnrate);
			break;
		case 'm':
			_char_replace(optarg, ',', ' ');
			construct_sm(&sm, optarg);
			printf("[s]%s\n", optarg);
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

	double *H = (double*) malloc(numcase * sm.numhid * sizeof(double));
	int *V = (int*) malloc(numcase * sm.numvis * sizeof(int));
	int *order = malloc(sizeof(int) * numcase);
	if (!(H && V && order)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}

	get_data(n_V, V, (size_t)numcase * sm.numvis * sizeof(int));
	get_data(n_H, H, (size_t)numcase * sm.numhid * sizeof(double));

	int iter;
	for (iter = 0; iter < iteration; iter++) {
		fprintf(stdout, "**** iter %d ****\n", iter);
		randperm(order, numcase);
		reorder(V, sizeof(int), order, numcase, sm.numvis);
		reorder(H, sizeof(double), order, numcase, sm.numhid);
/*		{ // for check gradient
		_pr_sm_info(&sm, "2, sm info");
		check_gradient(&sm, V, H, train_numcase);
		fprintf(stdout, "return !\n\n");
		return;
		}
*/		int xx, yy;
		for (xx = 0; xx < 2; xx++) {
			for (yy=0; yy < sm.numhid; yy++)
				fprintf(stdout, "%d", (int)(H[xx * sm.numhid + yy]));
			fprintf(stdout, "\n");
		}
		sm_train(&sm, learnrate, V, H, numcase, batchsize);
	}

	out_w(out, &sm);
	destroy_sm(&sm);
	free(V);
	free(H);
	free(order);
	return 0;
}
