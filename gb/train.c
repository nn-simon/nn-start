#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "model/gb.h"
#include "utils/utils.h"

#define MAX_BUF 128

static void _pr_gb_info(gb_info_t *gb, char *msg)
{
	fprintf(stdout, "%s:\n\tnetwork type:%d\n"
			"\tnumvisXnumhid:%dx%d\n"
			"\tlambda:%lf\n",
			msg, gb->type, gb->numvis, gb->numhid, gb->lambda);
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
	gb_info_t gb;
	char out[MAX_BUF];
	char n_V[MAX_BUF], hid_name[MAX_BUF];
	int numcase, numsample;
	int thr_num;
	int iteration, batchsize, mininumcase;
	//fun_get_hid get_hid;
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
			construct_gb(&gb, optarg);
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

	//get_hid = choose_hid(hid_name, gb.type);
//	if (get_hid == NULL)
//		exit(0);
	double *H = (double*) malloc(mininumcase * gb.numhid * sizeof(double));
	double *V = (double*) malloc((size_t)numcase * gb.numvis * sizeof(double));
	int *order = malloc(sizeof(int) * numcase);
	double *bh = (double *) malloc(gb.numhid * sizeof(double));
	if (!(H && V && order && bh)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	_pr_gb_info(&gb, "gb info");
	fprintf(stdout, "numcase:%d\niteration:%d\n"
			"mininumcase:%d\nhid_type:%s\n"
			"batchsize:%d\nlearnrate:%lf\n",
			numcase, iteration, mininumcase, hid_name, batchsize, learnrate);

	get_data(n_V, V, (size_t)numcase * gb.numvis * sizeof(double));
	// notice: when get_hid == gb_hid_sa_thr, reserved = &thr_num
	int totalbatch = numcase / mininumcase;
	int iter, curbatch = 0;
	open_random();
	for (iter = 0; iter < iteration; iter++) {
		if (curbatch >= totalbatch)
			curbatch = 0;
		if (curbatch == 0) {
			randperm(order, numcase);
			reorder(V, sizeof(int), order, numcase, gb.numvis);
		}
		double *V_batch = V + (size_t)curbatch * mininumcase * gb.numvis;
		fprintf(stdout, "**** iter %d, loc %ld, current data pointer %p ****\n", iter, (size_t)curbatch * mininumcase * gb.numvis, V_batch);

		gb_rand(&gb, NULL, V_batch, H, mininumcase, numsample, bh);
		check_gradient(&gb, V_batch, H, mininumcase);
		return;
		int xx, yy;
		for (xx = 0; xx < 2; xx++) {
			for (yy=0; yy < gb.numhid; yy++)
				fprintf(stdout, "%d", (int)(H[xx * gb.numhid + yy]));
			fprintf(stdout, "\n");
		}
		gb_train(&gb, learnrate, V_batch, H, mininumcase, batchsize);

		curbatch++;
	}
	close_random();

	out_w(out, &gb);
	destroy_gb(&gb);
	free(V);
	free(H);
	free(order);
	return 0;
}
