#include "sm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SM_MAX_BUF 128

static int _cmp_int(const void *a, const void *b)
{
	const int *ia = a;
	const int *ib = b;
	return *ia - *ib;
}

static void _get_v2h(sm_info_t *sm, char *file_v2h)
{
	FILE *fp = fopen(file_v2h, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s\n", file_v2h);
		exit(0);
	}
	int nv, nl;
	sm->len_v2h_max = 0;
	for (nv =0; nv < sm->numvis; nv++) {
		fscanf(fp, "%d", &sm->len_v2h[nv]);
		if (sm->len_v2h[nv] > sm->len_v2h_max)
			sm->len_v2h_max = sm->len_v2h[nv];
		sm->v2h[nv] = (int *) malloc(sm->len_v2h[nv] * sizeof(int));
		if (sm->v2h[nv] == NULL) {
			fprintf(stderr, "malloc error in _get_v2h");
			exit(0);
		}
		for (nl =0; nl < sm->len_v2h[nv]; nl++)
			fscanf(fp, "%d", &sm->v2h[nv][nl]);
		//qsort(sm->v2h[nv], sm->len_v2h[nv], sizeof(int), _cmp_int);
	}
	fclose(fp);
}

static void _get_w(sm_info_t *sm, char *file_w)
{
	FILE *fp = fopen(file_w, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s\n", file_w);
		exit(0);
	}
	int nv, len;
	size_t nread;
	for (nv =0; nv < sm->numvis; nv++) {
		len = sm->numclass[nv] * (sm->len_v2h[nv] + 1);
		nread = fread(sm->w->w[nv], sizeof(double), len, fp);
		if (nread != len) {
			fprintf(stderr, "read %s error: need read %d, actually read %ld!\n", file_w, len, nread);
			exit(0);
		}
	}
	nread = fread(sm->w->bh, sizeof(double), sm->numhid, fp);
	if (nread != sm->numhid) {
		fprintf(stderr, "read %s error: need read %d, actually read %ld!\n", file_w, sm->numhid, nread);
		exit(0);
	}
	nread = fread(sm->w->bm_w, sizeof(double), sm->numhid * sm->numhid, fp);
	if (nread != sm->numhid * sm->numhid) {
		fprintf(stderr, "read %s error: need read %d, actually read %ld!\n", file_w, sm->numhid * sm->numhid, nread);
		exit(0);
	}
	fclose(fp);
}

static void _get_h2v(sm_info_t *sm)
{
	int nv, nh, nl;
	memset(sm->len_h2v, 0, sm->numhid * sizeof(int));
	for (nv = 0; nv < sm->numvis; nv++) {
		for (nl = 0; nl < sm->len_v2h[nv]; nl++)
			sm->len_h2v[sm->v2h[nv][nl]]++;
	}
	for (nh = 0; nh < sm->numhid; nh++) {
		sm->h2v[nh] = (int *)malloc(sm->len_h2v[nh] * sizeof(int));
		sm->pos_h2v[nh] = (int *)malloc(sm->len_h2v[nh] * sizeof(int));
		if (!(sm->h2v[nh] && sm->pos_h2v[nh])) {
			fprintf(stderr, "malloc err in _get_h2v\n");
			exit(0);
		}
		sm->len_h2v[nh] = 0;
	}
	for (nv = 0; nv < sm->numvis; nv++) {
		for (nl = 0; nl < sm->len_v2h[nv]; nl++) {
			int id_h = sm->v2h[nv][nl];
			int count = sm->len_h2v[id_h];
			sm->h2v[id_h][count] = nv;
			sm->pos_h2v[id_h][count] = nl;
			sm->len_h2v[id_h]++;
		}
	}
/*
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
// */
}

void construct_sm_w(sm_info_t *sm, sm_w_t *w)
{
	w->w = (double **)malloc(sm->numvis * sizeof(double *));
	w->bh = (double *)malloc(sm->numhid * sizeof(double));
	w->bm_w = (double *)malloc(sm->numhid * sm->numhid * sizeof(double));
	if (!(w->w && w->bh && w->bm_w)) {
		fprintf(stderr, "malloc err in construct_sm\n");
		exit(0);
	}
	int nv;
	for (nv = 0; nv < sm->numvis; nv++) {
		w->w[nv] = (double *)malloc(sm->numclass[nv] * (sm->len_v2h[nv] + 1) * sizeof(double));
		if (w->w[nv] == NULL) {
			fprintf(stderr, "malloc err in construct_sm\n");
			exit(0);
		}
	}
}

void destroy_sm_w(sm_info_t *sm, sm_w_t *w)
{
	int nv;
	for (nv = 0; nv < sm->numvis; nv++)
		free(w->w[nv]);
	free(w->w);
	free(w->bh);
}

void construct_sm(sm_info_t *sm, char *argv)
{
	char file_w[SM_MAX_BUF];
	char file_v2h[SM_MAX_BUF];
	char file_pos[SM_MAX_BUF];
	char file_bm_pos[SM_MAX_BUF];
	char file_class[SM_MAX_BUF];

	sscanf(argv, "%d %d %s %s %s %s %s", &sm->numvis, &sm->numhid,
		file_pos, file_bm_pos, file_w, file_class, file_v2h);
	fprintf(stdout, "print in fun construct_sm:\n"
			"\tvisXhid:%dx%d\n"
			"\tpos|bm_pos|w|class|v2h:%s|%s|%s|%s|%s\n", sm->numvis, sm->numhid,
			file_pos, file_bm_pos, file_w, file_class, file_v2h);

	sm->numclass = (int *)malloc(sm->numvis * sizeof(int));
	sm->position = (int *)malloc(sm->numvis * sizeof(int));
	sm->len_v2h = (int *)malloc(sm->numvis * sizeof(int));
	sm->len_h2v = (int *)malloc(sm->numhid * sizeof(int));
	sm->v2h = (int **)malloc(sm->numvis * sizeof(int*));
	sm->h2v = (int **)malloc(sm->numhid * sizeof(int*));
	sm->pos_h2v = (int **)malloc(sm->numhid * sizeof(int*));
	sm->bm_pos = (uint8_t *)malloc(sm->numhid * sm->numhid * sizeof(uint8_t));
	sm->w = (sm_w_t *)malloc(sizeof(sm_w_t));

	if (!(sm->numclass && sm->position && sm->len_v2h && sm->len_h2v && sm->v2h && sm->h2v 
		&& sm->bm_pos && sm->w)) {
		fprintf(stderr, "malloc err in construct_sm\n");
		exit(0);
	}

	//numclass
	get_data(file_class, sm->numclass, sm->numvis * sizeof(int));
	pr_array(stdout, sm->numclass, 1, sm->numvis, 'i');
	//position
	get_data(file_pos, sm->position, sm->numvis * sizeof(int));
	//bm_pos
	get_data(file_bm_pos, sm->bm_pos, sm->numhid * sm->numhid * sizeof(uint8_t));
	int nv, nh, nl;
	sm->class_max = 0;
	for (nv = 0; nv < sm->numvis; nv++)
		if (sm->class_max < sm->numclass[nv])
			sm->class_max = sm->numclass[nv];
	for (nh = 0; nh < sm->numhid; nh++)
		sm->bm_pos[nh * sm->numhid + nh] = 0;
	_get_v2h(sm, file_v2h);
	construct_sm_w(sm, sm->w);
	_get_w(sm, file_w);
	_get_h2v(sm);
	for (nl = 0; nl < sm->numhid * sm->numhid; nl++)
		sm->w->bm_w[nl] = sm->w->bm_w[nl] * sm->bm_pos[nl];
}

void destroy_sm(sm_info_t *sm)
{
	int nv, nh;

	for (nv = 0; nv < sm->numvis; nv++)
		free(sm->v2h[nv]);
	for (nh =0; nh < sm->numhid; nh++) {
		free(sm->h2v[nh]);
		free(sm->pos_h2v[nh]);
	}
	destroy_sm_w(sm, sm->w);
	free(sm->numclass);
	free(sm->position);
	free(sm->w);
	free(sm->bm_pos);
	free(sm->len_v2h);
	free(sm->len_h2v);
	free(sm->pos_h2v);
	free(sm->v2h);
	free(sm->h2v);
}

void out_w(char *file, const sm_info_t *sm)
{
	FILE *fp = fopen(file, "w");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s\n", file);
		exit(0);
	}
	int nv, len;
	size_t nwrite;
	for (nv =0; nv < sm->numvis; nv++) {
		len = sm->numclass[nv] * (sm->len_v2h[nv] + 1);
		nwrite = fwrite(sm->w->w[nv], sizeof(double), len, fp);
		if (nwrite != len) {
			fprintf(stderr, "write %s error: need write %d, actually write %ld!\n", file, len, nwrite);
			exit(0);
		}
	}
	nwrite = fwrite(sm->w->bh, sizeof(double), sm->numhid, fp);
	if (nwrite != sm->numhid) {
		fprintf(stderr, "write %s error: need write %d, actually write %ld!\n", file, sm->numhid, nwrite);
		exit(0);
	}
	nwrite = fwrite(sm->w->bm_w, sizeof(double), sm->numhid * sm->numhid, fp);
	if (nwrite != sm->numhid * sm->numhid) {
		fprintf(stderr, "write %s error: need write %d, actually write %ld!\n", file, sm->numhid * sm->numhid, nwrite);
		exit(0);
	}
	fclose(fp);
}
