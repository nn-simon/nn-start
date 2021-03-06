#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "rsm.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include "train.h"
#include "classify.h"

#define MAX_BUF 128

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
	int nc;
	for (nc = 0; nc < numcase; nc++) {
		const uint8_t *d = data + nc * channelcase * numchannel;
		int *vv = V + nc * numvis * nummix;
		data2V_one(vv, numclass, d, channelcase, numchannel, position, numvis, mix, nummix, 1);
	}
}

static void _pr_rsm_info(rsm_info_t *rsm, char *msg)
{
	fprintf(stdout, "%s:\n\tnumvisXnumhid:%dx%d\n", msg, rsm->numvis, rsm->numhid);
}

static void _pr_train_info(train_info_t *train, char *msg)
{
	fprintf(stdout, "%s:\n\titeration:%d\n"
			"\tmininumcase:%d\n"
			"\tbatchsize:%d\n"
			"\tnummix:%d\n",
			msg, train->iteration, train->mininumcase, train->batchsize, train->nummix);
}

static void _pr_data_info(data_info_t *data, char *msg)
{
	fprintf(stdout, "%s:\n\tnumcaseXchannelcaseXnumchannel:%dx%dx%d\n",
		msg, data->numcase, data->channelcase, data->numchannel);
}

static void _pr_classify(classify_t *clssfy, char *msg)
{
	fprintf(stdout, "%s:\n\tnumclassXlencaseXnumcase:%dx%dx%d\n",
		msg, clssfy->numclass, clssfy->lencase, clssfy->numcase);
}

static void _char_replace(char *str, char before, char after)
{
	char *p;
	for (p = str; *p != '\0'; p++)
		if (*p == before)
			*p = after;
}

static void _labels2clssfy(classify_t *clssfy, const int * labels, int numcase, int nummix)
{
	if (nummix == 1) {
		memcpy(clssfy->labels, labels, numcase * sizeof(int));
		return;
	}
	int nm, nc;
	for (nc = 0; nc < numcase; nc++) {
		for (nm = 0; nm < nummix; nm++)
			clssfy->labels[nc * nummix + nm] = labels[nc];
	}
}

static int parse_command_line(int argc, char *argv[], train_info_t *train, rsm_info_t *rsm, data_info_t *data, classify_t *clssfy, char *out)
{
	char n_dtr[MAX_BUF], n_ltr[MAX_BUF];
	char n_w[MAX_BUF], n_pos[MAX_BUF], n_mix[MAX_BUF];
	char n_clssfy[MAX_BUF], n_class[MAX_BUF];

	char ch;
	while((ch = getopt(argc, argv, "c:n:r:o:t:T:")) != -1) {
		switch(ch) {
		case 'c':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s", &clssfy->numclass, n_clssfy);
			printf("[c]%s:%d,%s[%ld]\n", optarg, clssfy->numclass, n_clssfy, strlen(n_clssfy));
			break; 
		case 'n':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %d %d %s", 
				&train->iteration, &train->batchsize, &train->mininumcase, &train->nummix, n_mix);
			printf("[n]%s:%dx%dx%dx%d,%s[%ld]\n", optarg, train->iteration, train->batchsize, train->mininumcase, train->nummix, n_mix, strlen(n_mix));
			break;
		case 'r':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %s %s %s",
				&rsm->numvis, &rsm->numhid, n_class, n_pos, n_w);
			printf("[r]%s:%dx%d,%s[%ld],%s[%ld],%s[%ld]\n", optarg, rsm->numvis, rsm->numhid, n_class, strlen(n_class), n_pos, strlen(n_pos), n_w, strlen(n_w));
			break;
		case 'o':
			strncpy(out, optarg, MAX_BUF);
			break;
		case 't':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %d %s %s", &data->channelcase, &data->numchannel, &data->numcase, n_dtr, n_ltr);
			printf("[t]%s:%d,%s[%ld],%s[%ld]\n", optarg, data->numcase, n_dtr, strlen(n_dtr), n_ltr, strlen(n_ltr));
			break;
		default:
			fprintf(stderr, "don't know [%c]\n", ch);
			exit(0);
		}
	}

	fprintf(stdout, "other para:\n"
			"\tweight|class|position:%s|%s|%s\n"
			"\ttraindata|tainlabel:%s|%s\n"
			"\tmix|classify:%s|%s\n",
			n_w, n_class, n_pos,
			n_dtr, n_ltr, n_mix, n_clssfy);

	clssfy->lencase = rsm->numhid;
	clssfy->numcase = train->mininumcase * train->nummix;
	clssfy_build(clssfy);

	clssfy->w = malloc(clssfy->lencase * clssfy->numclass * sizeof(double));
	if (clssfy->w == NULL) {
		fprintf(stderr, "malloc clssfy->w error!\n");
		exit(0);
	}
	get_data(n_clssfy, clssfy->w, clssfy->lencase * clssfy->numclass * sizeof(double));

	_pr_train_info(train, "train info");
	_pr_rsm_info(rsm, "rsm info");
	_pr_data_info(data, "train data info");
	_pr_classify(clssfy, "clssfy");

	data->data = (uint8_t *) malloc((long)data->numcase * data->channelcase * data->numchannel * sizeof(uint8_t));
	data->labels = (int *) malloc(data->numcase * sizeof(int));
	if (!(data->data && data->labels)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_ltr, data->labels, data->numcase * sizeof(int));
	get_data(n_dtr, data->data, (long)data->numcase * data->channelcase * data->numchannel * sizeof(uint8_t));

	rsm->numclass = (int *) malloc(rsm->numvis * sizeof(int));
	rsm->position = (int*) malloc(rsm->numvis * sizeof(int));
	if (!(rsm->numclass && rsm->position)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_class, rsm->numclass, rsm->numvis * sizeof(int));
	get_data(n_pos, rsm->position, rsm->numvis * sizeof(int));

	int nv, lenw = 0;
	for (nv = 0; nv < rsm->numvis; nv++)
		lenw += rsm->numclass[nv] * (rsm->numhid + 1);
	double *w = (double*) malloc((lenw + rsm->numhid) * sizeof(double));
	if (!w) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_w, w, (lenw + rsm->numhid) * sizeof(double));
	rsm->w = w;
	rsm->bh = w + lenw;

	train->mix = (double *) malloc(train->nummix * data->numchannel * sizeof(double));
	if (!train->mix) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_mix, train->mix, train->nummix * data->numchannel * sizeof(double));
	return lenw;
}

static _free_mem(train_info_t *train, rsm_info_t *rsm, data_info_t *data, classify_t *clssfy)
{
	free(train->mix);
	free(rsm->position);
	free(rsm->numclass);
	free(rsm->w);
	free(data->labels);
	free(data->data);
	clssfy_clear(clssfy);
	free(clssfy->w);
}

int main(int argc, char *argv[])
{
	train_info_t train;
	rsm_info_t rsm;
	data_info_t data; //dtr: datatrain, dtt: datatest
	classify_t clssfy;
	char out[MAX_BUF];
	int lenw = parse_command_line(argc, argv, &train, &rsm, &data, &clssfy, out);

	int train_numcase = train.mininumcase * train.nummix;
	double *H = (double*) malloc(train_numcase * (rsm.numhid + 1) * sizeof(double));
	int *V = (int*) malloc(train_numcase * rsm.numvis * sizeof(int));
	int *order = malloc(sizeof(int) * (train_numcase > data.numcase ? train_numcase : data.numcase));
	if (!(H && V && order)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}

	int iter, curbatch = 0;
	int totalbatch = data.numcase / train.mininumcase;
	long minibatchlength = (long)train.mininumcase * data.channelcase * data.numchannel;
	double cost;

	init_hid_nag_ip_struct(clssfy.lencase, clssfy.numclass - 1);

	for (iter = 0; iter < train.iteration; iter++) {
		if (curbatch >= totalbatch)
			curbatch = 0;
		if (curbatch == 0) {
			randperm(order, data.numcase);
			reorder(data.data, sizeof(uint8_t), order, data.numcase, data.numchannel * data.channelcase);
			reorder(data.labels, sizeof(int), order, data.numcase, 1);
		}
		uint8_t *unlbl = data.data + curbatch * minibatchlength;
		fprintf(stdout, "**** iter %d, loc %ld, current data pointer %p ****\n", iter, curbatch * minibatchlength, unlbl);
		data2V(V, rsm.numclass, unlbl, train.mininumcase, data.channelcase, data.numchannel, rsm.position, rsm.numvis, train.mix, train.nummix);
		_labels2clssfy(&clssfy, data.labels + curbatch * train.mininumcase, train.mininumcase, train.nummix);
		classify_get_hid(rsm.w, rsm.bh, V, H, rsm.numvis, rsm.numhid, rsm.numclass, train_numcase, &clssfy);
		int xx, yy;
		for (xx = 0; xx < 1; xx++) {
			for (yy=0; yy < rsm.numhid; yy++)
				fprintf(stdout, "%d", (int)(H[xx * (rsm.numhid + 1) + yy]));
			fprintf(stdout, "\n");
		}
		randperm(order, train_numcase);
		reorder(V, sizeof(int), order, train_numcase, rsm.numvis);
		reorder(H, sizeof(double), order, train_numcase, rsm.numhid + 1);
		cost = rsm_train(rsm.w, rsm.bh, V, H, rsm.numvis, rsm.numhid, rsm.numclass, train_numcase, train.batchsize);

		curbatch++;
	}

	out_data(out, rsm.w, (lenw + rsm.numhid) * sizeof(double));
	free_hid_nag_ip_struct();

	free(V);
	free(H);
	free(order);
	return 0;
}
