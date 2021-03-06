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

static uint8_t *_data_mmap(long len_unlabeled, const char *filename, size_t *sz)
{
	size_t pagesize = getpagesize();
	size_t size_mmap = pagesize * ((len_unlabeled * sizeof(uint8_t) + pagesize - 1) / pagesize);
	*sz = size_mmap;
	int fd = open(filename, O_CREAT | O_RDONLY);
	uint8_t *unlabeled = (uint8_t*)mmap(NULL, size_mmap, PROT_READ, MAP_SHARED, fd, 0);
	if (unlabeled == (void *) -1) {
		fprintf(stderr, "mmap err: %d\n", errno);
		exit(0);
	}
	close(fd);
	fprintf(stdout, "data_mmap:\n"
			"\tlen_unlabeled:%ld\n"
			"\tpagesize:%ld\n"
			"\tsize_mmap:%ld\n"
			"\tdata pointer:%p\n", len_unlabeled, pagesize, size_mmap, unlabeled); 
	return unlabeled;
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

static void _pr_data_info(data_info_t *data, case_info_t *case_info, char *msg)
{
	fprintf(stdout, "%s:\n\tnumcaseXchannelcaseXnumchannel:%dx%dx%d\n",
		msg, data->numcase, case_info->channelcase, case_info->numchannel);
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

int main(int argc, char *argv[])
{
	train_info_t train;
	rsm_info_t rsm;
	data_info_t dtr, dtt; //dtr: datatrain, dtt: datatest
	case_info_t case_info;
	classify_t clssfy, clssfy_test;	
	char n_dtr[MAX_BUF], n_dtt[MAX_BUF], n_ltr[MAX_BUF], n_ltt[MAX_BUF];
	char n_w[MAX_BUF], n_pos[MAX_BUF], n_mix[MAX_BUF];
	char n_clssfy[MAX_BUF], n_class[MAX_BUF];
	char n_out[MAX_BUF];

	char ch;
	while((ch = getopt(argc, argv, "c:C:n:r:o:t:T:")) != -1) {
		switch(ch) {
		case 'c':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d", &case_info.channelcase, &case_info.numchannel);
			printf("[c]%s:%dx%d\n", optarg, case_info.channelcase, case_info.numchannel);
			break; 
		case 'C':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s", &clssfy.numclass, n_clssfy);
			printf("[C]%s:%d,%s[%ld]\n", optarg, clssfy.numclass, n_clssfy, strlen(n_clssfy));
			break; 
		case 'n':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %d %d %s", 
				&train.iteration, &train.batchsize, &train.mininumcase, &train.nummix, n_mix);
			printf("[n]%s:%dx%dx%dx%d,%s[%ld]\n", optarg, train.iteration, train.batchsize, train.mininumcase, train.nummix, n_mix, strlen(n_mix));
			break;
		case 'r':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %d %s %s %s",
				&rsm.numvis, &rsm.numhid, n_class, n_pos, n_w);
			printf("[r]%s:%dx%d,%s[%ld],%s[%ld],%s[%ld]\n", optarg, rsm.numvis, rsm.numhid, n_class, strlen(n_class), n_pos, strlen(n_pos), n_w, strlen(n_w));
			break;
		case 'o':
			strncpy(n_out, optarg, MAX_BUF);
			break;
		case 't':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s %s", &dtr.numcase, n_dtr, n_ltr);
			printf("[t]%s:%d,%s[%ld],%s[%ld]\n", optarg, dtr.numcase, n_dtr, strlen(n_dtr), n_ltr, strlen(n_ltr));
			break;
		case 'T':
			_char_replace(optarg, ',', ' ');
			sscanf(optarg, "%d %s %s", &dtt.numcase, n_dtt, n_ltt);
			printf("[T]%s:%d,%s[%ld],%s[%ld]\n", optarg, dtt.numcase, n_dtt, strlen(n_dtt), n_ltt, strlen(n_ltt));
			break;
		default:
			fprintf(stderr, "don't know [%c]\n", ch);
			exit(0);
		}
	}

	int train_numcase = train.mininumcase * train.nummix;
	int totalbatch = dtr.numcase / train.mininumcase;
	fprintf(stdout, "other para:\n\ttotalbatch:%d\n"
			"\ttrainnumcase:%d\n"
			"\tweight|class|position:%s|%s|%s\n"
			"\ttraindata|tainlabel|testdata|testlabel:%s|%s|%s|%s\n"
			"\tmix|classify:%s|%s\n",
			totalbatch, train_numcase, n_w, n_class, n_pos,
			n_dtr, n_ltr, n_dtt, n_ltt, n_mix, n_clssfy);

	clssfy.lencase = rsm.numhid;
	clssfy.numcase = train_numcase;
	clssfy_build(&clssfy);

	clssfy_test.numclass = clssfy.numclass;
	clssfy_test.lencase = rsm.numhid;
	clssfy_test.numcase = dtt.numcase * train.nummix;
	clssfy_build(&clssfy_test);
	clssfy.w = malloc(clssfy.lencase * clssfy.numclass * sizeof(double));
	if (clssfy.w == NULL) {
		fprintf(stderr, "malloc clssfy->w error!\n");
		exit(0);
	}
	get_data(n_clssfy, clssfy.w, clssfy.lencase * clssfy.numclass * sizeof(double));
	clssfy_test.w = clssfy.w;

	_pr_train_info(&train, "train info");
	_pr_rsm_info(&rsm, "rsm info");
	_pr_data_info(&dtr, &case_info,  "train data info");
	_pr_data_info(&dtt, &case_info, "test data info");
	_pr_classify(&clssfy, "clssfy");
	_pr_classify(&clssfy_test, "clssfy test");

	size_t size_mmap;
	dtr.data = _data_mmap((long)dtr.numcase * case_info.channelcase * case_info.numchannel, n_dtr, &size_mmap);
	dtr.labels = (int *) malloc(dtr.numcase * sizeof(int));
	dtt.data = (uint8_t *) malloc(dtt.numcase * case_info.channelcase * case_info.numchannel * sizeof(uint8_t));
	dtt.labels = (int *) malloc(dtt.numcase * sizeof(int));
	if (!(dtr.labels && dtt.data && dtt.labels)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_ltr, dtr.labels, dtr.numcase * sizeof(int));
	get_data(n_ltt, dtt.labels, dtt.numcase * sizeof(int));
	get_data(n_dtt, dtt.data, dtt.numcase * case_info.channelcase * case_info.numchannel * sizeof(uint8_t));

	rsm.numclass = (int *) malloc(rsm.numvis * sizeof(int));
	rsm.position = (int*) malloc(rsm.numvis * sizeof(int));
	if (!(rsm.numclass && rsm.position)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_class, rsm.numclass, rsm.numvis * sizeof(int));
	get_data(n_pos, rsm.position, rsm.numvis * sizeof(int));
	int nv, lenw = 0;
	for (nv = 0; nv < rsm.numvis; nv++)
		lenw += rsm.numclass[nv] * (rsm.numhid + 1);
	double *w = (double*) malloc((lenw + rsm.numhid) * sizeof(double));
	if (!w) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_w, w, (lenw + rsm.numhid) * sizeof(double));
	rsm.w = w;
	rsm.bh = w + lenw;

	train.mix = (double *) malloc(train.nummix * case_info.numchannel * sizeof(double));
	int _numcase = train.mininumcase > dtt.numcase ? train.mininumcase : dtt.numcase;
	_numcase *= train.nummix;
	double *H = (double*) malloc(_numcase * (rsm.numhid + 1) * sizeof(double));
	int *V = (int*) malloc(_numcase * rsm.numvis * sizeof(int));

	if (!(H && V && train.mix)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(n_mix, train.mix, train.nummix * case_info.numchannel * sizeof(double));

//	int xy;
//	for (xy = 0; xy < lenw; xy++)
//		w[xy] *= 0.01;


	int iter, curbatch = 0;
	double cost;

	clssfy_test.data = H;
	_labels2clssfy(&clssfy_test, dtt.labels, dtt.numcase, train.nummix);
	init_hid_nag_ip_struct(clssfy.lencase, clssfy.numclass - 1);
	//get_data(name_H, H, train_numcase * (numhid + 1) * sizeof(double));
	for (iter = 0; iter < train.iteration; iter++) {
		fprintf(stdout, "**** iter %d, loc %ld ****\n", iter, curbatch * (long)train.mininumcase * case_info.channelcase * case_info.numchannel);
		uint8_t *unlbl = dtr.data + curbatch * (long)train.mininumcase * case_info.channelcase * case_info.numchannel;
		printf("ok, %p\n", unlbl);
		data2V(V, rsm.numclass, unlbl, train.mininumcase, case_info.channelcase, case_info.numchannel, rsm.position, rsm.numvis, train.mix, train.nummix);
		//rsmhid_min(w, bh, V, H, numvis, numhid, numclass, train_numcase);
		_labels2clssfy(&clssfy, dtr.labels + curbatch * train.mininumcase, train.mininumcase, train.nummix);
		classify_get_hid(rsm.w, rsm.bh, V, H, rsm.numvis, rsm.numhid, rsm.numclass, train_numcase, &clssfy);
		int xx, yy;
		for (xx = 0; xx < 1; xx++) {
			for (yy=0; yy < rsm.numhid; yy++)
				fprintf(stdout, "%d", (int)(H[xx * (rsm.numhid + 1) + yy]));
			fprintf(stdout, "\n");
		}
		cost = rsm_train(rsm.w, rsm.bh, V, H, rsm.numvis, rsm.numhid, rsm.numclass, train_numcase, train.batchsize);

		//test	
		data2V(V, rsm.numclass, dtt.data, dtt.numcase, case_info.channelcase, case_info.numchannel, rsm.position, rsm.numvis, train.mix, train.nummix);
		rsmhid_min(rsm.w, rsm.bh, V, H, rsm.numvis, rsm.numhid, rsm.numclass, dtt.numcase * train.nummix, NULL);
		prediction(&clssfy_test);
		int right = 0, nttc;
		for (nttc = 0; nttc < clssfy_test.numcase; nttc++){
			if (clssfy_test.pred[nttc] == clssfy_test.labels[nttc])
				right++;
		//	fprintf(stdout, "(%d %d %d)", nttc, clssfy_test.pred[nttc], clssfy_test.labels[nttc]);
		}
		fprintf(stdout, "**** iter %d: curbatch %d's cost is %lf, the accuracy is %lf ****\n", iter, curbatch, cost, (double)right / clssfy_test.numcase);
		fprintf(stderr, "**** iter %d: curbatch %d's cost is %lf, the accuracy is %lf ****\n", iter, curbatch, cost, (double)right / clssfy_test.numcase);
		curbatch++;
		if (curbatch >= totalbatch)
			curbatch = 0;
	}
	free_hid_nag_ip_struct();
	out_data(n_out, w, (lenw + rsm.numhid) * sizeof(double));
	if (munmap(dtr.data, size_mmap) != 0) {
		fprintf(stderr, "munmap err: %d\n", errno);
		exit(0);
	}

	clssfy_clear(&clssfy);
	clssfy_clear(&clssfy_test);
	free(w);
	free(V);
	free(H);
	free(train.mix);
	free(rsm.position);
	free(rsm.numclass);
	free(dtr.labels);
	free(dtt.labels);
	free(dtt.data);

	return 0;
}
