#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb.h"

#define GB_MAX_BUF 128

static int _cmp_int(const void *a, const void *b)
{
	const int *ia = a;
	const int *ib = b;
	return *ia - *ib;
}

static void _get(FILE *fp, void *buf, size_t memsize, size_t len, char *msg)
{
	size_t nread = fread(buf, memsize, len, fp);
	if (nread != len) {
		fprintf(stderr, "%s: need read %ld, actually read %ld!\n", msg, len, nread);
		exit(0);
	}
}

static void _get_w(gb_info_t *gb, char *file)
{
	char msg[GB_MAX_BUF];
	sprintf(msg, "read %s err", file);

	FILE *fp = fopen(file, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s\n", file);
		exit(0);
	}
	_get(fp, gb->w->w, sizeof(double), gb->numvis * gb->numhid, msg);
	_get(fp, gb->w->bv, sizeof(double), gb->numvis, msg);
	_get(fp, gb->w->sigma, sizeof(double), gb->numvis, msg);
	_get(fp, gb->w->bh, sizeof(double), gb->numhid, msg);
	_get(fp, gb->w->bm_w, sizeof(double), gb->numhid * gb->numhid, msg);
	fclose(fp);
}

void construct_gb_w(gb_info_t *gb, gb_w_t *w)
{
	w->w = (double *)malloc(gb->numvis * gb->numhid * sizeof(double));
	w->bh = (double *)malloc(gb->numhid * sizeof(double));
	w->bv = (double *)malloc(gb->numvis * sizeof(double));
	w->sigma = (double *)malloc(gb->numvis * sizeof(double));
	w->bm_w = (double *)malloc(gb->numhid * gb->numhid * sizeof(double));
	if (!(w->w && w->bh && w->bm_w && w->bv && w->sigma)) {
		fprintf(stderr, "malloc err in construct_gb\n");
		exit(0);
	}
}

void destroy_gb_w(gb_info_t *gb, gb_w_t *w)
{
	free(w->w);
	free(w->bh);
	free(w->bm_w);
	free(w->bv);
	free(w->sigma);
}

void construct_gb(gb_info_t *gb, char *argv)
{
	char network_type[2][GB_MAX_BUF] = {{"restricted netowrk"}, {"no restricted network"}};
	char file_w[GB_MAX_BUF];
	char file_vh[GB_MAX_BUF];
	char file_bm[GB_MAX_BUF];

	sscanf(argv, "%d %d %d %lf %s %s %s", &gb->type, &gb->numvis, &gb->numhid, &gb->lambda,
		file_w, file_bm, file_vh);
	fprintf(stdout, "print in fun construct_gb:\n"
			"\ttype:%d[%s]\n"
			"\tvisXhid:%dx%d\n"
			"\tlambda:%lf\n"
			"\tvh|bm_pos|w:%s|%s|%s\n",
			gb->type, network_type[gb->type], gb->numvis, gb->numhid, gb->lambda,
			file_vh, file_bm, file_w);

	gb->vh_pos = (uint8_t *)malloc(gb->numhid * gb->numvis * sizeof(uint8_t));
	gb->bm_pos = (uint8_t *)malloc(gb->numhid * gb->numhid * sizeof(uint8_t));
	gb->w = (gb_w_t *)malloc(sizeof(gb_w_t));

	if (!(gb->vh_pos && gb->bm_pos && gb->w)) {
		fprintf(stderr, "malloc err in construct_gb\n");
		exit(0);
	}
	printf("bm: %s, vh: %s\n", file_bm, file_vh);
	get_data(file_bm, gb->bm_pos, gb->numhid * gb->numhid * sizeof(uint8_t));
	get_data(file_vh, gb->vh_pos, gb->numhid * gb->numvis * sizeof(uint8_t));
	construct_gb_w(gb, gb->w);
	_get_w(gb, file_w);
}

void destroy_gb(gb_info_t *gb)
{
	destroy_gb_w(gb, gb->w);
	free(gb->w);
	free(gb->bm_pos);
	free(gb->vh_pos);
}

static void _out(FILE *fp, void *buf, size_t memsize, size_t len, char *msg)
{
	size_t nwrite = fwrite(buf, memsize, len, fp);
	if (nwrite != len) {
		fprintf(stderr, "%s: need write %ld, actually write %ld!\n", msg, len, nwrite);
		exit(0);
	}
}

void out_w(char *file, const gb_info_t *gb)
{
	char msg[GB_MAX_BUF];
	sprintf(msg, "write %s error", file);

	FILE *fp = fopen(file, "w");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s\n", file);
		exit(0);
	}
	_out(fp, gb->w->w, sizeof(double), gb->numvis * gb->numhid, msg);
	_out(fp, gb->w->bv, sizeof(double), gb->numvis, msg);
	_out(fp, gb->w->sigma, sizeof(double), gb->numvis, msg);
	_out(fp, gb->w->bh, sizeof(double), gb->numhid, msg);
	_out(fp, gb->w->bm_w, sizeof(double), gb->numhid * gb->numhid, msg);
	fclose(fp);
}
