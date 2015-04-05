#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "model/sm.h"
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

#define MAX_HID_TYPE	16
#define NUM_HID_TYPE	2

int main(int argc, char *argv[])
{
	sm_info_t sm;
	char out[MAX_BUF];
	char n_V[MAX_BUF];
	int numcase;
	int numsample;
	int batchsize;
	char hid_name[MAX_BUF];
	char hid_type_name[MAX_HID_TYPE][MAX_BUF] = {"rand", "sa", "nag", "classify"};
	fun_get_hid get_hid[MAX_HID_TYPE] = {sm_hid_random, sm_hid_sa};//, sm_hid_nag, classify_get_hid};
	int hid_type_num = 0;

	char ch;
	while((ch = getopt(argc, argv, "d:m:b:o:f:")) != -1) {
		int i;
		switch(ch) {
		case 'b':
			batchsize = atoi(optarg);
			break;
		case 'd':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s", &numcase, n_V);
			printf("[t]%s:%d,%s[%ld]\n", optarg, numcase, n_V, strlen(n_V));
			break;
		case 'f':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s", &numsample, hid_name);
			printf("[t]%s:%d,%s[%ld]\n", optarg, numsample, hid_name, strlen(hid_name));
			for (i = 0; i < NUM_HID_TYPE; i++) {
				if (strncmp(hid_type_name[i], hid_name, MAX_BUF) == 0)
					break;
			}
			hid_type_num = i;
			if (hid_type_num >= NUM_HID_TYPE) {
				fprintf(stderr, "for parameter f, you have following choices:\n\t|");
				for (i = 0; i < NUM_HID_TYPE; i++)
					fprintf(stderr, "%s|", hid_type_name[i]);
				fprintf(stderr, "\nbut your parameter is %s\n", optarg);
				fprintf(stderr, "notice that, the default parameter is %s\n", hid_type_name[0]);
				exit(0);
			}
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

	double *H = (double*) malloc(batchsize * sm.numhid * sizeof(double));
	int *V = (int*) malloc(batchsize * sm.numvis * sizeof(int));
	double *bh = (double *) malloc(sm.numhid * sizeof(double));
	if (!(H && V && bh)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}

	FILE *fp = fopen(n_V, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s !\n", n_V);
		exit(0);
	}
	FILE *fout = fopen(out, "r");
	if (fout == NULL) {
		fprintf(stderr, "can't open %s !\n", out);
		exit(0);
	}

	while (numcase > 0) {
		if (numcase < batchsize)
			batchsize = numcase;
                if (fread(V, sizeof(int), batchsize * sm.numvis, fp) != batchsize * sm.numvis) {
                        fprintf(stderr, "read %s err!\n", n_V);
                        exit(0);
		}
		get_hid[hid_type_num](&sm, V, H, batchsize, numsample, bh);
                if (fwrite(H, sizeof(double), batchsize * sm.numhid, fout) != batchsize * sm.numhid) {
                        fprintf(stderr, "write %s err!\n", out);
                        exit(0);
		}
		numcase -= batchsize;
	}
	
	destroy_sm(&sm);
	free(bh);
	free(V);
	free(H);
	return 0;
}
