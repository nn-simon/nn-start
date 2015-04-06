#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "model/sm.h"
#include "get_hid/sm_hid.h"
#include "utils/utils.h"

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
#define NUM_HID_TYPE	3

int main(int argc, char *argv[])
{
	sm_info_t sm;
	classify_t clssfy;
	char n_clssfy[MAX_BUF] = "";
	char n_label[MAX_BUF];
	char out[MAX_BUF];
	char n_V[MAX_BUF];
	int numcase;
	int numsample;
	int batchsize;
	int thr_num;
	char hid_name[MAX_BUF];
	char hid_type_name[MAX_HID_TYPE][MAX_BUF] = {"rand", "sa", "sa_thread", "nag"};
	fun_get_hid get_hid[MAX_HID_TYPE] = {sm_hid_random, sm_hid_sa, sm_hid_sa_thr};//, sm_hid_nag, classify_get_hid};
	int hid_type_num = 0;

	char ch;
	while((ch = getopt(argc, argv, "d:m:b:o:f:c:")) != -1) {
		int i;
		switch(ch) {
		case 'b':
			batchsize = atoi(optarg);
			break;
		case 'c':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s %s", &clssfy.numclass, n_clssfy, n_label);
			//clssfy->lencase = sm->numhid;
			printf("[c]%s:%d,%s[%ld],%s[%ld]\n", optarg, clssfy.numclass, n_clssfy, strlen(n_clssfy), n_label, strlen(n_label));
			break; 
		case 'd':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s", &numcase, n_V);
			printf("[d]%s:%d,%s[%ld]\n", optarg, numcase, n_V, strlen(n_V));
			break;
		case 'f':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %s", &numsample, &thr_num, hid_name);
			printf("[f]%s:%dx%d,%s[%ld]\n", optarg, numsample, thr_num, hid_name, strlen(hid_name));
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
	_pr_sm_info(&sm, "sm info");
	fprintf(stdout, "numcase:%d\nhid_type:%s\n"
			"batchsize:%d\nnumsample:%d\nthr_num:%d\n",
			numcase, hid_type_name[hid_type_num], batchsize, numsample, thr_num);
	hid_nag_ip_t ip;
	FILE *flabel;
	if (n_clssfy[0] != '\0') {
		clssfy.lencase = sm.numhid;
		clssfy.numcase = batchsize;
		clssfy_build(&clssfy);
		get_data(n_clssfy, clssfy.w, clssfy.lencase * clssfy.numclass * sizeof(double));
		init_hid_nag_ip_struct(&ip, clssfy.lencase, clssfy.numclass - 1);	
		flabel = fopen(n_label, "r");
		if (flabel == NULL) {
			fprintf(stderr, "can't open n_label[%s] !\n", n_label);
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
		fprintf(stderr, "can't open n_V[%s] !\n", n_V);
		exit(0);
	}
	FILE *fout = fopen(out, "w");
	if (fout == NULL) {
		fprintf(stderr, "can't open %s !\n", out);
		exit(0);
	}

	open_random();
	void *reserved;
	switch (hid_type_num) {
		case 0:
		case 1:
			reserved = bh;
			break;
		case 2:
			reserved = &thr_num;
			break;
	}
	int cnt = 0;
	while (numcase > 0) {
		fprintf(stdout, "output: cnt %d, batchsize %d\n", cnt++, batchsize);
		if (numcase < batchsize)
			batchsize = numcase;
                if (fread(V, sizeof(int), batchsize * sm.numvis, fp) != batchsize * sm.numvis) {
                        fprintf(stderr, "read %s err!\n", n_V);
                        exit(0);
		}
		get_hid[hid_type_num](&sm, NULL, V, H, batchsize, numsample, reserved);
		if (n_clssfy[0] != '\0') {
		//	fprintf(stdout, "pred start\n");
			clssfy.data = H;
			prediction(&clssfy);
		//	pr_array(stdout, clssfy.pred, 1, 20, 'i');
		//	fprintf(stdout, "pred end\n");
                	if (fread(clssfy.labels, sizeof(int), batchsize, flabel) != batchsize) {
                        	fprintf(stderr, "read %s err!\n", n_label);
                        	exit(0);
			}
		//	pr_array(stdout, clssfy.labels, 1, 20, 'i');
			classify_get_hid(&sm, &ip, V, H, batchsize, &clssfy);
		}
                if (fwrite(H, sizeof(double), batchsize * sm.numhid, fout) != batchsize * sm.numhid) {
                        fprintf(stderr, "write %s err!\n", out);
                        exit(0);
		}
		numcase -= batchsize;
	}
	fclose(fout);
	fclose(fp);
	close_random();
	if (n_clssfy[0] != '\0') {
		fclose(flabel);
		clssfy_clear(&clssfy);
		free_hid_nag_ip_struct(&ip);
	}
	destroy_sm(&sm);
	free(bh);
	free(V);
	free(H);
	return 0;
}
