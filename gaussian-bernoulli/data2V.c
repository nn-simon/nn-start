#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include "utils/utils.h"

static void data2V(int *V, const int *numclass, const uint8_t *data, int channelcase, int numchannel, const int *position, int numvis, const double *mix, int nummix, int repetition)
{
        int nm, nv, nr;
        for (nm = 0; nm < nummix; nm++) {
                for (nv = 0; nv < numvis; nv++) {
                        int channel;
                        double tp = 0.0;
                        for (channel = 0; channel < numchannel; channel++) {
                                tp += data[channel * channelcase + position[nv]] * mix[nm * numchannel + channel] * numclass[nv] / 256.0;
                        }
                        V[nm * numvis + nv] = (int)tp;
                }
        }
        for (nr = 1; nr < repetition; nr++)
                memcpy(&V[nr * nummix * numvis], V, nummix * numvis * sizeof(int));
}

#define MAX_BUF 128

int main(int argc, char *argv[])
{
	int numcase, channelcase, numchannel, nummix;
	int numvis;
	char name_out[MAX_BUF];
	char name_data[MAX_BUF];
	char name_mix[MAX_BUF];
	char name_pos[MAX_BUF];
	char name_class[MAX_BUF];

	char ch;

	while((ch = getopt(argc, argv, "p:P:c:m:u:o:v:")) != -1) {
		switch(ch) {
                case 'p': //para
                        sscanf(optarg, "%dx%dx%dx%d", &numcase, &channelcase, &numchannel, &nummix);
                        break;
                case 'P': //postion
                        strncpy(name_pos, optarg, MAX_BUF);
                        break;
                case 'c':
                        strncpy(name_class, optarg, MAX_BUF);
                        break;
                case 'm':
                        strncpy(name_mix, optarg, MAX_BUF);
                        break;
                case 'u':
                        //data
                        strncpy(name_data, optarg, MAX_BUF);
                        break;
                case 'o':
                        //output
                        strncpy(name_out, optarg, MAX_BUF);
                        break;
                case 'v':
                        //output
			numvis = atoi(optarg);
                        break;
		default:
			fprintf(stderr, "don't know [%c]\n", ch);
			exit(0);
		}
	}

	int *numclass = (int *) malloc(numvis * sizeof(int));
	int lencase = channelcase * numchannel;
	uint8_t *data = (uint8_t *) malloc(lencase * sizeof(uint8_t));
	int *V = (int*) malloc(nummix * numvis * sizeof(int));
	int *position = (int *) malloc(numvis * sizeof(int));
	double *mix = (double *) malloc(nummix * numchannel * sizeof(double));
	if (!(V && data && mix && position && numclass)) {
		fprintf(stderr, "malloc error!\n");
		exit(0);
	}
	get_data(name_class, numclass, numvis * sizeof(int));
	get_data(name_pos, position, numvis * sizeof(int));
	get_data(name_mix, mix, nummix * numchannel * sizeof(double));

	int tpr = rand() % numvis;
	fprintf(stdout, "\t[p]numcaseXnumchannelXnumchannelXnumdim:%dx%dx%dx%d\n"
			"\t[s]vis:%d\n"
			"\tclass[%d]:%d\n"
			"\t[cuom]%s|%s|%s|%s\n", numcase, channelcase, numchannel, nummix,
			numvis, tpr, numclass[tpr], name_class, name_data, name_out, name_mix);

        FILE *fp = fopen(name_data, "r");
        if (fp == NULL) {
                fprintf(stderr, "can't open %s\n", name_data);
                exit(0);
        }
        FILE *out = fopen(name_out, "w");
        if (out == NULL) {
                fprintf(stderr, "can't open %s\n", name_out);
                exit(0);
        }
        int nc, nm;
        for (nc = 0; nc < numcase; nc++) {
                if (fread(data, sizeof(uint8_t), lencase, fp) != lencase) {
                        fprintf(stderr, "read %s err!\n", name_data);
                        exit(0);
                }
                data2V(V, numclass, data, channelcase, numchannel, position, numvis, mix, nummix, 1);
	//	pr_array(stdout, V, 1, 10, 'i');
	//	pr_array(stdout, V + numvis, 1, 10, 'i');
	//	pr_array(stdout, V + 2 * numvis, 1, 10, 'i');
                if (fwrite(V, sizeof(int), numvis * nummix, out) != numvis * nummix) {
                        fprintf(stderr, "write %s err!\n", name_out);
                        exit(0);
		}
        }
        fclose(out);
        fclose(fp);
	free(V);
	free(data);
	free(mix);
	free(position);
	free(numclass);

	return 0;
}
